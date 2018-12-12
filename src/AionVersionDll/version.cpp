#define WIN32_LEAN_AND_MEAN
#define _HAS_EXCEPTIONS 0

#include <windows.h>
#include <stdio.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include "detours.h"

static const char s_ipToReplace[16] = "70.5.0.18";
static char s_serverIp[16] = "";
static bool s_needPatch = false;

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

static unsigned long (PASCAL FAR *real_inet_addr)(_In_z_ const char FAR * cp) = inet_addr;
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

static decltype(SetCursorPos) *real_SetCursorPos = SetCursorPos;
BOOL
WINAPI
zzSetCursorPos(
    _In_ int X,
    _In_ int Y) {
    BOOL result = real_SetCursorPos(X, Y);
    HWND hwnd = GetActiveWindow();
    if (hwnd) {

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        WPARAM wParam = 
            (GetKeyState(VK_LBUTTON) & 0x8000) ? 0x0001 : 0 |
            (GetKeyState(VK_RBUTTON) & 0x8000) ? 0x0002 : 0 |
            (GetKeyState(VK_SHIFT) & 0x8000) ? 0x0004 : 0 |
            (GetKeyState(VK_CONTROL) & 0x8000) ? 0x0008 : 0 |
            (GetKeyState(VK_MBUTTON) & 0x8000) ? 0x0010 : 0 |
            (GetKeyState(VK_XBUTTON1) & 0x8000) ? 0x0020 : 0 |
            (GetKeyState(VK_XBUTTON2) & 0x8000) ? 0x0040 : 0;

        PostMessage(hwnd, WM_MOUSEMOVE, wParam, MAKELPARAM(pt.x, pt.y));
    }
    return result;
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

    if (strstr(GetCommandLineA(), "-win10-mouse-fix") > 0) {
        DetourAttach(&(PVOID&)real_SetCursorPos, zzSetCursorPos);
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