// render.cpp —— 画面绘制实现
// 渲染约定：
//   * 所有格子都用"全角"字符（汉字/全角符号），保证每格占 2 列，网格不会错位
//   * 未探索区域画全角空格"　"；看过但不在视野内的格子以暗色显示（迷雾记忆）
//   * 视野：以玩家为中心半径 9 格，山岩挡视线（Bresenham 直线检测）
#include "render.h"
#include "rng.h"

#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <iostream>

namespace yaota {

// ---- 基础输出 ----
void Renderer::clearScreen() {
    std::cout << "\033[2J\033[H";
}

void Renderer::print(const std::wstring& s, Color c, bool bold) {
    std::cout << colorCode(c);
    if (bold) std::cout << "\033[1m";
    std::cout << wstrToUtf8(s) << "\033[0m";
}

void Renderer::println(const std::wstring& s, Color c, bool bold) {
    print(s, c, bold);
    std::cout << "\n";
}

std::wstring Renderer::promptLine(const std::wstring& prompt) {
    print(prompt, Color::Cyan);
    std::cout << " ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        // stdin 结束（管道输入耗尽 / 终端关闭）：视为玩家离场，干净退出。
        // 否则主循环会拿到空输入无限重绘（脚本测试时曾刷出 6.9GB 输出）。
        std::cout << "\n";
        println(L"（输入流已尽，就此别过。）", Color::Dark);
        std::exit(0);
    }
    return utf8ToWstr(line);
}

void Renderer::pause() {
    println(L"");
    print(L"—— 按回车继续 ——", Color::Dark);
    std::string dummy;
    std::getline(std::cin, dummy);
}

int Renderer::menu(const std::wstring& title, const std::vector<std::wstring>& options,
                   bool allowCancel) {
    println(L"");
    println(L"┌─ " + title + L" ─────────────", Color::Yellow);
    for (size_t i = 0; i < options.size(); ++i)
        println(L"│  " + std::to_wstring(i + 1) + L". " + options[i]);
    if (allowCancel) println(L"│  . 取消");
    println(L"└─────────────────", Color::Yellow);

    while (true) {
        std::wstring in = promptLine(L"请选择:");
        if (in.empty()) continue;
        if (in[0] == L'.' && allowCancel) return -1;
        int n = 0;
        for (wchar_t c : in) {
            if (!std::iswdigit(c)) { n = -1; break; }
            n = n * 10 + (c - L'0');
        }
        if (n >= 1 && n <= (int)options.size()) return n - 1;
        println(L"输入无效。", Color::Red);
    }
}

bool Renderer::askYesNo(const std::wstring& question) {
    while (true) {
        std::wstring in = promptLine(question + L" [y/n]:");
        if (in.empty()) continue;
        wchar_t c = std::towlower(in[0]);
        if (c == L'y' || c == L'n') return c == L'y';
    }
}

// ---- 视野 ----
bool Renderer::lineOfSight(const Dungeon& d, int x0, int y0, int x1, int y1) {
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, x = x0, y = y0;
    while (!(x == x1 && y == y1)) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
        if (x == x1 && y == y1) break;   // 终点自身不挡自己
        if (d.blocksSight(x, y)) return false;
    }
    return true;
}

