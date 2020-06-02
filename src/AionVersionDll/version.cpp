#define WIN32_LEAN_AND_MEAN
#define _HAS_EXCEPTIONS 0
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <string>
#include <intrin.h>
#include <winsock2.h>
#include "detours.h"

static const char s_ipToReplace[16] = "70.5.0.18";
static char s_serverIp[16] = "";
static bool s_needPatch = false;
static bool s_gfxEnabled = false;
bool g_didStartup = false;
bool g_disableIpFix = false;

bool g_isCursorHidden = false;
bool g_windowedMousefixOnly = false;

int rawX = 0, rawY = 0;
bool g_isHandlingMouseDown = false;

bool g_stepGetCursorPos = 0;
int g_stepPanning = 0;

WNDPROC g_realWndProc;


#if 0
CRITICAL_SECTION cs;
static FILE* s_logFile = nullptr;

void init_log() {
    InitializeCriticalSection(&cs);
    s_logFile = _fsopen("version.log", "a+", SH_DENYWR);
}

void log(const char *format, ...) {

    if (!s_logFile) {
        return;
    }

    EnterCriticalSection(&cs);

    va_list(args);
    fprintf(s_logFile, "[%d-tid:%x] ", GetTickCount(), GetCurrentThreadId());
    va_start(args, format);
    vfprintf(s_logFile, format, args);
    fprintf(s_logFile, "\n");
    fflush(s_logFile);

    LeaveCriticalSection(&cs);
}
#else
#define init_log()
#endif

