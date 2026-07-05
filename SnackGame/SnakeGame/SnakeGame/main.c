#include "raylib.h"
#include "game.h"
#include "render.h"
#include "state.h"

/* 判断两个方向是否相反（用于忽略反向输入） */
static bool is_opposite(Direction a, Direction b)
{
    return (a == DIR_UP    && b == DIR_DOWN)  ||
           (a == DIR_DOWN  && b == DIR_UP)    ||
           (a == DIR_LEFT  && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

/* ────────────────────────────────────────────────────────────
 *  状态实现
 * ──────────────────────────────────────────────────────────── */

/* ── MENU ── */
static void menu_enter(SnakeGame *game)
{
    game->selected_menu = 0; /* 默认选中第 0 项 */
}

static void menu_update(SnakeGame *game)
{
    /* 上下切换选中项，播放菜单音效 */
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        game->selected_menu = (game->selected_menu - 1 + 3) % 3;
        PlaySound(game->menu_sound);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        game->selected_menu = (game->selected_menu + 1) % 3;
        PlaySound(game->menu_sound);
    }

    /* Enter 确认选择 */
    if (IsKeyPressed(KEY_ENTER))
    {
        switch (game->selected_menu)
        {
        case 0: change_state(game, PLAYING); break;
        case 1: change_state(game, HOW_TO_PLAY); break;
        case 2: /* 退出：关闭窗口，触发主循环退出 */
            CloseWindow();
            return;
        }
    }

    /* ESC 也可以直接退出 */
    if (IsKeyPressed(KEY_ESCAPE))
    {
        CloseWindow();
        return;
    }

    BeginDrawing();
    DrawMenu(game->selected_menu);
    EndDrawing();
}

static void menu_exit(SnakeGame *game)
{
    (void)game;
}

/* ── HOW_TO_PLAY ── */
static void how_to_play_update(SnakeGame *game)
{
    /* 按 B 返回菜单 */
    if (IsKeyPressed(KEY_B))
    {
        PlaySound(game->menu_sound);
        change_state(game, MENU);
    }

    BeginDrawing();
    DrawHowToPlay();
    EndDrawing();
}

/* ── PLAYING ── */
static void playing_enter(SnakeGame *game)
{
    /* 重置蛇：位置 (10,10)，长度 3，朝右 */
    if (game->snake.body != NULL)
        DCListDestroy(game->snake.body);
    game->snake = snake_create(10, 10, INITIAL_LENGTH, DIR_RIGHT);

    /* 一次性生成 15 个食物 */
    generate_all_foods(game->foods, FOOD_COUNT, &game->snake, GRID_SIZE);

    /* 重置计时器和速度 */
    game->move_timer = 0.0f;
    game->move_interval = get_move_interval(0);

    game->high_score_updated = false;
}

static void playing_update(SnakeGame *game)
{
    Snake *snake = &game->snake;

    /* 暂停切换（空格 / P） */
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P))
    {
        change_state(game, PAUSED);
        return;
    }

    /* 方向输入（WASD / 方向键），缓冲到 next_dir */
    if (snake->alive)
    {
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) snake->next_dir = DIR_UP;
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) snake->next_dir = DIR_DOWN;
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) snake->next_dir = DIR_LEFT;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) snake->next_dir = DIR_RIGHT;

        /* 反向操作忽略 */
        if (is_opposite(snake->next_dir, snake->dir))
            snake->next_dir = snake->dir;

        /* 移动计时器 */
        game->move_timer += GetFrameTime();
        if (game->move_timer >= game->move_interval)
        {
            game->move_timer -= game->move_interval;

            bool ate = false;
            int eaten_idx = -1;
            snake_move(snake, game->foods, FOOD_COUNT, &ate, &eaten_idx);

            if (ate && eaten_idx >= 0)
            {
                /* 播放吃食物音效 */
                PlaySound(game->eat_sound);
                /* 只补充被吃掉的那一个食物 */
                regenerate_food(&game->foods[eaten_idx], snake, game->foods, FOOD_COUNT, GRID_SIZE);
                game->move_interval = get_move_interval(snake->score);

                /* 更新最高分 */
                if (snake->score > game->high_score)
                {
                    game->high_score = snake->score;
                    game->high_score_updated = true;
                }
            }
        }

        /* 蛇死亡 → 进入 GAME_OVER */
        if (!snake->alive)
        {
            change_state(game, GAME_OVER);
            return;
        }
    }

    /* 渲染游戏画面 */
    BeginDrawing();
    ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });
    DrawGameGrid();
    DrawSnake(snake->body, snake->dir);
    DrawFoods(game->foods, FOOD_COUNT);
    DrawSidebar(snake->score, game->high_score, game->move_interval);
    EndDrawing();
}

