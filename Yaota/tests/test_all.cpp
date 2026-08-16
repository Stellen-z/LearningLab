// test_all.cpp —— 《妖塔》全量单元测试
// 覆盖目标：types(五行/境界) / rng / item / monster / dungeon / player /
//           combat / events / save / game(回合驱动)
// 配合 gcov -b 可测分支覆盖率（见 run_tests.sh）
#include "test_framework.h"

#include "../src/types.h"
#include "../src/rng.h"
#include "../src/item.h"
#include "../src/monster.h"
#include "../src/dungeon.h"
#include "../src/player.h"
#include "../src/combat.h"
#include "../src/events.h"
#include "../src/game.h"
#include "../src/save.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

using namespace yaota;

// ================= types：五行 =================

TEST(five_elements_matrix) {
    // 独立于实现重算期望值：克制 1.5 / 被克 0.7 / 相生 1.15 / 其他 1.0
    const Element all[5] = { Element::Jin, Element::Mu, Element::Shui, Element::Huo, Element::Tu };
    for (Element a : all) {
        for (Element d : all) {
            double m = elementMultiplier(a, d);
            bool isKe  = ((int)a == 0 && (int)d == 1) || ((int)a == 1 && (int)d == 4) ||
                         ((int)a == 4 && (int)d == 2) || ((int)a == 2 && (int)d == 3) ||
                         ((int)a == 3 && (int)d == 0);
            bool isSheng = ((int)a == 0 && (int)d == 2) || ((int)a == 2 && (int)d == 1) ||
                           ((int)a == 1 && (int)d == 3) || ((int)a == 3 && (int)d == 4) ||
                           ((int)a == 4 && (int)d == 0);
            if (a == d)        CHECK_EQ(m, 1.0);
            else if (isKe)     CHECK_EQ(m, 1.5);
            else if (isSheng)  CHECK_EQ(m, 1.15);
            else               CHECK_EQ(m, 0.7);
        }
    }
    // 名字宽度：单字符
    CHECK_EQ(std::wstring(elementWName(Element::Huo)), std::wstring(L"火"));
}

TEST(realms_table) {
    CHECK_EQ(realms().size(), (size_t)9);
    for (size_t i = 1; i < realms().size(); ++i) {
        CHECK_LE(realms()[i - 1].expNeed, realms()[i].expNeed);          // 修为门槛递增
        CHECK_GE(realms()[i].hpBonus, 1);                                 // 突破必涨血
        CHECK_LE(realms()[i].tribulation, realms()[i - 1].tribulation);   // 越高越难过劫
    }
}

// ================= rng =================

TEST(rng_bounds_and_pick) {
    Rng::get().seed(12345);
    for (int i = 0; i < 2000; ++i) {
        int v = Rng::get().range(-7, 13);
        CHECK_LE(-7, v);
        CHECK_LE(v, 13);
    }
    CHECK_EQ(Rng::get().chance(0.0), false);
    CHECK_EQ(Rng::get().chance(1.0), true);

    std::vector<int> bag = { 3, 8, 9 };
    for (int i = 0; i < 50; ++i) {
        int p = Rng::get().pick(bag);
        CHECK(p == 3 || p == 8 || p == 9);
    }
    int d = Rng::get().dice(10, 6);
    CHECK_LE(10, d);
    CHECK_LE(d, 60);
}

TEST(rng_deterministic_with_seed) {
    Rng::get().seed(4242);
    int a[5];
    for (int& v : a) v = Rng::get().range(0, 100000);
    Rng::get().seed(4242);
    for (int v : a) CHECK_EQ(Rng::get().range(0, 100000), v);
}

// ================= item =================

TEST(itemdex_integrity) {
    auto& dex = itemDex();
    CHECK_GE(dex.size(), (size_t)40);
    std::vector<int> ids;
    for (const auto& d : dex) {
        CHECK_NE(d.id, 0);
        CHECK(!d.name.empty());
        CHECK_EQ(d.name.find(L'\0'), std::wstring::npos);
        ids.push_back(d.id);
    }
    std::sort(ids.begin(), ids.end());
    CHECK_EQ(std::adjacent_find(ids.begin(), ids.end()), ids.end()); // id 不重复
    // 未知 id 回退到首条（不崩溃）
    CHECK_EQ(itemDef(99999).id, dex.front().id);
}

