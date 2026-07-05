#include "raylib.h"
#include "game.h"
#include "render.h"

int main(void)
{
    InitWindow(600, 600, "Snake Game");
    SetTargetFPS(60);

    DCListNode *body = DCListInit();
    Position p1 = { 10, 10 };
    Position p2 = { 9, 10 };
    Position p3 = { 8, 10 };
    DCListPushFront(body, p1);
    DCListPushBack(body, p2);
    DCListPushBack(body, p3);
    Direction testDir = DIR_RIGHT;
    Position foodPos = { 15, 10 };

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });
        DrawGameGrid();
        DrawSnake(body, testDir);
        DrawFood(foodPos);
        EndDrawing();
    }

    DCListDestroy(body);
    CloseWindow();
    return 0;
}
