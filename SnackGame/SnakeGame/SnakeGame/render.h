#ifndef RENDER_H
#define RENDER_H

#include "DCList.h"
#include "game.h"

void DrawGameGrid(void);
void DrawSnake(DCListNode *body, Direction dir);
void DrawFood(Position pos);

#endif