TEST(item_inventory_stacking_and_cap) {
    Player p;
    p.init(L"测试", Element::Jin);
    p.inventory.clear();

    CHECK(p.addItem(40, 2));            // 回气散 x2
    CHECK(p.addItem(40, 3));            // 叠加成 x5
    CHECK_EQ(p.inventory.size(), (size_t)1);
    CHECK_EQ(p.inventory[0].count, 5);

    CHECK(p.addItem(1, 1, 2));          // 带洗炼的武器独立成格
    CHECK_EQ(p.inventory.size(), (size_t)2);

    p.inventory.clear();
    for (int i = 0; i < 20; ++i) CHECK(p.addItem(1 + i, 1, i)); // 20 件不同武器填满
    CHECK_EQ(p.addItem(40, 1), false);  // 第 21 格失败
    CHECK_EQ(p.inventory.size(), (size_t)20);
}

// ================= monster =================

TEST(monsterdex_integrity) {
    auto& dex = monsterDex();
    CHECK_GE(dex.size(), (size_t)30);
    std::vector<int> ids;
    for (const auto& m : dex) {
        CHECK(!m.name.empty());
        CHECK_GE(m.hp, 1);
        CHECK_GE(m.exp, 1);
        CHECK_LE(m.minFloor, m.maxFloor == 0 ? 30 : m.maxFloor);
        ids.push_back(m.id);
    }
    std::sort(ids.begin(), ids.end());
    CHECK_EQ(std::adjacent_find(ids.begin(), ids.end()), ids.end());

    CHECK_EQ(aggroRange(monsterDef(7)), 9);   // 狼妖 Brave
    CHECK_EQ(aggroRange(monsterDef(0)), 7);   // 野狗妖 Melee
    CHECK_EQ(aggroRange(monsterDef(1)), 4);   // 树精 Guard

    Monster m(13, 0, 0); // 妖狐 Coward
    m.maxHp = m.hp = 100;
    CHECK_EQ(monsterFlees(m), false);
    m.hp = 40;
    CHECK_EQ(monsterFlees(m), true);
    Monster brave(7, 0, 0); // 狼妖 Brave 永不逃
    brave.maxHp = brave.hp = 100; brave.hp = 1;
    CHECK_EQ(monsterFlees(brave), false);
}

// ================= dungeon =================

TEST(dungeon_generation_invariants) {
    Rng::get().seed(777);
    for (int seed = 0; seed < 60; ++seed) {
        Dungeon d;
        d.generate(1 + seed % 30, 2, 2);

        CHECK_GE(d.rooms().size(), (size_t)1);
        CHECK(d.walkable(d.stairsX(), d.stairsY()));

        // 出生点附近 6 格内不放怪（spawn 传入 2,2 的场合）
        for (const auto& m : d.spawnedMonsters()) {
            CHECK(d.walkable(m.x, m.y));
            CHECK(std::hypot(m.x - 2, m.y - 2) >= 5.9);
            CHECK_GE(m.hp, 1);
        }
        // 掉落都在可走格上
        for (const auto& g : d.spawnedGroundItems())
            CHECK(d.walkable(g.x, g.y));

        // 地图边界全是墙
        for (int x = 0; x < Dungeon::W; ++x) {
            CHECK_EQ(d.blocksSight(x, 0), true);
            CHECK_EQ(d.blocksSight(x, Dungeon::H - 1), true);
        }
        // 楼梯不等于出生点
        CHECK_NE(d.stairsX() + d.stairsY(), 4);
    }
}

TEST(dungeon_deterministic_with_seed) {
    Rng::get().seed(20260816);
    Dungeon a;
    a.generate(5, 6, 6);
    Rng::get().seed(20260816);
    Dungeon b;
    b.generate(5, 6, 6);
    CHECK_EQ(a.stairsX(), b.stairsX());
    CHECK_EQ(a.stairsY(), b.stairsY());
    CHECK_EQ(a.rooms().size(), b.rooms().size());
    int walkA = 0, walkB = 0;
    for (int y = 0; y < Dungeon::H; ++y)
        for (int x = 0; x < Dungeon::W; ++x) {
            walkA += a.walkable(x, y) ? 1 : 0;
            walkB += b.walkable(x, y) ? 1 : 0;
        }
    CHECK_EQ(walkA, walkB);
}

