@echo off
rem 《妖塔》构建脚本（Windows CMD，需要 MinGW-w64 的 g++ 在 PATH 里）
cd /d "%~dp0"
g++ -std=c++17 -O2 -o yaota.exe src\*.cpp
if %errorlevel% neq 0 (
  echo 构建失败：请确认已安装 MinGW-w64 并把 g++ 加入 PATH
  pause
  exit /b 1
)
echo 构建完成: yaota.exe
pause
