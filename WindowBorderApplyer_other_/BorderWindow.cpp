#include "BorderWindow.h"
#include "ScalingUtil.h"
#include "Windowmodule.h"

#include <dwmapi.h>
#include <Windows.h>

#include <winrt/windows.foundation.h>

std::optional<RECT> GetFrameRect(HWND window, int borderlength)
{
	RECT rect;
	if (!SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect))))
	{
		// std:nullopt - 빈 optional 객체
		return std::nullopt;
	}

	rect.top -= borderlength;
	rect.left -= borderlength;
	rect.right += borderlength;
	rect.bottom += borderlength;

	return rect;
}

BorderWindow::BorderWindow(HWND window, int borderlength, COLORREF color) : window(nullptr), trackingwindow(window), borderlength(borderlength), bordercolor(color) { } // 프레임 그리기 

BorderWindow::~BorderWindow()
{
	if (frameDrawer)
	{
		frameDrawer->Hide();
		frameDrawer = nullptr;
	}

	if (window)
	{
		SetWindowLongPtrW(window, GWLP_USERDATA, 0);
		DestroyWindow(window);
	}
}

std::unique_ptr<BorderWindow> BorderWindow::Create(HWND targetwindow, HINSTANCE hInstance, int borderlength, COLORREF color)
{
	auto self = std::unique_ptr<BorderWindow>(new BorderWindow(targetwindow, borderlength, color));
	if (self->Init(hInstance))
		return self;

	return nullptr;
}


const wchar_t ToolWindowClassString[] = L"CustomWIndow_Border";

constexpr uint32_t Refresh_Border_Timer_Id = 123;
constexpr uint32_t Refresh_Border_Interval = 100;

bool BorderWindow::Init(HINSTANCE hInstance)
{
	if (!trackingwindow)
		return false;

	auto windowRectOpt = GetFrameRect(trackingwindow, borderlength);
	if (!windowRectOpt.has_value())
		return false;

	RECT windowRect = windowRectOpt.value();

	WNDCLASSEXW wce{};
	wce.cbSize = sizeof(WNDCLASSEX);
	wce.lpfnWndProc = s_WndProc;
	wce.hInstance = hInstance;
	wce.lpszClassName = ToolWindowClassString;
	wce.hCursor = LoadCursorW(nullptr, IDC_ARROW);

	RegisterClassExW(&wce);

	window = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		ToolWindowClassString,
		L"",
		WS_POPUP | WS_DISABLED,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		this);

	if (!window)
		return false;

	if (!SetLayeredWindowAttributes(window, RGB(0, 0, 0), 0, LWA_COLORKEY))
		return false;

	SetWindowPos(trackingwindow,
		window,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		SWP_NOMOVE | SWP_NOSIZE);

	bool val = true;
	DwmSetWindowAttribute(window, DWMWA_EXCLUDED_FROM_PEEK, &val, sizeof(val));

	frameDrawer = FrameDrawer::Create(window);
	if (!frameDrawer)
		return false;

	frameDrawer->Show();
	UpdateBorderPosition();
	UpdateBorderProperties();
	timer_id = SetTimer(window, Refresh_Border_Timer_Id, Refresh_Border_Interval, nullptr);

	return true;
}

void BorderWindow::UpdateBorderPosition() const
{
	if (!trackingwindow)
		return;

	// 트래킹 윈도우에 대한 width, height 를 borderlength를 반영하여 다시 로드한 객체
	auto rectOpt = GetFrameRect(trackingwindow, borderlength);
	if (!rectOpt.has_value())
	{
		frameDrawer->Hide();
		return;
	}

	RECT rect = rectOpt.value();
	SetWindowPos(window, trackingwindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOREDRAW | SWP_NOACTIVATE);
}

void BorderWindow::UpdateBorderProperties() const
{
	if (!trackingwindow || !frameDrawer)
		return;

	auto windowRectOpt = GetFrameRect(trackingwindow, borderlength);
	if (!windowRectOpt.has_value())
		return;

	const RECT windowRect = windowRectOpt.value();
	SetWindowPos(window, trackingwindow, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_NOREDRAW | SWP_NOACTIVATE);

	RECT frameRect{ 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top };

	//COLORREF color = RGB(107, 120, 138);
	COLORREF color = bordercolor;
	// Accent 컬러 설정

	float scalingF = ScalingUtil::ScalingF(trackingwindow);
	float thickness = 2 * scalingF;
	float cornerradius = 0.0;

	frameDrawer->SetBorderRect(frameRect, color, static_cast<int>(thickness), cornerradius);
}

void BorderWindow::SetBorderColor(COLORREF color)
{
	bordercolor = color;
}

LRESULT BorderWindow::WndProc(UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
	switch (message)
	{
	case WM_TIMER:
	{
		switch (wparam)
		{
		case Refresh_Border_Timer_Id:
			KillTimer(window, timer_id);
			timer_id = SetTimer(window, Refresh_Border_Timer_Id, Refresh_Border_Interval, nullptr);
			UpdateBorderPosition();
			UpdateBorderProperties();
			break;
		}
		break;
	}
	case WM_NCDESTROY:
		KillTimer(window, timer_id);
		::DefWindowProc(window, message, wparam, lparam);
		SetWindowLongPtr(window, GWLP_USERDATA, 0);
		break;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_SETCURSOR:
		return TRUE;

	default:
		return DefWindowProc(window, message, wparam, lparam);
	}
	return FALSE;
}
