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
#include "../src/render.h"
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
            bool isShengBy = ((int)d == 0 && (int)a == 2) || ((int)d == 2 && (int)a == 1) ||
                             ((int)d == 1 && (int)a == 3) || ((int)d == 3 && (int)a == 4) ||
                             ((int)d == 4 && (int)a == 0); // 他生我：中性 1.0
            if (a == d)        CHECK_EQ(m, 1.0);
            else if (isKe)     CHECK_EQ(m, 1.5);
            else if (isSheng)  CHECK_EQ(m, 1.15);
            else if (isShengBy) CHECK_EQ(m, 1.0);
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
    // 攻击远低于防御：保底伤害（暴击时乘区放大，允许 1~3）
    for (int i = 0; i < 100; ++i) {
        HitResult r = resolveHit(Element::Tu, 1, Element::Shui, 999, 0.0);
        CHECK_GE(r.damage, 1);
        CHECK_LE(r.damage, 3);
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

// ================= game（玩法逻辑，经 GamePeer 后门）=================

TEST(game_move_walls_and_logs) {
    Rng::get().seed(606);
    Game g;
    g.newGame(L"行者", Element::Jin);
    Player& p = g.debugPlayer();
    auto& logs = GamePeer::logs(g);

    // 越界：安静返回不崩
    p.x = 0; p.y = 0;
    GamePeer::move(g, -1, 0);

    // 地图上边框是墙 → 撞墙记日志
    size_t before = logs.size();
    p.x = 1; p.y = 1;
    GamePeer::move(g, 0, -1);
    CHECK(logs.size() > before);

    // 房间中心向右走：走了或被挡，坐标不会越界
    const Room& r = GamePeer::dungeon(g).rooms()[0];
    p.x = r.cx(); p.y = r.cy();
    GamePeer::move(g, 1, 0);
    CHECK(p.x >= 0 && p.x < Dungeon::W);
}

TEST(game_pickup_and_drop) {
    Rng::get().seed(607);
    Game g;
    g.newGame(L"行者", Element::Jin);
    Player& p = g.debugPlayer();
    p.inventory.clear();
    auto& ground = GamePeer::ground(g);
    ground.clear();   // 清掉本层随机掉落，精确控制测试场景

    ground.push_back(GroundItem(p.x, p.y, Item(41, 2)));   // 小还丹 x2
    GamePeer::pickup(g);
    CHECK_EQ(p.inventory.size(), (size_t)1);
    CHECK_EQ(p.inventory[0].defId, 41);
    CHECK_EQ(p.inventory[0].count, 2);
    CHECK_EQ(ground.size(), (size_t)0);

    GamePeer::drop(g, 0);
    CHECK_EQ(p.inventory.size(), (size_t)0);
    CHECK_EQ(ground.size(), (size_t)1);
    CHECK_EQ(ground[0].x, p.x);
    CHECK_EQ(ground[0].y, p.y);

    GamePeer::pickup(g);   // 捡回来
    CHECK_EQ(p.inventory.size(), (size_t)1);
    GamePeer::pickup(g);   // 空手拾取：只记日志不崩
    CHECK_EQ(p.inventory.size(), (size_t)1);
}

TEST(game_pill_effects) {
    Rng::get().seed(608);
    Game g;
    g.newGame(L"药人", Element::Mu);
    Player& p = g.debugPlayer();
    p.inventory.clear();

    p.hp = 10;
    p.addItem(40, 1);                       // 回气散 +40
    CHECK_EQ(GamePeer::use(g, 0), true);
    CHECK_EQ(p.hp, 50);
    CHECK_EQ(p.inventory.size(), (size_t)0);  // 用光自动移除

    p.exp = 5;
    p.addItem(44, 1);                       // 聚灵丹 +40 修为
    GamePeer::use(g, 0);
    CHECK_EQ(p.exp, 45);

    int mh = p.maxHp;
    p.addItem(45, 1); GamePeer::use(g, 0);  // 培元丹
    CHECK_EQ(p.maxHp, mh + 20);

    int a0 = p.atkTotal;
    p.addItem(46, 1); GamePeer::use(g, 0);  // 洗髓丹
    CHECK_EQ(p.atkTotal, a0 + 2);

    int d0 = p.defTotal;
    p.addItem(47, 1); GamePeer::use(g, 0);  // 金刚丹
    CHECK_EQ(p.defTotal, d0 + 2);

    p.tribulationBonus = 0;
    p.addItem(48, 1); GamePeer::use(g, 0);  // 破障丹
    CHECK_EQ(p.tribulationBonus, 15);

    p.hp = 1;
    p.addItem(50, 1); GamePeer::use(g, 0);  // 龟息丹：回复上限一半
    CHECK_EQ(p.hp, 1 + p.maxHp / 2);

    p.hp = 5;
    p.addItem(49, 1); GamePeer::use(g, 0);  // 解毒丹：保底回 15
    CHECK_EQ(p.hp, 20);
}

TEST(game_scroll_effects) {
    Rng::get().seed(609);
    Game g;
    g.newGame(L"符师", Element::Huo);
    Player& p = g.debugPlayer();
    p.maxHp = p.hp = 1000000;
    p.mp = 100;
    auto& mons = g.debugMonsters();
    mons.clear();

    // 火球符：波及相邻
    mons.push_back(Monster(0, p.x + 1, p.y));
    mons.push_back(Monster(0, p.x, p.y + 1));
    mons[0].maxHp = mons[0].hp = 1000;
    mons[1].maxHp = mons[1].hp = 1000;
    p.inventory.clear();
    p.addItem(60, 1);
    GamePeer::use(g, 0);
    CHECK(mons[0].hp < 1000);
    CHECK(mons[1].hp < 1000);
    CHECK(mons[0].alive && mons[1].alive);

    // 冰封符：震慑
    p.addItem(61, 1); GamePeer::use(g, 0);
    CHECK(mons[0].stunTurns > 0 || mons[1].stunTurns > 0);

    // 摄妖符：范围内震慑
    mons[0].stunTurns = 0;
    p.addItem(64, 1); GamePeer::use(g, 0);
    CHECK(mons[0].stunTurns > 0);

    // 天眼符
    CHECK_EQ(GamePeer::revealAll(g), false);
    p.addItem(63, 1); GamePeer::use(g, 0);
    CHECK_EQ(GamePeer::revealAll(g), true);

    // 五雷符：劈最近
    mons[0].hp = 5;
    int exp0 = p.exp;
    p.addItem(65, 1); GamePeer::use(g, 0);
    CHECK_EQ(mons[0].alive, false);
    CHECK_EQ(GamePeer::kills(g), 1);
    CHECK(p.exp > exp0);

    // 传送符：落到某个房间中心
    p.inventory.clear();
    p.addItem(62, 1); GamePeer::use(g, 0);
    bool inRoom = false;
    for (const auto& r : GamePeer::dungeon(g).rooms())
        if (r.cx() == p.x && r.cy() == p.y) inRoom = true;
    CHECK(inRoom);
}

TEST(game_equip_swap_and_refine) {
    Rng::get().seed(610);
    Game g;
    g.newGame(L"武人", Element::Jin);
    Player& p = g.debugPlayer();
    p.inventory.clear();

    p.addItem(1, 1);                        // 铁剑 攻+3
    int atk0 = p.atk();
    GamePeer::equip(g, 0);
    CHECK_EQ(p.weaponId, 1);
    CHECK_EQ(p.atk(), atk0 + 3);

    p.addItem(3, 1);                        // 换火浣鞭，旧剑回包
    GamePeer::equip(g, 0);
    CHECK_EQ(p.weaponId, 3);
    bool oldBack = false;
    for (const auto& it : p.inventory) if (it.defId == 1) oldBack = true;
    CHECK(oldBack);

    p.addItem(20, 1);
    GamePeer::equip(g, 1);                  // 法袍格
    CHECK_EQ(p.armorId, 20);

    // 炼化：奇物/炼材变灵石
    p.inventory.clear();
    p.addItem(80, 2);                       // 妖丹 x2 单价30
    p.addItem(91, 3);                       // 铁矿石 x3 单价8
    int gold0 = p.gold;
    GamePeer::refine(g);
    CHECK_EQ(p.gold, gold0 + 60 + 24);
    CHECK_EQ(p.inventory.size(), (size_t)0);
}

TEST(game_burst_and_meditate) {
    Rng::get().seed(611);
    Game g;
    g.newGame(L"爆发者", Element::Shui);
    Player& p = g.debugPlayer();
    p.maxHp = p.hp = 1000000;
    p.mp = 100;
    auto& mons = g.debugMonsters();
    mons.clear();
    mons.push_back(Monster(0, p.x + 1, p.y));
    mons[0].maxHp = mons[0].hp = 5000;

    GamePeer::burst(g);
    CHECK_EQ(p.mp, 85);
    CHECK(mons[0].hp < 5000);

    p.mp = 5;
    GamePeer::burst(g);                     // 灵力不足分支：无消耗无伤害
    CHECK_EQ(p.mp, 5);

    // 打坐：回蓝回红
    mons.clear();                           // 清场防偷袭
    p.hp = 10; p.mp = 0;
    GamePeer::meditate(g);
    CHECK(p.mp > 0);
    CHECK(p.hp > 10);
}

TEST(game_monster_melee_and_flee) {
    Rng::get().seed(612);
    Game g;
    g.newGame(L"活靶", Element::Tu);
    Player& p = g.debugPlayer();
    p.maxHp = p.hp = 1000000;
    auto& mons = g.debugMonsters();

    // 贴脸野狗妖连咬 20 回合：必有命中（单回合 5% 闪避）
    mons.clear();
    mons.push_back(Monster(0, p.x + 1, p.y));
    mons[0].maxHp = mons[0].hp = 100;
    int hp0 = p.hp;
    for (int i = 0; i < 20; ++i) GamePeer::monsterTurns(g);
    CHECK(p.hp < hp0);

    // 胆小妖狐残血逃跑（远离或原地，绝不靠近玩家）
    mons.clear();
    mons.push_back(Monster(13, p.x + 1, p.y));   // 玩家右侧
    mons[0].maxHp = 100; mons[0].hp = 10;
    int fx = mons[0].x;
    GamePeer::monsterTurns(g);
    CHECK_GE(mons[0].x, fx);   // 向右远离玩家（被挡则原地）
}

TEST(game_stairs_next_floor) {
    Rng::get().seed(613);
    Game g;
    g.newGame(L"登塔者", Element::Jin);
    Player& p = g.debugPlayer();
    p.floor = 3;
    auto& d = GamePeer::dungeon(g);
    p.x = d.stairsX();
    p.y = d.stairsY();
    GamePeer::stairs(g);
    CHECK_EQ(p.floor, 4);
    CHECK(d.walkable(p.x, p.y));            // 新层出生点合法
    CHECK(d.stairsX() != p.x || d.stairsY() != p.y);  // 楼梯重新放置
}

TEST(game_event_fx_all_kinds) {
    Rng::get().seed(614);
    Game g;
    g.newGame(L"奇缘者", Element::Mu);
    Player& p = g.debugPlayer();
    std::wstring out;

    p.inventory.clear();
    GamePeer::eventFx(g, FxHerbs, out);     // 灵草 x3
    bool herb = false;
    for (auto& it : p.inventory) if (it.defId == 90 && it.count >= 3) herb = true;
    CHECK(herb);

    int mh = p.maxHp;
    GamePeer::eventFx(g, FxMaxHpUp, out);
    CHECK_EQ(p.maxHp, mh + 15);
    GamePeer::eventFx(g, FxMaxHpDown, out);
    CHECK_EQ(p.maxHp, mh + 5);

    p.tribulationBonus = 0;
    GamePeer::eventFx(g, FxTribBuff, out);
    CHECK_EQ(p.tribulationBonus, 10);

    p.hp = 1; p.mp = 1;
    GamePeer::eventFx(g, FxHealFull, out);
    CHECK_EQ(p.hp, p.maxHp);
    CHECK_EQ(p.mp, p.maxMp);

    int a0 = p.atkTotal, d0 = p.defTotal;
    GamePeer::eventFx(g, FxLearnAtk, out);
    CHECK_EQ(p.atkTotal, a0 + 1);
    GamePeer::eventFx(g, FxLearnDef, out);
    CHECK_EQ(p.defTotal, d0 + 1);

    p.weaponId = -1;
    GamePeer::eventFx(g, FxUpgradeWeapon, out);   // 无武器分支
    CHECK_EQ(p.weaponId, -1);
    p.weaponId = 1; p.weaponBonus = 0;
    GamePeer::eventFx(g, FxUpgradeWeapon, out);
    CHECK_EQ(p.weaponBonus, 2);

    p.armorId = -1;
    GamePeer::eventFx(g, FxUpgradeArmor, out);
    CHECK_EQ(p.armorId, -1);
    p.armorId = 20; p.armorBonus = 0;
    GamePeer::eventFx(g, FxUpgradeArmor, out);
    CHECK_EQ(p.armorBonus, 2);

    auto& d = GamePeer::dungeon(g);
    GamePeer::eventFx(g, FxTeleportNear, out);
    CHECK(std::hypot(p.x - d.stairsX(), p.y - d.stairsY()) <= 4.5);

    p.inventory.clear();
    GamePeer::eventFx(g, FxRandomItem, out);
    CHECK_EQ(p.inventory.size(), (size_t)1);
    GamePeer::eventFx(g, FxRandomPill, out);
    CHECK_GE(p.inventory.size(), (size_t)1);

    GamePeer::eventFx(g, FxNothing, out);   // 不崩即可
    GamePeer::eventFx(g, FxNone, out);
}

// ================= 补充：工具函数与查询分支 =================

TEST(types_utils_roundtrip) {
    std::wstring ws = L"修仙ABC渡劫";
    CHECK_EQ(utf8ToWstr(wstrToUtf8(ws)), ws);
    CHECK_EQ(wstrToUtf8(L"ascii"), std::string("ascii"));

    for (int c = 0; c <= 8; ++c)
        CHECK(std::string(colorCode((Color)c)).size() > 0);
    for (int t = 0; t <= 5; ++t)
        CHECK(std::wstring(itemTypeWName((ItemType)t)).size() >= 2);
    for (int e = 0; e <= 4; ++e)
        CHECK_EQ(std::wstring(elementWName((Element)e)).size(), (size_t)1);
}

TEST(combat_ai_and_fallbacks) {
    CHECK_EQ(aggroRange(monsterDef(13)), 6);  // 妖狐 Coward
    CHECK_EQ(aggroRange(monsterDef(3)), 7);   // 火鼠 Sneaky → 默认档

    // Brave 加成分支与普通怪都过一遍
    Player p;
    p.init(L"靶子", Element::Jin);
    Monster brave(7, 0, 0), plain(5, 0, 0);
    brave.maxHp = brave.hp = 1;
    plain.maxHp = plain.hp = 1;
    for (int i = 0; i < 100; ++i) {
        HitResult a = monsterHits(p, brave);
        if (!a.dodged) CHECK_GE(a.damage, 1);
        HitResult b = monsterHits(p, plain);
        if (!b.dodged) CHECK_GE(b.damage, 1);
    }

    // 图鉴查询的未知 id 回退
    CHECK_EQ(eventDef(9999).id, eventDex().front().id);
    CHECK_EQ(monsterDef(9999).id, monsterDex().front().id);
}

// ================= render（非交互部分：绘制不读 stdin）=================

TEST(render_hud_map_log) {
    Rng::get().seed(701);
    Game g;
    g.newGame(L"画中人", Element::Jin);
    Player& p = g.debugPlayer();

    Renderer::clearScreen();
    Renderer::drawHud(p, 3);

    std::vector<char> explored;
    Renderer::drawMap(GamePeer::dungeon(g), p, g.debugMonsters(),
                      GamePeer::ground(g), false, explored);
    // 天眼全图分支 + 迷雾记忆复用
    Renderer::drawMap(GamePeer::dungeon(g), p, g.debugMonsters(),
                      GamePeer::ground(g), true, explored);
    Renderer::drawLog(GamePeer::logs(g));

    // 画完世界照常运转
    CHECK_EQ(GamePeer::over(g), false);
    CHECK_GE(GamePeer::logs(g).size(), (size_t)2);
}

int main() {
    return tf::runAll();
}
