// save.cpp —— 存档实现
// 设计取舍：只保存玩家状态与层数，进入游戏时重新生成本层地图。
// （完整的地图/妖怪快照序列化太啰嗦，对一局几分钟的 Roguelike 不值得；
//   读档相当于"在同一层重开一条命"，也算 Roguelike 的传统艺能。）
#include "save.h"

#include <fstream>
#include <sstream>
#include <string>

namespace yaota {

static const char* SAVE_PATH = "yaota_save.txt";

bool saveExists() {
    std::ifstream f(SAVE_PATH);
    // 注意：MinGW libstdc++ 下打开失败时 good() 仍可能为 true，必须用 is_open()
    return f.is_open();
}

bool saveToFile(const Player& p, int kills) {
    std::ofstream f(SAVE_PATH, std::ios::trunc);
    if (!f) return false;
    // 名字单独转 UTF-8 存一行，其余按 key value 存
    f << "name " << wstrToUtf8(p.name) << "\n";
    f << "spirit " << (int)p.spirit << "\n";
    f << "realm " << p.realmIdx << "\n";
    f << "exp " << p.exp << "\n";
    f << "hp " << p.hp << " " << p.maxHp << "\n";
    f << "mp " << p.mp << " " << p.maxMp << "\n";
    f << "atk " << p.atkTotal << "\n";
    f << "def " << p.defTotal << "\n";
    f << "gold " << p.gold << "\n";
    f << "floor " << p.floor << "\n";
    f << "weapon " << p.weaponId << " " << p.weaponBonus << "\n";
    f << "armor " << p.armorId << " " << p.armorBonus << "\n";
    f << "trib " << p.tribulationBonus << "\n";
    f << "kills " << kills << "\n";
    for (const auto& it : p.inventory)
        f << "inv " << it.defId << " " << it.count << " " << it.bonus << "\n";
    return true;
}

bool loadFromFile(Player& p, int& kills) {
    std::ifstream f(SAVE_PATH);
    if (!f) return false;

    std::string key;
    Player tmp; // 先读进临时对象，任何一步失败都不污染原状态
    kills = 0;
    bool gotName = false;

    while (f >> key) {
        if (key == "name") {
            std::string rest;
            std::getline(f, rest);
            // 去掉前导空格
            size_t s = rest.find_first_not_of(' ');
            if (s == std::string::npos) s = 0;
            tmp.name = utf8ToWstr(rest.substr(s));
            gotName = true;
        } else if (key == "spirit") { int v; f >> v; tmp.spirit = (Element)v; }
        else if (key == "realm")  f >> tmp.realmIdx;
        else if (key == "exp")    f >> tmp.exp;
        else if (key == "hp")     f >> tmp.hp >> tmp.maxHp;
        else if (key == "mp")     f >> tmp.mp >> tmp.maxMp;
        else if (key == "atk")    f >> tmp.atkTotal;
        else if (key == "def")    f >> tmp.defTotal;
        else if (key == "gold")   f >> tmp.gold;
        else if (key == "floor")  f >> tmp.floor;
        else if (key == "weapon") f >> tmp.weaponId >> tmp.weaponBonus;
        else if (key == "armor")  f >> tmp.armorId >> tmp.armorBonus;
        else if (key == "trib")   f >> tmp.tribulationBonus;
        else if (key == "kills")  f >> kills;
        else if (key == "inv") {
            int d, c, b; f >> d >> c >> b;
            tmp.inventory.push_back(Item(d, c, b));
        }
    }
    if (!gotName || tmp.maxHp <= 0 || tmp.floor < 1 || tmp.floor > 30) return false;
    if (tmp.hp < 1) tmp.hp = 1;

    p = tmp;
    return true;
}

} // namespace yaota
