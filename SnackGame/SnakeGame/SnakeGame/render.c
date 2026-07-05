#include "render.h"
#include "raylib.h"

#define CELL_SIZE 30
#define CELL_PADDING 1
#define CELL_ROUNDNESS 0.3f

void DrawGameGrid(void)
{
    Color gridColor = { 0x33, 0x33, 0x33, 0x40 };
    for (int i = 0; i <= GRID_SIZE; i++)
    {
        DrawLine(i * CELL_SIZE, 0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE, gridColor);
        DrawLine(0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE, i * CELL_SIZE, gridColor);
    }
}

static void DrawSnakeHead(int x, int y, Direction dir)
{
    Color headColor = { 0x0D, 0x47, 0xA1, 0xFF };
    float px = (float)(x * CELL_SIZE + CELL_PADDING);
    float py = (float)(y * CELL_SIZE + CELL_PADDING);
    float size = (float)(CELL_SIZE - CELL_PADDING * 2);
    DrawRectangleRounded((Rectangle){ px, py, size, size }, CELL_ROUNDNESS, 8, headColor);

    float cx = px + size / 2.0f;
    float cy = py + size / 2.0f;

    float eyeR = 3.5f;
    float pupilR = 1.8f;
    float eyeSpacing = 4.5f;
    float eyeForward = 1.5f;
    float pupilForward = 1.5f;

    float e1x, e1y, e2x, e2y;
    float p1x, p1y, p2x, p2y;
    float mx, my, mouthA1, mouthA2;

    switch (dir)
    {
    case DIR_RIGHT:
        e1x = cx + eyeForward;              e1y = cy - eyeSpacing;
        e2x = cx + eyeForward;              e2y = cy + eyeSpacing;
        p1x = e1x + pupilForward;           p1y = e1y;
        p2x = e2x + pupilForward;           p2y = e2y;
        mx = cx + 5.0f;                     my = cy;
        mouthA1 = -40.0f;                   mouthA2 = 40.0f;
        break;
    case DIR_LEFT:
        e1x = cx - eyeForward;              e1y = cy - eyeSpacing;
        e2x = cx - eyeForward;              e2y = cy + eyeSpacing;
        p1x = e1x - pupilForward;           p1y = e1y;
        p2x = e2x - pupilForward;           p2y = e2y;
        mx = cx - 5.0f;                     my = cy;
        mouthA1 = 140.0f;                   mouthA2 = 220.0f;
        break;
    case DIR_UP:
        e1x = cx - eyeSpacing;              e1y = cy - eyeForward;
        e2x = cx + eyeSpacing;              e2y = cy - eyeForward;
        p1x = e1x;                          p1y = e1y - pupilForward;
        p2x = e2x;                          p2y = e2y - pupilForward;
        mx = cx;                            my = cy - 5.0f;
        mouthA1 = 50.0f;                    mouthA2 = 130.0f;
        break;
    case DIR_DOWN:
        e1x = cx - eyeSpacing;              e1y = cy + eyeForward;
        e2x = cx + eyeSpacing;              e2y = cy + eyeForward;
        p1x = e1x;                          p1y = e1y + pupilForward;
        p2x = e2x;                          p2y = e2y + pupilForward;
        mx = cx;                            my = cy + 5.0f;
        mouthA1 = 230.0f;                   mouthA2 = 310.0f;
        break;
    }

    DrawCircleV((Vector2){ e1x, e1y }, eyeR, WHITE);
    DrawCircleV((Vector2){ e2x, e2y }, eyeR, WHITE);
    DrawCircleV((Vector2){ p1x, p1y }, pupilR, BLACK);
    DrawCircleV((Vector2){ p2x, p2y }, pupilR, BLACK);

    DrawRing((Vector2){ mx, my }, 3.0f, 4.5f, mouthA1, mouthA2, 8, WHITE);
}

static void DrawSnakeBody(int x, int y)
{
    Color bodyColor = { 0x42, 0xA5, 0xF5, 0xFF };
    float px = (float)(x * CELL_SIZE + CELL_PADDING);
    float py = (float)(y * CELL_SIZE + CELL_PADDING);
    float size = (float)(CELL_SIZE - CELL_PADDING * 2);
    DrawRectangleRounded((Rectangle){ px, py, size, size }, CELL_ROUNDNESS, 8, bodyColor);
}

void DrawSnake(DCListNode *body, Direction dir)
{
    if (body == NULL || DCListIsEmpty(body))
        return;

    DCListNode *node = body->next;
    int isHead = 1;

    while (node != body)
    {
        if (isHead)
        {
            DrawSnakeHead(node->data.x, node->data.y, dir);
            isHead = 0;
        }
        else
        {
            DrawSnakeBody(node->data.x, node->data.y);
        }
        node = node->next;
    }
}

void DrawFood(Position pos)
{
    Color foodColor = { 0xFF, 0xD7, 0x40, 0xFF };
    float px = (float)(pos.x * CELL_SIZE + CELL_PADDING);
    float py = (float)(pos.y * CELL_SIZE + CELL_PADDING);
    float size = (float)(CELL_SIZE - CELL_PADDING * 2);
    DrawRectangleRounded((Rectangle){ px, py, size, size }, CELL_ROUNDNESS, 8, foodColor);
}
