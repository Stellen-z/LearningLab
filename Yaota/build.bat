@echo off
rem 《妖塔》构建脚本（Windows CMD，需要 MinGW-w64 的 g++ 在 PATH 里）
rem -finput-charset: 源码 UTF-8；-static: 避免加载到 Git Bash 旧版 libstdc++ DLL 而崩溃
cd /d "%~dp0"
g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ -finput-charset=UTF-8 -fexec-charset=UTF-8 -o yaota.exe src\*.cpp
if %errorlevel% neq 0 (
  echo 构建失败：请确认已安装 MinGW-w64 并把 g++ 加入 PATH
  pause
  exit /b 1
)
echo 构建完成: yaota.exe
pause
