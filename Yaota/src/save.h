// save.h —— 存档：玩家状态 + 层数（地图为重新生成，见 save.cpp 注释）
#pragma once

#include "player.h"

namespace yaota {

bool saveExists();
// 存档写入 yaota_save.txt（与可执行文件同目录）
bool saveToFile(const Player& p, int kills);
// 读取成功返回 true 并填充 p 与 kills
bool loadFromFile(Player& p, int& kills);

} // namespace yaota
