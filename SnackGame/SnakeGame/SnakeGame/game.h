#ifndef GAME_H
#define GAME_H

#include "DCList.h"      /* 双向循环链表（存储 Position 坐标的通用数据结构） */
#include "raylib.h"      /* 第三方图形库（Sound 类型等） */
#include <stdbool.h>     /* C99 布尔类型 */

/* ── 游戏常量 ── */

#define GRID_SIZE  20    /* 网格大小：20×20 个格子 */
#define CELL_SIZE  30    /* 每格像素数：30×30 px */
#define INITIAL_LENGTH 3 /* 蛇初始长度（3 节） */
#define FOOD_COUNT  15   /* 网格上常驻食物数量 */

/* ── 方向枚举 ── */

typedef enum {
    DIR_UP,              /* 向上移动 */
    DIR_DOWN,            /* 向下移动 */
    DIR_LEFT,            /* 向左移动 */
    DIR_RIGHT            /* 向右移动 */
} Direction;

/* ── 游戏状态枚举（供 R4 状态机使用） ── */

typedef enum {
    MENU,                /* 主菜单 */
    PLAYING,             /* 游戏中 */
    PAUSED,              /* 暂停 */
    GAME_OVER,           /* 游戏结束 */
    HOW_TO_PLAY          /* 操作说明页 */
} GameState;

/* ── 蛇结构体 ── */
/* 蛇身用带头结点的双向循环链表存储，每个结点存一个 (x, y) 坐标 */
typedef struct {
    DCListNode *body;    /* 链表哨兵指针（head->next=蛇头，head->prev=蛇尾） */
    Direction dir;       /* 当前移动方向（每帧生效） */
    Direction next_dir;  /* 下一次移动方向（键盘缓冲，下一帧生效） */
    int score;           /* 当前得分 */
    bool alive;          /* 是否存活（死亡后停止移动） */
} Snake;

/* ── 食物结构体 ── */

typedef struct {
    Position pos;        /* 食物在网格中的坐标 */
    Color color;         /* 食物颜色（随机分配） */
    bool active;         /* 是否有效 */
} Food;

/* ── 游戏总控结构体 ── */
/* 包含蛇、食物、状态机所需的所有状态和计时数据 */
typedef struct {
    Snake snake;                     /* 蛇的数据 */
    Food foods[FOOD_COUNT];           /* 15 个食物（每位互不重叠、不在蛇身上） */
    GameState state;                 /* 当前游戏状态（R4 启用） */
    int selected_menu;               /* 菜单当前选中项（R4 启用） */

    float move_timer;                /* 移动累计计时器（每秒累加 GetFrameTime()） */
    float move_interval;             /* 移动触发间隔（秒） */

    Sound eat_sound;                 /* 吃食物音效（R4 加载） */
    Sound die_sound;                 /* 死亡音效（R4 加载） */
    Sound menu_sound;                /* 菜单切换音效（R4 加载） */

    int high_score;                  /* 历史最高分 */
    bool high_score_updated;         /* 本局是否更新了最高分 */
} SnakeGame;

/* ── 游戏逻辑函数声明 ── */

/* 创建一条蛇：起始坐标、长度、朝向 */
Snake snake_create(int head_x, int head_y, int length, Direction dir);

/* 驱动蛇移动一步，检测碰撞和多食物吃食（被吃的索引存入 *eaten_idx，未吃则为 -1） */
bool  snake_move(Snake *snake, Food *foods, int food_count, bool *ate, int *eaten_idx);

/* 检测坐标是否碰墙（越出网格） */
bool  wall_collided(Position pos, int grid_size);

/* 检测蛇头是否与蛇身重叠（自碰） */
bool  self_collided(Snake *snake);

/* 一次性生成 food_count 个食物，保证不在蛇身上且彼此不重叠 */
void  generate_all_foods(Food *foods, int food_count, Snake *snake, int grid_size);

/* 重新生成一个指定食物的位置（放入空位，避开蛇身和其他食物） */
void  regenerate_food(Food *food, Snake *snake, Food *all_foods, int food_count, int grid_size);

/* 根据分数计算移动间隔（速度） */
float get_move_interval(int score);

/* 从文件读取历史最高分 */
int   load_high_score(const char *filename);

/* 保存历史最高分到文件 */
void  save_high_score(const char *filename, int score);

#endif
