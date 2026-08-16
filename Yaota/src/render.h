// render.h —— 控制台渲染：汉字地图、状态栏、行事录、菜单
#pragma once

#include "types.h"
#include "dungeon.h"
#include "player.h"
#include "monster.h"
#include "item.h"

#include <vector>
#include <string>

namespace yaota {

class Renderer {
public:
    // ---- 基础输出 ----
    static void clearScreen();
    static void print(const std::wstring& s, Color c = Color::Default, bool bold = false);
    static void println(const std::wstring& s, Color c = Color::Default, bool bold = false);
    static std::wstring promptLine(const std::wstring& prompt);
    static void pause();

    // ---- 菜单：返回下标；allowCancel 时玩家可选 . 取消并返回 -1 ----
    static int menu(const std::wstring& title, const std::vector<std::wstring>& options,
                    bool allowCancel = true);
    static bool askYesNo(const std::wstring& question);

    // ---- 游戏画面 ----
    // explored 是"迷雾记忆"（走过/看过的格子），本函数会顺带更新它
    static void drawMap(const Dungeon& d, const Player& p,
                        const std::vector<Monster>& monsters,
                        const std::vector<GroundItem>& ground,
                        bool revealAll, std::vector<char>& explored);
    static void drawHud(const Player& p, int kills);
    static void drawLog(const std::vector<std::wstring>& logs);

    // ---- 静态页 ----
    static void drawHelp();
    static void drawMonstersDex();

private:
    static bool lineOfSight(const Dungeon& d, int x0, int y0, int x1, int y1);
    static Color elementColor(Element e);
    static Color itemColor(ItemType t);
    static void drawBar(int cur, int max, int width, Color c);
};

} // namespace yaota
