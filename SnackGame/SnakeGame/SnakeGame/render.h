#ifndef RENDER_H
#define RENDER_H

#include "DCList.h"
#include "game.h"

void DrawGameGrid(void);
void DrawSnake(DCListNode *body, Direction dir);
void DrawFoods(Food *foods, int count);
void DrawSidebar(int score, int high_score, float move_interval);
void DrawMenu(int selected);
void DrawHowToPlay(void);

#endif
