#include <shlwapi.h>
#include <stdio.h>
#include <windows.h>

// https://stackoverflow.com/a/8196291
BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

// https://www.codeproject.com/Answers/5272264/How-can-I-use-strcat-s-with-two-strings-to-print-i
void Trim(wchar_t* str)
{
    wchar_t* pos;
    if ((pos = wcschr(str, L'\n')) != NULL)
    {
        *pos = L'\0';
    }
}

void GetWSAPath(wchar_t* launchCmdline) {
    wchar_t AppDataPath[512];
    size_t AppDataPathSize;
    wchar_t executablePath[1024];
    wchar_t launchProcess[4096];
    _wgetenv_s(&AppDataPathSize, AppDataPath, L"LOCALAPPDATA");
    PathCombine(executablePath, AppDataPath, L"Microsoft\\WindowsApps\\MicrosoftCorporationII.WindowsSubsystemForAndroid_8wekyb3d8bbwe\\WsaClient.exe");
    swprintf_s(launchProcess, L"\"%s\"", executablePath);
    wcscat_s(launchCmdline, 4096, launchProcess);
    return;
}

int main(int argc, char** argv) {
    BOOL elevated = IsElevated();
    HWND hWnd = GetShellWindow();
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);
    HANDLE process = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, pid);
    SIZE_T size = NULL;
    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    auto p = (PPROC_THREAD_ATTRIBUTE_LIST)new char[size];
    InitializeProcThreadAttributeList(p, 1, 0, &size);
    STARTUPINFOEX StartupInfoEX = {};
    PROCESS_INFORMATION ProcessInfo;
    ULONG ReturnCode;
    wchar_t launchCmdline[8192];
    
    printf("WSAClientWrapper\nhttps://github.com/samicrusader/WSAClientWrapper\n--\n");
    if (elevated) {
        printf("You are running as administrator, elevating!\n");
        UpdateProcThreadAttribute(p, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &process, sizeof(process), NULL, NULL);
        StartupInfoEX.lpAttributeList = p;
    }
    else {
        printf("You are already a normal user, skipping elevation!\n");
    }
    StartupInfoEX.StartupInfo.cb = sizeof(StartupInfoEX);
    
    wcscpy_s(launchCmdline, 8192, L"");
    GetWSAPath(launchCmdline);
    for (int i = 1; argv[i] != NULL; i++) {
        wchar_t* vOut = new wchar_t[strlen(argv[i]) + 1];
        mbstowcs_s(NULL, vOut, strlen(argv[i]) + 1, argv[i], strlen(argv[i]));
        wcscat_s(launchCmdline, 4096, L" ");
        wcscat_s(launchCmdline, 4096, vOut);
    }
    wprintf(L"Launching WSAClient.exe with: %s...\n", launchCmdline);
    CreateProcessW(NULL, launchCmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE | EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &StartupInfoEX.StartupInfo, &ProcessInfo);
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    if (!GetExitCodeProcess(ProcessInfo.hProcess, &ReturnCode))
        ReturnCode = 0;
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
    delete[](char*)p;
    CloseHandle(process);

    return 0;
}