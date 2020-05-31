# Aion-Version-Dll
Aion 4.0 No-IP and Windows 10 mouse fix.

This is a patch for Aion to remove the IP restriction and fix various bugs to help keep the game playable.

## All options (pass to aion.bin):
|argument|description|
|--|--|
|`-disable-ip-fix`|Disables the IP fix so you can use other features standalone such as the mouse fix.|
|`-win10-mouse-fix`|Enables the mouse fix needed for the Windows bug introduced in Windows 10 Fall Creators Update. |
|`-win10-mouse-fix-autodetect`|Enables the mouse fix only if the Windows version is detected to have the mouse bug.|
|`-windowed-mouse-fix`|Can be used to enable only the windowed mouse fix (does not affect fullscreen modes). Fixes the problem of the mouse jumping to random locations. Works in Windows 7.|
|`-unlimited-gfx`|Enables the graphics sliders in the graphics options that are grayed out in Aion 4.x when running at a high resolution.|
|`-disable-xigncode`|Disables xigncode in Aion 5.8|
|`-fix-39-weapon-switch`|Fixes a bug in 3.9 which prevents weapon switching when being attacked|

## Changelog
New features for 1.6.1:
- For Aion 3.9, updated `-fix-39-weapon-switch` to allow switching a weapons from the skillbar.

New features for 1.6:
- Made some changes to the windowed mouse fix to prevent the cursor from getting stuck.
- The `-unlimited-gfx` option should work for windowed mode now.
- Added `-fix-39-weapon-switch` to fix the weapon switching bug in Aion 3.9 which prevents switching weapons while being attacked. The changed code matches Aion 4.0 which doesn't have the bug.

New features for 1.5:
- New windowed mouse fix implementation using rawinput.
- The windowed mouse fix also fixes the problem of the mouse cursor sometimes jumping out of the game window. Use the new `-windowed-mouse-fix` argument to enable only the windowed fix which should work on all operating systems. This argument can be combined with `-win10-mouse-fix` or `-win10-mouse-fix-autodetect`. Note, `-win10-mouse-fix` always enables the new windowed fix, and `-win10-mouse-fix-autodetect` enables the windowed fix only if it detects the Windows 10 mouse bug.
- Tested on 4.0 only.

New features for 1.4:
- The IP fix now supports Aion 5.8.
- Added `-disable-xigncode` command line option to prevent xigncode from loading. Only tested in 5.8.

New features for 1.3:
- Fix a bug for the windowed mode mouse fix that could make it stop working after moving the window.

New features for 1.2:
- The Windows 10 mouse fix now works for windowed mode.
- Added the option `-win10-mouse-fix-autodetect` which will automatically enable the mouse fix depending on the OS version.

New features for 1.1:
- Added option to enable all graphics options sliders (shadows, water quality, etc) which are otherwise disabled at high resolutions. This feature is enabled by passing the `-unlimited-gfx` command line argument to aion.bin.

Features:
- Allows the game client to connect to the game server IP specified by the -ip:x.x.x.x command line parameter. (Prevents the error message "No game server is available to the authorization server (6)".)
- Fixes the mouse issue on Windows 10 without needing to run a separate program. This feature is enabled by passing the `-win10-mouse-fix` command line argument to aion.bin.
- The IP fix can be disabled with `-disable-ip-fix` in case you only want to use the mouse fix.

## Download
Latest binaries: https://github.com/zzsort/Aion-Version-Dll/blob/master/release/AionVersionDll_bin_1.6.1.zip

## Install
Extract both version.dll files from the .zip file to the respective bin32 or bin64 folder under the Aion client root. Aion will automatically load version.dll when it launches.

Note: Existing patch dlls should be removed from the bin32/bin64 directories as they may have conflicting functionality.
- Remove: dbghelp.dll
- Remove: d3d8thk.dll
- Remove: d3dx9_38.dll

## Testing
Mainly tested with the Aion 4.0.0.12 game client on Windows 7 and Windows 10. 

## Building
This source code depends on Detours from https://github.com/Microsoft/Detours which must be built separately. Set the DETOURS_PATH environment variable to the Detours project root before opening the .sln.