bool GetServerIpFromCommandLine(char* buf, size_t bufsize) {
    const char* substr = strstr(GetCommandLineA(), "-ip:");
    if (substr) {
        int a, b, c, d;
        if (sscanf_s(substr + 4, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            return (unsigned) snprintf(buf, bufsize, "%d.%d.%d.%d", a, b, c, d) < bufsize;
        }
    }
    return false;
}

static decltype(inet_addr) *real_inet_addr = inet_addr;
unsigned long PASCAL FAR zzinet_addr(_In_z_ const char FAR * cp)
{
    if (!g_didStartup) {
        void OnStartup();
        OnStartup();
        g_didStartup = true;
    }

    if (!g_disableIpFix) {
        s_needPatch = true;
        if (!strcmp(cp, s_ipToReplace)) {
            return real_inet_addr(s_serverIp);
        }
    }
    return real_inet_addr(cp);
}

static decltype(lstrcpynA) *real_lstrcpynA = lstrcpynA;
LPSTR
WINAPI
zzlstrcpynA(
    _Out_writes_(iMaxLength) LPSTR lpString1,
    _In_ LPCSTR lpString2,
    _In_ int iMaxLength
) {
    if (s_needPatch) {
        if (!strcmp(lpString2, s_serverIp)) {
            s_needPatch = false;
            return real_lstrcpynA(lpString1, s_ipToReplace, iMaxLength);
        }
    }
    return real_lstrcpynA(lpString1, lpString2, iMaxLength);
}

static decltype(strtok_s) *real_strtok_s = nullptr;
char*  __cdecl zzstrtok_s(
    _Inout_opt_z_                 char*       _String,
    _In_z_                        char const* _Delimiter,
    _Inout_ _Deref_prepost_opt_z_ char**      _Context
) {
    if (s_needPatch && _String && !strcmp(_String, s_serverIp)) {
        strcpy(_String, s_ipToReplace);
        s_needPatch = false;
    }
    return strtok_s(_String, _Delimiter, _Context);
}


HWND clientHwnd = nullptr;
int clientLeft, clientTop;
bool isCursorHidden;
bool mouseHookInstalled = false;

int MakeMouseMoveWPARAM()
{
    return
        (GetKeyState(VK_LBUTTON) & 0x8000) ? 0x0001 : 0 |
        (GetKeyState(VK_RBUTTON) & 0x8000) ? 0x0002 : 0 |
        (GetKeyState(VK_SHIFT) & 0x8000) ? 0x0004 : 0 |
        (GetKeyState(VK_CONTROL) & 0x8000) ? 0x0008 : 0 |
        (GetKeyState(VK_MBUTTON) & 0x8000) ? 0x0010 : 0 |
        (GetKeyState(VK_XBUTTON1) & 0x8000) ? 0x0020 : 0 |
        (GetKeyState(VK_XBUTTON2) & 0x8000) ? 0x0040 : 0;
}

LPARAM MakeMouseMoveLPARAM(int x, int y) {
    return MAKELPARAM(x - clientLeft, y - clientTop);
}

bool AllButtonsUp() {
    return !(GetKeyState(VK_LBUTTON) & 0x8000) && !(GetKeyState(VK_RBUTTON) & 0x8000) && !(GetKeyState(VK_MBUTTON) & 0x8000);
}

HWND GetAionHwnd() {
    if (!clientHwnd) {
        HWND hwnd = GetActiveWindow();
        if (hwnd) {
            char name[64];
            GetClassNameA(hwnd, name, 64);
            if (!strncmp(name, "AIONClient", 10)) {
                clientHwnd = hwnd;
            }
        }
    }
    return clientHwnd;
}

bool IsWindowedMode() {
    return GetAionHwnd() && ((GetWindowLong(GetAionHwnd(), GWL_STYLE) & WS_CAPTION));
}

RAWINPUT* GetRawInput(HRAWINPUT ri) {

    UINT dwSize;

    GetRawInputData(ri, RID_INPUT, NULL, &dwSize,
        sizeof(RAWINPUTHEADER));
    LPBYTE lpb = new BYTE[dwSize];
    if (lpb == NULL)
    {
        return 0;
    }

    if (GetRawInputData(ri, RID_INPUT, lpb, &dwSize,
        sizeof(RAWINPUTHEADER)) != dwSize) {
        //log("GetRawInputData does not return correct size !");
    }

    RAWINPUT* raw = (RAWINPUT*)lpb;
    return raw;
}

LRESULT CALLBACK RawInputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (IsWindowedMode()) {
        switch (message) {
            case WM_INPUT:  
                {
                    g_stepGetCursorPos = 0;

                    RAWINPUT* raw = GetRawInput((HRAWINPUT)lParam);
                    if (!raw)
                        return 0;

                    if (raw->header.dwType != RIM_TYPEMOUSE)
                        return 0;

                    int newX = rawX + raw->data.mouse.lLastX;
                    int newY = rawY + raw->data.mouse.lLastY;
                    
                    rawX = newX;
                    rawY = newY;

                    if (g_isCursorHidden) {
                        CallWindowProc(g_realWndProc, hWnd, WM_MOUSEMOVE, MakeMouseMoveWPARAM(), MakeMouseMoveLPARAM(rawX, rawY));
                        g_stepPanning++;
                    }
                    else if (!g_isCursorHidden) {
                        ClipCursor(nullptr);
                        g_stepPanning = 0;
                    }
                    return 0;
                }
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_XBUTTONDOWN:
                {
                    g_isHandlingMouseDown = true;
                    LRESULT r = CallWindowProc(g_realWndProc, hWnd, message, wParam, lParam);
                    g_isHandlingMouseDown = false;
                    return r;
                }
            case WM_MOUSEMOVE:
                if (g_isCursorHidden) {

                    // WM_MOUSEMOVE is generated by the WM_INPUT handler
                    return 0;
                }
            case WM_CLOSE:
            case WM_DESTROY:
            case WM_NCDESTROY:
            case WM_KILLFOCUS:
                ClipCursor(nullptr);
                break;
        }
    }
    else if (message == WM_INPUT) {
        return 0;
    }
    return CallWindowProc(g_realWndProc, hWnd, message, wParam, lParam);
}

bool g_rawInputFixInstalled = false;
void InstallRawInputFix() {
    HWND hwnd = GetAionHwnd();
    if (!hwnd)
        return;

    g_realWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)RawInputWndProc);
    
    RAWINPUTDEVICE Rid;
    Rid.usUsagePage = 1;
    Rid.usUsage = 2;
    Rid.dwFlags = 0;
    Rid.hwndTarget = hwnd;

    RegisterRawInputDevices(&Rid, 1, sizeof(Rid));
    
    g_rawInputFixInstalled = true;
}

