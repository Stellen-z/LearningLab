// game.h —— 游戏总控：把所有系统粘成"一局游戏"
#pragma once

#include "player.h"
#include "dungeon.h"
#include "events.h"

#include <vector>

namespace yaota {

class Game {
public:
    void newGame(const std::wstring& name, Element spirit);
    bool loadGame();   // 成功返回 true（读档后调用 run 继续）
    void run();        // 主循环，直到死亡 / 飞升 / 退出

    // ---- 测试钩子：让单元测试能驱动内部回合与检查状态 ----
    void debugEndTurn() { endTurn(); }
    Player& debugPlayer() { return player_; }
    std::vector<Monster>& debugMonsters() { return monsters_; }
    Dungeon& debugDungeon() { return dungeon_; }

private:
    // ---- 玩家动作（大多消耗一回合）----
    void handleMove(int dx, int dy);
    void tryPickup();
    void tryStairs();
    void tryTribulate();
    void meditate();
    void spiritBurst();
    void refineJunk();
    void openInventory();
    void viewMonsterDex();
    bool useItem(size_t idx);   // 使用/饮用/激活，成功则消耗
    void equipItem(size_t idx);
    void dropItem(size_t idx);

    // ---- 世界运转 ----
    void endTurn();
    void monsterTurns();
    void attackMonster(Monster& m);
    void monsterAttack(Monster& m);
    void openChest();
    void triggerEvent();
    void applyEventChoice(const EventDef& ev, int choiceIdx);
    void applyEventFx(int fx, std::wstring& out);

    // ---- 工具 ----
    void setupFloor();          // 生成当前层并把玩家/妖怪/掉落就位
    Monster* monsterAt(int x, int y);
    GroundItem* groundItemAt(int x, int y);
    void log(const std::wstring& msg, Color = Color::Default) { logs_.push_back(msg); }
    void gameOver(bool ascended);
    void regenTick();

    Player player_;
    Dungeon dungeon_;
    std::vector<Monster>    monsters_;  // 本层活物（死了的留着占位，画图时跳过）
    std::vector<GroundItem> ground_;    // 本层地上掉落
    std::vector<char>       explored_;  // 迷雾记忆
    std::vector<std::wstring> logs_;    // 行事录（只留最近若干条）
    bool revealAll_ = false;            // 天眼符生效中
    bool over_      = false;
    int  kills_     = 0;
    int  turn_      = 0;
};

} // namespace yaota
