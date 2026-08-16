// player.h —— 玩家：属性、境界、背包、装备
#pragma once

#include "types.h"
#include "item.h"

namespace yaota {

struct Player {
    std::wstring name;
    Element  spirit;          // 灵根五行（决定你的攻击属性）
    int      realmIdx = 0;    // 当前境界下标
    int      exp = 0;         // 当前修为

    int      hp = 0,  maxHp = 0;    // 气血
    int      mp = 0,  maxMp = 0;    // 灵力
    int      atkTotal = 0;          // 攻击（含境界加成）
    int      defTotal = 0;          // 防御（含境界加成）
    int      gold = 0;              // 灵石

    int      x = 0, y = 0;          // 位置
    int      floor = 1;             // 当前层数
    bool     alive = true;

    int      weaponId = -1;         // 装备的法宝（itemDex id）
    int      weaponBonus = 0;       // 洗炼加成
    int      armorId = -1;          // 装备的法袍
    int      armorBonus = 0;

    int      tribulationBonus = 0;  // 破障丹：下次渡劫成功率加成（百分比）

    std::vector<Item> inventory;    // 背包（可叠加的自动并格）

    // ---- 派生属性 ----
    const RealmDef& realm() const { return realms()[realmIdx]; }
    int atk() const;   // 基础攻击 + 法宝
    int def() const;   // 基础防御 + 法袍

    // ---- 行为 ----
    void init(const std::wstring& n, Element spirit);
    bool addItem(int defId, int count = 1, int bonus = 0); // 成功并入背包
    bool readyToBreak() const;                              // 修为是否够突破
    // 渡劫：成功升境界；失败受重伤。返回 0=失败 1=成功
    bool tribulate(std::wstring& log);
};

} // namespace yaota
