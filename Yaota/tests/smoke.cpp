// tests/smoke.cpp —— 分步冒烟测试：stderr 无缓冲打印，定位崩溃点
#include "render.h"
#include "game.h"
#include "save.h"

#include <cstdio>

using namespace yaota;

#define TRACE(msg) std::fprintf(stderr, "[trace] %s\n", msg)

int main() {
    TRACE("start");

    TRACE("drawTitle parts");
    Renderer::clearScreen();
    Renderer::println(L"标题测试行");
    std::fflush(stdout);
    TRACE("println ok");

    Player p;
    TRACE("player init");
    p.init(L"测试散修", Element::Jin);
    std::fprintf(stderr, "[trace] hp=%d mp=%d inv=%zu\n", p.hp, p.mp, p.inventory.size());

    TRACE("dungeon generate");
    Dungeon d;
    d.generate(1, 2, 2);
    std::fprintf(stderr, "[trace] rooms=%zu monsters=%zu items=%zu stairs=(%d,%d)\n",
                 d.rooms().size(), d.spawnedMonsters().size(),
                 d.spawnedGroundItems().size(), d.stairsX(), d.stairsY());

    TRACE("hud");
    Renderer::drawHud(p, 0);
    std::fflush(stdout);
    TRACE("hud ok");

    auto mons = d.spawnedMonsters();
    auto grounds = d.spawnedGroundItems();
    std::vector<char> explored;
    p.x = d.rooms()[0].cx();
    p.y = d.rooms()[0].cy();

    TRACE("map");
    Renderer::drawMap(d, p, mons, grounds, false, explored);
    std::fflush(stdout);
    TRACE("map ok");

    TRACE("menu");
    std::vector<std::wstring> opts = { L"选项一", L"选项二" };
    // 不读输入，跳过 menu 交互，直接测事件系统
    TRACE("events dex");
    const EventDef& ev = randomEvent(1);
    std::fprintf(stderr, "[trace] event: %ls\n", ev.title.c_str());

    TRACE("game newGame");
    Game g;
    g.newGame(L"冒烟散修", Element::Huo);
    TRACE("newGame ok");

    TRACE("save");
    bool ok = saveToFile(p, 3);
    std::fprintf(stderr, "[trace] save=%d exists=%d\n", (int)ok, (int)saveExists());

    TRACE("all done");
    return 0;
}
