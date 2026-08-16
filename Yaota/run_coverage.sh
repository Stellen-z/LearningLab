#!/usr/bin/env bash
# 《妖塔》分支覆盖率脚本（gcov）
# 用法： ./run_coverage.sh
# 原理：--coverage 编译出带插桩的二进制，运行测试后产生 .gcda 计数文件，
#       gcov -b 解析出每个源文件的行覆盖与分支覆盖报告。
# 注意：不开 -O2——优化会合并/移动分支，覆盖率数据会失真。
set -e
cd "$(dirname "$0")"

g++ -std=c++17 -g --coverage -finput-charset=UTF-8 -fexec-charset=UTF-8 \
    -o cov_runner $(ls src/*.cpp | grep -v 'src/main.cpp') tests/test_all.cpp

./cov_runner > /dev/null   # 跑测试，收集计数

echo ""
echo "===== gcov 分支覆盖率 ====="
gcov -b cov_runner-*.gcda 2>/dev/null | grep -E "File|Lines executed|Branches executed|Taken at least|No branches" | sed 's/^/  /'

echo ""
echo "（详细逐行报告见 *.gcov 文件）"
