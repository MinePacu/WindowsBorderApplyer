#include "CaptionColorUtil.h"
#include <dwmapi.h>

CaptionColorUtil::CaptionColorUtil(COLORREF color) : CaptionColor(color) {}

COLORREF CaptionColorUtil::GetSettingCaptionColor()
{
	return CaptionColor;
}

/// <summary>
/// 캡션 색을 설정합니다.
/// </summary>
/// <param name="color"> - 캡션 색</param>
/// <returns>이 유틸 객체에 대하여 캡션 색이 올바르게 설정되면 true, 아니면 false를 반환합니다.</returns>
bool CaptionColorUtil::SetSettingCaptionColor(COLORREF color)
{
	CaptionColor = color;
	if (CaptionColor)
		return true;
	else
		return false;
}

bool CaptionColorUtil::SetCaptionColorToWindow(HWND window)
{
	auto hr = DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &CaptionColor, sizeof(CaptionColor));
	if (!SUCCEEDED(hr))
		return false;

	return true;
}

bool CaptionColorUtil::ResetDwmNonClient(HWND window)
{
	/*
	BOOL isNonClientRenderEnabled{ FALSE };
	auto hr = DwmGetWindowAttribute(window, DWMWA_NCRENDERING_ENABLED, &isNonClientRenderEnabled, sizeof(isNonClientRenderEnabled));

	if (!SUCCEEDED(hr))
		return false;

	if (isNonClientRenderEnabled == FALSE)
		return false;

	auto hr_ = S_OK;
	DWMNCRENDERINGPOLICY renderingPolicy = DWMNCRP_DISABLED;
	hr_ = DwmSetWindowAttribute(window, DWMWA_NCRENDERING_POLICY, &renderingPolicy, sizeof(renderingPolicy));
	if (!SUCCEEDED(hr_))
		return false;

	renderingPolicy = DWMNCRP_ENABLED;
	hr_ = DwmSetWindowAttribute(window, DWMWA_NCRENDERING_POLICY, &renderingPolicy, sizeof(renderingPolicy));
	if (!SUCCEEDED(hr_))
		return false;

	return true;
	*/
	COLORREF color = 0xFFFFFFFF;

	if (!SUCCEEDED(DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &color, sizeof(color))))
		return false;

	return true;
}
