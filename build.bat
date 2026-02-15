@echo off
REM ================================================================
REM SWG Decoration Plugin v2.0 - Build Script
REM Compiles DiscordProxy64.cpp + MinHook into discord-rpc.dll (x64)
REM
REM USAGE: Run this from a "x64 Native Tools Command Prompt for VS"
REM   OR:  Run from any cmd.exe and it will try to set up x64 env
REM ================================================================

REM Check if cl.exe is already available and targeting x64
cl /? >nul 2>&1
if %ERRORLEVEL% equ 0 (
    REM cl is available - check if it's x64 by looking for AMD64 in output
    cl 2>&1 | findstr /i "x64 amd64" >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo Using existing x64 compiler environment.
        goto :compile
    )
    echo WARNING: cl.exe found but may not be x64. Setting up x64 environment...
)

REM Try to find and call vcvarsall.bat for x64
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat"
)
if exist "C:\Program Files\Microsoft Visual Studio\2025\Preview\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2025\Preview\VC\Auxiliary\Build\vcvarsall.bat"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)

if "%VCVARS%"=="" (
    echo ERROR: Could not find Visual Studio vcvarsall.bat
    echo.
    echo Please run this from a "x64 Native Tools Command Prompt for VS"
    echo OR edit this script to point to your VS installation.
    exit /b 1
)

echo Setting up x64 build environment using:
echo   %VCVARS%
call "%VCVARS%" x64
if %ERRORLEVEL% neq 0 (
    echo ERROR: vcvarsall.bat failed. Try running from a x64 Native Tools Command Prompt.
    exit /b 1
)

:compile
echo.
echo ================================================================
echo Compiling SWG Decoration Plugin v2.0 (x64)
echo ================================================================
echo.

cl /nologo /EHsc /O2 /LD /DWIN32_LEAN_AND_MEAN /I"lib" ^
    DiscordProxy64.cpp ^
    lib\hook.c ^
    lib\buffer.c ^
    lib\trampoline.c ^
    lib\hde\hde64.c ^
    /link /OUT:discord-rpc.dll user32.lib kernel32.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo BUILD FAILED!
    echo.
    echo If you see "machine type 'x64' conflicts with 'X86'" errors,
    echo you need to run this from a x64 Native Tools Command Prompt.
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL: discord-rpc.dll
echo.
echo To install:
echo   1. Copy discord-rpc.dll to "C:\Games\SWG Restoration\x64\"
echo   2. Make sure discord-rpc-real.dll exists (renamed original)
echo   3. Launch via SWG Restoration launcher
echo.

REM Optionally auto-copy if the target directory exists
if exist "C:\Games\SWG Restoration\x64\" (
    echo Auto-copying to game directory...
    copy /Y discord-rpc.dll "C:\Games\SWG Restoration\x64\discord-rpc.dll"
    echo Done!
)