// ================= player =================

TEST(player_stats_and_equipment) {
    Player p;
    p.init(L"测试", Element::Shui);
    CHECK_EQ(p.hp, p.maxHp);
    CHECK_EQ(p.gold, 10);
    int bare = p.atk();
    CHECK_GE(bare, 5);

    p.weaponId = 1;  p.weaponBonus = 2;   // 铁剑(3) + 洗炼2
    CHECK_EQ(p.atk(), bare + 5);
    p.armorId = 20; p.armorBonus = 0;     // 粗布道袍(2)
    CHECK_EQ(p.def(), p.defTotal - 0 + 0 + 2 - 0 + 0); // defTotal + 2
}

TEST(player_breakthrough_paths) {
    // 第一境 100% 成功
    {
        Player p;
        p.init(L"测试", Element::Jin);
        p.exp = 1000;
        std::wstring msg;
        CHECK_EQ(p.readyToBreak(), true);
        CHECK_EQ(p.tribulate(msg), true);
        CHECK_EQ(p.realmIdx, 1);
        CHECK_EQ(p.hp, p.maxHp);        // 突破回满
    }
    // 第二境 95%：反复尝试直到两种结果都见过（失败概率 5%，300 次内必现）
    bool sawFail = false, sawOk = false;
    for (int i = 0; i < 300 && !(sawFail && sawOk); ++i) {
        Player p;
        p.init(L"测试", Element::Jin);
        p.realmIdx = 1;
        p.exp = realms()[1].expNeed + 50;
        p.hp = p.maxHp;
        std::wstring msg;
        if (p.tribulate(msg)) sawOk = true;
        else {
            sawFail = true;
            CHECK_LE(p.hp, p.maxHp / 2 + 1); // 失败掉半血
        }
    }
    CHECK(sawOk);
    CHECK(sawFail);
    // 修为不足不许渡劫
    {
        Player p;
        p.init(L"测试", Element::Jin);
        p.exp = 0;
        std::wstring msg;
        CHECK_EQ(p.readyToBreak(), false);
        CHECK_EQ(p.tribulate(msg), false);
    }
    // 满级不再突破
    {
        Player p;
        p.init(L"测试", Element::Jin);
        p.realmIdx = (int)realms().size() - 1;
        CHECK_EQ(p.readyToBreak(), false);
    }
}

// ================= combat =================

TEST(combat_resolve_hit) {
    Rng::get().seed(31337);
    // 闪避率 0：绝不闪避
    for (int i = 0; i < 500; ++i) {
        HitResult r = resolveHit(Element::Jin, 50, Element::Mu, 10, 0.0);
        CHECK_EQ(r.dodged, false);
        CHECK_GE(r.damage, 1);
        double expect[] = { 0.7, 1.0, 1.15, 1.5 };
        bool multOk = false;
        for (double e : expect) multOk = multOk || std::abs(r.elemMult - e) < 1e-9;
        CHECK(multOk);
    }
    // 攻击远低于防御：保底 1 点
    for (int i = 0; i < 100; ++i) {
        HitResult r = resolveHit(Element::Tu, 1, Element::Shui, 999, 0.0);
        CHECK_EQ(r.damage, 1);
    }
    // 闪避率 100%：必定闪避
    for (int i = 0; i < 100; ++i) {
        HitResult r = resolveHit(Element::Jin, 10, Element::Jin, 0, 1.0);
        CHECK_EQ(r.dodged, true);
    }
    // 玩家/妖怪互打的伤害有界
    Player p;
    p.init(L"测试", Element::Huo);
    Monster m(11, 0, 0); // 火蜥蜴
    m.maxHp = m.hp = monsterDef(11).hp;
    for (int i = 0; i < 200; ++i) {
        HitResult a = playerHits(p, m);
        if (!a.dodged) CHECK_GE(a.damage, 1);   // 闪避时伤害为 0 是正常行为
        HitResult b = monsterHits(p, m);
        if (!b.dodged) CHECK_GE(b.damage, 1);
    }
}

