// main.cpp —— 程序入口：控制台初始化、标题画面、创建角色
#include "game.h"
#include "render.h"
#include "save.h"
#include "types.h"

#include <iostream>

#ifdef _WIN32
#define NOMINMAX        // 阻止 windows.h 的 min/max 宏污染（会炸掉 std::min/std::max）
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace yaota;

static void setupConsole() {
#ifdef _WIN32
    // 中文输出 + ANSI 转义码（颜色/清屏）
    SetConsoleOutputCP(65001);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

static void drawTitle() {
    Renderer::clearScreen();
    Renderer::println(L"");
    Renderer::println(L"    山  山  山  山  山  山  山  山  山  山  山  山", Color::Dark);
    Renderer::println(L"    ╔═══════════════════════════════════════╗", Color::Yellow);
    Renderer::println(L"    ║              妖  塔                  ║", Color::Yellow, true);
    Renderer::println(L"    ║        一 座 会 咬 人 的 塔          ║", Color::Red);
    Renderer::println(L"    ╚═══════════════════════════════════════╝", Color::Yellow);
    Renderer::println(L"    山  山  山  山  山  山  山  山  山  山  山  山", Color::Dark);
    Renderer::println(L"");
    Renderer::println(L"    三十层妖塔，塔塔有妖。练气入塔，飞升出塔。", Color::Default);
    Renderer::println(L"    至于中间发生什么——看你自己的造化了。", Color::Default);
    Renderer::println(L"");
}

int main() {
    setupConsole();

    while (true) {
        drawTitle();

        std::vector<std::wstring> opts;
        opts.push_back(L"入塔（新的征程）");
        if (saveExists()) opts.push_back(L"续世（读取存档）");
        opts.push_back(L"翻阅操作玉简");
        opts.push_back(L"离去了");

        int pick = Renderer::menu(L"塔门前", opts, false);
        if (pick < 0) break;

        if (opts[pick] == L"入塔（新的征程）") {
            // ---- 创建角色 ----
            Renderer::clearScreen();
            Renderer::println(L"");
            std::wstring name = Renderer::promptLine(L"道号如何称呼？（回车默认「散修」）:");
            if (name.empty()) name = L"散修";

            Renderer::println(L"");
            Renderer::println(L" 灵根决定你的攻击属性，也决定谁克你、你克谁：", Color::Default);
            int e = Renderer::menu(L"你身怀何种灵根？",
                { L"金灵根 —— 克木，被火克。锋锐无俦。",
                  L"木灵根 —— 克土，被金克。生机绵长。",
                  L"水灵根 —— 克火，被土克。柔韧多变。",
                  L"火灵根 —— 克金，被水克。霸道刚猛。",
                  L"土灵根 —— 克水，被木克。厚积薄发。" },
                false);
            Element spirit = (Element)e;

            Renderer::clearScreen();
            Renderer::println(L"");
            Renderer::println(L"    「" + name + L"，塔门之后，莫回头。」", Color::Magenta);
            Renderer::println(L"");
            Renderer::pause();

            Game g;
            g.newGame(name, spirit);
            g.run();
        } else if (opts[pick] == L"续世（读取存档）") {
            Game g;
            if (g.loadGame()) {
                g.run();
            } else {
                Renderer::println(L"存档损坏，此世无法挽回。", Color::Red);
                Renderer::pause();
            }
        } else if (opts[pick] == L"翻阅操作玉简") {
            Renderer::drawHelp();
        } else {
            break;
        }
    }

    Renderer::clearScreen();
    Renderer::println(L"");
    Renderer::println(L"    塔门缓缓合拢。江湖路远，后会有期。", Color::Dark);
    Renderer::println(L"");
    return 0;
}
