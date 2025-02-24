#pragma once

#include <mutex>

#include <d2d1.h>
#include <dwrite.h>
#include <winrt/base.h>

class FrameDrawer
{
public:
	static std::unique_ptr<FrameDrawer> Create(HWND window);
	
	FrameDrawer(HWND window);
	FrameDrawer(FrameDrawer&& other) = default;

	bool Init();

	void Show();
	void Hide();
	void SetBorderRect(RECT windowRect, COLORREF color, int thickness, float radius);

private:
	struct DrawableRect
	{
		std::optional<D2D1_RECT_F> rect;
		std::optional<D2D1_ROUNDED_RECT> roundedRect;
		D2D1_COLOR_F bordercolor;
		int thickness;
	};

	HWND window = nullptr;
	size_t renderTargetSizeHash = {};
	winrt::com_ptr<ID2D1HwndRenderTarget> renderTarget;
	winrt::com_ptr<ID2D1SolidColorBrush> borderBrush;
	DrawableRect sceneRect = {};

	bool CreateRenderTargets(const RECT& clientRect);

	static ID2D1Factory* GetD2D1Factory();
	static IDWriteFactory* GetWriteFactory();
	static D2D1_COLOR_F ConvertColor(COLORREF color);
	static D2D1_ROUNDED_RECT ConvertRECT(RECT rect, int thickness, float radius);
	static D2D1_RECT_F ConvertRECT(RECT rect, int thickness);
	
	void Render();
};