@echo off
rem Yaota MSVC build script (VS2026 / VS2022).
rem NOTE: keep this file ASCII-only -- cmd.exe parses batch files in the
rem       system codepage (GBK on zh-CN), UTF-8 Chinese comments break it.
rem Source files carry UTF-8 BOM so MSVC reads them correctly; /utf-8 is
rem a belt-and-suspenders extra.
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSROOT=%%i"
)
if not defined VSROOT set "VSROOT=D:\Program Files\Microsoft Visual Studio\18\Community"

if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" (
  echo [ERROR] vcvars64.bat not found. Install the "Desktop development with C++" workload.
  exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

cd /d "%~dp0"
cl /utf-8 /EHsc /std:c++17 /O2 /W3 src\*.cpp /Fe:yaota_msvc.exe
if %errorlevel% neq 0 (
  echo [ERROR] MSVC build failed.
  exit /b 1
)
del /q src\*.obj >nul 2>&1
echo Build OK: yaota_msvc.exe