static decltype(GetCursorPos) *real_GetCursorPos = GetCursorPos;
static decltype(SetCursorPos) *real_SetCursorPos = SetCursorPos;
BOOL
WINAPI
zzSetCursorPos(
    _In_ int X,
    _In_ int Y) {

    BOOL result = TRUE;

    if (!IsWindowedMode())
    {
        result = real_SetCursorPos(X, Y);

        if (!g_windowedMousefixOnly && GetActiveWindow() == clientHwnd) {
            POINT pt;
            real_GetCursorPos(&pt);
            ScreenToClient(clientHwnd, &pt);

            PostMessage(clientHwnd, WM_MOUSEMOVE, MakeMouseMoveWPARAM(), MAKELPARAM(pt.x, pt.y));
        }
    }
    else
    {
        if (!g_rawInputFixInstalled) {
            InstallRawInputFix();
        }

        if (g_stepGetCursorPos == 0 && g_rawInputFixInstalled && g_isHandlingMouseDown) {
            rawX = X;
            rawY = Y;
        }
    }
    return result;
}

BOOL
WINAPI
zzGetCursorPos(
    _Out_ LPPOINT lpPoint) {

    if (IsWindowedMode())
    {
        g_stepGetCursorPos++;

        if (g_stepGetCursorPos == 1 && g_stepPanning > 0 && g_rawInputFixInstalled && g_isCursorHidden) {
            lpPoint->x = rawX;
            lpPoint->y = rawY;
            return TRUE;
        }
    }
    return real_GetCursorPos(lpPoint);
}

static decltype(SetCursor) *real_SetCursor = SetCursor;
HCURSOR
WINAPI
zzSetCursor(
    _In_opt_ HCURSOR hCursor) {

    if (IsWindowedMode()) {
        bool wasHidden = g_isCursorHidden;

        g_isCursorHidden = !hCursor;

        if (g_rawInputFixInstalled) {
            if (g_isCursorHidden && !wasHidden && GetFocus() == GetAionHwnd()) {
                // lock the cursor pos while hidden
                RECT rc;
                real_GetCursorPos((LPPOINT)&rc);
                rc.right = rc.left + 1;
                rc.bottom = rc.top + 1;
                ClipCursor(&rc);
            }
            else {
                ClipCursor(nullptr);
            }
        }
    }
    return real_SetCursor(hCursor);
}

void EnableHighQualityGraphicsOptions()
{
    MEMORY_BASIC_INFORMATION mbi = {};
    if (!VirtualQuery(GetModuleHandle(L"crysystem") + 0x1000, &mbi, sizeof(mbi))) {
        return;
    }

    if (!(mbi.AllocationProtect & 0xF0)) {
        return;
    }

    DWORD oldProtect;
    if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return;
    }

    char* c = (char*)mbi.BaseAddress;
    char* end = c + mbi.RegionSize - sizeof(DWORD);

    for (; c < end; c++) {
        DWORD* d = (DWORD*)c;
        if (*d == 1920 * 1200) {
            *d = 4096 * 4096;
            char* e = c - 0x100;
            char* e_end = c + 0x100;
            e_end = min(end, e_end);
            for (; e < e_end; e++) {
                DWORD* d2 = (DWORD*)e;
                if (*d2 == 2560 * 1600) {
                    *d2 = 4096 * 4096;
                    break;
                }
            }
            break;
        }
    }

    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, oldProtect, &oldProtect);
}

void InstallHighQualityGraphicsFix() {
    if (!s_gfxEnabled) {
        EnableHighQualityGraphicsOptions();
        s_gfxEnabled = true;
    }
}

static decltype(ChangeDisplaySettingsA) *real_ChangeDisplaySettingsA = ChangeDisplaySettingsA;
LONG
WINAPI
zzChangeDisplaySettingsA(
    _In_opt_ DEVMODEA* lpDevMode,
    _In_ DWORD dwFlags) {
    InstallHighQualityGraphicsFix();
    return real_ChangeDisplaySettingsA(lpDevMode, dwFlags);
}

