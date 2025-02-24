#pragma once

#include <ShObjIdl.h>
#include <optional>

class VirtualDesktopUtil
{
public:
	VirtualDesktopUtil();
	~VirtualDesktopUtil();

	bool IsWindowsOnCurrentDesktop(HWND window) const;

	std::optional<GUID> GetDesktopId(HWND window) const;

private:
	IVirtualDesktopManager* vdManager;
};