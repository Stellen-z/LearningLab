// dungeon.h —— 随机地图生成：房间 + 走廊 + 楼梯 + 落物点
#pragma once

#include "types.h"
#include "item.h"
#include "monster.h"

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
