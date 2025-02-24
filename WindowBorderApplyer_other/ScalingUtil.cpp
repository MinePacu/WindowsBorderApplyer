#include "ScalingUtil.h"
#include <ShellScalingApi.h>

float ScalingUtil::ScalingF(HWND window) noexcept
{
	UINT dpi = 96;
	auto targetdisplay = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

	if (targetdisplay != nullptr)
	{
		UINT dummy = 0;
		auto res = GetDpiForMonitor(targetdisplay, MDT_EFFECTIVE_DPI, &dpi, &dummy);

		if (res != S_OK)
			return 1.0f;

		return dpi / 96.0f;
	}

	else
		return 1.0f;
}