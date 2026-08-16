// test_framework.h —— 自制迷你单元测试框架（零依赖，纯标准库）
// 用法：
//   TEST(用例名) { CHECK(x == 1); CHECK_EQ(f(), 2); }
//   int main() { return tf::runAll(); }
// 设计要点：
//   * TEST 宏在编译期把用例注册进全局表（静态 Registrar 对象）
//   * CHECK 系列统计断言通过/失败数，失败时打印用例、表达式、位置
//   * runAll 汇总用例数与断言数，失败返回非零（CI 可用退出码判断）
#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace tf {

struct Case {
    std::string name;
    void (*fn)();
};

inline std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}
inline int& passed() { static int n = 0; return n; }
inline int& failed() { static int n = 0; return n; }
inline std::string& currentCase() { static std::string c; return c; }

struct Registrar {
    Registrar(const char* name, void (*fn)()) {
        registry().push_back({name, fn});
    }
};

inline void report(bool ok, const char* expr, const char* file, int line) {
    if (ok) { passed()++; return; }
    failed()++;
    std::printf("    [断言失败] %s\n        位置: %s:%d  用例: %s\n",
                expr, file, line, currentCase().c_str());
}

inline int runAll() {
    int failedCases = 0;
    for (const Case& c : registry()) {
        currentCase() = c.name;
        int before = failed();
        std::printf("[ RUN ] %s\n", c.name.c_str());
        c.fn();
        if (failed() > before) {
            failedCases++;
            std::printf("[FAIL] %s\n", c.name.c_str());
        } else {
            std::printf("[ ok ] %s\n", c.name.c_str());
        }
    }
    std::printf("\n===== 测试汇总 =====\n");
    std::printf("用例: %zu 个（失败 %d 个）\n", registry().size(), failedCases);
    std::printf("断言: %d 通过, %d 失败\n", passed(), failed());
    return failedCases == 0 ? 0 : 1;
}

} // namespace tf

#define TEST(name)                                                        \
    static void yaota_test_##name();                                      \
    static tf::Registrar yaota_reg_##name(#name, &yaota_test_##name);     \
    static void yaota_test_##name()

#define CHECK(expr)      tf::report((expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b)   tf::report((a) == (b), #a " == " #b, __FILE__, __LINE__)
#define CHECK_NE(a, b)   tf::report((a) != (b), #a " != " #b, __FILE__, __LINE__)
#define CHECK_LE(a, b)   tf::report((a) <= (b), #a " <= " #b, __FILE__, __LINE__)
#define CHECK_GE(a, b)   tf::report((a) >= (b), #a " >= " #b, __FILE__, __LINE__)
