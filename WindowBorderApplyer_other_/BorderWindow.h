#pragma once
#include <Windows.h>
#include <dwmapi.h>

#include "FrameDrawer.h"

class BorderWindow
{
	BorderWindow(HWND window, int borderlength, COLORREF color);
	BorderWindow(BorderWindow&& other) = default;

public:
	static std::unique_ptr<BorderWindow> Create(HWND targetwindow, HINSTANCE hinstance, int borderlength, COLORREF color);
	~BorderWindow();

	void SetBorderColor(COLORREF color);

	void UpdateBorderPosition() const;
	void UpdateBorderProperties() const;

private:
	UINT_PTR timer_id = {};
	HWND window = {};
	HWND trackingwindow = {};
	COLORREF bordercolor;
	std::unique_ptr<FrameDrawer> frameDrawer;
	int borderlength = 1;

	LRESULT WndProc(UINT message, WPARAM wparam, LPARAM lparam) noexcept;

	bool Init(HINSTANCE hInstance);

protected:
	/// <summary>
	/// 애플리케이션 데이터를 창에 전달하기 위하여 창 메모리에 추가 데이터를 저장한 후, 커스텀된 창 프로시저 혹은 원래 프로시저를 호출합니다.
	/// </summary>
	/// <param name="window">: 추가 데이터를 저장할 Window 핸들러</param>
	/// <param name="message">: 창 메시지</param>
	/// <param name="wparam">: 사용되지 않음</param>
	/// <param name="lparam">: WM_CREATE 메시지인 경우, CREATESTRUCT 구조체에 대한 포인터</param>
	/// <returns>프로시저에서 반환된 LRESULT</returns>
	static LRESULT CALLBACK s_WndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
	{
		auto thisRef = reinterpret_cast<BorderWindow*>(GetWindowLongPtr(window, GWLP_USERDATA));
		if ((thisRef == nullptr) && (message == WM_CREATE))
		{
			auto createstruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
			thisRef = static_cast<BorderWindow*>(createstruct->lpCreateParams);
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thisRef));
		}

		return (thisRef != nullptr) ? thisRef->WndProc(message, wparam, lparam) : DefWindowProc(window, message, wparam, lparam);
	}
};
