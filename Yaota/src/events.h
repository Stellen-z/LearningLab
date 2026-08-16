// events.h —— 奇遇事件：踩到祭坛(坛)时随机触发一个
#pragma once

#include "types.h"
#include <vector>

namespace yaota {

// 选项的特殊效果（数字增减之外的花活）
enum EventFx {
    FxNone = 0,
    FxRandomItem,    // 随机得一件本层物品
    FxRandomPill,    // 随机得一颗丹药
    FxHealFull,      // 气血灵力全满
    FxUpgradeWeapon, // 当前法宝洗炼 +2
    FxUpgradeArmor,  // 当前法袍洗炼 +2
    FxTeleportNear,  // 传送到楼梯附近
    FxTribBuff,      // 下次渡劫成功率 +10%
    FxHerbs,         // 得灵草 x3
    FxMaxHpUp,       // 气血上限 +15
    FxMaxHpDown,     // 气血上限 -10（代价）
    FxLearnAtk,      // 攻击 +1
    FxLearnDef,      // 防御 +1
    FxNothing,       // 什么都没发生（有时候就是最好的结果）
};

struct EventChoice {
    std::wstring text;     // 选项名
    std::wstring outcome;  // 结算文案
    int dHp = 0, dMp = 0, dExp = 0, dGold = 0; // 增减（负数=付出/受伤）
    int fx = FxNone;
};

struct EventDef {
    int id;
    std::wstring title;    // 标题
    std::wstring text;     // 场景描述
    std::vector<EventChoice> choices;
    int minFloor = 1;      // 最早出现层数
};

const std::vector<EventDef>& eventDex();
const EventDef& eventDef(int id);
// 随机挑一条适合当前层的奇遇
const EventDef& randomEvent(int floor);

} // namespace yaota
