// rng.h —— 全局随机数源（mt19937），所有随机都从这里走，方便做种子复现
#pragma once

#include <random>
#include <vector>

namespace yaota {

class Rng {
public:
    static Rng& get() {
        static Rng inst;
        return inst;
    }

    void seed(unsigned s) { eng_.seed(s); }

    // [a, b] 闭区间整数
    int range(int a, int b) {
        std::uniform_int_distribution<int> d(a, b);
        return d(eng_);
    }

    // 0.0 ~ 1.0
    double uniform() {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        return d(eng_);
    }

    // 概率为 p 的事件是否发生
    bool chance(double p) { return uniform() < p; }

    // 从向量里随机挑一个
    template <typename T>
    const T& pick(const std::vector<T>& v) {
        return v[range(0, (int)v.size() - 1)];
    }

    // 掷 n 个骰子（打宝、掉落常用）
    int dice(int count, int sides) {
        int sum = 0;
        for (int i = 0; i < count; ++i) sum += range(1, sides);
        return sum;
    }

private:
    Rng() : eng_(std::random_device{}()) {}
    std::mt19937 eng_;
};

} // namespace yaota
