#!/usr/bin/env bash
# 《妖塔》构建脚本（Git Bash / Linux / macOS）
# 用法： ./build.sh
#
# 两个关键参数的由来（踩坑记录）：
#   -finput-charset=UTF-8   源码是 UTF-8，MinGW 默认按系统码页解析会乱码/编译错误
#   -static                 静态链接运行时。若运行时加载了 Git Bash 自带的
#                           libstdc++-6.dll（旧版 ABI），ifstream 构造会段错误
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ \
    -finput-charset=UTF-8 -fexec-charset=UTF-8 \
    -o yaota src/*.cpp
echo "构建完成: ./yaota"