/* ── PAUSED ── */
static void paused_update(SnakeGame *game)
{
    /* 先渲染游戏画面作为背景 */
    BeginDrawing();
    ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });
    DrawGameGrid();
    DrawSnake(game->snake.body, game->snake.dir);
    DrawFoods(game->foods, FOOD_COUNT);
    DrawSidebar(game->snake.score, game->high_score, game->move_interval);

    /* 半透明遮罩 */
    int gw = GRID_SIZE * CELL_SIZE;
    DrawRectangle(0, 0, gw, gw, (Color){ 0, 0, 0, 0x80 });

    /* PAUSED 文字 */
    const char *pausedText = "PAUSED";
    int tw = MeasureText(pausedText, 48);
    DrawText(pausedText, (gw - tw) / 2, gw / 2 - 30, 48, WHITE);

    const char *hint = "Press Space / P to Resume";
    int hs = 20;
    int hw = MeasureText(hint, hs);
    DrawText(hint, (gw - hw) / 2, gw / 2 + 20, hs, (Color){ 0xB0, 0xB0, 0xB0, 0xFF });
    EndDrawing();

    /* 恢复游戏 */
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P))
        change_state(game, PLAYING);

    /* ESC 直接退出 */
    if (IsKeyPressed(KEY_ESCAPE))
        change_state(game, MENU);
}

/* ── GAME_OVER ── */
static void gameover_enter(SnakeGame *game)
{
    /* 播放死亡音效 */
    PlaySound(game->die_sound);

    /* 保存最高分 */
    if (game->high_score_updated)
        save_high_score("highscore.dat", game->high_score);
}

static void gameover_update(SnakeGame *game)
{
    /* 渲染游戏画面 + 遮罩 */
    BeginDrawing();
    ClearBackground((Color){ 0x12, 0x12, 0x12, 0xFF });
    DrawGameGrid();
    DrawSnake(game->snake.body, game->snake.dir);
    DrawFoods(game->foods, FOOD_COUNT);
    DrawSidebar(game->snake.score, game->high_score, game->move_interval);

    int gw = GRID_SIZE * CELL_SIZE;
    DrawRectangle(0, 0, gw, gw, (Color){ 0, 0, 0, 0x80 });

    /* GAME OVER 文字 */
    const char *overText = "GAME OVER";
    int tw = MeasureText(overText, 48);
    DrawText(overText, (gw - tw) / 2, gw / 2 - 50, 48, WHITE);

    /* 提示按任意键 */
    const char *hint = "Press any key to return to Menu";
    int hs = 20;
    int hw = MeasureText(hint, hs);
    DrawText(hint, (gw - hw) / 2, gw / 2 + 10, hs, (Color){ 0xB0, 0xB0, 0xB0, 0xFF });
    EndDrawing();

    /* 任意键回菜单 */
    int key = GetKeyPressed();
    if (key != 0 && key != KEY_ESCAPE)
    {
        PlaySound(game->menu_sound);
        change_state(game, MENU);
    }

    /* ESC 返回菜单 */
    if (IsKeyPressed(KEY_ESCAPE))
    {
        PlaySound(game->menu_sound);
        change_state(game, MENU);
    }
}

static void gameover_exit(SnakeGame *game)
{
    save_high_score("highscore.dat", game->high_score);
}

/* ────────────────────────────────────────────────────────────
 *  主函数
 * ──────────────────────────────────────────────────────────── */

int main(void)
{
    /* 窗口 840×600：左侧 600px 网格 + 右侧 240px 侧边栏 */
    InitWindow(840, 600, "Snake Game");
    SetTargetFPS(60);
    InitAudioDevice();

    /* 初始化游戏状态 */
    SnakeGame game = { 0 };
    game.state = MENU;
    game.selected_menu = 0;
    game.high_score = load_high_score("highscore.dat");
    game.high_score_updated = false;

    /* 加载音效 */
    game.eat_sound  = LoadSound("resource/eat.wav");
    game.die_sound  = LoadSound("resource/die.wav");
    game.menu_sound = LoadSound("resource/menu.wav");

    /* ── 注册 5 个状态处理器 ── */
    StateHandler menuHandler     = { menu_enter, menu_update, menu_exit };
    StateHandler howToPlayHdl    = { NULL, how_to_play_update, NULL };
    StateHandler playingHdl      = { playing_enter, playing_update, NULL };
    StateHandler pausedHdl       = { NULL, paused_update, NULL };
    StateHandler gameOverHdl     = { gameover_enter, gameover_update, gameover_exit };

    register_state(MENU,        menuHandler);
    register_state(HOW_TO_PLAY, howToPlayHdl);
    register_state(PLAYING,     playingHdl);
    register_state(PAUSED,      pausedHdl);
    register_state(GAME_OVER,   gameOverHdl);

    /* 手动调用 MENU enter（初始状态不触发 change_state 的 enter） */
    StateHandler *handlers = get_handlers();
    if (handlers[MENU].enter)
        handlers[MENU].enter(&game);

    /* 主循环：一行驱动整个游戏 */
    while (!WindowShouldClose())
    {
        handlers[game.state].update(&game);
    }

    /* 如果是菜单退出（case 2），game.state 被手动设为 GAME_OVER，直接退出循环 */

    /* 清理资源 */
    if (game.snake.body != NULL)
        DCListDestroy(game.snake.body);
    UnloadSound(game.eat_sound);
    UnloadSound(game.die_sound);
    UnloadSound(game.menu_sound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
