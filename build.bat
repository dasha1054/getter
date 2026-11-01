@echo off
REM ============================================================
REM  build.bat - 3D shooter (Quake 1 style) for Windows
REM  Auto-detects: MSVC (cl.exe) -> MinGW-w64 (g++.exe)
REM  Result: quake3d.exe (single static binary, no DLLs needed with MinGW -static)
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

set SRC=src\main.cpp src\game.cpp src\map.cpp src\weapons.cpp src\renderer.cpp src\platform_win.cpp
set OUT=quake3d.exe

where cl.exe >nul 2>nul
if %errorlevel%==0 goto :msvc

where g++.exe >nul 2>nul
if %errorlevel%==0 goto :mingw

echo [ERROR] No compiler found.
echo   Option A: Install Visual Studio Build Tools and run this script
echo             from "Developer Command Prompt for VS" (cl.exe must be in PATH).
echo   Option B: Install MinGW-w64 (e.g. winlibs.com or MSYS2 mingw-w64-x86_64-gcc)
echo             and make sure g++.exe is in PATH.
pause
exit /b 1

:msvc
echo [INFO] Building with MSVC (cl.exe)...
REM Make sure link.exe and headers are available; if not, try vcvarsauto via vswhere
cl /nologo /EHsc /O2 /std:c++17 ^
   /Fe:%OUT% %SRC% ^
   /link user32.lib gdi32.lib winmm.lib
if %errorlevel% neq 0 (
   echo [ERROR] MSVC build failed. Try running from Developer Command Prompt.
   pause
   exit /b 1
)
goto :done

:mingw
echo [INFO] Building with MinGW-w64 (g++.exe)...
g++ -std=c++17 -O2 -DNDEBUG ^
    -o %OUT% %SRC% ^
    -static -lgdi32 -luser32 -Wl,-subsystem,console
if %errorlevel% neq 0 (
   echo [ERROR] MinGW build failed.
   pause
   exit /b 1
)
goto :done

:done
if exist %OUT% (
   echo.
   echo [OK] Built %OUT%. Run it:  %OUT%
   echo      Controls: WASD+mouse, Space=jump, LMB=fire, 1-8=weapons, R=restart, Esc=quit
) else (
   echo [ERROR] Output binary missing.
   pause
   exit /b 1
)
endlocal
