@echo off
echo Building withdll.exe from Detours samples...

:: Try to find a VS developer command prompt environment
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo.
    echo ERROR: 'cl' not found. You need to run this from a
    echo "Developer Command Prompt for VS" or "x86 Native Tools Command Prompt".
    echo.
    echo Alternatively, open that prompt and run:
    echo   cd C:\Games\Detours\samples\withdll
    echo   nmake
    echo.
    echo Then come back and run this script again.
    pause
    exit /b 1
)

:: Build withdll if it doesn't exist yet
if not exist "C:\Games\Detours\bin.X86\withdll.exe" (
    echo Compiling withdll.exe...
    pushd "C:\Games\Detours\samples\withdll"
    nmake
    popd
)

if not exist "C:\Games\Detours\bin.X86\withdll.exe" (
    echo ERROR: withdll.exe was not built successfully.
    pause
    exit /b 1
)

echo Launching SWG Restoration with decoration plugin...
cd /d "C:\Games\SWG Restoration"
"C:\Games\Detours\bin.X86\withdll.exe" /d:SWGCommandExtension.dll "SwgClient_r.exe"