// ---- 地图 ----
void Renderer::drawMap(const Dungeon& d, const Player& p,
                       const std::vector<Monster>& monsters,
                       const std::vector<GroundItem>& ground,
                       bool revealAll, std::vector<char>& explored) {
    constexpr int RADIUS = 9;
    if ((int)explored.size() != Dungeon::W * Dungeon::H)
        explored.assign(Dungeon::W * Dungeon::H, 0);

    // 先更新迷雾记忆
    if (revealAll) {
        for (auto& e : explored) e = 1;
    } else {
        for (int y = 0; y < Dungeon::H; ++y) {
            for (int x = 0; x < Dungeon::W; ++x) {
                double dist = std::hypot(x - p.x, y - p.y);
                if (dist <= RADIUS && lineOfSight(d, p.x, p.y, x, y))
                    explored[y * Dungeon::W + x] = 1;
            }
        }
    }

    println(L"", Color::Default);
    for (int y = 0; y < Dungeon::H; ++y) {
        print(L"│", Color::Dark);
        for (int x = 0; x < Dungeon::W; ++x) {
            bool seen = explored[y * Dungeon::W + x];
            if (!seen) { std::cout << "　"; continue; }

            double dist = std::hypot(x - p.x, y - p.y);
            bool visible = revealAll ||
                (dist <= RADIUS && lineOfSight(d, p.x, p.y, x, y));

            // 视野内：优先画活物，其次玩家/掉落，最后地形
            if (visible) {
                const Monster* m = nullptr;
                for (const auto& mm : monsters)
                    if (mm.alive && mm.x == x && mm.y == y) { m = &mm; break; }
                if (m) {
                    print(std::wstring(1, monsterDef(m->defId).glyph),
                          elementColor(monsterDef(m->defId).element));
                    continue;
                }
                if (x == p.x && y == p.y) {
                    print(L"仙", Color::Cyan, true);
                    continue;
                }
                const Item* gi = nullptr;
                for (const auto& g : ground)
                    if (g.x == x && g.y == y) { gi = &g.item; break; }
                if (gi) {
                    print(std::wstring(1, itemDef(gi->defId).glyph),
                          itemColor(itemDef(gi->defId).type));
                    continue;
                }
            }

            // 地形（迷雾记忆里的格子统一暗色）
            Tile t = d.at(x, y);
            Color base = Color::Dark;
            switch (t) {
                case Tile::Wall:
                    print(L"山", base);
                    break;
                case Tile::Stairs:
                    print(L"梯", visible ? Color::Yellow : base, visible);
                    break;
                case Tile::Altar:
                    print(L"坛", visible ? Color::Magenta : base);
                    break;
                case Tile::Herb:
                    print(L"草", visible ? Color::Green : base);
                    break;
                case Tile::Chest:
                    print(L"箱", visible ? Color::Yellow : base);
                    break;
                case Tile::Floor:
                default:
                    print(L"．", base);
                    break;
            }
        }
        std::cout << "\n";
    }
}

// ---- 配色 ----
Color Renderer::elementColor(Element e) {
    switch (e) {
        case Element::Jin:  return Color::White;
        case Element::Mu:   return Color::Green;
        case Element::Shui: return Color::Blue;
        case Element::Huo:  return Color::Red;
        case Element::Tu:   return Color::Yellow;
    }
    return Color::Default;
}

Color Renderer::itemColor(ItemType t) {
    switch (t) {
        case ItemType::Weapon:   return Color::Cyan;
        case ItemType::Armor:    return Color::Blue;
        case ItemType::Pill:     return Color::Green;
        case ItemType::Scroll:   return Color::Magenta;
        case ItemType::Treasure: return Color::Yellow;
        case ItemType::Material: return Color::Dark;
    }
    return Color::Default;
}

void Renderer::drawBar(int cur, int max, int width, Color c) {
    if (max < 1) max = 1;
    int filled = (int)((double)cur / max * width + 0.5);
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    print(std::wstring(filled, L'█'), c);
    print(std::wstring(width - filled, L'░'), Color::Dark);
}

