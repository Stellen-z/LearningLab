// ============================================================================
//  《妖塔》—— 中文修仙 Roguelike（单文件合集版）
// ============================================================================
//  你扮演一名练气期散修，闯随机生成的 30 层妖塔：斩妖、炼丹、夺宝、
//  突破境界，最终渡劫飞升。地图用汉字渲染：仙=你，妖/魔/鬼=妖怪，
//  山=岩壁，梯=楼梯，坛=祭坛（奇遇），草=灵草，箱=宝箱。
//
//  本文件由 src/ 多文件工程按依赖顺序合并而成（详见同目录 README.md）。
//  编译（MinGW 两个参数都不能省，原因见 README"开发实录"）：
//    g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ \
//        -finput-charset=UTF-8 -fexec-charset=UTF-8 yaota_all.cpp -o yaota
//
//  系统一览：五行相生相克 / 九境界渡劫 / 随机地图（房间+走廊+迷雾视野）
//  妖怪图鉴 32 种（4 类 AI）/ 物品 47 件 / 奇遇事件 34 条
//  战斗（克制·暴击·闪避）/ 背包装备 / 存档 / 汉字渲染
//  测试：tests/ 下 28 用例 16000+ 断言全绿，覆盖率见 README
// ============================================================================
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // main.cpp 段的控制台初始化需要
#endif

// ============================== types.h ==============================

// types.h —— 妖塔全局基础类型：五行、境界、通用小工具
// 这个文件是整个游戏的"世界观常量"，改这里就能改游戏平衡。


namespace yaota {

// ============ 五行 ============
// 金木水火土。相克循环：金克木、木克土、土克水、水克火、火克金
// 相生循环：金生水、水生木、木生火、火生土、土生金
enum class Element {
    Jin   = 0, // 金
    Mu    = 1, // 木
    Shui  = 2, // 水
    Huo   = 3, // 火
    Tu    = 4, // 土
};

inline const wchar_t* elementWName(Element e) {
    switch (e) {
        case Element::Jin:  return L"金";
        case Element::Mu:   return L"木";
        case Element::Shui: return L"水";
        case Element::Huo:  return L"火";
        case Element::Tu:   return L"土";
    }
    return L"?";
}

// 攻击方五行 -> 防守方五行 的伤害倍率
// 克制 1.5 | 被克 0.7 | 相生 1.15 | 同属性 1.0
inline double elementMultiplier(Element atk, Element def) {
    // 相克链：金(0)->木(1)->土(4)->水(2)->火(3)->金(0)
    static const int keCycle[5] = { 1, 4, 3, 0, 2 }; // keCycle[i] = i 克的属性
    if (keCycle[(int)atk] == (int)def) return 1.5;   // 我克他
    if (keCycle[(int)def] == (int)atk) return 0.7;   // 他克我
    // 相生链：金(0)->水(2)->木(1)->火(3)->土(4)->金(0)
    static const int shengCycle[5] = { 2, 3, 1, 4, 0 }; // shengCycle[i] = i 生 的属性
    if (shengCycle[(int)atk] == (int)def) return 1.15;  // 我生他（气焰压过去）
    return 1.0;
}

// ============ 境界 ============
struct RealmDef {
    std::wstring name;   // 境界名
    int    expNeed;      // 突破所需修为
    int    hpBonus;      // 突破后气血上限提升
    int    mpBonus;      // 灵力上限提升
    int    atkBonus;     // 攻击提升
    int    defBonus;     // 防御提升
    double tribulation;  // 渡劫成功率
};

// 九大境界，从练气到渡劫。凑够修为后按 T 尝试突破渡劫。
inline const std::vector<RealmDef>& realms() {
    static const std::vector<RealmDef> r = {
        { L"练气",   30,    20,  10,  2,  1, 1.00 },
        { L"筑基",   90,    40,  20,  4,  3, 0.95 },
        { L"金丹",   260,   80,  40,  8,  6, 0.90 },
        { L"元婴",   700,  150,  70, 14, 10, 0.85 },
        { L"化神",  1600,  260, 120, 22, 16, 0.80 },
        { L"炼虚",  3600,  420, 200, 34, 24, 0.75 },
        { L"合体",  8000,  700, 330, 52, 36, 0.70 },
        { L"大乘", 18000, 1100, 520, 78, 55, 0.65 },
        { L"渡劫", 40000, 1800, 800, 118, 82, 0.60 },
    };
    return r;
}

// ============ 地图方块 ============
enum class Tile {
    Wall    = 0, // 山岩
    Floor   = 1, // 石地
    Stairs  = 2, // 上行梯
    Altar   = 3, // 祭坛（触发奇遇）
    Herb    = 4, // 灵草（可采）
    Chest   = 5, // 宝箱
};

// ============ 物品类型 ============
enum class ItemType {
    Weapon,   // 法宝（武器）
    Armor,    // 护甲（法袍）
    Pill,     // 丹药（消耗品）
    Scroll,   // 卷轴（消耗品）
    Treasure, // 奇物（卖钱/特殊）
    Material, // 炼材（卖钱）
};

inline const wchar_t* itemTypeWName(ItemType t) {
    switch (t) {
        case ItemType::Weapon:   return L"法宝";
        case ItemType::Armor:    return L"法袍";
        case ItemType::Pill:     return L"丹药";
        case ItemType::Scroll:   return L"卷轴";
        case ItemType::Treasure: return L"奇物";
        case ItemType::Material: return L"炼材";
    }
    return L"?";
}

// ============ 简单的控制台颜色 ============
enum class Color {
    Default, Red, Green, Yellow, Blue, Magenta, Cyan, White, Dark
};

inline const char* colorCode(Color c) {
    switch (c) {
        case Color::Red:     return "\033[31m";
        case Color::Green:   return "\033[32m";
        case Color::Yellow:  return "\033[33m";
        case Color::Blue:    return "\033[34m";
        case Color::Magenta: return "\033[35m";
        case Color::Cyan:    return "\033[36m";
        case Color::White:   return "\033[37m";
        case Color::Dark:    return "\033[90m";
        default:             return "\033[0m";
    }
}

// ============ 小工具 ============
inline std::wstring to_wstr(const std::string& s) {
    std::wstring ws;
    for (unsigned char c : s) ws += (wchar_t)c; // 仅用于 ASCII 输入
    return ws;
}

// UTF-8 -> 宽字符串（控制台切到 65001 后，窄输入就是 UTF-8 字节）
inline std::wstring utf8ToWstr(const std::string& s) {
    std::wstring out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { out += (wchar_t)c; i += 1; }
        else if ((c >> 5) == 0x6 && i + 1 < s.size()) {
            out += (wchar_t)(((c & 0x1F) << 6) | (s[i + 1] & 0x3F)); i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < s.size()) {
            out += (wchar_t)(((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F));
            i += 3;
        } else { out += L'?'; i += 1; }
    }
    return out;
}

inline std::string wstrToUtf8(const std::wstring& ws) {
    // 简易 UTF-32/宽字符 -> UTF-8 转换（源码字面量都是 BMP 内汉字，够用）
    std::string out;
    for (wchar_t wc : ws) {
        unsigned int v = (unsigned int)wc;
        if (v < 0x80) {
            out += (char)v;
        } else if (v < 0x800) {
            out += (char)(0xC0 | (v >> 6));
            out += (char)(0x80 | (v & 0x3F));
        } else {
            out += (char)(0xE0 | (v >> 12));
            out += (char)(0x80 | ((v >> 6) & 0x3F));
            out += (char)(0x80 | (v & 0x3F));
        }
    }
    return out;
}

} // namespace yaota

// ============================== rng.h ==============================

// rng.h —— 全局随机数源（mt19937），所有随机都从这里走，方便做种子复现


namespace yaota {

class Rng {
public:
    static Rng& get() {
        static Rng inst;
        return inst;
    }

    void seed(unsigned s) { eng_.seed(s); }

    // [a, b] 闭区间整数
    int range(int a, int b) {
        std::uniform_int_distribution<int> d(a, b);
        return d(eng_);
    }

    // 0.0 ~ 1.0
    double uniform() {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        return d(eng_);
    }

    // 概率为 p 的事件是否发生
    bool chance(double p) { return uniform() < p; }

    // 从向量里随机挑一个
    template <typename T>
    const T& pick(const std::vector<T>& v) {
        return v[range(0, (int)v.size() - 1)];
    }

    // 掷 n 个骰子（打宝、掉落常用）
    int dice(int count, int sides) {
        int sum = 0;
        for (int i = 0; i < count; ++i) sum += range(1, sides);
        return sum;
    }

private:
    Rng() : eng_(std::random_device{}()) {}
    std::mt19937 eng_;
};

} // namespace yaota

// ============================== item.h ==============================

// item.h —— 物品定义（图鉴条目）与运行时实例


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

// ============================== monster.h ==============================

// monster.h —— 妖怪图鉴定义与运行时实例


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

// ============================== events.h ==============================

// events.h —— 奇遇事件：踩到祭坛(坛)时随机触发一个


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

// ============================== dungeon.h ==============================

// dungeon.h —— 随机地图生成：房间 + 走廊 + 楼梯 + 落物点


namespace yaota {

struct Room {
    int x, y, w, h; // 左上角 + 尺寸
    int cx() const { return x + w / 2; }
    int cy() const { return y + h / 2; }
    bool intersects(const Room& o) const {
        return x - 1 < o.x + o.w + 1 && x + w + 1 > o.x - 1 &&
               y - 1 < o.y + o.h + 1 && y + h + 1 > o.y - 1;
    }
};

class Dungeon {
public:
    static constexpr int W = 48;   // 地图宽（格）
    static constexpr int H = 22;   // 地图高（格）

    void generate(int floor, int playerX, int playerY);
    void clear();

    Tile  at(int x, int y) const;
    Tile& at(int x, int y);
    bool  inBounds(int x, int y) const;
    bool  walkable(int x, int y) const;          // 石地/楼梯/祭坛/灵草/宝箱可走
    bool  blocksSight(int x, int y) const;       // 山岩挡视线

    const std::vector<Room>& rooms() const { return rooms_; }

    // 生成时顺带布置的活物与掉落（Game 会搬走并清空）
    std::vector<Monster>&      spawnedMonsters() { return monsters_; }
    std::vector<GroundItem>&   spawnedGroundItems() { return groundItems_; }

    // 楼梯位置
    int stairsX() const { return stairsX_; }
    int stairsY() const { return stairsY_; }

    // 距离某点最远的房间中心（放楼梯用）
    const Room& farthestRoomFrom(int px, int py) const;

private:
    void carveRoom(const Room& r);
    void carveCorridor(int x1, int y1, int x2, int y2);
    void placeFeatures(int floor, int playerX, int playerY);

    std::vector<Tile>        tiles_;
    std::vector<Room>        rooms_;
    std::vector<Monster>     monsters_;
    std::vector<GroundItem>  groundItems_;
    int stairsX_ = 0, stairsY_ = 0;
};

} // namespace yaota

// ============================== player.h ==============================

// player.h —— 玩家：属性、境界、背包、装备


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

// ============================== combat.h ==============================

// combat.h —— 战斗结算公式（纯函数，谁都能调）


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

// ============================== render.h ==============================

// render.h —— 控制台渲染：汉字地图、状态栏、行事录、菜单



namespace yaota {

class Renderer {
public:
    // ---- 基础输出 ----
    static void clearScreen();
    static void print(const std::wstring& s, Color c = Color::Default, bool bold = false);
    static void println(const std::wstring& s, Color c = Color::Default, bool bold = false);
    static std::wstring promptLine(const std::wstring& prompt);
    static void pause();

    // ---- 菜单：返回下标；allowCancel 时玩家可选 . 取消并返回 -1 ----
    static int menu(const std::wstring& title, const std::vector<std::wstring>& options,
                    bool allowCancel = true);
    static bool askYesNo(const std::wstring& question);

    // ---- 游戏画面 ----
    // explored 是"迷雾记忆"（走过/看过的格子），本函数会顺带更新它
    static void drawMap(const Dungeon& d, const Player& p,
                        const std::vector<Monster>& monsters,
                        const std::vector<GroundItem>& ground,
                        bool revealAll, std::vector<char>& explored);
    static void drawHud(const Player& p, int kills);
    static void drawLog(const std::vector<std::wstring>& logs);

    // ---- 静态页 ----
    static void drawHelp();
    static void drawMonstersDex();

private:
    static bool lineOfSight(const Dungeon& d, int x0, int y0, int x1, int y1);
    static Color elementColor(Element e);
    static Color itemColor(ItemType t);
    static void drawBar(int cur, int max, int width, Color c);
};

} // namespace yaota

// ============================== save.h ==============================

// save.h —— 存档：玩家状态 + 层数（地图为重新生成，见 save.cpp 注释）


namespace yaota {

bool saveExists();
// 存档写入 yaota_save.txt（与可执行文件同目录）
bool saveToFile(const Player& p, int kills);
// 读取成功返回 true 并填充 p 与 kills
bool loadFromFile(Player& p, int& kills);

} // namespace yaota

// ============================== game.h ==============================

// game.h —— 游戏总控：把所有系统粘成"一局游戏"



