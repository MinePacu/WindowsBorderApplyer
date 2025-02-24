#pragma once
#include <windows.h>

class CaptionColorUtil
{
public:
	CaptionColorUtil(COLORREF color);

	COLORREF GetSettingCaptionColor();
	bool SetSettingCaptionColor(COLORREF color);

	bool SetCaptionColorToWindow(HWND window);

	bool ResetDwmNonClient(HWND window);

private:
	COLORREF CaptionColor{};
};