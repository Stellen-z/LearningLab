#include "render.h"
#include "raylib.h"

/* ── 网格常量 ── */
#define CELL_SIZE 30         /* 每格像素数 */
#define CELL_PADDING 1       /* 图形内缩（不让相邻格贴死） */
#define CELL_ROUNDNESS 0.3f  /* 圆角比例（0=直角, 1=正圆） */

/* ── 侧边栏常量 ── */
#define SIDEBAR_X (GRID_SIZE * CELL_SIZE)      /* 侧边栏起始 x（紧接网格右缘） */
#define SIDEBAR_W 240                          /* 侧边栏宽度 */
#define SIDEBAR_CX (SIDEBAR_X + SIDEBAR_W / 2)  /* 侧边栏水平中线 x */

/*
* 绘制 20×20 网格线
* 每个格子 30×30 像素，横纵各画 21 条线（含边界）。
*/
void DrawGameGrid(void)
{
    Color gridColor = { 0x55, 0x55, 0x55, 0x90 };
    for (int i = 0; i <= GRID_SIZE; i++)
    {
        /* 竖线 */
        DrawLine(i * CELL_SIZE, 0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE, gridColor);
        /* 横线 */
        DrawLine(0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE, i * CELL_SIZE, gridColor);
    }
}

/*
* 绘制蛇头（圆角方块 + 眼睛 + 嘴巴）
* 蛇头用深蓝色圆角方块，白色眼球 + 黑色瞳孔 + 嘴巴弧线。
* 眼睛和嘴巴的位置和朝向根据 dir 动态调整，使蛇头有"面向"感。
*
* @param x    格子 x 坐标
* @param y    格子 y 坐标
* @param dir  蛇头朝向（眼睛/嘴巴方向据此变化）
*/
static void DrawSnakeHead(int x, int y, Direction dir)
{
    /* 深蓝色圆角方块作为蛇头底色 */
    Color headColor = { 0x0D, 0x47, 0xA1, 0xFF };
    float px = (float)(x * CELL_SIZE + CELL_PADDING);
    float py = (float)(y * CELL_SIZE + CELL_PADDING);
    float size = (float)(CELL_SIZE - CELL_PADDING * 2);
    DrawRectangleRounded((Rectangle){ px, py, size, size }, CELL_ROUNDNESS, 8, headColor);

    /* 面部中心坐标 */
    float cx = px + size / 2.0f;
    float cy = py + size / 2.0f;

    /* 眼睛和瞳孔的尺寸参数 */
    float eyeR = 3.5f;           /* 眼白半径 */
    float pupilR = 1.8f;         /* 瞳孔半径 */
    float eyeSpacing = 4.5f;     /* 两眼到面部中轴的距离 */
    float eyeForward = 1.5f;     /* 眼睛向运动方向的偏移 */
    float pupilForward = 1.5f;   /* 瞳孔相对眼白的向前偏移 */

    float e1x, e1y, e2x, e2y;    /* 两只眼白中心 */
    float p1x, p1y, p2x, p2y;    /* 两个瞳孔中心 */
    float mx, my, mouthA1, mouthA2; /* 嘴巴中心 + 弧线起止角度 */

    /* 根据朝向计算眼睛和嘴巴的渲染参数 */
    switch (dir)
    {
    case DIR_RIGHT:   /* 朝右：眼睛和嘴偏右侧 */
        e1x = cx + eyeForward;    e1y = cy - eyeSpacing;
        e2x = cx + eyeForward;    e2y = cy + eyeSpacing;
        p1x = e1x + pupilForward; p1y = e1y;
        p2x = e2x + pupilForward; p2y = e2y;
        mx = cx + 5.0f;           my = cy;
        mouthA1 = -40.0f;          mouthA2 = 40.0f;    /* 从 -40° 到 40°，右侧弧 */
        break;
    case DIR_LEFT:    /* 朝左：眼睛和嘴偏左侧 */
        e1x = cx - eyeForward;    e1y = cy - eyeSpacing;
        e2x = cx - eyeForward;    e2y = cy + eyeSpacing;
        p1x = e1x - pupilForward; p1y = e1y;
        p2x = e2x - pupilForward; p2y = e2y;
        mx = cx - 5.0f;           my = cy;
        mouthA1 = 140.0f;          mouthA2 = 220.0f;   /* 左侧弧 */
        break;
    case DIR_UP:      /* 朝上：眼睛和嘴偏上侧 */
        e1x = cx - eyeSpacing;    e1y = cy - eyeForward;
        e2x = cx + eyeSpacing;    e2y = cy - eyeForward;
        p1x = e1x;                p1y = e1y - pupilForward;
        p2x = e2x;                p2y = e2y - pupilForward;
        mx = cx;                  my = cy - 5.0f;
        mouthA1 = 50.0f;           mouthA2 = 130.0f;   /* 上方弧 */
        break;
    case DIR_DOWN:    /* 朝下：眼睛和嘴偏下侧 */
        e1x = cx - eyeSpacing;    e1y = cy + eyeForward;
        e2x = cx + eyeSpacing;    e2y = cy + eyeForward;
        p1x = e1x;                p1y = e1y + pupilForward;
        p2x = e2x;                p2y = e2y + pupilForward;
        mx = cx;                  my = cy + 5.0f;
        mouthA1 = 230.0f;          mouthA2 = 310.0f;   /* 下方弧 */
        break;
    }

    /* 绘制眼白 */
    DrawCircleV((Vector2){ e1x, e1y }, eyeR, WHITE);
    DrawCircleV((Vector2){ e2x, e2y }, eyeR, WHITE);
    /* 绘制瞳孔 */
    DrawCircleV((Vector2){ p1x, p1y }, pupilR, BLACK);
    DrawCircleV((Vector2){ p2x, p2y }, pupilR, BLACK);

    /* 绘制嘴巴：环形弧线（内径 3px，外径 4.5px，1.5px 宽的弧形） */
    DrawRing((Vector2){ mx, my }, 3.0f, 4.5f, mouthA1, mouthA2, 8, WHITE);
}