namespace yaota {

class Game {
    // 测试后门：单元测试通过 GamePeer 触达私有玩法逻辑（见 tests/test_all.cpp）
    friend class GamePeer;

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
    void log(const std::wstring& msg, Color = Color::Default) {
        logs_.push_back(msg);
        if (logs_.size() > 64) logs_.erase(logs_.begin(), logs_.end() - 64); // 防止无上限增长
    }
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

// GamePeer —— 测试专用转发器：以友元身份把非交互的私有玩法逻辑
// 暴露成可直接调用的静态函数。交互式流程（菜单/暂停/主循环）不在此列，
// 它们由脚本灌入式冒烟测试覆盖。
class GamePeer {
public:
    static void move(Game& g, int dx, int dy)      { g.handleMove(dx, dy); }
    static void pickup(Game& g)                    { g.tryPickup(); }
    static void stairs(Game& g)                    { g.tryStairs(); }
    static void meditate(Game& g)                  { g.meditate(); }
    static void burst(Game& g)                     { g.spiritBurst(); }
    static void refine(Game& g)                    { g.refineJunk(); }
    static void monsterTurns(Game& g)              { g.monsterTurns(); }
    static void attack(Game& g, Monster& m)        { g.attackMonster(m); }
    static bool use(Game& g, size_t i)             { return g.useItem(i); }
    static void equip(Game& g, size_t i)           { g.equipItem(i); }
    static void drop(Game& g, size_t i)            { g.dropItem(i); }
    static void eventFx(Game& g, int fx, std::wstring& out) { g.applyEventFx(fx, out); }
    static void endTurn(Game& g)                   { g.endTurn(); }

    static Dungeon& dungeon(Game& g)               { return g.dungeon_; }
    static std::vector<GroundItem>& ground(Game& g){ return g.ground_; }
    static std::vector<std::wstring>& logs(Game& g){ return g.logs_; }
    static bool& revealAll(Game& g)                { return g.revealAll_; }
    static bool over(Game& g)                      { return g.over_; }
    static int kills(Game& g)                      { return g.kills_; }
};

} // namespace yaota

// ============================== dungeon.cpp ==============================

// dungeon.cpp —— 妖塔每一层的随机生成
// 思路（经典 Roguelike 做法）：
//   1. 整张图先填满山岩
//   2. 随机撒若干不重叠的矩形房间，挖空成石地
//   3. 把房间中心按顺序用 L 形走廊连起来
//   4. 离玩家最远的房间放楼梯
//   5. 撒妖怪、掉落、祭坛、灵草、宝箱


namespace yaota {

void Dungeon::clear() {
    tiles_.assign(W * H, Tile::Wall);
    rooms_.clear();
    monsters_.clear();
    groundItems_.clear();
    stairsX_ = stairsY_ = -1;
}

void Dungeon::generate(int floor, int playerX, int playerY) {
    clear();

    // ---- 1. 撒房间：尝试 N 次，留下互不重叠的 ----
    const int wantRooms = 5 + std::min(4, floor / 6); // 深层更大更复杂
    for (int attempt = 0; attempt < 60 && (int)rooms_.size() < wantRooms; ++attempt) {
        Room r;
        r.w = Rng::get().range(4, 9);
        r.h = Rng::get().range(3, 6);
        r.x = Rng::get().range(1, W - r.w - 2);
        r.y = Rng::get().range(1, H - r.h - 2);
        bool ok = true;
        for (const Room& old : rooms_) {
            if (r.intersects(old)) { ok = false; break; }
        }
        if (ok) rooms_.push_back(r);
    }
    // 保底：万一一次都没生成（几乎不可能），硬塞一个中间大房间
    if (rooms_.empty()) rooms_.push_back({ W / 2 - 4, H / 2 - 3, 9, 6 });

    for (const Room& r : rooms_) carveRoom(r);

    // ---- 2. L 形走廊把房间串成一条链 ----
    for (size_t i = 1; i < rooms_.size(); ++i) {
        carveCorridor(rooms_[i - 1].cx(), rooms_[i - 1].cy(),
                      rooms_[i].cx(),     rooms_[i].cy());
    }
    // 再随机补一两条捷径走廊，让地图有环、可以绕路逃生
    if (rooms_.size() >= 4) {
        int extra = Rng::get().range(1, 2);
        for (int i = 0; i < extra; ++i) {
            const Room& a = Rng::get().pick(rooms_);
            const Room& b = Rng::get().pick(rooms_);
            if (&a != &b) carveCorridor(a.cx(), a.cy(), b.cx(), b.cy());
        }
    }

    // ---- 3. 楼梯放在离玩家最远的房间 ----
    const Room& far = farthestRoomFrom(playerX, playerY);
    stairsX_ = far.cx();
    stairsY_ = far.cy();
    at(stairsX_, stairsY_) = Tile::Stairs;

    placeFeatures(floor, playerX, playerY);
}

void Dungeon::carveRoom(const Room& r) {
    for (int y = r.y; y < r.y + r.h; ++y)
        for (int x = r.x; x < r.x + r.w; ++x)
            if (inBounds(x, y)) at(x, y) = Tile::Floor;
}

void Dungeon::carveCorridor(int x1, int y1, int x2, int y2) {
    // 先横后竖（或先竖后横，随机），路过的地方挖成石地
    if (Rng::get().chance(0.5)) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
            if (inBounds(x, y1) && at(x, y1) == Tile::Wall) at(x, y1) = Tile::Floor;
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
            if (inBounds(x2, y) && at(x2, y) == Tile::Wall) at(x2, y) = Tile::Floor;
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
            if (inBounds(x1, y) && at(x1, y) == Tile::Wall) at(x1, y) = Tile::Floor;
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
            if (inBounds(x, y2) && at(x, y2) == Tile::Wall) at(x, y2) = Tile::Floor;
    }
}

const Room& Dungeon::farthestRoomFrom(int px, int py) const {
    const Room* best = &rooms_[0];
    double bestD = -1;
    for (const Room& r : rooms_) {
        double d = std::hypot(r.cx() - px, r.cy() - py);
        if (d > bestD) { bestD = d; best = &r; }
    }
    return *best;
}

void Dungeon::placeFeatures(int floor, int playerX, int playerY) {
    // 在房间里随机挑一个内部点
    auto roomSpot = [&](const Room& r) {
        int x = Rng::get().range(r.x, r.x + r.w - 1);
        int y = Rng::get().range(r.y, r.y + r.h - 1);
        return std::make_pair(x, y);
    };

    // ---- 妖怪：数量随层数上涨，出生点离玩家至少 6 格 ----
    int monsterCount = std::min(12, 4 + floor / 2);
    auto& mdex = monsterDex();
    for (int i = 0; i < monsterCount; ++i) {
        // 收集本层出没的妖怪
        std::vector<int> eligible;
        for (const auto& m : mdex)
            if (floor >= m.minFloor && (m.maxFloor == 0 || floor <= m.maxFloor))
                eligible.push_back(m.id);
        if (eligible.empty()) break;

        for (int tries = 0; tries < 30; ++tries) {
            auto [x, y] = roomSpot(Rng::get().pick(rooms_));
            double dist = std::hypot(x - playerX, y - playerY);
            if (dist < 6) continue;
            if (!walkable(x, y) || at(x, y) == Tile::Stairs) continue;

            int id = Rng::get().pick(eligible);
            const MonsterDef& def = monsterDef(id);
            Monster m(id, x, y);
            // 深层妖怪气血小幅成长（每 5 层 +15%）
            double scale = 1.0 + (floor / 5) * 0.15;
            m.maxHp = m.hp = (int)(def.hp * scale);
            monsters_.push_back(m);
            break;
        }
    }

    // ---- 地上掉落 ----
    auto& idex = itemDex();
    int dropCount = Rng::get().range(2, 4);
    for (int i = 0; i < dropCount; ++i) {
        std::vector<int> eligible;
        for (const auto& it : idex)
            if (floor >= it.minFloor) eligible.push_back(it.id);
        if (eligible.empty()) break;
        for (int tries = 0; tries < 20; ++tries) {
            auto [x, y] = roomSpot(Rng::get().pick(rooms_));
            if (at(x, y) != Tile::Floor) continue;
            groundItems_.push_back(GroundItem(x, y, Item(Rng::get().pick(eligible))));
            break;
        }
    }

    // ---- 祭坛（奇遇）、灵草（采了得材料）、宝箱 ----
    int altars = Rng::get().range(0, 2);
    for (int i = 0; i < altars; ++i) {
        auto [x, y] = roomSpot(Rng::get().pick(rooms_));
        if (at(x, y) == Tile::Floor) at(x, y) = Tile::Altar;
    }
    int herbs = Rng::get().range(1, 3);
    for (int i = 0; i < herbs; ++i) {
        auto [x, y] = roomSpot(Rng::get().pick(rooms_));
        if (at(x, y) == Tile::Floor) at(x, y) = Tile::Herb;
    }
    int chests = Rng::get().range(0, 1 + (floor >= 10 ? 1 : 0));
    for (int i = 0; i < chests; ++i) {
        auto [x, y] = roomSpot(Rng::get().pick(rooms_));
        if (at(x, y) == Tile::Floor) at(x, y) = Tile::Chest;
    }
}

// ---- 基础查询 ----
Tile  Dungeon::at(int x, int y) const { return tiles_[(size_t)y * W + x]; }
Tile& Dungeon::at(int x, int y)       { return tiles_[(size_t)y * W + x]; }
bool  Dungeon::inBounds(int x, int y) const { return x >= 0 && x < W && y >= 0 && y < H; }

bool Dungeon::walkable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    Tile t = at(x, y);
    return t == Tile::Floor || t == Tile::Stairs || t == Tile::Altar ||
           t == Tile::Herb || t == Tile::Chest;
}

bool Dungeon::blocksSight(int x, int y) const {
    if (!inBounds(x, y)) return true;
    return at(x, y) == Tile::Wall;
}

} // namespace yaota

// ============================== items_data.cpp ==============================

// items_data.cpp —— 物品图鉴全表（法宝 / 法袍 / 丹药 / 卷轴 / 奇物 / 炼材）
// 平衡设计：
//   * 法宝 power = 攻击加成；法袍 power = 防御加成
//   * 丹药 power = 回复量（特殊丹药靠 id 在使用逻辑里特判）
//   * minFloor 控制"越深的层出越好的东西"

