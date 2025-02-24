#include "Windowmodule.h"

#include <windows.h>
#include <dwmapi.h>
#include <iostream>

Windowmodule::Windowmodule(byte r, byte g, byte b, COLORREF captionColor) : hinstance(reinterpret_cast<HINSTANCE>(&__ImageBase))
{
	s_instance = this;
	borderedWindows;

	color = RGB(r, g, b);
	CaptionColor = captionColor;
	if (InitToolWindow())
	{
		SubToEvent();
	}
}

Windowmodule::~Windowmodule()
{
	running = false;
	CleanupBorderWindows();
}

LRESULT Windowmodule::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
	return 0;
}

const static wchar_t* TOOL_WINDOW_CLASS_STRING = L"CustomWIndowTool";

bool Windowmodule::InitToolWindow()
{
	WNDCLASSEXW wce{ };
	wce.cbSize = sizeof(WNDCLASSEX);
	wce.lpfnWndProc = WndProc_Helper;
	wce.hInstance = hinstance;
	wce.lpszClassName = TOOL_WINDOW_CLASS_STRING;

	RegisterClassExW(&wce);

	window = CreateWindowExW(WS_EX_TOOLWINDOW, TOOL_WINDOW_CLASS_STRING, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, hinstance, this);
	if (!window)
		return false;

	return true;
}

void Windowmodule::ProcessCommand(HWND hwnd)
{
	return;
}

void Windowmodule::SubToEvent()
{
	std::array<DWORD, 7> events_to_sub = {
		EVENT_OBJECT_LOCATIONCHANGE,
		EVENT_SYSTEM_MINIMIZESTART,
		EVENT_SYSTEM_MINIMIZEEND,
		EVENT_SYSTEM_MOVESIZEEND,
		EVENT_SYSTEM_FOREGROUND,
		EVENT_OBJECT_DESTROY,
		EVENT_OBJECT_FOCUS
	};

	for (const auto event : events_to_sub)
	{
		auto hook = SetWinEventHook(event, event, nullptr, WinHookProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
		winEventHook = hook;
		if (hook)
			staticWinEventHooks.emplace_back(hook);
		else
			return;
	}
}

bool Windowmodule::RefreshHwnds(std::vector<HWND> hwnds_)
{
	hwnds = hwnds_;
	if (&hwnds != nullptr)
		return true;
	else
		return false;
}

void Windowmodule::AddHwnd(HWND window)
{
	hwnds.push_back(window);
	AssignBorder(window);
}

bool Windowmodule::FindHwnd(HWND window)
{
	if (std::find(hwnds.begin(), hwnds.end(), window) != hwnds.end())
		return true;
	else
		return false;
}

void Windowmodule::TrackingWindows()
{
	for (HWND hwnd : hwnds)
	{
		AssignBorder(hwnd);
	}
}

bool Windowmodule::AssignBorder(HWND hwnd)
{
	std::cerr << "VirtualDesktopUtil.IsWindowsOnCurrentDesktop(hwnd) :  " << virtualDesktopUtil.IsWindowsOnCurrentDesktop(hwnd) << std::endl;
	if (virtualDesktopUtil.IsWindowsOnCurrentDesktop(hwnd))
	{
		auto border = BorderWindow::Create(hwnd, hinstance, 3, color);
		if (border)
			borderedWindows[hwnd] = std::move(border);
	}
	else
		borderedWindows[hwnd] = nullptr;

	return true;
}

void Windowmodule::ClearBorderWindows()
{
	borderedWindows.clear();
}

void Windowmodule::CleanupBorderWindows() noexcept
{
	for (const auto& [window, border] : borderedWindows)
	{
		if (border)
			borderedWindows[window] = nullptr;
	}

	if (window)
	{
		DestroyWindow(window);
		window = nullptr;
	}
	UnregisterClass(TOOL_WINDOW_CLASS_STRING, reinterpret_cast<HINSTANCE>(&__ImageBase));

	staticWinEventHooks.clear();
	UnhookWinEvent(winEventHook);

	hwnds.clear();
	borderedWindows.clear();

	s_instance = nullptr;
}

void Windowmodule::RestoreDwmMica(int buildVersion)
{
	if (buildVersion < 22523 && buildVersion >= 22000)
	{
		UINT val = 1;
		for (HWND hwnd : hwnds)
		{
			// 1029 -> DWMMA_MICA_EFFECT
			DwmSetWindowAttribute(hwnd, 1029, &val, sizeof(val));
		}
	}

	else if (buildVersion >= 22523)
	{
		UINT enablemica = 2;
		for (HWND hwnd : hwnds)
		{
			DwmSetWindowAttribute(hwnd, 38, &enablemica, sizeof(enablemica));
		}
	}
}

void Windowmodule::ControlWinHookEvent(WinEventHook* data) noexcept
{
	if (!data->hwnd)
		return;

	// 최소화를 포함하여 Visible이 false인 윈도우 걸러내기
	std::vector<HWND> ClosedWindow{};
	for (const auto& [window, border] : borderedWindows)
	{
		bool isWindowOpened = IsWindowVisible(window);
		if (isWindowOpened == false)
			ClosedWindow.push_back(window);
	}

	for (const auto window : ClosedWindow)
	{
		borderedWindows.erase(window);
		hwnds.erase(std::find(hwnds.begin(), hwnds.end(), window));
	}
	hwnds.shrink_to_fit();


	// 윈도우 변경 처리
	switch (data->event)
	{
		// OBJECT의 위치, 모양, 크기가 변경됨
	case EVENT_OBJECT_LOCATIONCHANGE:
	{
		auto temp = borderedWindows.find(data->hwnd);
		if (temp != borderedWindows.end())
		{
			const auto& border = temp->second;
			if (border)
				border->UpdateBorderPosition();
		}
	}
	break;
	// 창 최소화
	case EVENT_SYSTEM_MINIMIZESTART:
	{
		auto temp = borderedWindows.find(data->hwnd);
		if (temp != borderedWindows.end())
			borderedWindows[data->hwnd] = nullptr;
	}
	break;
	// 최소화 해제
	case EVENT_SYSTEM_MINIMIZEEND:
	{
		auto temp = borderedWindows.find(data->hwnd);
		if (temp != borderedWindows.end())
			AssignBorder(data->hwnd);
	}
	break;
	// 창 이동 또는 크기 조정 완료
	case EVENT_SYSTEM_MOVESIZEEND:
	{
		auto temp = borderedWindows.find(data->hwnd);
		if (temp != borderedWindows.end())
		{
			const auto& border = temp->second;
			if (border)
				border->UpdateBorderPosition();
		}
	}
	break;
	// 창이 포그라운드 창으로 변경
	case EVENT_SYSTEM_FOREGROUND:
	{
		RefreshBorders();
	}
	break;
	case EVENT_OBJECT_FOCUS:
	{

	}
	break;
	default:
		break;
	}
}

void Windowmodule::RefreshBorders() noexcept
{
	if (borderedWindows.empty())
		return;

	for (const auto& [window, border] : borderedWindows)
	{
		if (virtualDesktopUtil.IsWindowsOnCurrentDesktop(window))
		{
			if (!border)
				AssignBorder(window);
		}
		else
		{
			if (border)
				borderedWindows[window] = nullptr;
		}
	}
}