/*
* 绘制蛇身一节（天蓝色圆角方块，无面部特征）
* @param x  格子 x 坐标
* @param y  格子 y 坐标
*/
static void DrawSnakeBody(int x, int y)
{
    Color bodyColor = { 0x42, 0xA5, 0xF5, 0xFF };
    float px = (float)(x * CELL_SIZE + CELL_PADDING);
    float py = (float)(y * CELL_SIZE + CELL_PADDING);
    float size = (float)(CELL_SIZE - CELL_PADDING * 2);
    DrawRectangleRounded((Rectangle){ px, py, size, size }, CELL_ROUNDNESS, 8, bodyColor);
}

/*
* 绘制整条蛇
* 从链表哨兵 next 开始遍历，第1个结点用 DrawSnakeHead（含表情），
* 后续结点用 DrawSnakeBody（纯色方块）。
*
* @param body  链表哨兵指针（空链表或 NULL 则不渲染）
* @param dir   蛇头朝向（控制表情方向）
*/
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
            DrawSnakeHead(node->data.x, node->data.y, dir);  /* 蛇头：有表情 */
            isHead = 0;
        }
        else
        {
            DrawSnakeBody(node->data.x, node->data.y);        /* 蛇身：普通方块 */
        }
        node = node->next;
    }
}

/*
* 绘制食物
* 金黄色圆角方块，醒目易识别。
* @param pos  食物的网格坐标
*/
void DrawFoods(Food *foods, int count)
{
    Color foodColor = { 0xFF, 0xD7, 0x40, 0xFF };
    for (int i = 0; i < count; i++)
    {
        if (!foods[i].active) continue;
        float px = (float)(foods[i].pos.x * CELL_SIZE + CELL_PADDING);
        float py = (float)(foods[i].pos.y * CELL_SIZE + CELL_PADDING);
        float size = (float)(CELL_SIZE - CELL_PADDING * 2);
        DrawRectangleRounded((Rectangle){ px, py, size, size }, CELL_ROUNDNESS, 8, foodColor);
    }
}

/* ── 侧边栏文本渲染 ── */

static Font g_sidebarFont = { 0 };   /* Verdana 字体对象（懒加载） */
static bool g_fontLoaded = false;     /* 是否已加载字体 */

/*
* 获取侧边栏渲染字体（懒加载）
* 首次调用时从 Windows 字体目录加载 Verdana Regular 96px，
* 高分辨率基础字号使缩小渲染时保留更多字形细节。
* @return  Verdana Font 对象
*/
static Font GetSidebarFont(void)
{
    if (!g_fontLoaded)
    {
        g_sidebarFont = LoadFontEx("C:/Windows/Fonts/verdana.ttf", 96, 0, 0);
        g_fontLoaded = true;
    }
    return g_sidebarFont;
}