namespace yaota {

const std::vector<ItemDef>& itemDex() {
    static const std::vector<ItemDef> dex = {
        // ================= 法宝（武器） =================
        {  1, L"铁剑",       L'剑', ItemType::Weapon, Element::Jin,   3,   15,  1, L"凡铁打造，剑刃有缺口，聊胜于无。" },
        {  2, L"青竹杖",     L'杖', ItemType::Weapon, Element::Mu,    4,   18,  1, L"后山砍的青竹，自带三分草木灵气。" },
        {  3, L"火浣鞭",     L'鞭', ItemType::Weapon, Element::Huo,   6,   40,  3, L"鞭梢缠着火浣纱，抽在人身上会烧起来。" },
        {  4, L"玄冰刺",     L'刺', ItemType::Weapon, Element::Shui,  7,   48,  4, L"寒潭底捞出的冰锥，千年不化。" },
        {  5, L"裂山斧",     L'斧', ItemType::Weapon, Element::Tu,    9,   70,  6, L"斧刃过处，山石如豆腐般裂开。" },
        {  6, L"银月双环",   L'环', ItemType::Weapon, Element::Jin,  11,   95,  8, L"一对银环，掷出时如月牙交叠。" },
        {  7, L"缠魂藤",     L'藤', ItemType::Weapon, Element::Mu,   12,  105,  9, L"妖藤炼制，缠住敌人生生吸取精血。" },
        {  8, L"赤霄剑",     L'剑', ItemType::Weapon, Element::Huo,  15,  150, 12, L"剑身赤红，出鞘时有龙吟，传闻沾过龙血。" },
        {  9, L"碧波刃",     L'刃', ItemType::Weapon, Element::Shui, 16,  160, 13, L"薄如蝉翼的水属性飞刃，杀人不见血。" },
        { 10, L"崩岳锤",     L'锤', ItemType::Weapon, Element::Tu,   19,  210, 15, L"一锤落下，方圆丈内地面龟裂。" },
        { 11, L"诛邪剑",     L'剑', ItemType::Weapon, Element::Jin,  24,  300, 18, L"剑格上刻着\"诛邪\"二字，专斩妖魔。" },
        { 12, L"建木枝",     L'枝', ItemType::Weapon, Element::Mu,   26,  330, 20, L"神木建木的一根断枝，隐隐有生机流转。" },
        { 13, L"焚天戟",     L'戟', ItemType::Weapon, Element::Huo,  31,  420, 23, L"传说此戟一出，天都要被烧穿一个洞。" },
        { 14, L"弱水绫",     L'绫', ItemType::Weapon, Element::Shui, 33,  450, 25, L"轻若鸿毛，可弱水三千，触之即沉。" },
        { 15, L"撼地印",     L'印', ItemType::Weapon, Element::Tu,   38,  560, 27, L"上古大能的镇物法印，一印可撼山岳。" },

        // ================= 法袍（护甲） =================
        { 20, L"粗布道袍",   L'袍', ItemType::Armor, Element::Tu,     2,   10,  1, L"洗得发白的旧道袍，胜在透气。" },
        { 21, L"藤甲",       L'甲', ItemType::Armor, Element::Mu,     3,   20,  2, L"山中妖藤编成，轻便结实。" },
        { 22, L"铜鳞软甲",   L'甲', ItemType::Armor, Element::Jin,    5,   45,  5, L"一片片铜鳞缀成，刀剑难透。" },
        { 23, L"水纹长衫",   L'衫', ItemType::Armor, Element::Shui,   7,   70,  8, L"衫上水纹流转，可卸去大力。" },
        { 24, L"火浣衣",     L'衣', ItemType::Armor, Element::Huo,    9,  100, 11, L"入火不焚，越烧越亮。" },
        { 25, L"玄龟甲袍",   L'袍', ItemType::Armor, Element::Shui,  12,  150, 14, L"以千年玄龟腹甲磨制，厚重难破。" },
        { 26, L"金丝仙衣",   L'衣', ItemType::Armor, Element::Jin,   15,  210, 17, L"金丝银线织就，刀枪不入，水火不侵。" },
        { 27, L"万年沉木甲", L'甲', ItemType::Armor, Element::Mu,    18,  280, 20, L"沉万年之木，韧过精钢。" },
        { 28, L"离火法袍",   L'袍', ItemType::Armor, Element::Huo,   22,  380, 24, L"袍上绣着离火符文，妖物近身即灼。" },
        { 29, L"后土宝铠",   L'铠', ItemType::Armor, Element::Tu,    26,  500, 27, L"承后土之德，穿它的人很难被杀死。" },

        // ================= 丹药 =================
        // power = 回复量；特殊效果按 id 特判
        { 40, L"回气散",     L'丹', ItemType::Pill, Element::Mu,    40,   12,  1, L"最粗浅的药散，运气回气聊应急。" },
        { 41, L"小还丹",     L'丹', ItemType::Pill, Element::Mu,   100,   35,  4, L"外伤内损皆可用，散修居家必备。" },
        { 42, L"大还丹",     L'丹', ItemType::Pill, Element::Mu,   260,   90, 12, L"只要还有一口气，一颗下去就能爬起来。" },
        { 43, L"九转续命丹", L'丹', ItemType::Pill, Element::Mu,   600,  260, 20, L"九转丹成，阎王见了都摇头。" },
        { 44, L"聚灵丹",     L'丹', ItemType::Pill, Element::Shui,  40,   60,  6, L"服下后灵气汇聚，修为噌噌上涨。（+40 修为）" },
        { 45, L"培元丹",     L'丹', ItemType::Pill, Element::Tu,    20,   80,  8, L"固本培元，气血上限永久 +20。" },
        { 46, L"洗髓丹",     L'丹', ItemType::Pill, Element::Jin,    2,  120, 10, L"洗经伐髓，攻击永久 +2。" },
        { 47, L"金刚丹",     L'丹', ItemType::Pill, Element::Tu,    2,  120, 10, L"筋骨如金刚，防御永久 +2。" },
        { 48, L"破障丹",     L'丹', ItemType::Pill, Element::Huo,   15,  150,  5, L"冲关破障，下一次渡劫成功率 +15%。" },
        { 49, L"解毒丹",     L'丹', ItemType::Pill, Element::Mu,     0,   25,  2, L"一味解毒的常备药，苦得皱眉。" },
        { 50, L"龟息丹",     L'丹', ItemType::Pill, Element::Shui, 999,  200, 15, L"回复一半上限的气血，气息绵长如龟。" },

        // ================= 卷轴 =================
        // power = 效果强度
        { 60, L"火球符",     L'符', ItemType::Scroll, Element::Huo,  30,   30,  3, L"掷出即炸，身边妖怪统统吃一发火球。" },
        { 61, L"冰封符",     L'符', ItemType::Scroll, Element::Shui,  3,   45,  5, L"寒气扩散，冻住身边的妖怪动弹不得。" },
        { 62, L"传送符",     L'符', ItemType::Scroll, Element::Tu,    0,   25,  2, L"撕开就走，随机传到本层某处。" },
        { 63, L"天眼符",     L'符', ItemType::Scroll, Element::Jin,   0,   40,  4, L"开天眼，本层地图尽收眼底。" },
        { 64, L"摄妖符",     L'符', ItemType::Scroll, Element::Jin,   2,   70,  8, L"符光一闪，视野内的妖怪集体一僵。" },
        { 65, L"五雷符",     L'符', ItemType::Scroll, Element::Jin, 120,  110, 12, L"引天雷入符，劈最近的妖怪一下狠的。" },

        // ================= 奇物（值钱） =================
        { 80, L"妖丹",       L'丹', ItemType::Treasure, Element::Tu,    0,   30,  1, L"妖怪体内凝结的精丹，值点灵石。" },
        { 81, L"千年灵芝",   L'芝', ItemType::Treasure, Element::Mu,    0,   60,  6, L"千年灵物，药香扑鼻，卖个好价钱。" },
        { 82, L"龙鳞",       L'鳞', ItemType::Treasure, Element::Jin,   0,  150, 14, L"真正的龙鳞！大妖见了都要眼红。" },
        { 83, L"凤羽",       L'羽', ItemType::Treasure, Element::Huo,   0,  300, 22, L"凤凰遗羽，握在手里微微发烫。" },
        { 84, L"混沌石",     L'石', ItemType::Treasure, Element::Tu,    0,  600, 26, L"天地初开时的一粒混沌，无价之宝。" },

        // ================= 炼材（卖钱） =================
        { 90, L"灵草",       L'草', ItemType::Material, Element::Mu,    0,   12,  1, L"带着灵气的野草，炼丹的底料。" },
        { 91, L"铁矿石",     L'矿', ItemType::Material, Element::Jin,   0,    8,  1, L"黑黢黢的矿石，炼器坊按斤收。" },
        { 92, L"妖骨",       L'骨', ItemType::Material, Element::Tu,    0,   18,  3, L"妖怪的骨头，硬而不朽。" },
        { 93, L"星辰砂",     L'砂', ItemType::Material, Element::Huo,   0,   45, 10, L"夜里会发光的细砂，据说是星星的碎屑。" },
    };
    return dex;
}

const ItemDef& itemDef(int id) {
    for (const auto& d : itemDex())
        if (d.id == id) return d;
    return itemDex().front(); // 不该发生
}

} // namespace yaota

// ============================== monsters_data.cpp ==============================

// monsters_data.cpp —— 妖怪图鉴全表（32 种，从野狗妖到守层者）
// 平衡设计：
//   * exp/gold 大致跟 hp+atk 走，深层妖更肥
//   * ai 决定行为：Brave 追得远打得狠，Coward 会逃，Guard 只守房间，Sneaky 闪避高

namespace yaota {

const std::vector<MonsterDef>& monsterDex() {
    static const std::vector<MonsterDef> dex = {
        {  0, L"野狗妖",     L'妖', Element::Jin,   15,  5,  1,   8,   3,  1,  5, AiType::Melee,  L"成精的野狗，只会龇牙，塔底一抓一大把。" },
        {  1, L"树精",       L'妖', Element::Mu,    25,  6,  3,  12,   5,  1,  6, AiType::Guard,   L"扎根在塔里的老树成精，挪不动窝，也别想绕过它。" },
        {  2, L"石魄",       L'妖', Element::Tu,    30,  7,  6,  15,   6,  2,  8, AiType::Guard,   L"一块山岩吸了妖气有了意识，皮糙肉厚。" },
        {  3, L"火鼠",       L'妖', Element::Huo,   18,  9,  2,  14,   7,  2,  7, AiType::Sneaky,  L"浑身冒火的小老鼠，滑不留手，还会偷丹药。" },
        {  4, L"水鬼",       L'鬼', Element::Shui,  22,  8,  3,  15,   8,  3,  9, AiType::Melee,   L"淹死在塔中暗河的冤魂，见人就拖下水。" },
        {  5, L"骷髅兵",     L'骨', Element::Jin,   26, 10,  4,  17,   9,  3, 10, AiType::Melee,   L"生前是守塔的士卒，死后还在巡逻。" },
        {  6, L"鬼火",       L'火', Element::Huo,   12, 11,  0,  16,  10,  4, 10, AiType::Sneaky,  L"幽幽一点绿火，飘忽不定，沾上就烧。" },
        {  7, L"狼妖",       L'妖', Element::Jin,   35, 12,  4,  22,  12,  5, 12, AiType::Brave,   L"塔中狼群的头狼，记仇，一旦结仇追你三层。" },
        {  8, L"毒蛛",       L'妖', Element::Mu,    28, 11,  3,  21,  11,  5, 12, AiType::Sneaky,  L"指甲盖大的毒牙，咬一口连着疼好几下。" },
        {  9, L"食人花",     L'妖', Element::Mu,    45, 13,  5,  26,  14,  6, 13, AiType::Guard,   L"花开艳丽，花下白骨累累。" },
        { 10, L"岩甲龟",     L'妖', Element::Tu,    70, 10, 12,  30,  16,  7, 15, AiType::Guard,   L"缩进壳里的时候，趁早绕道。" },
        { 11, L"火蜥蜴",     L'妖', Element::Huo,   40, 16,  6,  28,  15,  7, 14, AiType::Melee,   L"在岩浆里泡澡的蜥蜴，脾气火爆。" },
        { 12, L"寒冰鲤",     L'妖', Element::Shui,  38, 15,  7,  27,  15,  8, 15, AiType::Melee,   L"离水也能活的冰鲤，尾巴扫过来像鞭子抽。" },
        { 13, L"妖狐",       L'妖', Element::Mu,    50, 17,  6,  34,  20,  9, 16, AiType::Coward,  L"九条尾巴只剩三条，狡猾得很，打不过就跑。" },
        { 14, L"白骨将军",   L'骨', Element::Jin,   60, 18,  8,  40,  24, 10, 18, AiType::Brave,   L"锈迹斑斑的盔甲，枪法却一点没锈。" },
        { 15, L"血蝠",       L'妖', Element::Huo,   45, 17,  4,  33,  18, 11, 18, AiType::Coward,  L"吸饱了血就飞走，饿疯了才回来拼命。" },
        { 16, L"罗刹鬼",     L'鬼', Element::Huo,   55, 20,  6,  38,  22, 10, 18, AiType::Sneaky,  L"笑起来最好看的鬼，出手也最黑。" },
        { 17, L"双头蛇",     L'妖', Element::Huo,   65, 19,  8,  40,  23, 12, 19, AiType::Melee,   L"两个头商量好了一起咬你。" },
        { 18, L"幽冥水母",   L'妖', Element::Shui,  58, 20,  7,  39,  22, 13, 20, AiType::Sneaky,  L"飘在半空，伞盖下拖着长长的毒丝。" },
        { 19, L"黑山老妖",   L'妖', Element::Tu,    90, 18, 12,  48,  30, 12, 20, AiType::Guard,   L"盘踞一方的老妖怪，塔都得给它让地方。" },
        { 20, L"铁背蜈蚣",   L'妖', Element::Jin,   75, 18, 11,  44,  26, 13, 21, AiType::Melee,   L"甲壳比铁还硬，一百多只脚跑得飞快。" },
        { 21, L"灵猿",       L'妖', Element::Mu,    70, 21,  8,  46,  28, 14, 22, AiType::Brave,   L"通人性的白猿，抢你的丹药还会朝你做鬼脸。" },
        { 22, L"玄龟",       L'妖', Element::Shui, 110, 16, 16,  52,  32, 15, 24, AiType::Guard,   L"驮着座小山似的龟壳，慢，但你也打不动它。" },
        { 23, L"赤炎魔",     L'魔', Element::Huo,   85, 26, 10,  55,  36, 16, 25, AiType::Brave,   L"由塔中怨火凝成的魔物，越烧越旺。" },
        { 24, L"剑灵",       L'灵', Element::Jin,   80, 28,  9,  54,  35, 17, 26, AiType::Sneaky,  L"前人遗剑所化之灵，出剑快过你的眼睛。" },
        { 25, L"妖僧",       L'妖', Element::Tu,   100, 22, 14,  58,  40, 18, 27, AiType::Guard,   L"念的是魔经，敲的是妖鼓，度的是它自己。" },
        { 26, L"九婴蛇",     L'妖', Element::Shui, 120, 27, 13,  64,  45, 20, 28, AiType::Melee,   L"九个脑袋轮流咬，断一个还有八个。" },
        { 27, L"雷兽",       L'妖', Element::Jin,   105, 30, 12,  66,  48, 21, 29, AiType::Brave,   L"吼一声就是一道雷，塔顶的雷都是它招的。" },
        { 28, L"修罗",       L'魔', Element::Huo,   130, 33, 14,  74,  55, 23, 30, AiType::Brave,   L"战意化形的杀神，见了它就别想好好说话。" },
        { 29, L"太阴魔君",   L'魔', Element::Shui, 150, 30, 18,  82,  62, 25, 30, AiType::Sneaky,  L"月色越深它越强，塔中三十层的阴影都是它的。" },
        { 30, L"混沌魔兽",   L'魔', Element::Tu,   180, 35, 20,  95,  75, 27, 30, AiType::Melee,   L"天地杂质所化的凶兽，没有形状，只有饿。" },
        { 31, L"妖塔守层者", L'塔', Element::Jin,  200, 38, 22, 120,  99, 28, 30, AiType::Brave,   L"塔灵亲手捏的守卫，杀它等于在拆塔。" },
    };
    return dex;
}

const MonsterDef& monsterDef(int id) {
    for (const auto& d : monsterDex())
        if (d.id == id) return d;
    return monsterDex().front(); // 不该发生
}

} // namespace yaota

// ============================== events_data.cpp ==============================

// events_data.cpp —— 奇遇事件全表（34 条）
// 设计原则：每个选择都有代价或风险，没有白给的午餐；文案要有"妖塔味"。

