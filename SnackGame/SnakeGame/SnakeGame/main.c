#include "raylib.h"
#include "game.h"
#include "render.h"

/* 判断两个方向是否相反 */
static bool is_opposite(Direction a, Direction b)
{
    return (a == DIR_UP    && b == DIR_DOWN)  ||
           (a == DIR_DOWN  && b == DIR_UP)    ||
           (a == DIR_LEFT  && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

int main(void)
{
    /* 初始化窗口 — 左侧600px游戏网格 + 右侧240px侧边栏 */
    InitWindow(840, 600, "Snake Game");
    SetTargetFPS(60);

    /* 创建初始蛇：位置(10,10)，长度3，朝右 */
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);

    /* 生成第一个食物 */
    Food food = { { 0, 0 }, true };
    generate_food(&food.pos, &snake, GRID_SIZE);

    /* 读取历史最高分 */
    int high_score = load_high_score("highscore.dat");

    /* 移动计时器与速度 */
    float move_timer = 0.0f;
    float move_interval = get_move_interval(snake.score);

    /* 游戏状态标记 */
    bool paused = false;
    bool ate = false;

    while (!WindowShouldClose())
    {
        /* ── 键盘输入 ── */
        if (!paused && snake.alive)
        {
            if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) snake.next_dir = DIR_UP;
            if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) snake.next_dir = DIR_DOWN;
            if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) snake.next_dir = DIR_LEFT;
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) snake.next_dir = DIR_RIGHT;
        }

        /* 反向操作忽略 */
        if (is_opposite(snake.next_dir, snake.dir))
            snake.next_dir = snake.dir;

        /* 暂停切换 */
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P))
            paused = !paused;

        /* 退出（ESC 键） */
        if (IsKeyPressed(KEY_ESCAPE))
            break;

        /* ── 移动逻辑 ── */
        if (!paused && snake.alive)
        {
            move_timer += GetFrameTime();
            if (move_timer >= move_interval)
            {
                move_timer -= move_interval;

                /* 移动蛇，检测是否吃到食物 */
                snake_move(&snake, food.pos, &ate);

                if (ate)
                {
                    /* 吃到食物：生成新食物，更新速度 */
                    generate_food(&food.pos, &snake, GRID_SIZE);
                    move_interval = get_move_interval(snake.score);

                    /* 更新最高分 */
                    if (snake.score > high_score)
                        high_score = snake.score;
                }
            }
        }

        /* 死亡时保存最高分 */
        if (!snake.alive)
            save_high_score("highscore.dat", high_score);

        /* 重玩：按 R 键重置游戏 */
        if (!snake.alive && IsKeyPressed(KEY_R))
        {
            DCListDestroy(snake.body);
            snake = snake_create(10, 10, 3, DIR_RIGHT);
            generate_food(&food.pos, &snake, GRID_SIZE);
            move_timer = 0.0f;
            move_interval = get_move_interval(snake.score);
        }

        /* ── 渲染 ── */
        BeginDrawing();
        ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });

        DrawGameGrid();
        DrawSnake(snake.body, snake.dir);
        DrawFood(food.pos);
        DrawSidebar(snake.score, high_score, move_interval);

        /* 暂停遮罩 */
        if (paused && snake.alive)
        {
            DrawRectangle(0, 0, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE,
                          (Color){ 0, 0, 0, 0x80 });
            const char *pausedText = "PAUSED";
            int tw = MeasureText(pausedText, 48);
            DrawText(pausedText, (GRID_SIZE * CELL_SIZE - tw) / 2,
                     GRID_SIZE * CELL_SIZE / 2 - 24, 48, WHITE);
        }

        /* 死亡遮罩 */
        if (!snake.alive)
        {
            DrawRectangle(0, 0, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE,
                          (Color){ 0, 0, 0, 0x80 });
            const char *overText = "GAME OVER";
            int tw = MeasureText(overText, 48);
            DrawText(overText, (GRID_SIZE * CELL_SIZE - tw) / 2,
                     GRID_SIZE * CELL_SIZE / 2 - 50, 48, WHITE);

            const char *restartText = "Press R to Restart";
            tw = MeasureText(restartText, 20);
            DrawText(restartText, (GRID_SIZE * CELL_SIZE - tw) / 2,
                     GRID_SIZE * CELL_SIZE / 2 + 10, 20, (Color){ 0xB0, 0xB0, 0xB0, 0xFF });
        }

        EndDrawing();
    }

    /* 退出时保存最高分并清理资源 */
    if (snake.alive)
        save_high_score("highscore.dat", high_score);

    DCListDestroy(snake.body);
    CloseWindow();
    return 0;
}
