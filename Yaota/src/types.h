// types.h —— 妖塔全局基础类型：五行、境界、通用小工具
// 这个文件是整个游戏的"世界观常量"，改这里就能改游戏平衡。
#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace yaota {

// ============ 五行 ============
// 金木水火土。相克循环：金克木、木克土、土克水、水克火、火克金
// 相生循环：金生水、水生木、木生火、火生土、土生金
enum class Element {
    Jin   = 0, // 金
    Mu    = 1, // 木
    Shui  = 2, // 水
    Huo   = 3, // 火
    Tu    = 4, // 土
};

inline const wchar_t* elementWName(Element e) {
    switch (e) {
        case Element::Jin:  return L"金";
        case Element::Mu:   return L"木";
        case Element::Shui: return L"水";
        case Element::Huo:  return L"火";
        case Element::Tu:   return L"土";
    }
    return L"?";
}

// 攻击方五行 -> 防守方五行 的伤害倍率
// 克制 1.5 | 被克 0.7 | 相生 1.15 | 同属性 1.0
inline double elementMultiplier(Element atk, Element def) {
    // 相克链：金(0)->木(1)->土(4)->水(2)->火(3)->金(0)
    static const int keCycle[5] = { 1, 4, 3, 0, 2 }; // keCycle[i] = i 克的属性
    if (keCycle[(int)atk] == (int)def) return 1.5;   // 我克他
    if (keCycle[(int)def] == (int)atk) return 0.7;   // 他克我
    // 相生链：金(0)->水(2)->木(1)->火(3)->土(4)->金(0)
    static const int shengCycle[5] = { 2, 3, 1, 4, 0 }; // shengCycle[i] = i 生 的属性
    if (shengCycle[(int)atk] == (int)def) return 1.15;  // 我生他（气焰压过去）
    return 1.0;
}

// ============ 境界 ============
struct RealmDef {
    std::wstring name;   // 境界名
    int    expNeed;      // 突破所需修为
    int    hpBonus;      // 突破后气血上限提升
    int    mpBonus;      // 灵力上限提升
    int    atkBonus;     // 攻击提升
    int    defBonus;     // 防御提升
    double tribulation;  // 渡劫成功率
};

// 九大境界，从练气到渡劫。凑够修为后按 T 尝试突破渡劫。
inline const std::vector<RealmDef>& realms() {
    static const std::vector<RealmDef> r = {
        { L"练气",   30,    20,  10,  2,  1, 1.00 },
        { L"筑基",   90,    40,  20,  4,  3, 0.95 },
        { L"金丹",   260,   80,  40,  8,  6, 0.90 },
        { L"元婴",   700,  150,  70, 14, 10, 0.85 },
        { L"化神",  1600,  260, 120, 22, 16, 0.80 },
        { L"炼虚",  3600,  420, 200, 34, 24, 0.75 },
        { L"合体",  8000,  700, 330, 52, 36, 0.70 },
        { L"大乘", 18000, 1100, 520, 78, 55, 0.65 },
        { L"渡劫", 40000, 1800, 800, 118, 82, 0.60 },
    };
    return r;
}

// ============ 地图方块 ============
enum class Tile {
    Wall    = 0, // 山岩
    Floor   = 1, // 石地
    Stairs  = 2, // 上行梯
    Altar   = 3, // 祭坛（触发奇遇）
    Herb    = 4, // 灵草（可采）
    Chest   = 5, // 宝箱
};

// ============ 物品类型 ============
enum class ItemType {
    Weapon,   // 法宝（武器）
    Armor,    // 护甲（法袍）
    Pill,     // 丹药（消耗品）
    Scroll,   // 卷轴（消耗品）
    Treasure, // 奇物（卖钱/特殊）
    Material, // 炼材（卖钱）
};

inline const wchar_t* itemTypeWName(ItemType t) {
    switch (t) {
        case ItemType::Weapon:   return L"法宝";
        case ItemType::Armor:    return L"法袍";
        case ItemType::Pill:     return L"丹药";
        case ItemType::Scroll:   return L"卷轴";
        case ItemType::Treasure: return L"奇物";
        case ItemType::Material: return L"炼材";
    }
    return L"?";
}

// ============ 简单的控制台颜色 ============
enum class Color {
    Default, Red, Green, Yellow, Blue, Magenta, Cyan, White, Dark
};

inline const char* colorCode(Color c) {
    switch (c) {
        case Color::Red:     return "\033[31m";
        case Color::Green:   return "\033[32m";
        case Color::Yellow:  return "\033[33m";
        case Color::Blue:    return "\033[34m";
        case Color::Magenta: return "\033[35m";
        case Color::Cyan:    return "\033[36m";
        case Color::White:   return "\033[37m";
        case Color::Dark:    return "\033[90m";
        default:             return "\033[0m";
    }
}

// ============ 小工具 ============
inline std::wstring to_wstr(const std::string& s) {
    std::wstring ws;
    for (unsigned char c : s) ws += (wchar_t)c; // 仅用于 ASCII 输入
    return ws;
}

// UTF-8 -> 宽字符串（控制台切到 65001 后，窄输入就是 UTF-8 字节）
inline std::wstring utf8ToWstr(const std::string& s) {
    std::wstring out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { out += (wchar_t)c; i += 1; }
        else if ((c >> 5) == 0x6 && i + 1 < s.size()) {
            out += (wchar_t)(((c & 0x1F) << 6) | (s[i + 1] & 0x3F)); i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < s.size()) {
            out += (wchar_t)(((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F));
            i += 3;
        } else { out += L'?'; i += 1; }
    }
    return out;
}

inline std::string wstrToUtf8(const std::wstring& ws) {
    // 简易 UTF-32/宽字符 -> UTF-8 转换（源码字面量都是 BMP 内汉字，够用）
    std::string out;
    for (wchar_t wc : ws) {
        unsigned int v = (unsigned int)wc;
        if (v < 0x80) {
            out += (char)v;
        } else if (v < 0x800) {
            out += (char)(0xC0 | (v >> 6));
            out += (char)(0x80 | (v & 0x3F));
        } else {
            out += (char)(0xE0 | (v >> 12));
            out += (char)(0x80 | ((v >> 6) & 0x3F));
            out += (char)(0x80 | (v & 0x3F));
        }
    }
    return out;
}

} // namespace yaota