namespace yaota {

const std::vector<EventDef>& eventDex() {
    static const std::vector<EventDef> dex = {
        { 1, L"破旧祭坛", L"一座布满裂痕的石祭坛，香炉里还插着三根没烧完的残香。坛前刻着一行小字：心诚则灵，心贪则焚。",
            {
                { L"上三炷香，诚心祈愿", L"香烟袅袅升起，一缕暖流涌入四肢百骸。", 20, 10, 8, 0, FxNone },
                { L"把香炉里的残香摸走", L"指尖刚碰到香炉，一股焦糊味窜上天灵盖——祭坛生气了。", -15, 0, 0, 0, FxRandomPill },
                { L"磕三个响头就走", L"礼多人不怪，妖坛也一样。你隐约觉得功德+1。", 5, 5, 3, 0, FxNone },
            } },
        { 2, L"无名道人遗蜕", L"墙角盘坐着一具修士遗骨，僧袍虽朽，脊背却挺得笔直。他手边放着一个褪色的储物袋。",
            {
                { L"取走储物袋", L"袋里哗啦啦倒出一把灵石。道人遗骨忽然散了架——像是叹了口气。", 0, 0, -5, 40, FxNone },
                { L"叩首行礼，不取一物", L"你三拜而起，忽觉丹田一热，似有前辈护佑。", 10, 10, 15, 0, FxNone },
                { L"以灵力护住遗骨", L"你渡入一缕灵力，遗骨化尘而散，原地留下一颗温润的丹药。", -10, 0, 5, 0, FxRandomPill },
            } },
        { 3, L"血色池潭", L"一间石室中央蓄着一池暗红色的液体，表面无风自动。池边石壁上刻满密文，大多被血渍糊住了。",
            {
                { L"跳进去泡一泡", L"滚烫！像被扔进炼丹炉！熬过最初十息后，四肢百骸竟暖洋洋的。", -20, 0, 25, 0, FxMaxHpUp },
                { L"装一瓶带走", L"你灌了一小瓶。瓶子入手冰凉，不知是福是祸。", 0, 0, 0, 0, FxRandomItem },
                { L"绕着走", L"君子不立危墙之下。你贴墙绕过血池，什么也没发生。", 0, 0, 0, 0, FxNothing },
            } },
        { 4, L"残破剑冢", L"荒地上斜插着上百柄断剑，剑身锈迹斑斑，却仍隐隐有剑鸣之声，如泣如诉。",
            {
                { L"尝试拔出最深处的断剑", L"你双手握柄，气血翻涌——剑没拔出来，虎口先裂了。但那一瞬间你好像摸到了一点剑意。", -10, 0, 20, 0, FxLearnAtk },
                { L"祭拜剑冢", L"你整衣下拜。剑鸣声渐止，仿佛百年孤独终于有人听见。", 5, 5, 10, 0, FxNone },
                { L"拆几块剑铁卖钱", L"好铁！炼器坊最爱这种老料。", 0, 0, 0, 25, FxNone },
            } },
        { 5, L"妖市小贩", L"一个獐头鼠目的妖修支着地摊，货架上摆着几粒来路不明的丹药。他冲你挤挤眼：\"道友，看看货？\"",
            {
                { L"买丹药（-30 灵石）", L"小贩麻利地递来丹药，顺手抹了下鼻子。希望这丹没掺面粉。", 0, 0, 0, -30, FxRandomPill },
                { L"卖掉背包里的破烂", L"小贩挑挑拣拣，扔给你一小袋灵石。", 0, 0, 0, 30, FxNone },
                { L"转身就走", L"小贩在你背后嘟囔：\"不买别摸啊……\"", 0, 0, 0, 0, FxNothing },
            } },
        { 6, L"古镜", L"一面铜镜嵌在石壁中，镜面光可鉴人。你凑近时，镜中的\"你\"却慢了半拍才抬头。",
            {
                { L"仔细端详镜中人", L"镜中人对你露出一个不属于你的微笑。你惊出一身冷汗，却在此刻心境澄明。", -5, 20, 18, 0, FxNone },
                { L"一拳砸碎", L"铜镜应声而碎。碎片里流出一摊银液，凝成了灵石。", -8, 0, 0, 35, FxNone },
                { L"不敢多看，快步离开", L"身后传来极轻的一声叹息。你没有回头。", 0, 0, 0, 0, FxNothing },
            } },
        { 7, L"石桌残局", L"石桌上刻着一盘残棋，黑白子犬牙交错。落子处似有微光流转——这是一局以灵力为子的棋。",
            {
                { L"执子对弈", L"你苦思冥想，落下三子后棋盘灵光一闪：\"负我半目，饶你性命。\"石桌裂开，滚出一颗丹药。", -10, 15, 12, 0, FxRandomPill },
                { L"胡乱拍散棋子", L"棋局崩碎的刹那，一股灵力反噬而来。看来掀棋盘在哪都不受欢迎。", -20, -10, 0, 0, FxNone },
            } },
        { 8, L"醉妖", L"一个酒气熏天的人形妖怪抱着酒葫芦躺在你脚边，嘴里嘟囔：\"再来……再来一坛……\"",
            {
                { L"陪它拼酒", L"三坛下肚，妖怪搂着你的肩膀称兄道弟，临走塞给你一件它\"顺手拿的\"东西。", -8, -15, 10, 0, FxRandomItem },
                { L"趁醉摸走酒葫芦", L"葫芦入手沉甸甸的，晃一晃，里面居然是灵液！", -5, 0, 0, 20, FxRandomPill },
                { L"绕过去", L"你蹑手蹑脚绕开。妖怪翻了个身，继续打呼。", 0, 0, 0, 0, FxNothing },
            } },
        { 9, L"许愿井", L"一口古井，井水漆黑如墨。井沿挂着木牌：投灵石，得所愿。落款被水渍晕开，看不清是谁。",
            {
                { L"投入 20 灵石许愿", L"井水泛起涟漪，一物自井中缓缓浮起。", 0, 0, 0, -20, FxRandomItem },
                { L"投入全部灵石", L"井水翻涌，你心跳到了嗓子眼——浮上来的东西闪着金光！", 0, 0, 5, 0, FxRandomItem },
                { L"往井里看一眼", L"黑水中映出你自己的脸，苍老了三十岁。你猛地后退，坐了半天 cold sweat。", -10, 0, 8, 0, FxNone },
            } },
        { 10, L"封印之门", L"一扇青铜巨门横在面前，门上贴满褪色的封条。门缝里透出丝丝寒气，隐约有低吼声。",
            {
                { L"撕开封条闯进去", L"封条崩断的瞬间，一股罡风扑面而来！你在千钧一发之际捞到了门后的东西，代价是一身伤。", -30, 0, 30, 0, FxRandomItem },
                { L"以血献祭，祈祷开启", L"血珠渗入铜纹，巨门无声开启一线。里面传出一个苍老的声音：\"有胆识，进来拿吧。\"", -15, 0, 20, 0, FxRandomPill },
                { L"识时务者为俊杰", L"你后退三步，郑重行礼后离开。门后的低吼声渐渐平息。", 0, 5, 3, 0, FxNone },
            }, 5 },
        { 11, L"灵泉", L"岩缝间涌出一汪清泉，水汽氤氲，隐有清香。泉水里沉着几粒圆润的石子，似有灵性。",
            {
                { L"痛饮一番", L"甘冽的泉水顺着喉咙流下去，连日的伤势竟痊愈了大半。", 60, 20, 0, 0, FxNone },
                { L"运功吸收泉眼灵气", L"你盘坐泉边吐纳一个时辰，只觉神清气爽，修为精进。", 10, 30, 22, 0, FxNone },
            } },
        { 12, L"白骨打坐处", L"一具白骨保持着五心朝天的打坐姿势，骨架周围灵气凝而不散。它坐化的位置，恰好是一处灵穴。",
            {
                { L"在同一位置打坐", L"你与白骨相对而坐。灵穴灵气源源不断涌入，你如身在云海。", -5, 25, 28, 0, FxNone },
                { L"挪开白骨，独占灵穴", L"白骨散架的刹那，灵气骤然紊乱。你强吸一口，噎了个正着。", -12, 10, 12, 0, FxMaxHpDown },
            } },
        { 13, L"塔灵低语", L"四周忽然安静下来。一个既非男亦非女的声音在你耳边响起：\"你，第 1087 个。前面 1086 个，都没能出去。\"",
            {
                { L"静静聆听", L"\"……但他们也没你这么穷。\"低语渐远，你在原地捡到一小袋灵石——像是施舍。", 0, 0, 5, 30, FxNone },
                { L"大声喝问塔的秘密", L"低语停顿片刻：\"问得好。奖励你——多知道一点。\"一段晦涩的功法口诀涌入脑海。", -10, -15, 35, 0, FxNone },
                { L"捂住耳朵装没听见", L"低语化作一声轻笑。有时候，装傻能多活很久。", 0, 0, 0, 0, FxNothing },
            }, 8 },
        { 14, L"妖兽幼崽", L"一只小妖兽被荆棘缠住，看见你，它龇着奶牙发出威吓的呜咽。它母亲八成凶多吉少。",
            {
                { L"解开荆棘放它走", L"小家伙愣了愣，蹭了蹭你的裤脚，消失在黑暗里。你心头一暖。", 15, 10, 12, 0, FxNone },
                { L"……烤了吧", L"外焦里嫩，唇齿留香。你获得了一股燥热的精气，和一丝说不清的愧疚。", 30, 0, -10, 0, FxNone },
            } },
        { 15, L"传送阵残骸", L"地上刻着半座残缺的法阵，符文大多已经黯淡。看阵纹走势，这里曾经能通向塔的高层。",
            {
                { L"注入灵力强行催动", L"法阵爆出刺目光华，你眼前一花——居然真的挪了个地方，就在楼梯附近！", -15, 0, 0, 0, FxTeleportNear },
                { L"拆走阵眼灵石", L"阵眼灵石品相极佳，就是拆的时候被电了一下。", -8, 0, 0, 45, FxNone },
            }, 4 },
        { 16, L"诡异琴声", L"不知从哪一层飘来断断续续的琴声，如泣如诉。循声望去，长廊尽头似乎立着一个抚琴的白影。",
            {
                { L"循声而去", L"白影、琴声、长廊，在你走到第十步时同时消失。原地留着一支玉簪，入手温润。", -10, 0, 10, 0, FxRandomItem },
                { L"以灵力护心，充耳不闻", L"琴声在你耳边拔高又拔高，最终悻悻散去。你的道心经受住了一次考验。", -5, 10, 18, 0, FxNone },
            } },
        { 17, L"丹房残炉", L"一间丹房，丹炉尚有余温。炉边散落着几株药龄不错的药材，以及一本烧掉一半的丹方。",
            {
                { L"开炉取丹", L"炉膛里躺着三粒品相完好的丹药！前人欠的火候，你捡了便宜。", 20, 10, 0, 0, FxRandomPill },
                { L"照着残方现炼一炉", L"成色堪忧，药力时灵时不灵，但你确实学到了东西。", -8, -10, 15, 0, FxRandomPill },
                { L"拆丹炉的灵纹", L"灵纹入眼的瞬间，你对防具的理解深了一层。", -5, 0, 0, 0, FxUpgradeArmor },
            } },
        { 18, L"雷击木", L"一棵焦黑的古木拦腰而断，断口处电弧还在噼啪游走。这是被天雷劈过的灵木，可遇不可求。",
            {
                { L"掰一段带走", L"雷击木入手沉重。炼器坊会为它打破头。", -5, 0, 0, 0, FxHerbs },
                { L"盘坐树下，引雷入体", L"电流窜过四肢百骸，痛入骨髓——但你隐约摸到了雷劫的脾气。下次渡劫，或许能从容些。", -25, 0, 10, 0, FxTribBuff },
            }, 6 },
        { 19, L"迷路的商人", L"一个背着大包袱的凡人商贩缩在墙角，看见你差点哭出来：\"仙师！俺跟商队走散了，这塔里到处是妖怪……\"",
            {
                { L"护送他一程", L"你护着商人穿过半层塔。分别时他把包袱里最值钱的东西塞给了你。", -15, -10, 15, 50, FxNone },
                { L"打劫他", L"商贩抖如筛糠，交出了全部盘缠。他看你的眼神，比妖怪还让你难受。", 0, 0, -15, 80, FxNone },
                { L"指个方向让他自己走", L"\"谢、谢仙师！\"他连滚带爬地跑了。你远远听见一声惨叫。唉。", 0, 0, -3, 10, FxNone },
            } },
        { 20, L"月下棋圣", L"月光透过塔顶裂缝洒落，照着一位白衣人影。他执黑子悬于半空，头也不抬：\"来者何人？可敢对弈一局？\"",
            {
                { L"坐下对弈", L"三炷香后，你投子认负。白衣人轻笑：\"输得漂亮。\"袖袍一挥，一缕棋道真意渡入你识海。", -5, -20, 40, 0, FxNone },
                { L"求教 instead", L"你以晚辈之礼求教。白衣人讲棋如讲道，你如醍醐灌顶。", -5, 10, 25, 0, FxNone },
                { L"婉拒离去", L"\"无缘。\"白衣人化作月光消散。", 0, 5, 3, 0, FxNone },
            }, 10 },
        { 21, L"血祭坛", L"这座祭坛通体暗红，坛面的凹槽里积着经年的血垢。它不需要香火——它要血。",
            {
                { L"割掌献祭", L"血落进凹槽，祭坛嗡鸣着亮起。你的伤换来了修为的暴涨。贪婪，还是果决？", -35, 0, 45, 0, FxNone },
                { L"用妖血代替（若背包有妖丹）", L"你把一枚妖丹按进凹槽。祭坛将信将疑地收下了。", -5, 0, 20, 0, FxNone },
                { L"掉头就走", L"有些便宜，占一次就没命占第二次。", 0, 0, 0, 0, FxNothing },
            }, 3 },
        { 22, L"千年蛛网", L"一张巨网封住了半个洞窟，丝线在幽光下泛着冷芒。网深处，裹着几具干瘪的\"茧\"。",
            {
                { L"放火焚网", L"烈焰腾起，蛛丝蜷缩成焦黑的团块。火光里滚出一些蛛妖攒的家当。", -5, 0, 0, 30, FxNone },
                { L"割开蛛茧搜刮", L"茧里的修士早已干枯，遗物倒是保存完好。愿逝者安息。", 0, 0, 5, 0, FxRandomItem },
                { L"小心绕行", L"你贴着边缘挪了过去。黑暗深处，八只眼睛目送你离开。", 0, 0, 0, 0, FxNothing },
            } },
        { 23, L"装死的妖怪", L"一只遍体鳞伤的大妖瘫在路中央，胸口还在微弱起伏。它看见你，眼珠转了转，装得更像了。",
            {
                { L"补一刀", L"大妖至死都没睁开眼。斩杀将死之妖，也算修罗道的一课。", -5, 0, 30, 15, FxNone },
                { L"喂它一粒丹药", L"丹药入腹，大妖缓缓睁眼，深深看了你一眼，化作一道流光消失。你掌心多了一片鳞甲。", -20, 0, 10, 0, FxRandomItem },
                { L"当没看见", L"你走过去十步，身后传来窸窸窣窣爬走的声音。", 0, 0, 0, 0, FxNothing },
            } },
        { 24, L"镜花水月", L"前方景象忽然扭曲——你看见了记忆里的画面：故乡的院子，檐下的雨，还有某个再也见不到的人。",
            {
                { L"沉入幻境", L"你泪流满面地醒过来，幻境虽假，情绪却淬炼了道心。大悲大喜，皆是修行。", -10, 30, 35, 0, FxNone },
                { L"咬破舌尖，强破幻境", L"血腥味炸开清醒的一瞬，幻境如镜面般碎裂。碎片深处似有别的东西掉了出来。", -15, -10, 5, 0, FxRandomItem },
            } },
        { 25, L"断剑重铸", L"一座废弃的炼器炉前插着一把断成两截的飞剑。炉边的石碑写着：献剑者，可得一炼。",
            {
                { L"献上当前法宝重铸", L"飞剑与你手中的法宝在炉中合而为一，重铸后的它锋锐更胜从前。", -10, 0, 0, 0, FxUpgradeWeapon },
                { L"只借炉火，自炼护甲", L"你把法袍投入炉中回炉。火候虽糙，成色反而更纯了。", -10, 0, 0, 0, FxUpgradeArmor },
                { L"没装备可献，黯然离开", L"石碑上的字黯淡下去，像是一声叹息。", 0, 0, 0, 0, FxNothing },
            }, 5 },
        { 26, L"妖丹拍卖会", L"几个妖修围着一块空地叫价，中间的玉盘里躺着一颗流光溢彩的上品妖丹。见你过来，众妖齐齐回头。",
            {
                { L"出价竞拍（-60 灵石）", L"众妖见是人类，愣是没敢跟你抬价。上品妖丹，到手。", 0, 0, 5, -60, FxRandomItem },
                { L"砸场子", L"你一声炸喝，众妖四散。玉盘、妖丹，连同垫布都成了你的。就是手有点疼。", -25, -10, 10, 70, FxNone },
                { L"看热闹", L"拍卖继续。你听了一炷香的妖语价还价，学了几句妖族黑话。", 0, 0, 5, 0, FxNone },
            }, 8 },
        { 27, L"上古傀儡", L"一尊丈高的石傀儡半跪在尘土里，胸口嵌着一枚黯淡的灵核。它胸口刻着符文：敕令·守塔·第七卫。",
            {
                { L"唤醒它", L"灵核亮起，傀儡缓缓抬头。它上下打量你三息，忽然单膝跪地：\"新……主人。\"随后解下自己的护甲双手奉上。", -15, 0, 20, 0, FxUpgradeArmor },
                { L"拆解灵核和符纹", L"好东西！上古炼材，灵石和材料双收。傀儡至死保持着单膝跪地的姿势。", -8, 0, 0, 55, FxHerbs },
            }, 9 },
        { 28, L"蒲团", L"一个蒲团孤零零地摆在空房间正中，干净得不像话。坐上去的冲动油然而生。",
            {
                { L"盘坐吐纳", L"一坐就是三个时辰。醒来时神完气足，连旧伤都淡了几分。", 40, 30, 8, 0, FxNone },
                { L"掀开看看下面", L"蒲团下压着一张泛黄的符纸——前人的私藏。", 0, 0, 0, 0, FxRandomItem },
            } },
        { 29, L"禁制符箓", L"墙上贴着一张品相完好的金色符箓，符纹繁复如星图。这是塔的原主人留下的禁制，没人知道撕下来会发生什么。",
            {
                { L"小心翼翼揭下来", L"符箓离墙的瞬间，墙上浮现出一行小字：\"善取者，用之；妄取者，噬之。\"好在你是前者。", -8, 0, 0, 0, FxRandomItem },
                { L"以灵力临摹一份", L"临摹废了你半成灵力，但一笔一画之间，对符道的理解入了门。", -5, -20, 15, 0, FxNone },
                { L"不碰", L"金色符箓静静贴在墙上。有些门，不开比开好。", 0, 0, 0, 0, FxNothing },
            }, 4 },
        { 30, L"轮回井", L"又一口井。但这口井的水面上，倒映的不是你的脸，而是无数个转瞬即逝的陌生面孔。",
            {
                { L"凝神窥探", L"一张张面孔闪过，你恍惚看见了自己的前世——一只……猴子？修为大涨，道心微裂。", -20, 20, 45, 0, FxNone },
                { L"朝井里扔块石头", L"咚。井底传来一声闷响，然后是一声愤怒的：\"谁啊！\"", -5, 0, 5, 15, FxNone },
                { L"敬而远之", L"轮回之事，窥之不祥。你低头快步走过。", 5, 5, 0, 0, FxNothing },
            }, 12 },
        { 31, L"灵田", L"一小畦灵田藏在石室里，田里灵光闪烁的灵草长势喜人。田头立着草人，歪着头\"看\"着你。",
            {
                { L"恭敬采摘", L"你只取三成，留七分生机。草人朝你弯了弯腰。", 10, 10, 5, 0, FxHerbs },
                { L"连根拔起一锅端", L"你拔得兴起，最后一株离土时，整片灵田的灵气轰然溃散。草人头也掉了。", -15, 0, 0, 20, FxHerbs },
            }, 2 },
        { 32, L"走火入魔的修士", L"一名修士盘坐在地，周身黑气翻腾，时而低吼时而呓语——走火入魔的征兆。他怀里的储物袋就在你手边。",
            {
                { L"渡灵力救他", L"你以自身灵力护住他心脉，黑气渐散。他醒后一言不发，把储物袋塞给你，深深一揖。", -25, 0, 20, 0, FxRandomItem },
                { L"趁魔取宝", L"储物袋到手的同时，他体内一缕魔气窜进了你的经脉。钱没白拿，罪也没少受。", -10, -15, -5, 60, FxNone },
                { L"守到他醒 or 走", L"你远远守了半个时辰，见他黑气渐平，才悄悄离开。举手之劳。", 5, 5, 8, 0, FxNone },
            } },
        { 33, L"塔层塌陷", L"脚下忽然传来细碎的裂响——这一层要塌了！尘土簌簌落下，不远处还压着半截身影在呼救。",
            {
                { L"先救人再跑", L"你拽出一个吓傻的散修，两人连滚带爬冲出塌陷区。他塞给你一袋灵石，语无伦次地道谢。", -18, 0, 20, 45, FxNone },
                { L"先抢浮财", L"塌方掀开的地缝里露出前人遗藏，你抓了一把就跑。身后的呼救声被轰鸣淹没。", -8, 0, -8, 70, FxNone },
                { L"自己狂奔", L"你一路狂奔出塌陷区，回头望去，那截呼救的手臂已经不见了。", 0, 0, -5, 0, FxNone },
            }, 7 },
        { 34, L"妖狐引路", L"一只三尾白狐蹲在路口，尾巴摇得优雅。见你注意它，便朝某个方向迈出两步，又回头看你。",
            {
                { L"跟着它走", L"白狐七拐八绕，竟把你带到了楼梯口。它蹭蹭你的脚踝，一溜烟跑了。", -5, 5, 8, 0, FxTeleportNear },
                { L"警惕地拒绝", L"白狐眼中闪过一丝人性化的失望，摇着尾巴消失在阴影里。", 0, 0, 0, 0, FxNothing },
                { L"试图抓住它", L"你扑了个空，脸着地。白狐蹲在房梁上，笑得前仰后合。", -10, 0, 5, 0, FxNone },
            } },
    };
    return dex;
}

const EventDef& eventDef(int id) {
    for (const auto& e : eventDex())
        if (e.id == id) return e;
    return eventDex().front();
}

const EventDef& randomEvent(int floor) {
    std::vector<int> pool;
    for (const auto& e : eventDex())
        if (floor >= e.minFloor) pool.push_back(e.id);
    return eventDef(Rng::get().pick(pool));
}

} // namespace yaota

