// dungeon.cpp —— 妖塔每一层的随机生成
// 思路（经典 Roguelike 做法）：
//   1. 整张图先填满山岩
//   2. 随机撒若干不重叠的矩形房间，挖空成石地
//   3. 把房间中心按顺序用 L 形走廊连起来
//   4. 离玩家最远的房间放楼梯
//   5. 撒妖怪、掉落、祭坛、灵草、宝箱
#include "dungeon.h"
#include "rng.h"

#include <algorithm>
#include <cmath>

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

    // ---- 4. 楼梯放在离玩家最远的房间 ----
    // （变量名避开 far——windows.h 会把它定义成宏，单文件合并版会撞名）
    const Room& farRoom = farthestRoomFrom(playerX, playerY);
    stairsX_ = farRoom.cx();
    stairsY_ = farRoom.cy();
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