// ================= events =================

TEST(events_integrity_and_sampling) {
    auto& dex = eventDex();
    CHECK_GE(dex.size(), (size_t)30);
    for (const auto& e : dex) {
        CHECK_GE(e.choices.size(), (size_t)2);
        CHECK(!e.title.empty());
        CHECK(!e.text.empty());
        for (const auto& c : e.choices)
            CHECK(!c.outcome.empty());
    }
    // 采样：第 1 层只出 minFloor=1 的事件；第 30 层全池
    Rng::get().seed(99);
    for (int i = 0; i < 300; ++i) {
        const EventDef& e = randomEvent(1);
        CHECK_LE(e.minFloor, 1);
    }
    for (int i = 0; i < 300; ++i) {
        const EventDef& e = randomEvent(30);
        CHECK_LE(e.minFloor, 30);
    }
}

// ================= save =================

TEST(save_roundtrip) {
    std::remove("yaota_save.txt");
    Player p;
    p.init(L"存档人", Element::Tu);
    p.inventory.clear();   // 排除新手礼包干扰，精确控制背包内容
    p.realmIdx = 3;
    p.exp = 777;
    p.hp = 123; p.maxHp = 456;
    p.gold = 8899;
    p.floor = 12;
    p.weaponId = 8; p.weaponBonus = 3;
    p.tribulationBonus = 15;
    p.addItem(41, 4);
    p.addItem(60, 1);

    CHECK_EQ(saveExists(), false);
    CHECK_EQ(saveToFile(p, 21), true);
    CHECK_EQ(saveExists(), true);

    Player q;
    int kills = -1;
    CHECK_EQ(loadFromFile(q, kills), true);
    CHECK_EQ(q.name, std::wstring(L"存档人"));
    CHECK_EQ(q.realmIdx, 3);
    CHECK_EQ(q.exp, 777);
    CHECK_EQ(q.hp, 123);
    CHECK_EQ(q.maxHp, 456);
    CHECK_EQ(q.gold, 8899);
    CHECK_EQ(q.floor, 12);
    CHECK_EQ(q.weaponId, 8);
    CHECK_EQ(q.weaponBonus, 3);
    CHECK_EQ(q.tribulationBonus, 15);
    CHECK_EQ(kills, 21);
    CHECK_EQ(q.inventory.size(), (size_t)2);
    CHECK_EQ(q.inventory[0].defId, 41);
    CHECK_EQ(q.inventory[0].count, 4);
    std::remove("yaota_save.txt");
}

TEST(save_rejects_garbage) {
    {
        FILE* f = std::fopen("yaota_save.txt", "w");
        std::fputs("not a save file\n", f);
        std::fclose(f);
    }
    Player p;
    int kills = 0;
    CHECK_EQ(loadFromFile(p, kills), false);
    std::remove("yaota_save.txt");
}

// ================= game（回合驱动）=================

TEST(game_endturn_simulation) {
    Rng::get().seed(8888);
    Game g;
    g.newGame(L"模拟人", Element::Mu);
    Player& p = g.debugPlayer();

    // 血量拉高，专注验证回合推进与妖怪 AI 不崩
    p.maxHp = 1000000;
    p.hp = 1000000;
    for (int i = 0; i < 300; ++i) {
        g.debugEndTurn();
        CHECK(p.alive);
        CHECK_LE(p.hp, p.maxHp);
    }
    // 妖怪要么活着在界内，要么死了
    auto& mons = g.debugMonsters();
    for (const auto& m : mons) {
        if (m.alive) {
            CHECK_LE(0, m.x);
            CHECK_LE(m.x, Dungeon::W - 1);
            CHECK_LE(0, m.y);
            CHECK_LE(m.y, Dungeon::H - 1);
            CHECK(g.debugDungeon().walkable(m.x, m.y));
        }
    }
}

int main() {
    return tf::runAll();
}
