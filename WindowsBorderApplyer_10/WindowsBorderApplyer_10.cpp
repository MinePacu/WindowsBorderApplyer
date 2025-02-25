#include <iostream>
#include <vector>
#include <thread>
#include <Windows.h>
#include <chrono>
#include <set>
#include <dwmapi.h> // DwmSetWindowAttribute 함수를 사용하기 위해 추가
#include <fstream> // 파일 입출력을 위해 추가
#include <iomanip> // std::put_time을 사용하기 위해 추가
#include <ctime> // std::localtime_s을 사용하기 위해 추가
#include <mutex> // std::mutex를 사용하기 위해 추가
#include <filesystem> // 파일 시스템 작업을 위해 추가
#include <Psapi.h> // GetModuleFileNameEx 함수를 사용하기 위해 추가

#pragma comment(lib, "Psapi.lib") // Psapi.lib 라이브러리 링크
#pragma comment(lib, "Dwmapi.lib") // Dwmapi.lib 라이브러리 링크

std::mutex logMutex; // 로그 파일 접근을 위한 뮤텍스
const std::wstring logFileName = L"error_log.txt";
const std::size_t maxLogFileSize = 1024 * 1024; // 1MB

enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// 관리자 권한으로 실행 중인지 확인하는 함수
static bool IsRunAsAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup)) {
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        FreeSid(pAdministratorsGroup);
    }

    return fIsRunAsAdmin;
}

// 관리자 권한으로 재실행하는 함수
static void RestartAsAdmin() {
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
                std::wcout << L"User refused to allow privileges elevation." << std::endl;
            }
        }
    }
}

// 창 핸들러를 수집하는 함수
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

static void rotateLogFile() {
    if (std::filesystem::exists(logFileName) && std::filesystem::file_size(logFileName) >= maxLogFileSize) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c);
        std::wstringstream newLogFileName;
        newLogFileName << L"error_log_" << std::put_time(&now_tm, L"%Y%m%d%H%M%S") << L".txt";
        std::filesystem::rename(logFileName, newLogFileName.str());
    }
}

static void logMessage(LogLevel level, const std::wstring& message, const char* function, int line) {
    std::lock_guard<std::mutex> lock(logMutex);
    rotateLogFile();
    std::wofstream logFile(logFileName, std::ios_base::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c);
        logFile << std::put_time(&now_tm, L"%Y-%m-%d %H:%M:%S") << L" - ";

        switch (level) {
        case LOG_INFO:
            logFile << L"INFO";
            break;
        case LOG_WARNING:
            logFile << L"WARNING";
            break;
        case LOG_ERROR:
            logFile << L"ERROR";
            break;
        }

        logFile << L" [" << function << L":" << line << L"] - " << message << std::endl;
        logFile.close();
    }
}

#define LOG_ERROR(message) logMessage(LOG_ERROR, message, __FUNCTION__, __LINE__)

// 프로세스 이름을 가져오는 함수
static std::wstring getProcessName(HWND hwnd) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processName[MAX_PATH] = L"<unknown>";
        if (GetModuleFileNameEx(hProcess, NULL, processName, MAX_PATH)) {
            CloseHandle(hProcess);
            return std::wstring(processName);
        }
        CloseHandle(hProcess);
    }
    return L"<unknown>";
}

static void setWindowBorderColor(HWND hwnd, COLORREF color) {
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &color, sizeof(color));
    if (FAILED(hr)) {
        std::wstring processName = getProcessName(hwnd);
        std::wstring errorMessage = L"Failed to set window border color for HWND: " + std::to_wstring(reinterpret_cast<uintptr_t>(hwnd)) + L" (Process: " + processName + L") with error code: " + std::to_wstring(hr);
        std::wcerr << errorMessage << std::endl;
        LOG_ERROR(errorMessage);
    }
}

static void printWindowHandles(std::set<HWND>& modifiedWindows, COLORREF color) {
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

        // 창 모서리 색깔을 설정된 색상으로 변경
        setWindowBorderColor(hwnd, color);

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

    // 창 제목을 동적으로 변경
    std::wstring newTitle = L"WindowBorderApplyer - " + std::to_wstring(windowHandles.size()) + L" windows collected";
    SetConsoleTitle(newTitle.c_str());
}

int main(int argc, char* argv[]) {
    // 콘솔 창 제목 설정
    SetConsoleTitle(L"WindowBorderApplyer");

    // 관리자 권한으로 실행 중인지 확인
    if (!IsRunAsAdmin()) {
        RestartAsAdmin();
        return 0;
    }

    // 기본 색상은 하늘색으로 설정
    COLORREF color = RGB(135, 206, 235);

    // 명령줄 인수로 RGB 값을 받아서 색상 설정
    if (argc == 4) {
        int r = std::stoi(argv[1]);
        int g = std::stoi(argv[2]);
        int b = std::stoi(argv[3]);
        color = RGB(r, g, b);
    }

    // 콘솔 출력 코드 페이지를 UTF-8로 설정
    SetConsoleOutputCP(CP_UTF8);
    std::wcout.imbue(std::locale(""));

    std::wcout << L"Press Ctrl+C to exit..." << std::endl;

    // 이미 색깔을 변경한 창 핸들러를 추적하기 위한 집합
    std::set<HWND> modifiedWindows;

    // 주기적으로 작업을 수행하기 위한 스레드
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    std::thread worker([&modifiedWindows, color, hEvent]() {
        while (WaitForSingleObject(hEvent, 10000) == WAIT_TIMEOUT) { // 10초마다 창 핸들러 수집 및 출력
            printWindowHandles(modifiedWindows, color);
        }
        });

    // 메인 스레드는 종료를 기다림
    worker.join();
    CloseHandle(hEvent);

    return 0;
}
