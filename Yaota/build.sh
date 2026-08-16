#!/usr/bin/env bash
# 《妖塔》构建脚本（Git Bash / Linux / macOS）
# 用法： ./build.sh
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -o yaota src/*.cpp
echo "构建完成: ./yaota"
