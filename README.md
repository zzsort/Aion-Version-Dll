# Aion-Version-Dll
Aion 4.0 No-IP and Windows 10 mouse fix.

Features:
- Allows the game client to connect to the game server IP specified by the -ip:x.x.x.x command line parameter. (Prevents the error message "No game server is available to the authorization server (6)".)
- Fixes the mouse issue on Windows 10 without needing to run a separate program. This feature is enabled by passing the -win10-mouse-fix command line argument to aion.bin.
- The IP fix can be disabled with -disable-ip-fix in case you only want to use the mouse fix.
- Added option to enable all graphics options sliders (shadows, water quality, etc) which are otherwise disabled at high resolutions. This feature is enabled by passing the -unlimited-gfx command line argument to aion.bin.

To install, copy each version.dll to the respective bin32 or bin64 folder under the Aion client root.

Note: Existing patch dlls should be removed from the bin32/bin64 directories as they may have conflicting functionality.
- Remove: dbghelp.dll
- Remove: d3d8thk.dll
- Remove: d3dx9_38.dll

Tested with the Aion 4.0.0.12 game client on Windows 7 and Windows 10. 

Source depends on: https://github.com/Microsoft/Detours
