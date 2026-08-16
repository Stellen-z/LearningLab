// combat.h —— 战斗结算公式（纯函数，谁都能调）
#pragma once

#include "player.h"
#include "monster.h"
#include "rng.h"

namespace yaota {

struct HitResult {
    bool   dodged = false;
    bool   crit = false;
    int    damage = 0;
    double elemMult = 1.0;   // 五行倍率（>1 克制，<1 被克）
};

// 通用一次攻击结算：atkElem 打 defElem，攻击力 atk 对防御 def
inline HitResult resolveHit(Element atkElem, int atk, Element defElem, int def,
                            double dodgeChance) {
    HitResult r;
    if (Rng::get().chance(dodgeChance)) {
        r.dodged = true;
        return r;
    }
    r.elemMult = elementMultiplier(atkElem, defElem);
    r.crit = Rng::get().chance(0.10); // 10% 暴击
    // 基础伤害 = 攻击 - 防御，浮动的正负 15%
    double base = atk - def * 0.8;
    if (base < 1) base = 1;
    double variance = 0.85 + Rng::get().uniform() * 0.30;
    double dmg = base * variance * r.elemMult;
    if (r.crit) dmg *= 1.8;
    r.damage = std::max(1, (int)dmg);
    return r;
}

// 玩家打妖怪（妖怪闪避看它的 AI 类型）
inline HitResult playerHits(const Player& p, const Monster& m) {
    const MonsterDef& d = monsterDef(m.defId);
    double dodge = 0.08;
    if (d.ai == AiType::Sneaky) dodge = 0.22;
    return resolveHit(p.spirit, p.atk(), d.element, d.def, dodge);
}

// 妖怪打玩家
inline HitResult monsterHits(const Player& p, const Monster& m) {
    const MonsterDef& d = monsterDef(m.defId);
    int atk = d.atk;
    if (d.ai == AiType::Brave) atk = (int)(atk * 1.15); // 凶悍加成
    return resolveHit(d.element, atk, p.spirit, p.def(), 0.05);
}

// AI 胆小判定：残血就跑
inline bool monsterFlees(const Monster& m) {
    const MonsterDef& d = monsterDef(m.defId);
    return d.ai == AiType::Coward && m.hp < m.maxHp / 2;
}

// AI 追击视野半径
inline int aggroRange(const MonsterDef& d) {
    switch (d.ai) {
        case AiType::Brave:  return 9;
        case AiType::Guard:  return 4;
        case AiType::Coward: return 6;
        default:             return 7;
    }
}

} // namespace yaota
