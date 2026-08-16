// item.h —— 物品定义（图鉴条目）与运行时实例
#pragma once

#include "types.h"

namespace yaota {

// 一件物品的"图鉴定义"（静态数据，全局一份）
struct ItemDef {
    int      id;
    std::wstring name;      // 名称
    wchar_t  glyph;         // 地图上的字
    ItemType type;          // 大类
    Element  element;       // 五行属性（法宝/丹药有属性加成）
    int      power;         // 威力：武器=攻击，护甲=防御，丹药=回复量，卷轴=效果强度
    int      price;         // 灵石基准价（奇物/炼材直接卖这个数）
    int      minFloor;      // 最低出现层数（越深的东西越好）
    std::wstring desc;      // 图鉴描述（有味道的文案）
};

// 运行时的一件物品（背包里）
struct Item {
    int defId;
    int count;   // 叠加数量（丹药/卷轴/材料可叠）
    int bonus;   // 祭坛/奇遇洗炼出来的额外威力，默认 0

    Item(int d = -1, int c = 1, int b = 0) : defId(d), count(c), bonus(b) {}
};

// 地上的掉落（带坐标）
struct GroundItem {
    int  x, y;
    Item item;
    GroundItem(int px = 0, int py = 0, Item it = Item()) : x(px), y(py), item(it) {}
};

// 图鉴访问（数据在 items_data.cpp 里）
const std::vector<ItemDef>& itemDex();
const ItemDef& itemDef(int id);

} // namespace yaota
