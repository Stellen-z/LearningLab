#!/usr/bin/env bash
# 《妖塔》单元测试脚本：构建并运行全部用例
# 用法： ./run_tests.sh
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -g -finput-charset=UTF-8 -fexec-charset=UTF-8 \
    -o test_runner $(ls src/*.cpp | grep -v 'src/main.cpp') tests/test_all.cpp
./test_runner
