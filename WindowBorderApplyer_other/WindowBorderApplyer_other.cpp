#include <windows.h>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <iostream>
#include "Windowmodule.h" // Change from FrameDrawer.h to Windowmodule.h

std::unordered_set<HWND> processedWindows;
std::mutex mtx;

BOOL IsRunAsAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(
        &NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup)) {
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        FreeSid(pAdministratorsGroup);
    }

    return fIsRunAsAdmin;
}

void RestartAsAdmin() {
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteEx(&sei)) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_CANCELLED) {
                std::cerr << "User refused to allow privileges elevation." << std::endl;
            }
        }
    }
}

void ClearConsole() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (hConsole == INVALID_HANDLE_VALUE) return;

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count)) return;

    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) return;

    SetConsoleCursorPosition(hConsole, homeCoords);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    std::lock_guard<std::mutex> lock(mtx);
    if (processedWindows.find(hwnd) == processedWindows.end()) {
        processedWindows.insert(hwnd);
    }
    return TRUE;
}

static std::vector<HWND> collectWindowHandles() {
    std::vector<HWND> windowHandles;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
            auto& handles = *reinterpret_cast<std::vector<HWND>*>(lParam);
            handles.push_back(hwnd);
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&windowHandles));

    return windowHandles;
}

static void printWindowHandles(Windowmodule& windowModule, std::unordered_set<HWND>& modifiedWindows) {
    // 창 핸들러 수집
    auto windowHandles = collectWindowHandles();

    // 콘솔 커서를 맨 위로 이동
    COORD coord = { 0, 0 };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

    std::wcout << L"Collected " << windowHandles.size() << L" window handles:" << std::endl;
    for (const auto& hwnd : windowHandles) {
        std::wcout << hwnd << std::endl;

        // 이미 색깔을 변경한 창 핸들러는 건너뜀
        if (modifiedWindows.find(hwnd) != modifiedWindows.end()) {
            continue;
        }

        // Windowmodule을 사용하여 창의 모서리를 추가합니다.
        windowModule.AddHwnd(hwnd);

        // 변경한 창 핸들러를 집합에 추가
        modifiedWindows.insert(hwnd);
    }

    // 닫힌 창 핸들러를 집합에서 제거
    for (auto it = modifiedWindows.begin(); it != modifiedWindows.end(); ) {
        if (!IsWindow(*it)) {
            it = modifiedWindows.erase(it);
        }
        else {
            ++it;
        }
    }
}

int main() {
    if (!IsRunAsAdmin()) {
        RestartAsAdmin();
        return 0;
    }

    std::wcout << L"Press Ctrl+C to exit..." << std::endl;

    // 이미 색깔을 변경한 창 핸들러를 추적하기 위한 집합
    std::unordered_set<HWND> modifiedWindows;

    // Windowmodule 객체를 미리 생성합니다.
    Windowmodule windowModule(255, 165, 0, RGB(255, 165, 0)); // 주황색으로 설정

    // 주기적으로 작업을 수행하기 위한 스레드
    std::thread worker([&windowModule, &modifiedWindows]() {
        while (true) {
            printWindowHandles(windowModule, modifiedWindows);
            std::this_thread::sleep_for(std::chrono::seconds(10)); // 10초마다 창 핸들러 수집 및 출력
        }
        });

    // 메인 스레드는 종료를 기다림
    worker.join();

    return 0;
}