// ============================== player.cpp ==============================

// player.cpp —— 玩家的成长逻辑

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

// ============================== render.cpp ==============================

// render.cpp —— 画面绘制实现
// 渲染约定：
//   * 所有格子都用"全角"字符（汉字/全角符号），保证每格占 2 列，网格不会错位
//   * 未探索区域画全角空格"　"；看过但不在视野内的格子以暗色显示（迷雾记忆）
//   * 视野：以玩家为中心半径 9 格，山岩挡视线（Bresenham 直线检测）


namespace yaota {

// ---- 基础输出 ----
void Renderer::clearScreen() {
    std::cout << "\033[2J\033[H";
}

void Renderer::print(const std::wstring& s, Color c, bool bold) {
    std::cout << colorCode(c);
    if (bold) std::cout << "\033[1m";
    std::cout << wstrToUtf8(s) << "\033[0m";
}

void Renderer::println(const std::wstring& s, Color c, bool bold) {
    print(s, c, bold);
    std::cout << "\n";
}

std::wstring Renderer::promptLine(const std::wstring& prompt) {
    print(prompt, Color::Cyan);
    std::cout << " ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        // stdin 结束（管道输入耗尽 / 终端关闭）：视为玩家离场，干净退出。
        // 否则主循环会拿到空输入无限重绘（脚本测试时曾刷出 6.9GB 输出）。
        std::cout << "\n";
        println(L"（输入流已尽，就此别过。）", Color::Dark);
        std::exit(0);
    }
    return utf8ToWstr(line);
}

void Renderer::pause() {
    println(L"");
    print(L"—— 按回车继续 ——", Color::Dark);
    std::string dummy;
    std::getline(std::cin, dummy);
}

int Renderer::menu(const std::wstring& title, const std::vector<std::wstring>& options,
                   bool allowCancel) {
    println(L"");
    println(L"┌─ " + title + L" ─────────────", Color::Yellow);
    for (size_t i = 0; i < options.size(); ++i)
        println(L"│  " + std::to_wstring(i + 1) + L". " + options[i]);
    if (allowCancel) println(L"│  . 取消");
    println(L"└─────────────────", Color::Yellow);

    while (true) {
        std::wstring in = promptLine(L"请选择:");
        if (in.empty()) continue;
        if (in[0] == L'.' && allowCancel) return -1;
        int n = 0;
        for (wchar_t c : in) {
            if (!std::iswdigit(c)) { n = -1; break; }
            n = n * 10 + (c - L'0');
        }
        if (n >= 1 && n <= (int)options.size()) return n - 1;
        println(L"输入无效。", Color::Red);
    }
}

bool Renderer::askYesNo(const std::wstring& question) {
    while (true) {
        std::wstring in = promptLine(question + L" [y/n]:");
        if (in.empty()) continue;
        wchar_t c = std::towlower(in[0]);
        if (c == L'y' || c == L'n') return c == L'y';
    }
}

// ---- 视野 ----
bool Renderer::lineOfSight(const Dungeon& d, int x0, int y0, int x1, int y1) {
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, x = x0, y = y0;
    while (!(x == x1 && y == y1)) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
        if (x == x1 && y == y1) break;   // 终点自身不挡自己
        if (d.blocksSight(x, y)) return false;
    }
    return true;
}

// ---- 地图 ----
void Renderer::drawMap(const Dungeon& d, const Player& p,
                       const std::vector<Monster>& monsters,
                       const std::vector<GroundItem>& ground,
                       bool revealAll, std::vector<char>& explored) {
    constexpr int RADIUS = 9;
    if ((int)explored.size() != Dungeon::W * Dungeon::H)
        explored.assign(Dungeon::W * Dungeon::H, 0);

    // 先更新迷雾记忆
    if (revealAll) {
        for (auto& e : explored) e = 1;
    } else {
        for (int y = 0; y < Dungeon::H; ++y) {
            for (int x = 0; x < Dungeon::W; ++x) {
                double dist = std::hypot(x - p.x, y - p.y);
                if (dist <= RADIUS && lineOfSight(d, p.x, p.y, x, y))
                    explored[y * Dungeon::W + x] = 1;
            }
        }
    }

    println(L"", Color::Default);
    for (int y = 0; y < Dungeon::H; ++y) {
        print(L"│", Color::Dark);
        for (int x = 0; x < Dungeon::W; ++x) {
            bool seen = explored[y * Dungeon::W + x];
            if (!seen) { std::cout << "　"; continue; }

            double dist = std::hypot(x - p.x, y - p.y);
            bool visible = revealAll ||
                (dist <= RADIUS && lineOfSight(d, p.x, p.y, x, y));

            // 视野内：优先画活物，其次玩家/掉落，最后地形
            if (visible) {
                const Monster* m = nullptr;
                for (const auto& mm : monsters)
                    if (mm.alive && mm.x == x && mm.y == y) { m = &mm; break; }
                if (m) {
                    print(std::wstring(1, monsterDef(m->defId).glyph),
                          elementColor(monsterDef(m->defId).element));
                    continue;
                }
                if (x == p.x && y == p.y) {
                    print(L"仙", Color::Cyan, true);
                    continue;
                }
                const Item* gi = nullptr;
                for (const auto& g : ground)
                    if (g.x == x && g.y == y) { gi = &g.item; break; }
                if (gi) {
                    print(std::wstring(1, itemDef(gi->defId).glyph),
                          itemColor(itemDef(gi->defId).type));
                    continue;
                }
            }

            // 地形（迷雾记忆里的格子统一暗色）
            Tile t = d.at(x, y);
            Color base = Color::Dark;
            switch (t) {
                case Tile::Wall:
                    print(L"山", base);
                    break;
                case Tile::Stairs:
                    print(L"梯", visible ? Color::Yellow : base, visible);
                    break;
                case Tile::Altar:
                    print(L"坛", visible ? Color::Magenta : base);
                    break;
                case Tile::Herb:
                    print(L"草", visible ? Color::Green : base);
                    break;
                case Tile::Chest:
                    print(L"箱", visible ? Color::Yellow : base);
                    break;
                case Tile::Floor:
                default:
                    print(L"．", base);
                    break;
            }
        }
        std::cout << "\n";
    }
}