BYTE* FindBytePattern(BYTE* start, int search_len, BYTE* pattern, int pattern_len) {
    for (BYTE *end = start + search_len; start < end; start++) {
        if (*start == *pattern) {
            if (!memcmp(start, pattern, pattern_len)) {
                return start;
            }
        }
    }
    return nullptr;
}

static decltype(LoadLibraryW) *real_LoadLibraryW = LoadLibraryW;
_Ret_maybenull_
HMODULE
WINAPI
zzLoadLibraryW(
    _In_ LPCWSTR lpLibFileName
) {
    // intercept xigncode
    if (wcsstr(lpLibFileName, L".xem")) {

        // jump to the success branch in the caller:

#if defined(_M_AMD64)

        // mov eax, 1
        BYTE pattern[] = { 0xb8, 1, 0, 0, 0 };

        BYTE* r = FindBytePattern((BYTE*)_ReturnAddress(), 1000, pattern, 5);

#elif defined(_M_IX86)

        //004091b2 33c0            xor     eax,eax
        //004091b4 83c40c          add     esp,0Ch
        //004091b7 40              inc     eax
        BYTE pattern[] = { 0x33, 0xc0, 0x83, 0xc4, 0x0c, 0x40 };

        BYTE* r = FindBytePattern((BYTE*)_ReturnAddress(), 1000, pattern, 6);

#endif

        if (r) {
            *(intptr_t*)_AddressOfReturnAddress() = (intptr_t)r;
            return nullptr;
        }
    }

    return real_LoadLibraryW(lpLibFileName);
}

static decltype(MessageBoxW) *real_MessageBoxW = MessageBoxW;
int
WINAPI
zzMessageBoxW(
    _In_opt_ HWND hWnd,
    _In_opt_ LPCWSTR lpText,
    _In_opt_ LPCWSTR lpCaption,
    _In_ UINT uType) {

    //Suppress the popup:
    //Your PC has a high likelihood of getting hacked into or infected by viruses.\n Install vaccines to scan your computer, and use appropriate security services.
    if (lpText && !wcsncmp(lpText, L"Your PC", 7)) {
        return MB_OK;
    }
    return real_MessageBoxW(hWnd, lpText, lpCaption, uType);
}

void InstallWeaponSwitchFix() {
    MEMORY_BASIC_INFORMATION mbi = {};
    if (!VirtualQuery(GetModuleHandle(L"game") + 0x1000, &mbi, sizeof(mbi))) {
        return;
    }

    if (!(mbi.AllocationProtect & 0xF0)) {
        return;
    }

    DWORD oldProtect;
    if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return;
    }

    bool foundChangeWeapon = false;
    bool foundSkillbarWeapon = false;

    char* c = (char*)mbi.BaseAddress;
    char* end = c + mbi.RegionSize - sizeof(DWORD);


#if defined(_M_AMD64)
    
    for (; c < end; c++) {
        DWORD* d = (DWORD*)c;
        if (*d == 1300920) {
            if (!foundChangeWeapon) {
                // change je to jge:
                // 4183FB35      cmp  r11d, 35
                // 0F8488000000  je   Game + 3E8D9
                BYTE pattern[] = { 0x35, 0x0f, 0x84 };
                BYTE* p = FindBytePattern((BYTE*)d - 150, 150, pattern, 3);
                if (p) {
                    p[2] = 0x8D;
                    foundChangeWeapon = true;
                    continue;
                }
            }
            if (!foundSkillbarWeapon) {
                // 74 54     je
                BYTE pattern[] = { 0x74, 0x54 };
                BYTE* p = FindBytePattern((BYTE*)d - 100, 100, pattern, 2);
                if (p) {
                    p[0] = 0xEB;
                    foundSkillbarWeapon = true;
                }
            }
            if (foundChangeWeapon && foundSkillbarWeapon) {
                break;
            }
        }
    }