void Renderer::drawHud(const Player& p, int kills) {
    println(L"╔══════════════════════════════════════════════╗", Color::Yellow);
    std::wstring title = L"║ 妖塔 · 第 " + std::to_wstring(p.floor) + L" 层";
    title += L"（斩妖 " + std::to_wstring(kills) + L"）";
    print(title, Color::Yellow, true);
    println(L"", Color::Default);

    print(L" " + p.name, Color::White, true);
    print(L" 【" + std::wstring(elementWName(p.spirit)) + L"灵根·");
    print(p.realm().name, Color::Magenta);
    println(L"】", Color::Default);

    bool ready = p.readyToBreak();
    print(L" 气血 ");
    drawBar(p.hp, p.maxHp, 10, p.hp * 3 < p.maxHp ? Color::Red : Color::Green);
    print(L" " + std::to_wstring(p.hp) + L"/" + std::to_wstring(p.maxHp) + L"    灵力 ");
    drawBar(p.mp, p.maxMp, 8, Color::Blue);
    print(L" " + std::to_wstring(p.mp) + L"/" + std::to_wstring(p.maxMp));
    println(L"", Color::Default);

    print(L" 攻击 " + std::to_wstring(p.atk()) + L"  防御 " + std::to_wstring(p.def()));
    print(L"  灵石 " + std::to_wstring(p.gold), Color::Yellow);
    print(L"  修为 " + std::to_wstring(p.exp) + L"/" + std::to_wstring(p.realm().expNeed));
    println(L"", Color::Default);

    print(L" 法宝: ");
    if (p.weaponId >= 0) {
        const ItemDef& w = itemDef(p.weaponId);
        print(w.name, Color::Cyan);
        int pw = w.power + p.weaponBonus;
        print(L"(攻+" + std::to_wstring(pw) + L")");
    } else print(L"赤手空拳", Color::Dark);
    print(L"   法袍: ");
    if (p.armorId >= 0) {
        const ItemDef& a = itemDef(p.armorId);
        print(a.name, Color::Blue);
        int pa = a.power + p.armorBonus;
        print(L"(防+" + std::to_wstring(pa) + L")");
    } else print(L"布衣", Color::Dark);
    println(L"", Color::Default);

    if (ready) println(L" ★ 修为已足，按 [b] 冲击境界（渡劫）！", Color::Yellow, true);
    if (p.tribulationBonus > 0)
        println(L" ☆ 破障之力护体：下次渡劫成功率 +" +
                std::to_wstring(p.tribulationBonus) + L"%", Color::Magenta);
}

void Renderer::drawLog(const std::vector<std::wstring>& logs) {
    println(L"├─ 行事录 ─────────────────────────────────────", Color::Dark);
    size_t start = logs.size() > 6 ? logs.size() - 6 : 0;
    for (size_t i = start; i < logs.size(); ++i)
        println(L"│ " + logs[i], Color::Default);
    for (size_t i = logs.size(); i < start + 6; ++i)
        println(L"│", Color::Dark);
    println(L"╚══════════════════════════════════════════════╝", Color::Dark);
}

void Renderer::drawHelp() {
    clearScreen();
    println(L"══════════════ 《妖塔》操作说明 ══════════════", Color::Yellow);
    println(L"");
    println(L" 移动/攻击：w 上、s 下、a 左、d 右（走向妖怪即攻击）");
    println(L" g  拾取脚下的物品");
    println(L" i  打开背包（使用/装备/丢弃）");
    println(L" t  踏上楼梯去往上一层");
    println(L" b  冲击境界（修为足够时渡劫突破）");
    println(L" c  打坐调息（回灵力，但会经过时间）");
    println(L" x  灵力爆发（耗 15 灵力，重击身边所有妖怪）");
    println(L" v  炼化奇物与炼材，换取灵石");
    println(L" m  翻阅妖怪图鉴");
    println(L" S  保存进度");
    println(L" .  原地等待一回合");
    println(L" q  放弃此世（退出）");
    println(L"");
    println(L" 五行相克：金克木、木克土、土克水、水克火、火克金", Color::Magenta);
    println(L" 攻击克制属性伤害 x1.5，被克 x0.7，选对灵根事半功倍。", Color::Magenta);
    println(L" 第 30 层塔顶即可渡劫飞升（通关）。");
    pause();
}

void Renderer::drawMonstersDex() {
    clearScreen();
    println(L"══════════════ 妖怪图鉴 ══════════════", Color::Yellow);
    for (const auto& m : monsterDex()) {
        print(L" " + std::wstring(1, m.glyph) + L" ", elementColor(m.element));
        print(m.name, Color::White, true);
        print(L" [" + std::wstring(elementWName(m.element)) + L"] ");
        println(L"血" + std::to_wstring(m.hp) + L" 攻" + std::to_wstring(m.atk) +
                L" 防" + std::to_wstring(m.def) +
                L" · 第" + std::to_wstring(m.minFloor) + L"~" +
                (m.maxFloor ? std::to_wstring(m.maxFloor) : L"30") + L"层", Color::Dark);
        println(L"    " + m.desc, Color::Dark);
    }
    pause();
}

} // namespace yaota