// ---- 配色 ----
Color Renderer::elementColor(Element e) {
    switch (e) {
        case Element::Jin:  return Color::White;
        case Element::Mu:   return Color::Green;
        case Element::Shui: return Color::Blue;
        case Element::Huo:  return Color::Red;
        case Element::Tu:   return Color::Yellow;
    }
    return Color::Default;
}

Color Renderer::itemColor(ItemType t) {
    switch (t) {
        case ItemType::Weapon:   return Color::Cyan;
        case ItemType::Armor:    return Color::Blue;
        case ItemType::Pill:     return Color::Green;
        case ItemType::Scroll:   return Color::Magenta;
        case ItemType::Treasure: return Color::Yellow;
        case ItemType::Material: return Color::Dark;
    }
    return Color::Default;
}

void Renderer::drawBar(int cur, int max, int width, Color c) {
    if (max < 1) max = 1;
    int filled = (int)((double)cur / max * width + 0.5);
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    print(std::wstring(filled, L'█'), c);
    print(std::wstring(width - filled, L'░'), Color::Dark);
}

void Renderer::drawHud(const Player& p, int kills) {
    println(L"╔══════════════════════════════════════════════╗", Color::Yellow);
    std::wstring title = L"║ 妖塔 · 第 " + std::to_wstring(p.floor) + L" 层";
    title += L"（斩妖 " + std::to_wstring(kills) + L"）";
    print(title, Color::Yellow, true);
    println(L"", Color::Default);

    print(L" " + p.name, Color::White, true);
    print(L" 【" + std::wstring(elementWName(p.spirit)) + L"灵根·");
    print(p.realm().name, Color::Magenta);
    println(L"】", Color::Default);

    bool ready = p.readyToBreak();
    print(L" 气血 ");
    drawBar(p.hp, p.maxHp, 10, p.hp * 3 < p.maxHp ? Color::Red : Color::Green);
    print(L" " + std::to_wstring(p.hp) + L"/" + std::to_wstring(p.maxHp) + L"    灵力 ");
    drawBar(p.mp, p.maxMp, 8, Color::Blue);
    print(L" " + std::to_wstring(p.mp) + L"/" + std::to_wstring(p.maxMp));
    println(L"", Color::Default);

    print(L" 攻击 " + std::to_wstring(p.atk()) + L"  防御 " + std::to_wstring(p.def()));
    print(L"  灵石 " + std::to_wstring(p.gold), Color::Yellow);
    print(L"  修为 " + std::to_wstring(p.exp) + L"/" + std::to_wstring(p.realm().expNeed));
    println(L"", Color::Default);

    print(L" 法宝: ");
    if (p.weaponId >= 0) {
        const ItemDef& w = itemDef(p.weaponId);
        print(w.name, Color::Cyan);
        int pw = w.power + p.weaponBonus;
        print(L"(攻+" + std::to_wstring(pw) + L")");
    } else print(L"赤手空拳", Color::Dark);
    print(L"   法袍: ");
    if (p.armorId >= 0) {
        const ItemDef& a = itemDef(p.armorId);
        print(a.name, Color::Blue);
        int pa = a.power + p.armorBonus;
        print(L"(防+" + std::to_wstring(pa) + L")");
    } else print(L"布衣", Color::Dark);
    println(L"", Color::Default);

    if (ready) println(L" ★ 修为已足，按 [b] 冲击境界（渡劫）！", Color::Yellow, true);
    if (p.tribulationBonus > 0)
        println(L" ☆ 破障之力护体：下次渡劫成功率 +" +
                std::to_wstring(p.tribulationBonus) + L"%", Color::Magenta);
}

void Renderer::drawLog(const std::vector<std::wstring>& logs) {
    println(L"├─ 行事录 ─────────────────────────────────────", Color::Dark);
    size_t start = logs.size() > 6 ? logs.size() - 6 : 0;
    for (size_t i = start; i < logs.size(); ++i)
        println(L"│ " + logs[i], Color::Default);
    for (size_t i = logs.size(); i < start + 6; ++i)
        println(L"│", Color::Dark);
    println(L"╚══════════════════════════════════════════════╝", Color::Dark);
}

void Renderer::drawHelp() {
    clearScreen();
    println(L"══════════════ 《妖塔》操作说明 ══════════════", Color::Yellow);
    println(L"");
    println(L" 移动/攻击：w 上、s 下、a 左、d 右（走向妖怪即攻击）");
    println(L" g  拾取脚下的物品");
    println(L" i  打开背包（使用/装备/丢弃）");
    println(L" t  踏上楼梯去往上一层");
    println(L" b  冲击境界（修为足够时渡劫突破）");
    println(L" c  打坐调息（回灵力，但会经过时间）");
    println(L" x  灵力爆发（耗 15 灵力，重击身边所有妖怪）");
    println(L" v  炼化奇物与炼材，换取灵石");
    println(L" m  翻阅妖怪图鉴");
    println(L" S  保存进度");
    println(L" .  原地等待一回合");
    println(L" q  放弃此世（退出）");
    println(L"");
    println(L" 五行相克：金克木、木克土、土克水、水克火、火克金", Color::Magenta);
    println(L" 攻击克制属性伤害 x1.5，被克 x0.7，选对灵根事半功倍。", Color::Magenta);
    println(L" 第 30 层塔顶即可渡劫飞升（通关）。");
    pause();
}

void Renderer::drawMonstersDex() {
    clearScreen();
    println(L"══════════════ 妖怪图鉴 ══════════════", Color::Yellow);
    for (const auto& m : monsterDex()) {
        print(L" " + std::wstring(1, m.glyph) + L" ", elementColor(m.element));
        print(m.name, Color::White, true);
        print(L" [" + std::wstring(elementWName(m.element)) + L"] ");
        println(L"血" + std::to_wstring(m.hp) + L" 攻" + std::to_wstring(m.atk) +
                L" 防" + std::to_wstring(m.def) +
                L" · 第" + std::to_wstring(m.minFloor) + L"~" +
                (m.maxFloor ? std::to_wstring(m.maxFloor) : L"30") + L"层", Color::Dark);
        println(L"    " + m.desc, Color::Dark);
    }
    pause();
}

} // namespace yaota

// ============================== save.cpp ==============================

// save.cpp —— 存档实现
// 设计取舍：只保存玩家状态与层数，进入游戏时重新生成本层地图。
// （完整的地图/妖怪快照序列化太啰嗦，对一局几分钟的 Roguelike 不值得；
//   读档相当于"在同一层重开一条命"，也算 Roguelike 的传统艺能。）


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

// ============================== game.cpp ==============================

// game.cpp —— 主循环与各系统的粘合逻辑