/*
* 在侧边栏中水平居中绘制文本
* 先计算文本实际宽度，再根据侧边栏中线偏移到居中位置渲染。
*
* @param text      要绘制的文本
* @param y         文本渲染的垂直起始像素坐标
* @param fontSize  字号（像素）
* @param color     文本颜色
*/
static void DrawCenteredText(const char *text, int y, int fontSize, Color color)
{
    Font font = GetSidebarFont();
    Vector2 size = MeasureTextEx(font, text, (float)fontSize, 0);
    Vector2 pos = { SIDEBAR_CX - size.x / 2.0f, (float)y };
    DrawTextEx(font, text, pos, (float)fontSize, 0, color);
}

/*
* 绘制右侧信息侧边栏
* 分为四个区块（每个区块有独立配色）：
*   SCORE — 当前分数（琥珀色标题 + 奶油黄大号数值）
*   HIGH SCORE — 历史最高分（橙红色标题 + 浅橙数值）
*   SPEED — 当前速度等级（翠绿色标题 + 中绿等级）
*   操作提示 — 键位说明（紫色文字）
*
* @param score          当前分数
* @param high_score     历史最高分
* @param move_interval  当前移动间隔（用于计算速度等级）
*/
void DrawSidebar(int score, int high_score, float move_interval)
{
    int winH = GRID_SIZE * CELL_SIZE;  /* 侧边栏高度 = 游戏网格高 */

    /* 侧边栏背景：暗黑色，与主背景 #121212 形成自然过渡 */
    DrawRectangle(SIDEBAR_X, 0, SIDEBAR_W, winH, (Color){ 0x1A, 0x1A, 0x1A, 0xFF });

    /* ── 全局颜色定义 ── */
    Color dividerColor     = { 0x44, 0x44, 0x44, 0xFF };
    Color scoreTitleColor  = { 0xFF, 0xB3, 0x00, 0xFF };   /* 琥珀色 */
    Color scoreValueColor  = { 0xFF, 0xE0, 0x82, 0xFF };   /* 奶油黄 */
    Color hsTitleColor     = { 0xFF, 0x8A, 0x65, 0xFF };   /* 橙红色 */
    Color hsValueColor     = { 0xFF, 0xAB, 0x91, 0xFF };   /* 浅橙色 */
    Color speedTitleColor  = { 0x66, 0xBB, 0x6A, 0xFF };   /* 翠绿色 */
    Color speedValueColor  = { 0x81, 0xC7, 0x84, 0xFF };   /* 中绿色 */
    Color ctrlKeyColor     = { 0xBA, 0x68, 0xC8, 0xFF };   /* 紫色 */

    /* 左侧分隔线：区分游戏区域和信息栏 */
    DrawLine(SIDEBAR_X, 0, SIDEBAR_X, winH, dividerColor);

    /* ── 整体从 y=150 开始，使内容在 600px 高度内垂直居中 ── */
    int y = 150;
    char buf[32];

    /* 1. SCORE 区块 */
    DrawCenteredText("SCORE", y, 16, scoreTitleColor);
    y += 24;
    sprintf_s(buf, sizeof(buf), "%d", score);
    DrawCenteredText(buf, y, 36, scoreValueColor);   /* 36px 大号数值 */

    /* 2. HIGH SCORE 区块 */
    y += 55;
    DrawCenteredText("HIGH SCORE", y, 16, hsTitleColor);
    y += 24;
    sprintf_s(buf, sizeof(buf), "%d", high_score);
    DrawCenteredText(buf, y, 22, hsValueColor);      /* 22px 中等数值 */

    /* 3. SPEED 区块 */
    y += 50;
    DrawCenteredText("SPEED", y, 16, speedTitleColor);
    y += 24;
    /* 根据移动间隔计算速度等级：0.2s=1级，每减 0.02s 升 1 级 */
    int level = (int)((0.2f - move_interval) / 0.02f) + 1;
    if (level < 1) level = 1;
    sprintf_s(buf, sizeof(buf), "Level %d", level);
    DrawCenteredText(buf, y, 22, speedValueColor);

    /* ── 分隔线 ── */
    y += 45;
    DrawLine(SIDEBAR_X + 20, y, SIDEBAR_X + SIDEBAR_W - 20, y, dividerColor);

    /* 4. 操作提示区块：键位用 → 连接功能说明，一行一条 */
    y += 30;
    DrawCenteredText("WASD / Arrow  ->  Move",  y, 16, ctrlKeyColor);
    y += 28;
    DrawCenteredText("Space / P  ->  Pause",     y, 16, ctrlKeyColor);
    y += 28;
    DrawCenteredText("B  ->  Back",               y, 16, ctrlKeyColor);
    y += 28;
    DrawCenteredText("ESC  ->  Exit",              y, 16, ctrlKeyColor);
}

