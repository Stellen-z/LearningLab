// game.cpp —— 主循环与各系统的粘合逻辑
#include "game.h"
#include "render.h"
#include "combat.h"
#include "save.h"
#include "rng.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <cstdlib>

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

// 生成本层：先随机生成一次拿到出生房间，再用真正的出生点重新生成，
// 保证妖怪不会贴脸刷新、楼梯离出生点足够远。
void Game::setupFloor() {
    int f = player_.floor;
    dungeon_.generate(f, 2, 2);
    int sx = dungeon_.rooms()[0].cx();
    int sy = dungeon_.rooms()[0].cy();
    dungeon_.generate(f, sx, sy);

    player_.x = sx;
    player_.y = sy;

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
