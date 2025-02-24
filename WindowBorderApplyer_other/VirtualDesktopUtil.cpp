#include "VirtualDesktopUtil.h"
#include <wil/registry.h>
#include <iostream>

const wchar_t RegKeyVirtualDesktop[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktop";

HKEY OpenVirtualDesktopsRegKey()
{
    HKEY hkey{ nullptr };
    if (RegOpenKeyEx(HKEY_CURRENT_USER, RegKeyVirtualDesktop, 0, KEY_ALL_ACCESS, &hkey))
        return hkey;

    return nullptr;
}

HKEY GetVirtualDesktopRegKey()
{
    static wil::unique_hkey virtualDesktopREGKey{ OpenVirtualDesktopsRegKey() };
    return virtualDesktopREGKey.get();
}

VirtualDesktopUtil::VirtualDesktopUtil()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "Failed to initialize COM library. Error code: " << hr << std::endl;
        return;
    }

    hr = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&vdManager));
    if (FAILED(hr)) {
        std::cerr << "CoCreateInstance failed. Error code: " << hr << std::endl;
        return;
    }
}

VirtualDesktopUtil::~VirtualDesktopUtil()
{
    if (vdManager)
        vdManager->Release();
    CoUninitialize();
}

bool VirtualDesktopUtil::IsWindowsOnCurrentDesktop(HWND window) const
{
    std::optional<GUID> id = GetDesktopId(window);
    return id.has_value();
}

std::optional<GUID> VirtualDesktopUtil::GetDesktopId(HWND window) const
{
    GUID id;
    BOOL IsWindowOnCurrentDesktop = FALSE;
    HRESULT hr = CoInitialize(NULL);
    auto res = vdManager->IsWindowOnCurrentVirtualDesktop(window, &IsWindowOnCurrentDesktop);
    if (vdManager && res == S_OK && IsWindowOnCurrentDesktop)
    {
        if (vdManager->GetWindowDesktopId(window, &id) == S_OK && id != GUID_NULL)
            return id;
    }

    return std::nullopt;
}

