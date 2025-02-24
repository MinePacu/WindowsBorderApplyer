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
	/// ���ø����̼� �����͸� â�� �����ϱ� ���Ͽ� â �޸𸮿� �߰� �����͸� ������ ��, Ŀ���ҵ� â ���ν��� Ȥ�� ���� ���ν����� ȣ���մϴ�.
	/// </summary>
	/// <param name="window">: �߰� �����͸� ������ Window �ڵ鷯</param>
	/// <param name="message">: â �޽���</param>
	/// <param name="wparam">: ������ ����</param>
	/// <param name="lparam">: WM_CREATE �޽����� ���, CREATESTRUCT ����ü�� ���� ������</param>
	/// <returns>���ν������� ��ȯ�� LRESULT</returns>
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
