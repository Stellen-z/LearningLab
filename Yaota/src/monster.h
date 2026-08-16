// monster.h —— 妖怪图鉴定义与运行时实例
#pragma once

#include "types.h"

namespace yaota {

// 妖怪 AI 风格：决定它追你时有多聪明
enum class AiType {
    Melee,    // 普通近战：看见你就直线追
    Brave,    // 凶悍：追击范围更大，攻击加成
    Coward,   // 胆小：血少了一半就逃窜
    Guard,    // 守宝：只守在自己的地盘（房间）里
    Sneaky,   // 狡诈：会绕到侧面偷袭（简单表现为闪避更高）
};

struct MonsterDef {
    int      id;
    std::wstring name;    // 妖名
    wchar_t  glyph;       // 地图上的字
    Element  element;     // 五行
    int      hp;          // 气血
    int      atk;         // 攻击
    int      def;         // 防御
    int      exp;         // 击杀修为
    int      gold;        // 掉灵石
    int      minFloor;    // 出现层数下限
    int      maxFloor;    // 出现层数上限（0 = 不限）
    AiType   ai;          // 脑子
    std::wstring desc;    // 图鉴一句话
};

// 运行时的一只妖怪
struct Monster {
    int  defId;
    int  x, y;
    int  hp;
    int  maxHp;
    bool alive;
    int  stunTurns;   // 被震慑/冰冻的回合数
    int  homeX, homeY; // Guard 型的看守点
    int  poisoned;    // 中毒剩余回合

    Monster(int d = -1, int px = 0, int py = 0)
        : defId(d), x(px), y(py), hp(0), maxHp(0), alive(true), stunTurns(0),
          homeX(px), homeY(py), poisoned(0) {}
};

const std::vector<MonsterDef>& monsterDex();
const MonsterDef& monsterDef(int id);

} // namespace yaota