#elif defined(_M_IX86)

    for (; c < end; c++) {
        DWORD* d = (DWORD*)c;
        if (*d == 1300920) {
            if (!foundChangeWeapon) {
                // change je to jge:
                // 83bd34ffffff35  cmp     dword ptr[ebp - 0CCh], 35h
                // 744e            je      Game + 0x46ec
                BYTE pattern[] = { 0x35, 0x74 };
                BYTE* p = FindBytePattern((BYTE*)d - 150, 150, pattern, 2);
                if (p) {
                    p[1] = 0x7D;
                    foundChangeWeapon = true;
                    continue;
                }
            }
            if (!foundSkillbarWeapon) {
                // 74 31     je Game+188DB
                BYTE pattern[] = { 0x74, 0x31 };
                BYTE* p = FindBytePattern((BYTE*)d - 100, 100, pattern, 2);
                if (p) {
                    p[0] = 0xEB;
                    foundSkillbarWeapon = true;
                }
            }
            if (foundChangeWeapon && foundSkillbarWeapon) {
                break;
            }
        }
    }

#endif

    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, oldProtect, &oldProtect);
}

bool GetOSVersion(RTL_OSVERSIONINFOW& osver) {
    osver = {sizeof(osver)};
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion) {
            if (RtlGetVersion(&osver) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool NeedsWin10MouseFix() {
    RTL_OSVERSIONINFOW osver;
    return (GetOSVersion(osver) && osver.dwMajorVersion == 10 &&
        osver.dwMinorVersion >= 0 && osver.dwBuildNumber >= 15063);
}

void InstallPatch() 
{
    init_log();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    g_disableIpFix = strstr(GetCommandLineA(), "-disable-ip-fix");
    DetourAttach(&(PVOID&)real_inet_addr, zzinet_addr);

    if (!g_disableIpFix) {
        if (GetServerIpFromCommandLine(s_serverIp, sizeof(s_serverIp))) {
            // 4.x and earlier
            DetourAttach(&(PVOID&)real_lstrcpynA, zzlstrcpynA);

            // 5.x
            HMODULE hmoduleMsvcr120 = GetModuleHandleA("msvcr120");
            if (hmoduleMsvcr120) {
                s_needPatch = true;
                *(void**)&real_strtok_s = GetProcAddress(hmoduleMsvcr120, "strtok_s");
                DetourAttach(&(PVOID&)real_strtok_s, zzstrtok_s);
            }
        }
    }

    bool installMouseFix = false;
    if (strstr(GetCommandLineA(), "-win10-mouse-fix-autodetect") > 0) {
        installMouseFix = NeedsWin10MouseFix();
    } else if (strstr(GetCommandLineA(), "-win10-mouse-fix") > 0) {
        installMouseFix = true;
    } 
    
    if (!installMouseFix && strstr(GetCommandLineA(), "-windowed-mouse-fix") > 0) {
        installMouseFix = true;
        g_windowedMousefixOnly = true;
    }

    if (installMouseFix) {
        DetourAttach(&(PVOID&)real_SetCursorPos, zzSetCursorPos);
        DetourAttach(&(PVOID&)real_GetCursorPos, zzGetCursorPos);
        DetourAttach(&(PVOID&)real_SetCursor, zzSetCursor);
    }

    if (strstr(GetCommandLineA(), "-unlimited-gfx") > 0) {
        DetourAttach(&(PVOID&)real_ChangeDisplaySettingsA, zzChangeDisplaySettingsA);
    }

    if (strstr(GetCommandLineA(), "-disable-xigncode")) {
        DetourAttach(&(PVOID&)real_LoadLibraryW, zzLoadLibraryW);
        DetourAttach(&(PVOID&)real_MessageBoxW, zzMessageBoxW);
    }

    LONG error = DetourTransactionCommit();
}

void OnStartup() {
    InstallHighQualityGraphicsFix();

    if (strstr(GetCommandLineA(), "-fix-39-weapon-switch") > 0) {
        InstallWeaponSwitchFix();
    }
}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        InstallPatch();
    }
    return TRUE;
}