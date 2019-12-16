#define WIN32_LEAN_AND_MEAN
#define _HAS_EXCEPTIONS 0
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <winsock2.h>
#include "detours.h"

static const char s_ipToReplace[16] = "70.5.0.18";
static char s_serverIp[16] = "";
static bool s_needPatch = false;
static bool s_gfxEnabled = false;

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
    s_needPatch = true;
    if (!strcmp(cp, s_ipToReplace)) {
        return real_inet_addr(s_serverIp);
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


HWND clientHwnd = nullptr;
HWND activeHwnd = nullptr;
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

bool getIsCursorHidden() {
    CURSORINFO ci = {sizeof(ci)};
    GetCursorInfo(&ci);
    return (ci.flags & 3) == 0;
}

HWND GetAionHwnd() {
    if (!clientHwnd) {
        clientHwnd = GetActiveWindow();
    }
    return clientHwnd;
}

bool IsWindowedMode() {
    return GetAionHwnd() && ((GetWindowLong(GetAionHwnd(), GWL_STYLE) & WS_CAPTION));
}

void WindowedMouseHookProc(int msg, MSLLHOOKSTRUCT* hs) {
    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            if (!clientHwnd || GetForegroundWindow() != clientHwnd) {
                return;
            }

            if (!activeHwnd) {
                WINDOWINFO windowinfo = {sizeof(WINDOWINFO)};
                GetWindowInfo(clientHwnd, &windowinfo);
                clientLeft = windowinfo.rcClient.left;
                clientTop = windowinfo.rcClient.top;
                activeHwnd = clientHwnd;
                isCursorHidden = false;
            }
            break;
        case WM_MOUSEMOVE:
            if (!activeHwnd) {
                break;
            }

            if (AllButtonsUp()) {
                activeHwnd = nullptr;
                break;
            }

            if (!isCursorHidden) {
                isCursorHidden = getIsCursorHidden();
                if (!isCursorHidden) {
                    break;
                }
            }

            SendMessage(activeHwnd, WM_MOUSEMOVE, MakeMouseMoveWPARAM(), MakeMouseMoveLPARAM(hs->pt.x, hs->pt.y));
            break;
    }
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        if (IsWindowedMode()) {
            WindowedMouseHookProc((int)wParam, (MSLLHOOKSTRUCT*)lParam);
        } else {
            // uninstalling the hook in fullscreen modes reduces an issue with the camera jumping/spinning.
            PostThreadMessage(GetCurrentThreadId(), WM_QUIT, 0, 0);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

DWORD WINAPI MouseFixThread(LPVOID lParam)
{
    HHOOK hook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(0), 0);
    if (hook) {
        MSG message;
        while (GetMessage(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        UnhookWindowsHookEx(hook);
        mouseHookInstalled = false;
    }
    return 0;
}

void InstallHookIfNeeded() {
    if (!mouseHookInstalled) {
        if (IsWindowedMode()) {
            mouseHookInstalled = true;
            CreateThread(nullptr, 0, MouseFixThread, nullptr, 0, nullptr);
        }
    }
}

static decltype(SetCursorPos) *real_SetCursorPos = SetCursorPos;
BOOL
WINAPI
zzSetCursorPos(
    _In_ int X,
    _In_ int Y) {

    BOOL result = real_SetCursorPos(X, Y);

    InstallHookIfNeeded();

    if (!IsWindowedMode())
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(clientHwnd, &pt);

        PostMessage(clientHwnd, WM_MOUSEMOVE, MakeMouseMoveWPARAM(), MAKELPARAM(pt.x, pt.y));
    }
    return result;
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

static decltype(ChangeDisplaySettingsA) *real_ChangeDisplaySettingsA = ChangeDisplaySettingsA;
LONG
WINAPI
zzChangeDisplaySettingsA(
    _In_opt_ DEVMODEA* lpDevMode,
    _In_ DWORD dwFlags) {
    if (!s_gfxEnabled) {
        EnableHighQualityGraphicsOptions();
        s_gfxEnabled = true;
    }
    return real_ChangeDisplaySettingsA(lpDevMode, dwFlags);
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
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (!strstr(GetCommandLineA(), "-disable-ip-fix")) {
        if (GetServerIpFromCommandLine(s_serverIp, sizeof(s_serverIp))) {
            DetourAttach(&(PVOID&)real_inet_addr, zzinet_addr);
            DetourAttach(&(PVOID&)real_lstrcpynA, zzlstrcpynA);
        }
    }

    if (strstr(GetCommandLineA(), "-win10-mouse-fix-autodetect") > 0) {
        if (NeedsWin10MouseFix()) {
            DetourAttach(&(PVOID&)real_SetCursorPos, zzSetCursorPos);
        }
    } else if (strstr(GetCommandLineA(), "-win10-mouse-fix") > 0) {
        DetourAttach(&(PVOID&)real_SetCursorPos, zzSetCursorPos);
    }

    if (strstr(GetCommandLineA(), "-unlimited-gfx") > 0) {
        DetourAttach(&(PVOID&)real_ChangeDisplaySettingsA, zzChangeDisplaySettingsA);
    }

    LONG error = DetourTransactionCommit();
}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        InstallPatch();
    }
    return TRUE;
}