/*
* 绘制主菜单界面
* 标题 "SNAKE GAME" 居中大号显示，下方三个选项用 ↑↓ 切换。
* 当前选中项（selected[i]）以金黄色块高亮 + 白色文字渲染，
* 未选中项以浅灰色文字渲染。
*
* @param selected  当前选中项索引（0=开始, 1=操作说明, 2=退出）
*/
void DrawMenu(int selected)
{
    int winW = GRID_SIZE * CELL_SIZE;
    int winH = GRID_SIZE * CELL_SIZE;
    Font font = GetSidebarFont();
    ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });

    const char *title = "SNAKE GAME";
    int titleSize = 52;
    Vector2 titleSz = MeasureTextEx(font, title, (float)titleSize, 0);
    DrawTextEx(font, title, (Vector2){ (winW - titleSz.x) / 2.0f, 100 }, (float)titleSize, 0, (Color){ 0x42, 0xA5, 0xF5, 0xFF });

    const char *items[] = { "Play", "How to Play", "Exit" };
    int itemCount = 3;
    int startY = 250;
    int itemGap = 55;
    Color selBg = { 0xFF, 0xB3, 0x00, 0xFF };
    Color selText = WHITE;
    Color normText = { 0x90, 0x90, 0x90, 0xFF };

    for (int i = 0; i < itemCount; i++)
    {
        int y = startY + i * itemGap;
        int fs = 28;
        Vector2 isz = MeasureTextEx(font, items[i], (float)fs, 0);
        int x = (int)((winW - isz.x) / 2.0f);
        int padX = 24;
        int padY = 8;

        if (i == selected)
        {
            DrawRectangleRounded(
                (Rectangle){ (float)(x - padX), (float)y - padY, isz.x + padX * 2.0f, (float)(fs + padY * 2) },
                0.2f, 8, selBg);
            DrawTextEx(font, items[i], (Vector2){ (float)x, (float)y }, (float)fs, 0, selText);
        }
        else
        {
            DrawTextEx(font, items[i], (Vector2){ (float)x, (float)y }, (float)fs, 0, normText);
        }
    }

    const char *hint = "Arrow Keys / W S  ->  Select   |   Enter  ->  Confirm";
    int hs = 16;
    Vector2 hintSz = MeasureTextEx(font, hint, (float)hs, 0);
    DrawTextEx(font, hint, (Vector2){ (winW - hintSz.x) / 2.0f, (float)(winH - 50) }, (float)hs, 0, (Color){ 0x66, 0x66, 0x66, 0xFF });
}

/*
* 绘制操作说明页
* 列出全部按键及其功能，底部提示按 B 返回菜单。
*/
void DrawHowToPlay(void)
{
    int winW = GRID_SIZE * CELL_SIZE;
    int winH = GRID_SIZE * CELL_SIZE;
    Font font = GetSidebarFont();
    ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });

    int y = 60;
    int leftX = 100;
    Color titleColor = { 0x42, 0xA5, 0xF5, 0xFF };
    Color keyColor = (Color){ 0xE0, 0xE0, 0xE0, 0xFF };
    Color descColor = (Color){ 0xA0, 0xA0, 0xA0, 0xFF };

    DrawTextEx(font, "HOW TO PLAY", (Vector2){ (float)leftX, (float)y }, 36, 0, titleColor);
    y += 60;

    static const struct { const char *key; const char *desc; } rows[] = {
        { "WASD / Arrow Keys", "Move the snake (Up / Down / Left / Right)" },
        { "Space / P",         "Pause / Resume the game" },
        { "B",                 "Back to Main Menu" },
        { "ESC",               "Exit the game" },
        { "R (after death)",   "Restart the game" },
    };
    int count = sizeof(rows) / sizeof(rows[0]);

    for (int i = 0; i < count; i++)
    {
        DrawTextEx(font, rows[i].key,  (Vector2){ (float)leftX, (float)y }, 22, 0, keyColor);
        DrawTextEx(font, rows[i].desc, (Vector2){ (float)(leftX + 260), (float)y }, 20, 0, descColor);
        y += 38;
    }

    const char *hint = "Press B to return to Menu";
    int hs = 16;
    Vector2 hintSz = MeasureTextEx(font, hint, (float)hs, 0);
    DrawTextEx(font, hint, (Vector2){ (winW - hintSz.x) / 2.0f, (float)(winH - 50) }, (float)hs, 0, (Color){ 0x66, 0x66, 0x66, 0xFF });
}
