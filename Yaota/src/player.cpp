// player.cpp —— 玩家的成长逻辑
#include "player.h"
#include "rng.h"

namespace yaota {

int Player::atk() const {
    int w = 0;
    if (weaponId >= 0) w = itemDef(weaponId).power + weaponBonus;
    return atkTotal + w;
}

int Player::def() const {
    int a = 0;
    if (armorId >= 0) a = itemDef(armorId).power + armorBonus;
    return defTotal + a;
}

void Player::init(const std::wstring& n, Element spirit) {
    name = n;
    this->spirit = spirit;
    realmIdx = 0;
    exp = 0;
    maxHp = hp = 100;
    maxMp = mp = 30;
    atkTotal = 5;
    defTotal = 2;
    gold = 10;
    floor = 1;
    alive = true;
    weaponId = armorId = -1;
    weaponBonus = armorBonus = 0;
    tribulationBonus = 0;
    inventory.clear();
    // 新手礼包：三样保命的东西
    addItem(40, 2);  // 回气散 x2
    addItem(62, 1);  // 传送符 x1
    addItem(90, 1);  // 灵草 x1
}

bool Player::addItem(int defId, int count, int bonus) {
    if (defId < 0) return false;
    const ItemDef& d = itemDef(defId);
    // 可叠加类型：丹药/卷轴/材料/奇物（无 bonus 差异时并格）
    if (d.type != ItemType::Weapon && d.type != ItemType::Armor && bonus == 0) {
        for (auto& it : inventory) {
            if (it.defId == defId && it.bonus == 0) {
                it.count += count;
                return true;
            }
        }
    }
    if (inventory.size() >= 20) return false; // 背包上限 20 格
    inventory.push_back(Item(defId, count, bonus));
    return true;
}

bool Player::readyToBreak() const {
    // 最高境界之上没有了
    if (realmIdx + 1 >= (int)realms().size()) return false;
    return exp >= realms()[realmIdx].expNeed;
}

bool Player::tribulate(std::wstring& log) {
    if (!readyToBreak()) {
        log = L"修为未满，强行渡劫只会送命。";
        return false;
    }
    const RealmDef& next = realms()[realmIdx + 1];
    double p = next.tribulation + tribulationBonus / 100.0;
    tribulationBonus = 0; // 破障丹只管这一次

    if (Rng::get().chance(p)) {
        exp -= realms()[realmIdx].expNeed; // 消耗修为
        realmIdx++;
        maxHp += next.hpBonus;  hp = maxHp;   // 突破后气血回满
        maxMp += next.mpBonus;  mp = maxMp;
        atkTotal += next.atkBonus;
        defTotal += next.defBonus;
        log = L"【渡劫成功】天地灵气灌体而出，你已迈入" + next.name + L"之境！";
        return true;
    }
    // 失败：重伤但不死（修炼者的倔强）
    exp = (int)(exp * 0.6);
    hp = std::max(1, hp / 2);
    log = L"【渡劫失败】雷劫劈得你皮开肉绽……修为倒退，气血减半。休整后再来。";
    return false;
}

} // namespace yaota
