#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HMODULE s_realVersionModule = nullptr;

void* FindRealAddress(PCSTR name)
{
    if (!s_realVersionModule)
    {
        WCHAR path[MAX_PATH];
        GetSystemDirectory(path, ARRAYSIZE(path));
        wcscat_s(path, L"\\version.dll");
        s_realVersionModule = LoadLibrary(path);
    }

    return GetProcAddress(s_realVersionModule, name);
}

static void* real_VerFindFileA = nullptr;
DWORD
APIENTRY
zzVerFindFileA(
    _In_                         DWORD uFlags,
    _In_                         LPCSTR szFileName,
    _In_opt_                     LPCSTR szWinDir,
    _In_                         LPCSTR szAppDir,
    _Out_writes_(*puCurDirLen)   LPSTR szCurDir,
    _Inout_                      PUINT puCurDirLen,
    _Out_writes_(*puDestDirLen)  LPSTR szDestDir,
    _Inout_                      PUINT puDestDirLen
) {
    if (!real_VerFindFileA) {
        real_VerFindFileA = FindRealAddress("VerFindFileA");
    }
    return ((decltype(*::VerFindFileA)*)real_VerFindFileA)
        (uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
}

static void* real_VerFindFileW = nullptr;
DWORD APIENTRY zzVerFindFileW(
    _In_                         DWORD uFlags,
    _In_                         LPCWSTR szFileName,
    _In_opt_                     LPCWSTR szWinDir,
    _In_                         LPCWSTR szAppDir,
    _Out_writes_(*puCurDirLen)   LPWSTR szCurDir,
    _Inout_                      PUINT puCurDirLen,
    _Out_writes_(*puDestDirLen)  LPWSTR szDestDir,
    _Inout_                      PUINT puDestDirLen
) {
    if (!real_VerFindFileW) {
        real_VerFindFileW = FindRealAddress("VerFindFileW");
    }
    return ((decltype(*::VerFindFileW)*)real_VerFindFileW)
        (uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
}

static void* real_VerInstallFileA = nullptr;
DWORD APIENTRY zzVerInstallFileA(
    _In_                         DWORD uFlags,
    _In_                         LPCSTR szSrcFileName,
    _In_                         LPCSTR szDestFileName,
    _In_                         LPCSTR szSrcDir,
    _In_                         LPCSTR szDestDir,
    _In_                         LPCSTR szCurDir,
    _Out_writes_(*puTmpFileLen)  LPSTR szTmpFile,
    _Inout_                      PUINT puTmpFileLen
) {
    if (!real_VerInstallFileA) {
        real_VerInstallFileA = FindRealAddress("VerInstallFileA");
    }
    return ((decltype(*::VerInstallFileA)*)real_VerInstallFileA)
        (uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
}

static void* real_VerInstallFileW = nullptr;
DWORD APIENTRY zzVerInstallFileW(
    _In_                         DWORD uFlags,
    _In_                         LPCWSTR szSrcFileName,
    _In_                         LPCWSTR szDestFileName,
    _In_                         LPCWSTR szSrcDir,
    _In_                         LPCWSTR szDestDir,
    _In_                         LPCWSTR szCurDir,
    _Out_writes_(*puTmpFileLen)  LPWSTR szTmpFile,
    _Inout_                      PUINT puTmpFileLen
) {
    if (!real_VerInstallFileW) {
        real_VerInstallFileW = FindRealAddress("VerInstallFileW");
    }
    return ((decltype(*::VerInstallFileW)*)real_VerInstallFileW)
        (uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
}

static void* real_GetFileVersionInfoSizeA = nullptr;
DWORD APIENTRY zzGetFileVersionInfoSizeA(
    _In_        LPCSTR lptstrFilename, /* Filename of version stamped file */
    _Out_opt_ LPDWORD lpdwHandle       /* Information for use by GetFileVersionInfo */
) {
    if (!real_GetFileVersionInfoSizeA) {
        real_GetFileVersionInfoSizeA = FindRealAddress("GetFileVersionInfoSizeA");
    }
    return ((decltype(*::GetFileVersionInfoSizeA)*)real_GetFileVersionInfoSizeA)
        (lptstrFilename, lpdwHandle);
}

static void* real_GetFileVersionInfoSizeW = nullptr;
DWORD APIENTRY zzGetFileVersionInfoSizeW(
    _In_        LPCWSTR lptstrFilename, /* Filename of version stamped file */
    _Out_opt_ LPDWORD lpdwHandle       /* Information for use by GetFileVersionInfo */
) {
    if (!real_GetFileVersionInfoSizeW) {
        real_GetFileVersionInfoSizeW = FindRealAddress("GetFileVersionInfoSizeW");
    }
    return ((decltype(*::GetFileVersionInfoSizeW)*)real_GetFileVersionInfoSizeW)
        (lptstrFilename, lpdwHandle);
}

static void* real_GetFileVersionInfoA = nullptr;
BOOL APIENTRY zzGetFileVersionInfoA(
    _In_                LPCSTR lptstrFilename, /* Filename of version stamped file */
    _Reserved_          DWORD dwHandle,          /* Information from GetFileVersionSize */
    _In_                DWORD dwLen,             /* Length of buffer for info */
    _Out_writes_bytes_(dwLen) LPVOID lpData            /* Buffer to place the data structure */
) {
    if (!real_GetFileVersionInfoA) {
        real_GetFileVersionInfoA = FindRealAddress("GetFileVersionInfoA");
    }
    return ((decltype(*::GetFileVersionInfoA)*)real_GetFileVersionInfoA)
        (lptstrFilename, dwHandle, dwLen, lpData);
}

static void* real_GetFileVersionInfoW = nullptr;
BOOL APIENTRY zzGetFileVersionInfoW(
    _In_                LPCWSTR lptstrFilename, /* Filename of version stamped file */
    _Reserved_          DWORD dwHandle,          /* Information from GetFileVersionSize */
    _In_                DWORD dwLen,             /* Length of buffer for info */
    _Out_writes_bytes_(dwLen) LPVOID lpData            /* Buffer to place the data structure */
) {
    if (!real_GetFileVersionInfoW) {
        real_GetFileVersionInfoW = FindRealAddress("GetFileVersionInfoW");
    }
    return ((decltype(*::GetFileVersionInfoW)*)real_GetFileVersionInfoW)
        (lptstrFilename, dwHandle, dwLen, lpData);
}

static void* real_GetFileVersionInfoSizeExA = nullptr;
DWORD APIENTRY zzGetFileVersionInfoSizeExA(_In_ DWORD dwFlags, _In_ LPCSTR lpwstrFilename, _Out_ LPDWORD lpdwHandle) {
    if (!real_GetFileVersionInfoSizeExA) {
        real_GetFileVersionInfoSizeExA = FindRealAddress("GetFileVersionInfoSizeExA");
    }
    return ((decltype(*::GetFileVersionInfoSizeExA)*)real_GetFileVersionInfoSizeExA)
        (dwFlags, lpwstrFilename, lpdwHandle);
}

static void* real_GetFileVersionInfoSizeExW = nullptr;
DWORD APIENTRY zzGetFileVersionInfoSizeExW(_In_ DWORD dwFlags, _In_ LPCWSTR lpwstrFilename, _Out_ LPDWORD lpdwHandle)
{
    if (!real_GetFileVersionInfoSizeExW) {
        real_GetFileVersionInfoSizeExW = FindRealAddress("GetFileVersionInfoSizeExW");
    }
    return ((decltype(*::GetFileVersionInfoSizeExW)*)real_GetFileVersionInfoSizeExW)
        (dwFlags, lpwstrFilename, lpdwHandle);
}

static void* real_GetFileVersionInfoExA = nullptr;
BOOL APIENTRY zzGetFileVersionInfoExA(_In_ DWORD dwFlags,
    _In_ LPCSTR lpwstrFilename,
    _Reserved_ DWORD dwHandle,
    _In_ DWORD dwLen,
    _Out_writes_bytes_(dwLen) LPVOID lpData)
{
    if (!real_GetFileVersionInfoExA) {
        real_GetFileVersionInfoExA = FindRealAddress("GetFileVersionInfoExA");
    }
    return ((decltype(*::GetFileVersionInfoExA)*)real_GetFileVersionInfoExA)
        (dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

static void* real_GetFileVersionInfoExW = nullptr;
BOOL APIENTRY zzGetFileVersionInfoExW(_In_ DWORD dwFlags,
    _In_ LPCWSTR lpwstrFilename,
    _Reserved_ DWORD dwHandle,
    _In_ DWORD dwLen,
    _Out_writes_bytes_(dwLen) LPVOID lpData)
{
    if (!real_GetFileVersionInfoExW) {
        real_GetFileVersionInfoExW = FindRealAddress("GetFileVersionInfoExW");
    }
    return ((decltype(*::GetFileVersionInfoExW)*)real_GetFileVersionInfoExW)
        (dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

static void* real_VerLanguageNameA = nullptr;
DWORD
APIENTRY
zzVerLanguageNameA(
    _In_                  DWORD wLang,
    _Out_writes_(cchLang) LPSTR szLang,
    _In_                  DWORD cchLang
) {
    if (!real_VerLanguageNameA) {
        real_VerLanguageNameA = FindRealAddress("VerLanguageNameA");
    }
    return ((decltype(*::VerLanguageNameA)*)real_VerLanguageNameA)
        (wLang, szLang, cchLang);
}

static void* real_VerLanguageNameW = nullptr;
DWORD
APIENTRY
zzVerLanguageNameW(
    _In_                  DWORD wLang,
    _Out_writes_(cchLang) LPWSTR szLang,
    _In_                  DWORD cchLang
) {
    if (!real_VerLanguageNameW) {
        real_VerLanguageNameW = FindRealAddress("VerLanguageNameW");
    }
    return ((decltype(*::VerLanguageNameW)*)real_VerLanguageNameW)
        (wLang, szLang, cchLang);
}

static void* real_VerQueryValueA = nullptr;
BOOL
APIENTRY
zzVerQueryValueA(
    _In_ LPCVOID pBlock,
    _In_ LPCSTR lpSubBlock,
    _Outptr_result_buffer_(_Inexpressible_("buffer can be PWSTR or DWORD*")) LPVOID * lplpBuffer,
    _Out_ PUINT puLen
) {
    if (!real_VerQueryValueA) {
        real_VerQueryValueA = FindRealAddress("VerQueryValueA");
    }
    return ((decltype(*::VerQueryValueA)*)real_VerQueryValueA)
        (pBlock, lpSubBlock, lplpBuffer, puLen);
}

static void* real_VerQueryValueW = nullptr;
BOOL
APIENTRY
zzVerQueryValueW(
    _In_ LPCVOID pBlock,
    _In_ LPCWSTR lpSubBlock,
    _Outptr_result_buffer_(_Inexpressible_("buffer can be PWSTR or DWORD*")) LPVOID * lplpBuffer,
    _Out_ PUINT puLen
) {
    if (!real_VerQueryValueW) {
        real_VerQueryValueW = FindRealAddress("VerQueryValueW");
    }
    return ((decltype(*::VerQueryValueW)*)real_VerQueryValueW)
        (pBlock, lpSubBlock, lplpBuffer, puLen);
}