#include "FrameDrawer.h"
#include "pch.h"

#include <dwmapi.h>

namespace
{
	size_t D2DRectHash(D2D1_SIZE_U rect)
	{
		using repr_t = uint64_t;
		static_assert(sizeof(D2D1_SIZE_U) == sizeof(repr_t));
		std::hash<repr_t> hasher{};
		return hasher(*reinterpret_cast<const repr_t*>(&rect));
	}
}

std::unique_ptr<FrameDrawer> FrameDrawer::Create(HWND window)
{
	auto self = std::make_unique<FrameDrawer>(window);
	if (self->Init())
		return self;

	return nullptr;
}

FrameDrawer::FrameDrawer(HWND window) : window(window) {}

bool FrameDrawer::Init()
{
	RECT clientRect;
	if (!SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &clientRect, sizeof(clientRect))))
		return false;

	return CreateRenderTargets(clientRect);
}

bool FrameDrawer::CreateRenderTargets(const RECT& clientRect)
{
	constexpr float dpi = 96.f;
	const auto renderTargetProperties =
		D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
			dpi,
			dpi);

	const auto renderTargetSize = D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	const auto rectHash = D2DRectHash(renderTargetSize);
	if (renderTarget && rectHash == renderTargetSizeHash)
		return true;

	renderTarget = nullptr;

	const auto hwndRenderTargetProperties = D2D1::HwndRenderTargetProperties(window, renderTargetSize, D2D1_PRESENT_OPTIONS_NONE);

	if (!SUCCEEDED(GetD2D1Factory()->CreateHwndRenderTarget(renderTargetProperties, hwndRenderTargetProperties, renderTarget.put())) || !renderTarget)
		return false;

	renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	renderTargetSizeHash = rectHash;

	return true;
}

void FrameDrawer::Show()
{
	ShowWindow(window, SW_SHOWNA);
	Render();
}

void FrameDrawer::Hide()
{
	ShowWindow(window, SW_HIDE);
}

void FrameDrawer::SetBorderRect(RECT windowRect, COLORREF color, int thickness, float radius)
{
	// FrameDrawer.h 에 정의된 DrawableRect 구조체에 대한 객체 만들기
	// c# 에서는 var newSceneRect = new DrawableRect() { bordercolor = ConvertColor(color), thickness = thickness }; 와 같다.
	auto newSceneRect = DrawableRect{};
	newSceneRect.bordercolor = ConvertColor(color);
	newSceneRect.thickness = thickness;

	if ((radius == 0) == false)
		newSceneRect.roundedRect = ConvertRECT(windowRect, thickness, radius);

	else
		newSceneRect.rect = ConvertRECT(windowRect, thickness);

	const bool colorUpdated = std::memcmp(&sceneRect.bordercolor, &newSceneRect.bordercolor, sizeof(newSceneRect.bordercolor));
	const bool thicknessUpdated = sceneRect.thickness != newSceneRect.thickness;
	const bool cornersUpdated = (sceneRect.rect.has_value() != newSceneRect.rect.has_value()) || (sceneRect.roundedRect.has_value() != newSceneRect.roundedRect.has_value());

	// 하나라도 업데이트되어 true인 경우, 프레임을 다시 그릴 필요가 있음을 업데이트함
	const bool needsRedraw = colorUpdated || thicknessUpdated || cornersUpdated;

	RECT clientRect;
	if (!SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &clientRect, sizeof(clientRect))))
		return;

	sceneRect = std::move(newSceneRect);

	const auto renderTargetSize = D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	const auto recthash = D2DRectHash(renderTargetSize);

	const bool DesiredSize = (recthash == renderTargetSizeHash) && renderTarget;
	if (DesiredSize)
	{
		const bool resizeOk = renderTarget && SUCCEEDED(renderTarget->Resize(renderTargetSize));
		if (resizeOk == false)
		{
			if (CreateRenderTargets(clientRect) == false)
			{
				// 예외 처리
			}
		}
		else
			renderTargetSizeHash = recthash;
	}

	if (colorUpdated)
	{
		borderBrush = nullptr;
		if (renderTarget)
			renderTarget->CreateSolidColorBrush(sceneRect.bordercolor, borderBrush.put());
	}

	if (!DesiredSize || needsRedraw)
		Render();
}

void FrameDrawer::Render()
{
	if (!renderTarget || !borderBrush)
		return;

	renderTarget->BeginDraw();
	renderTarget->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));

	if (sceneRect.roundedRect)
		renderTarget->DrawRoundedRectangle(sceneRect.roundedRect.value(), borderBrush.get(), static_cast<float>(sceneRect.thickness));

	else if (sceneRect.rect)
		renderTarget->DrawRectangle(sceneRect.rect.value(), borderBrush.get(), static_cast<float>(sceneRect.thickness));

	renderTarget->EndDraw();
}

ID2D1Factory* FrameDrawer::GetD2D1Factory()
{
	static auto D2D1Factory = []
	{
		ID2D1Factory* res = nullptr;
		D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &res);
		return res;
	}();
	return D2D1Factory;
}

IDWriteFactory* FrameDrawer::GetWriteFactory()
{
	static auto DWriteFactory = []
	{
		IUnknown* res = nullptr;
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &res);
		return reinterpret_cast<IDWriteFactory*>(res);
	}();
	return DWriteFactory;
}

D2D1_COLOR_F FrameDrawer::ConvertColor(COLORREF color)
{
	return D2D1::ColorF(GetRValue(color) / 255.f, GetGValue(color) / 255.f, GetBValue(color) / 255.f, 1.f);
}

D2D1_ROUNDED_RECT FrameDrawer::ConvertRECT(RECT rect, int thickness, float radius)
{
	float halfThickness = thickness / 2.0f;

	auto d2d1Rect = D2D1::RectF(static_cast<float>(rect.left) + halfThickness + 1, static_cast<float>(rect.top) + halfThickness + 1, static_cast<float>(rect.right) - halfThickness - 1, static_cast<float>(rect.bottom) - halfThickness - 1);
	return D2D1::RoundedRect(d2d1Rect, radius, radius);
}

D2D1_RECT_F FrameDrawer::ConvertRECT(RECT rect, int thickness)
{
	float halfThickness = thickness / 2.0f;

	return D2D1::RectF(static_cast<float>(rect.left) + halfThickness + 1, static_cast<float>(rect.top) + halfThickness + 1, static_cast<float>(rect.right) - halfThickness - 1, static_cast<float>(rect.bottom) - halfThickness - 1);
}