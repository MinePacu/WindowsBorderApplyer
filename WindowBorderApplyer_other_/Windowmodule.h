#pragma once

#include <Windows.h>
#include <vector>
#include <map>
#include <memory>

#include "BorderWindow.h"
#include "WinEventHook.h"
#include "VirtualDesktopUtil.h"
#include "CaptionColorUtil.h"


class Windowmodule
{
public:
	Windowmodule(byte r, byte g, byte b, COLORREF captionColor);
	~Windowmodule();

	UINT cornerPreference = 0;
	COLORREF CaptionColor;
	int BuildVer = 0;

	bool RefreshHwnds(std::vector<HWND> hwndlist);
	bool AssignBorder(HWND window);
	void AddHwnd(HWND window);
	bool FindHwnd(HWND window);

	void ClearBorderWindows();
	void CleanupBorderWindows() noexcept;

	void RestoreDwmMica(int buildVersion);
	void TrackingWindows();

protected:
	static LRESULT CALLBACK WndProc_Helper(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
	{
		// 윈도우 인스턴스에서 추가 메모리 주소 로드 -> Windowmodule 객체로 변환
		auto thisRef = reinterpret_cast<Windowmodule*>(GetWindowLongPtr(window, GWLP_USERDATA));
		if (!thisRef && (message == WM_CREATE))
		{
			const auto createstruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
			thisRef = static_cast<Windowmodule*>(createstruct->lpCreateParams);
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thisRef));
		}
		return (thisRef != nullptr) ? thisRef->WndProc(window, message, wparam, lparam) : DefWindowProc(window, message, wparam, lparam);
	}

private:
	static inline Windowmodule* s_instance = nullptr;
	std::vector<HWINEVENTHOOK> staticWinEventHooks{};
	VirtualDesktopUtil virtualDesktopUtil;
	CaptionColorUtil captionColorUtil{ CaptionColor };

	HWND window{ nullptr };
	HINSTANCE hinstance;
	std::map<HWND, std::unique_ptr<BorderWindow>> borderedWindows{};
	HANDLE hBorderedEvent;
	HWINEVENTHOOK winEventHook;
	std::thread thread;

	std::vector<HWND> hwnds{};

	bool running = true;

	COLORREF color;

	LRESULT WndProc(HWND, UINT, WPARAM, LPARAM) noexcept;

	void ControlWinHookEvent(WinEventHook* data) noexcept;

	bool InitToolWindow();
	void SubToEvent();

	void ProcessCommand(HWND window);

	void RefreshBorders() noexcept;


	static void CALLBACK WinHookProc(HWINEVENTHOOK winEventhook,
		DWORD event,
		HWND window,
		LONG obj,
		LONG child,
		DWORD eventThread,
		DWORD eventTime)
	{
		WinEventHook data{ event, window, obj, child, eventThread, eventTime };
		if (s_instance)
			s_instance->ControlWinHookEvent(&data);
	}

};

/// <summary> Windowmodule 클래스가 포함된 모듈에 대한 핸들러 </summary>
extern "C" IMAGE_DOS_HEADER __ImageBase;
