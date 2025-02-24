#include "VirtualDesktopUtil.h"
// #include <wil/registry.h> // Commenting out the problematic include

#include <windows.h> // Adding necessary includes
#include <shlwapi.h>

const wchar_t RegKeyVirtualDesktop[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktop";

HKEY OpenVirtualDesktopsRegKey()
{
    HKEY hkey{nullptr};
    if (RegOpenKeyEx(HKEY_CURRENT_USER, RegKeyVirtualDesktop, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
        return hkey;

    return nullptr;
}

HKEY GetVirtualDesktopRegKey()
{
    static HKEY virtualDesktopREGKey{ OpenVirtualDesktopsRegKey() };
    return virtualDesktopREGKey;
}

VirtualDesktopUtil::VirtualDesktopUtil()
{
    auto res = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&vdManager));
    if (FAILED(res))
        return;
}

VirtualDesktopUtil::~VirtualDesktopUtil()
{
    if (vdManager)
        vdManager->Release();
}

bool VirtualDesktopUtil::IsWindowsOnCurrentDesktop(HWND window) const
{
    auto id = GetDesktopId(window);
    return id.has_value();
}

std::optional<GUID> VirtualDesktopUtil::GetDesktopId(HWND window) const
{
    GUID id;
    BOOL IsWindowOnCurrentDesktop = FALSE;
    if (vdManager && vdManager->IsWindowOnCurrentVirtualDesktop(window, &IsWindowOnCurrentDesktop) == S_OK && IsWindowOnCurrentDesktop)
    {
        if (vdManager->GetWindowDesktopId(window, &id) == S_OK && id != GUID_NULL)
            return id;
    }

    return std::nullopt;
}