namespace yaota {

// 便捷：数字转宽字符文本
static std::wstring num(int v) { return std::to_wstring(v); }
static std::wstring sgn(int v) { return v >= 0 ? L"+" + num(v) : num(v); }

static int sign(int v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// ================= 开局 / 读档 =================

void Game::newGame(const std::wstring& name, Element spirit) {
    player_.init(name, spirit);
    kills_ = 0;
    turn_ = 0;
    logs_.clear();
    revealAll_ = false;
    setupFloor();
    log(L"你推开妖塔斑驳的塔门，一股混杂着妖气与陈年香火的气息扑面而来。");
    log(L"（wasd 移动，走向妖怪即是攻击。按 ? 随时查看操作说明）");
}

bool Game::loadGame() {
    if (!loadFromFile(player_, kills_)) return false;
    turn_ = 0;
    logs_.clear();
    revealAll_ = false;
    setupFloor();
    log(L"你在第 " + num(player_.floor) + L" 层悠悠转醒——这一层的妖怪，已经换了一批新面孔。");
    return true;
}

// 生成本层：一次生成，玩家落在首个房间中心；
// 离玩家太近的妖怪挪去最远房间（守着楼梯，正好当拦路妖）。
// （旧实现生成两次、拿第一次的房间中心当出生点——第二次重生成后
//   那里可能是山岩，玩家会卡在墙里出生。单元测试抓出来的。）
void Game::setupFloor() {
    int f = player_.floor;
    dungeon_.generate(f, 2, 2);

    int sx = dungeon_.rooms()[0].cx();
    int sy = dungeon_.rooms()[0].cy();
    player_.x = sx;
    player_.y = sy;

    for (auto& m : dungeon_.spawnedMonsters()) {
        if (std::hypot(m.x - sx, m.y - sy) < 6) {
            const Room& far = dungeon_.farthestRoomFrom(sx, sy);
            m.x = m.homeX = far.cx();
            m.y = m.homeY = far.cy();
        }
    }

    monsters_ = dungeon_.spawnedMonsters();
    dungeon_.spawnedMonsters().clear();
    ground_ = dungeon_.spawnedGroundItems();
    dungeon_.spawnedGroundItems().clear();

    explored_.assign(Dungeon::W * Dungeon::H, 0);
    revealAll_ = false;
}

// ================= 主循环 =================

void Game::run() {
    while (!over_) {
        Renderer::clearScreen();
        Renderer::drawHud(player_, kills_);
        Renderer::drawMap(dungeon_, player_, monsters_, ground_, revealAll_, explored_);
        Renderer::drawLog(logs_);

        std::wstring in = Renderer::promptLine(
            L"[wasd]动 [g]拾取 [i]背包 [t]上楼 [b]突破 [c]打坐 [x]爆发 [v]炼化 [m]图鉴 [S]存档 [?]帮助");
        if (in.empty()) continue;
        wchar_t raw = in[0];
        wchar_t c = std::towlower(raw);

        bool takesTurn = true;
        switch (c) {
            case L'w': handleMove(0, -1); break;
            case L's':
                if (raw == L'S') {
                    // 大写 S 是存档，小写 s 才是向下移动
                    takesTurn = false;
                    log(saveToFile(player_, kills_) ? L"【存档】此世种种，已录于卷轴。"
                                                    : L"【存档失败】写入出错了。");
                } else {
                    handleMove(0, 1);
                }
                break;
            case L'a': handleMove(-1, 0); break;
            case L'd': handleMove(1,  0); break;
            case L'g': tryPickup(); break;
            case L'i': openInventory(); break;
            case L't': tryStairs(); break;
            case L'b': tryTribulate(); break;
            case L'c': meditate(); break;
            case L'x': spiritBurst(); break;
            case L'v': refineJunk(); break;
            case L'm': viewMonsterDex(); takesTurn = false; break;
            case L'?': Renderer::drawHelp(); takesTurn = false; break;
            case L'.': case L'5': log(L"你屏息凝神，原地守了一拍。"); break;
            case L'q':
                takesTurn = false;
                if (Renderer::askYesNo(L"当真要放弃此世？")) {
                    log(L"你盘膝坐定，散去一身修为，化作流光消散……");
                    gameOver(false);
                }
                break;
            default:
                takesTurn = false;
                log(L"（没听懂这个口诀。按 ? 查看操作说明）");
                break;
        }
        if (takesTurn && !over_) endTurn();
    }
}

// ================= 玩家动作 =================

void Game::handleMove(int dx, int dy) {
    int nx = player_.x + dx, ny = player_.y + dy;
    if (!dungeon_.inBounds(nx, ny)) return;

    Monster* m = monsterAt(nx, ny);
    if (m) { attackMonster(*m); return; }

    if (!dungeon_.walkable(nx, ny)) {
        log(L"山岩挡路。");
        return;
    }

    player_.x = nx;
    player_.y = ny;

    Tile t = dungeon_.at(nx, ny);
    switch (t) {
        case Tile::Herb: {
            if (player_.addItem(90, 1)) {
                log(L"你采下一株灵草收入囊中。");
                dungeon_.at(nx, ny) = Tile::Floor;
            } else {
                log(L"灵草就在脚下，但背包已满。");
            }
            break;
        }
        case Tile::Chest: {
            openChest();
            dungeon_.at(nx, ny) = Tile::Floor;
            break;
        }
        case Tile::Altar: {
            dungeon_.at(nx, ny) = Tile::Floor;
            triggerEvent();
            break;
        }
        case Tile::Stairs: {
            log(L"一道石梯盘旋而上（按 [t] 踏上）。");
            break;
        }
        default:
            break;
    }
}

void Game::tryPickup() {
    GroundItem* g = groundItemAt(player_.x, player_.y);
    if (!g) { log(L"脚下空无一物。"); return; }
    const ItemDef& def = itemDef(g->item.defId);
    if (player_.addItem(g->item.defId, g->item.count, g->item.bonus)) {
        log(L"拾起 " + def.name +
            (g->item.count > 1 ? L" x" + num(g->item.count) : L"") +
            (g->item.bonus > 0 ? L"（洗炼+" + num(g->item.bonus) + L"）" : L""));
        g->item.defId = -1; // 标记删除
        ground_.erase(std::remove_if(ground_.begin(), ground_.end(),
                                     [](const GroundItem& gi) { return gi.item.defId < 0; }),
                      ground_.end());
    } else {
        log(L"背包已满，拿不动了。");
    }
}

void Game::tryStairs() {
    if (dungeon_.at(player_.x, player_.y) != Tile::Stairs) {
        log(L"附近没有楼梯。找找地图上的「梯」字。");
        return;
    }
    if (player_.floor >= 30) { gameOver(true); return; }

    player_.floor++;
    // 上楼喘口气：回复一成状态
    player_.hp = std::min(player_.maxHp, player_.hp + player_.maxHp / 10);
    player_.mp = std::min(player_.maxMp, player_.mp + player_.maxMp / 10);
    setupFloor();
    log(L"你拾级而上——妖塔第 " + num(player_.floor) + L" 层，妖气更盛了。");
    if (player_.floor == 30)
        log(L"塔顶近在咫尺，雷云在头顶盘旋不散。登上最后的楼梯，便是渡劫飞升之时！");
}

void Game::tryTribulate() {
    if (!player_.readyToBreak()) {
        if (player_.realmIdx + 1 >= (int)realms().size())
            log(L"你已站在此界修行的尽头——只差最后一步登天。");
        else
            log(L"修为未满（" + num(player_.exp) + L"/" +
                num(player_.realm().expNeed) + L"），还压不住更高的境界。");
        return;
    }
    const RealmDef& next = realms()[player_.realmIdx + 1];
    Renderer::clearScreen();
    Renderer::println(L"");
    Renderer::println(L"    乌云汇聚，雷光在云层深处酝酿……", Color::Yellow);
    Renderer::println(L"    冲击【" + next.name + L"】之境，成功率 " +
                      num((int)((next.tribulation + player_.tribulationBonus / 100.0) * 100)) + L"%",
                      Color::Magenta);
    Renderer::println(L"");
    if (!Renderer::askYesNo(L"迎劫而上？")) { log(L"你按下躁动的灵气，静待时机。"); return; }

    std::wstring msg;
    bool ok = player_.tribulate(msg);
    Renderer::clearScreen();
    if (ok) {
        Renderer::println(L"");
        Renderer::println(L"    轰——！！", Color::Yellow, true);
        Renderer::println(L"    " + msg, Color::Green, true);
        Renderer::println(L"");
    } else {
        Renderer::println(L"");
        Renderer::println(L"    轰——！！", Color::Red, true);
        Renderer::println(L"    " + msg, Color::Red);
        Renderer::println(L"");
    }
    log(msg);
    Renderer::pause();
}

void Game::meditate() {
    int mpGain = 8 + 2 * player_.realmIdx;
    player_.mp = std::min(player_.maxMp, player_.mp + mpGain);
    player_.hp = std::min(player_.maxHp, player_.hp + 4);
    log(L"你盘膝吐纳，灵力回升 " + num(mpGain) + L" 点。");

    // 打坐有风险：两成几率被附近的妖物偷袭
    if (Rng::get().chance(0.20)) {
        for (auto& m : monsters_) {
            if (!m.alive) continue;
            int d = std::max(std::abs(m.x - player_.x), std::abs(m.y - player_.y));
            if (d <= 7) {
                log(L"【偷袭】打坐入定之际，" + monsterDef(m.defId).name + L"悄悄逼近！");
                monsterAttack(m);
                break;
            }
        }
    }
}

void Game::spiritBurst() {
    if (player_.mp < 15) {
        log(L"灵力不足 15 点，压榨丹田也挤不出来了。");
        return;
    }
    player_.mp -= 15;
    bool any = false;
    for (auto& m : monsters_) {
        if (!m.alive) continue;
        if (std::abs(m.x - player_.x) + std::abs(m.y - player_.y) == 1) {
            any = true;
            HitResult r = resolveHit(player_.spirit, (int)(player_.atk() * 1.5),
                                     monsterDef(m.defId).element, monsterDef(m.defId).def, 0.0);
            m.hp -= r.damage;
            log(L"灵力如潮涌出！" + monsterDef(m.defId).name + L"被震退，受创 " +
                num(r.damage) + L" 点！");
            if (m.hp <= 0) {
                m.alive = false;
                kills_++;
                const MonsterDef& def = monsterDef(m.defId);
                player_.exp += def.exp;
                player_.gold += def.gold;
                log(L"【击杀】" + def.name + L"化为黑雾散去（修为+" +
                    num(def.exp) + L"，灵石+" + num(def.gold) + L"）");
            }
        }
    }
    if (!any) log(L"灵力在经脉中炸开一圈涟漪……身边空无一人。");
}

void Game::refineJunk() {
    int gained = 0, cnt = 0;
    for (auto it = player_.inventory.begin(); it != player_.inventory.end();) {
        const ItemDef& def = itemDef(it->defId);
        if (def.type == ItemType::Treasure || def.type == ItemType::Material) {
            gained += def.price * it->count;
            cnt += it->count;
            it = player_.inventory.erase(it);
        } else {
            ++it;
        }
    }
    if (cnt == 0) { log(L"背包里没有可炼化的奇物或炼材。"); return; }
    player_.gold += gained;
    log(L"你默运玄功，将 " + num(cnt) + L" 件之物炼化成纯粹灵气，得灵石 " + num(gained) + L" 枚。");
}

void Game::openInventory() {
    while (true) {
        if (player_.inventory.empty()) {
            log(L"背包空空如也。");
            return;
        }
        std::vector<std::wstring> opts;
        for (const auto& it : player_.inventory) {
            const ItemDef& def = itemDef(it.defId);
            std::wstring line = std::wstring(1, def.glyph) + L" " + def.name +
                (it.count > 1 ? L" x" + num(it.count) : L"") +
                (it.bonus > 0 ? L" (洗炼+" + num(it.bonus) + L")" : L"") +
                L"  · " + std::wstring(itemTypeWName(def.type));
            opts.push_back(line);
        }
        int idx = Renderer::menu(L"背包（共 " + num((int)player_.inventory.size()) + L"/20 格）", opts);
        if (idx < 0) return;

        const ItemDef& def = itemDef(player_.inventory[idx].defId);
        std::vector<std::wstring> acts;
        if (def.type == ItemType::Pill || def.type == ItemType::Scroll) acts.push_back(L"使用");
        if (def.type == ItemType::Weapon || def.type == ItemType::Armor) acts.push_back(L"装备");
        if (def.type == ItemType::Treasure || def.type == ItemType::Material) acts.push_back(L"炼化此物");
        acts.push_back(L"丢弃");

        Renderer::clearScreen();
        Renderer::println(L"【" + def.name + L"】" + std::wstring(itemTypeWName(def.type)) +
                          L" · " + std::wstring(elementWName(def.element)) + L"属性", Color::Yellow);
        Renderer::println(L"" + def.desc, Color::Default);
        int act = Renderer::menu(L"拿它怎么办？", acts);
        if (act < 0) continue;

        std::wstring a = acts[act];
        if (a == L"使用") {
            if (useItem(idx)) return; // 消耗一回合
        } else if (a == L"装备") {
            equipItem(idx);
            return;
        } else if (a == L"炼化此物") {
            const auto& it = player_.inventory[idx];
            int g = def.price * it.count;
            player_.gold += g;
            player_.inventory.erase(player_.inventory.begin() + idx);
            log(L"炼化 " + def.name + L"，得灵石 " + num(g) + L" 枚。");
            return;
        } else if (a == L"丢弃") {
            dropItem(idx);
            return;
        }
    }
}

bool Game::useItem(size_t idx) {
    Item& it = player_.inventory[idx];
    const ItemDef& def = itemDef(it.defId);
    std::wstring msg;

    auto consume = [&]() {
        it.count--;
        if (it.count <= 0)
            player_.inventory.erase(player_.inventory.begin() + idx);
    };

    if (def.type == ItemType::Pill) {
        switch (def.id) {
            case 44: // 聚灵丹
                player_.exp += 40;
                msg = L"药力化作精纯修为（+40）。";
                break;
            case 45: // 培元丹
                player_.maxHp += 20; player_.hp += 20;
                msg = L"气血上限永久 +20。";
                break;
            case 46: // 洗髓丹
                player_.atkTotal += 2;
                msg = L"洗经伐髓，攻击永久 +2。";
                break;
            case 47: // 金刚丹
                player_.defTotal += 2;
                msg = L"筋骨如铁，防御永久 +2。";
                break;
            case 48: // 破障丹
                player_.tribulationBonus = std::min(30, player_.tribulationBonus + 15);
                msg = L"丹力护住识海，下次渡劫成功率 +15%。";
                break;
            case 49: // 解毒丹（本作暂时没有中毒机制，保底回一点血）
                player_.hp = std::min(player_.maxHp, player_.hp + 15);
                msg = L"苦归苦，多少补了点气血（+15）。";
                break;
            case 50: { // 龟息丹
                int heal = player_.maxHp / 2;
                player_.hp = std::min(player_.maxHp, player_.hp + heal);
                msg = L"气息绵长如龟，气血回复 " + num(heal) + L" 点。";
                break;
            }
            default: { // 40~43 各类回血丹
                int heal = def.power;
                player_.hp = std::min(player_.maxHp, player_.hp + heal);
                msg = L"药力化开，气血回复 " + num(heal) + L" 点。";
                break;
            }
        }
        consume();
        log(L"你服下" + def.name + L"。" + msg);
        if (player_.readyToBreak())
            log(L"丹力激荡——修为已足，可尝试突破【" + realms()[player_.realmIdx + 1].name + L"】！");
        return true;
    }

    if (def.type == ItemType::Scroll) {
        switch (def.id) {
            case 60: { // 火球符
                int dmg = 30 + player_.floor * 2;
                int hit = 0;
                for (auto& m : monsters_) {
                    if (!m.alive) continue;
                    if (std::max(std::abs(m.x - player_.x), std::abs(m.y - player_.y)) <= 2) {
                        m.hp -= dmg; hit++;
                        if (m.hp <= 0) {
                            m.alive = false; kills_++;
                            const MonsterDef& md = monsterDef(m.defId);
                            player_.exp += md.exp; player_.gold += md.gold;
                            log(L"火球吞没了" + md.name + L"！");
                        }
                    }
                }
                msg = hit ? L"轰！烈焰四溅，波及 " + num(hit) + L" 只妖怪（各 -" + num(dmg) + L"）。"
                          : L"火球在空荡的石室里炸开，烧了个寂寞。";
                break;
            }
            case 61: { // 冰封符
                int hit = 0;
                for (auto& m : monsters_) {
                    if (!m.alive) continue;
                    if (std::max(std::abs(m.x - player_.x), std::abs(m.y - player_.y)) <= 3) {
                        m.stunTurns = 3; hit++;
                    }
                }
                msg = hit ? L"寒气凝霜，" + num(hit) + L" 只妖怪被冻住了（3 回合）。"
                          : L"寒气散去，无妖可冻。";
                break;
            }
            case 62: { // 传送符
                const Room& r = Rng::get().pick(dungeon_.rooms());
                player_.x = r.cx();
                player_.y = r.cy();
                msg = L"眼前光景一晃，你已身处别处。";
                break;
            }
            case 63: // 天眼符
                revealAll_ = true;
                msg = L"天眼开！本层布局尽收眼底。";
                break;
            case 64: { // 摄妖符
                int hit = 0;
                for (auto& m : monsters_) {
                    if (!m.alive) continue;
                    if (std::max(std::abs(m.x - player_.x), std::abs(m.y - player_.y)) <= 6) {
                        m.stunTurns = 2; hit++;
                    }
                }
                msg = hit ? L"符光扫过，" + num(hit) + L" 只妖怪僵在原地（2 回合）。"
                          : L"符光扫过，四下无妖。";
                break;
            }
            case 65: { // 五雷符
                Monster* best = nullptr;
                int bestD = 1 << 30;
                for (auto& m : monsters_) {
                    if (!m.alive) continue;
                    int d = std::abs(m.x - player_.x) + std::abs(m.y - player_.y);
                    if (d < bestD) { bestD = d; best = &m; }
                }
                if (best) {
                    best->hp -= 120;
                    const MonsterDef& md = monsterDef(best->defId);
                    if (best->hp <= 0) {
                        best->alive = false; kills_++;
                        player_.exp += md.exp; player_.gold += md.gold;
                        msg = L"一道天雷劈下，" + md.name + L"当场灰飞烟灭！";
                    } else {
                        msg = L"一道天雷劈中" + md.name + L"，重创 120 点！";
                    }
                } else {
                    msg = L"雷光在塔顶炸响，却劈了个空。";
                }
                break;
            }
            default:
                msg = L"符纸忽然自燃，什么也没发生。";
                break;
        }
        consume();
        log(L"你祭出" + def.name + L"。" + msg);
        return true;
    }

    log(L"这东西不是用嘴吃的。");
    return false;
}

void Game::equipItem(size_t idx) {
    Item& it = player_.inventory[idx];
    const ItemDef& def = itemDef(it.defId);
    if (def.type == ItemType::Weapon) {
        int oldId = player_.weaponId, oldB = player_.weaponBonus;
        player_.weaponId = it.defId;
        player_.weaponBonus = it.bonus;
        player_.inventory.erase(player_.inventory.begin() + idx);
        if (oldId >= 0) player_.addItem(oldId, 1, oldB);
        log(L"你祭起【" + def.name + L"】，剑光（宝光）大盛。");
    } else if (def.type == ItemType::Armor) {
        int oldId = player_.armorId, oldB = player_.armorBonus;
        player_.armorId = it.defId;
        player_.armorBonus = it.bonus;
        player_.inventory.erase(player_.inventory.begin() + idx);
        if (oldId >= 0) player_.addItem(oldId, 1, oldB);
        log(L"你换上【" + def.name + L"】，顿觉踏实了几分。");
    }
}

void Game::dropItem(size_t idx) {
    const ItemDef& def = itemDef(player_.inventory[idx].defId);
    ground_.push_back(GroundItem(player_.x, player_.y, player_.inventory[idx]));
    player_.inventory.erase(player_.inventory.begin() + idx);
    log(L"你把" + def.name + L"放在了地上。");
}

void Game::viewMonsterDex() {
    Renderer::drawMonstersDex();
}

// ================= 世界运转 =================

void Game::endTurn() {
    turn_++;
    monsterTurns();
    regenTick();
}

void Game::regenTick() {
    if (player_.hp <= 0) { gameOver(false); return; }
    // 行走间缓慢回复
    if (turn_ % 3 == 0 && player_.hp < player_.maxHp) player_.hp++;
    if (turn_ % 2 == 0 && player_.mp < player_.maxMp) player_.mp++;
}

Monster* Game::monsterAt(int x, int y) {
    for (auto& m : monsters_)
        if (m.alive && m.x == x && m.y == y) return &m;
    return nullptr;
}

GroundItem* Game::groundItemAt(int x, int y) {
    for (auto& g : ground_)
        if (g.x == x && g.y == y) return &g;
    return nullptr;
}

void Game::attackMonster(Monster& m) {
    const MonsterDef& def = monsterDef(m.defId);
    HitResult r = playerHits(player_, m);
    if (r.dodged) {
        log(def.name + L"身形一晃，躲过了你的攻击！");
        return;
    }
    m.hp -= r.damage;
    std::wstring note;
    if (r.elemMult > 1.2) note = L"（五行克制！）";
    else if (r.elemMult < 0.8) note = L"（属性被克）";
    log(L"你一击打在" + def.name + L"身上，造成 " + num(r.damage) + L" 点伤害" +
        (r.crit ? L"，暴击！" : L"。") + note);

    if (m.hp <= 0) {
        m.alive = false;
        kills_++;
        player_.exp += def.exp;
        player_.gold += def.gold;
        log(L"【击杀】" + def.name + L"凄鸣一声化作黑雾（修为+" + num(def.exp) +
            L"，灵石+" + num(def.gold) + L"）", Color::Yellow);
        // 两成半几率掉落
        if (Rng::get().chance(0.25)) {
            std::vector<int> elig;
            for (const auto& d : itemDex())
                if (player_.floor >= d.minFloor) elig.push_back(d.id);
            ground_.push_back(GroundItem(m.x, m.y, Item(Rng::get().pick(elig))));
            log(L"它身后掉落了什么东西。");
        }
        if (player_.readyToBreak())
            log(L"★ 妖力入体，修为已足——按 [b] 冲击【" +
                realms()[player_.realmIdx + 1].name + L"】之境！", Color::Yellow);
    }
}

void Game::monsterAttack(Monster& m) {
    const MonsterDef& def = monsterDef(m.defId);
    HitResult r = monsterHits(player_, m);
    if (r.dodged) {
        log(def.name + L"扑向你，被你侧身让开！");
        return;
    }
    player_.hp -= r.damage;
    std::wstring note;
    if (r.elemMult > 1.2) note = L"（五行被克！）";
    log(def.name + L"朝你发难，你受了 " + num(r.damage) + L" 点伤" +
        (r.crit ? L"，伤得不轻！" : L"。") + note);
    if (player_.hp <= 0) gameOver(false);
}

void Game::monsterTurns() {
    for (auto& m : monsters_) {
        if (!m.alive) continue;
        if (m.stunTurns > 0) { --m.stunTurns; continue; }

        const MonsterDef& def = monsterDef(m.defId);
        int ddx = player_.x - m.x, ddy = player_.y - m.y;
        int cheb = std::max(std::abs(ddx), std::abs(ddy));
        int manh = std::abs(ddx) + std::abs(ddy);

        // 一步移动的辅助：目标格可走、无其他妖怪、不是玩家本格
        auto tryStep = [&](int sx, int sy) {
            int nx = m.x + sx, ny = m.y + sy;
            if (!dungeon_.walkable(nx, ny)) return false;
            if (nx == player_.x && ny == player_.y) return false;
            if (monsterAt(nx, ny)) return false;
            m.x = nx; m.y = ny;
            return true;
        };

        // 胆小的残血就逃
        if (monsterFlees(m)) {
            if (!tryStep(sign(-ddx), 0)) tryStep(0, sign(-ddy));
            continue;
        }

        bool aggro = cheb <= aggroRange(def);
        if (def.ai == AiType::Guard) {
            int homeDist = std::max(std::abs(m.x - m.homeX), std::abs(m.y - m.homeY));
            if (!aggro || homeDist > 5) {
                // 不追或离巢太远：回巢
                int hx = sign(m.homeX - m.x), hy = sign(m.homeY - m.y);
                if (hx || hy) { if (!tryStep(hx, 0)) tryStep(0, hy); }
                continue;
            }
        }

        if (manh == 1) { monsterAttack(m); if (over_) return; continue; }

        if (aggro) {
            // 优先走差值更大的轴
            if (std::abs(ddx) >= std::abs(ddy)) {
                if (!tryStep(sign(ddx), 0)) tryStep(0, sign(ddy));
            } else {
                if (!tryStep(0, sign(ddy))) tryStep(sign(ddx), 0);
            }
        } else if (Rng::get().chance(0.25)) {
            // 闲逛
            int dir = Rng::get().range(0, 3);
            tryStep(dir == 0 ? 1 : dir == 1 ? -1 : 0, dir == 2 ? 1 : dir == 3 ? -1 : 0);
        }
    }
}

void Game::openChest() {
    int gold = Rng::get().dice(3, 8) + player_.floor;
    player_.gold += gold;
    log(L"【宝箱】你撬开木箱，得灵石 " + num(gold) + L" 枚。", Color::Yellow);
    if (Rng::get().chance(0.8)) {
        std::vector<int> elig;
        for (const auto& d : itemDex())
            if (player_.floor >= d.minFloor) elig.push_back(d.id);
        int id = Rng::get().pick(elig);
        if (player_.addItem(id))
            log(L"箱中还有一件【" + itemDef(id).name + L"】。", Color::Yellow);
        else
            ground_.push_back(GroundItem(player_.x, player_.y, Item(id)));
    }
}

void Game::triggerEvent() {
    const EventDef& ev = randomEvent(player_.floor);
    Renderer::clearScreen();
    Renderer::println(L"");
    Renderer::println(L"════════ 【" + ev.title + L"】 ════════", Color::Magenta, true);
    Renderer::println(L"");
    // 描述逐行显示（自动按宽度简单换行）
    for (int i = 0; i * 34 < (int)ev.text.size(); ++i) {
        int end = std::min((i + 1) * 34, (int)ev.text.size());
        Renderer::println(L"  " + ev.text.substr(i * 34, end - i * 34));
    }
    Renderer::println(L"");

    std::vector<std::wstring> opts;
    for (const auto& c : ev.choices) opts.push_back(c.text);
    int pick = Renderer::menu(L"如何行事？", opts, false);
    applyEventChoice(ev, pick);
}

void Game::applyEventChoice(const EventDef& ev, int choiceIdx) {
    const EventChoice& c = ev.choices[choiceIdx];

    // 数字结算（事件伤不死人，最低留 1 点气血——妖塔虽险，总留一线）
    if (c.dHp != 0) {
        player_.hp = std::max(1, player_.hp + c.dHp);
        if (player_.hp > player_.maxHp) player_.hp = player_.maxHp;
    }
    if (c.dMp != 0) player_.mp = std::max(0, std::min(player_.maxMp, player_.mp + c.dMp));
    if (c.dExp != 0) player_.exp = std::max(0, player_.exp + c.dExp);
    if (c.dGold != 0) player_.gold = std::max(0, player_.gold + c.dGold);

    std::wstring fxNote;
    applyEventFx(c.fx, fxNote);

    Renderer::clearScreen();
    Renderer::println(L"");
    Renderer::println(L"  " + c.outcome, Color::Green);
    std::wstring stats;
    if (c.dHp)   stats += L" 气血" + sgn(c.dHp);
    if (c.dMp)   stats += L" 灵力" + sgn(c.dMp);
    if (c.dExp)  stats += L" 修为" + sgn(c.dExp);
    if (c.dGold) stats += L" 灵石" + sgn(c.dGold);
    if (!stats.empty()) Renderer::println(L" （" + stats + L" ）", Color::Cyan);
    if (!fxNote.empty()) Renderer::println(L" ☆ " + fxNote, Color::Magenta);
    Renderer::pause();
    log(L"【" + ev.title + L"】" + c.outcome);
    if (player_.readyToBreak())
        log(L"★ 修为已足——按 [b] 冲击【" + realms()[player_.realmIdx + 1].name + L"】之境！", Color::Yellow);
}

void Game::applyEventFx(int fx, std::wstring& out) {
    switch (fx) {
        case FxRandomItem: {
            std::vector<int> elig;
            for (const auto& d : itemDex())
                if (player_.floor >= d.minFloor) elig.push_back(d.id);
            if (elig.empty()) break;
            int id = Rng::get().pick(elig);
            if (player_.addItem(id))
                out = L"得到【" + itemDef(id).name + L"】。";
            else
                out = L"捡到一件东西，可惜背包塞不下，只得放弃。";
            break;
        }
        case FxRandomPill: {
            static const int pills[] = { 40, 41, 42, 43, 44, 50 };
            int id = pills[Rng::get().range(0, 5)];
            player_.addItem(id);
            out = L"得到【" + itemDef(id).name + L"】x1。";
            break;
        }
        case FxHealFull:
            player_.hp = player_.maxHp;
            player_.mp = player_.maxMp;
            out = L"气血灵力尽数回复。";
            break;
        case FxUpgradeWeapon:
            if (player_.weaponId >= 0) {
                player_.weaponBonus += 2;
                out = L"法宝【" + itemDef(player_.weaponId).name + L"】经洗炼，威力 +2。";
            } else out = L"你手中没有法宝，白白错过了机缘。";
            break;
        case FxUpgradeArmor:
            if (player_.armorId >= 0) {
                player_.armorBonus += 2;
                out = L"法袍【" + itemDef(player_.armorId).name + L"】经回炉，防御 +2。";
            } else out = L"你身上没有法袍，白白错过了机缘。";
            break;
        case FxTeleportNear: {
            // 螺旋找楼梯旁一个可走格
            int sx = dungeon_.stairsX(), sy = dungeon_.stairsY();
            bool done = false;
            for (int r = 1; r <= 4 && !done; ++r) {
                for (int dx = -r; dx <= r && !done; ++dx) {
                    for (int dy = -r; dy <= r && !done; ++dy) {
                        int nx = sx + dx, ny = sy + dy;
                        if (dungeon_.walkable(nx, ny) && !monsterAt(nx, ny)) {
                            player_.x = nx; player_.y = ny;
                            done = true;
                        }
                    }
                }
            }
            out = L"你被移到了楼梯近旁。";
            break;
        }
        case FxTribBuff:
            player_.tribulationBonus = std::min(30, player_.tribulationBonus + 10);
            out = L"下次渡劫，成功率 +10%。";
            break;
        case FxHerbs:
            if (player_.addItem(90, 3)) out = L"得到灵草 x3。";
            else out = L"三株灵草，可惜背包已满。";
            break;
        case FxMaxHpUp:
            player_.maxHp += 15;
            player_.hp += 15;
            out = L"气血上限 +15。";
            break;
        case FxMaxHpDown:
            player_.maxHp = std::max(30, player_.maxHp - 10);
            player_.hp = std::min(player_.hp, player_.maxHp);
            out = L"气血上限 -10。";
            break;
        case FxLearnAtk:
            player_.atkTotal += 1;
            out = L"攻击永久 +1。";
            break;
        case FxLearnDef:
            player_.defTotal += 1;
            out = L"防御永久 +1。";
            break;
        case FxNothing:
        case FxNone:
        default:
            break;
    }
}

void Game::gameOver(bool ascended) {
    over_ = true;
    Renderer::clearScreen();
    Renderer::println(L"");
    if (ascended) {
        Renderer::println(L"    ╔═══════════════════════════════╗", Color::Yellow);
        Renderer::println(L"    ║   雷 散 云 开 ， 仙 门 大 张   ║", Color::Yellow, true);
        Renderer::println(L"    ║        渡 劫 飞 升 ！         ║", Color::Yellow, true);
        Renderer::println(L"    ╚═══════════════════════════════╝", Color::Yellow);
        Renderer::println(L"");
        Renderer::println(L"    三十层妖塔被你踩在脚下。回望来路，", Color::Default);
        Renderer::println(L"    那些斩过的妖、结过的缘、起过的贪念，", Color::Default);
        Renderer::println(L"    都成了脚下的一片云。", Color::Default);
    } else {
        Renderer::println(L"    ╔═══════════════════════════════╗", Color::Red);
        Renderer::println(L"    ║       道 消 身 陨 ……         ║", Color::Red, true);
        Renderer::println(L"    ╚═══════════════════════════════╝", Color::Red);
        Renderer::println(L"");
        Renderer::println(L"    塔中又多了一具无人知晓的白骨。", Color::Dark);
        Renderer::println(L"    下一世，记得看清楚五行克制再出手。", Color::Dark);
    }
    Renderer::println(L"");
    Renderer::println(L"    此世终焉于第 " + num(player_.floor) + L" 层 · " +
                      player_.realm().name + L"之境", Color::Cyan);
    Renderer::println(L"    斩妖 " + num(kills_) + L" · 历 " + num(turn_) + L" 拍 · 遗灵石 " +
                      num(player_.gold) + L" 枚", Color::Cyan);
    Renderer::println(L"");
    Renderer::pause();
}

} // namespace yaota

// ============================== main.cpp ==============================

// main.cpp —— 程序入口：控制台初始化、标题画面、创建角色


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
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
