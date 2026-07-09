/*
* test_game.cpp — 游戏逻辑测试
* 测试蛇的创建、移动、碰撞检测（墙壁/自身）、食物生成、
* 历史最高分读写等全部游戏核心逻辑函数。
*/

#include <gtest/gtest.h>

extern "C" {
#include "game.h"
#include "DCList.h"
}

/* 辅助函数：构造一个 active = true 的单食物，用于蛇移动测试 */
static Food make_single_food(int x, int y)
{
    Food f;
    f.pos.x = x;
    f.pos.y = y;
    f.active = true;
    return f;
}

/* 创建朝右蛇，长度 3，验证蛇头蛇尾坐标和初始状态 */
TEST(GameTest, SnakeCreate)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    EXPECT_EQ(snake.dir, DIR_RIGHT);
    EXPECT_EQ(snake.next_dir, DIR_RIGHT);
    EXPECT_EQ(snake.score, 0);
    EXPECT_TRUE(snake.alive);

    DCListNode *head = snake.body->next;
    EXPECT_EQ(head->data.x, 10);
    EXPECT_EQ(head->data.y, 10);

    DCListNode *tail = snake.body->prev;
    EXPECT_EQ(tail->data.x, 8);
    EXPECT_EQ(tail->data.y, 10);

    DCListDestroy(snake.body);
}

/* 创建朝上蛇，验证蛇尾在蛇头下方（身体按反方向排列） */
TEST(GameTest, SnakeCreateUp)
{
    Snake snake = snake_create(5, 5, 3, DIR_UP);

    DCListNode *head = snake.body->next;
    EXPECT_EQ(head->data.x, 5);
    EXPECT_EQ(head->data.y, 5);

    DCListNode *tail = snake.body->prev;
    EXPECT_EQ(tail->data.x, 5);
    EXPECT_EQ(tail->data.y, 7);

    DCListDestroy(snake.body);
}

/* 朝右移动 1 步，验证新蛇头坐标和没有吃到食物 */
TEST(GameTest, SnakeMoveRight)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    snake.next_dir = DIR_RIGHT;

    Food foods[1] = { make_single_food(-1, -1) };
    bool ate = false;
    int eaten_idx = -1;
    bool alive = snake_move(&snake, foods, 1, &ate, &eaten_idx);

    EXPECT_TRUE(alive);
    EXPECT_FALSE(ate);
    EXPECT_EQ(eaten_idx, -1);

    DCListNode *head = snake.body->next;
    EXPECT_EQ(head->data.x, 11);
    EXPECT_EQ(head->data.y, 10);

    DCListDestroy(snake.body);
}

/* 移动未碰到食物，ate 返回 false，蛇长度不变 */
TEST(GameTest, SnakeMoveNoEatLengthUnchanged)
{
    Snake snake = snake_create(5, 5, 3, DIR_DOWN);
    snake.next_dir = DIR_DOWN;

    Food foods[1] = { make_single_food(99, 99) };
    bool ate = true;
    int eaten_idx = -1;
    snake_move(&snake, foods, 1, &ate, &eaten_idx);

    EXPECT_FALSE(ate);
    DCListDestroy(snake.body);
}

/* 上下方向互为相反 */
TEST(GameTest, OppositeUpDown)
{
    EXPECT_TRUE(is_opposite(DIR_UP, DIR_DOWN));
    EXPECT_TRUE(is_opposite(DIR_DOWN, DIR_UP));
}

/* 左右方向互为相反 */
TEST(GameTest, OppositeLeftRight)
{
    EXPECT_TRUE(is_opposite(DIR_LEFT, DIR_RIGHT));
    EXPECT_TRUE(is_opposite(DIR_RIGHT, DIR_LEFT));
}

/* 非相反方向的组合全部返回 false */
TEST(GameTest, OppositeFalse)
{
    EXPECT_FALSE(is_opposite(DIR_UP, DIR_LEFT));
    EXPECT_FALSE(is_opposite(DIR_UP, DIR_RIGHT));
    EXPECT_FALSE(is_opposite(DIR_DOWN, DIR_LEFT));
    EXPECT_FALSE(is_opposite(DIR_DOWN, DIR_RIGHT));
    EXPECT_FALSE(is_opposite(DIR_LEFT, DIR_UP));
    EXPECT_FALSE(is_opposite(DIR_LEFT, DIR_DOWN));
    EXPECT_FALSE(is_opposite(DIR_RIGHT, DIR_UP));
    EXPECT_FALSE(is_opposite(DIR_RIGHT, DIR_DOWN));
}

/* 相同方向不算相反 */
TEST(GameTest, SameDirNotOpposite)
{
    EXPECT_FALSE(is_opposite(DIR_UP, DIR_UP));
    EXPECT_FALSE(is_opposite(DIR_DOWN, DIR_DOWN));
    EXPECT_FALSE(is_opposite(DIR_LEFT, DIR_LEFT));
    EXPECT_FALSE(is_opposite(DIR_RIGHT, DIR_RIGHT));
}

/* 蛇头移动到食物位置，验证吃食效果：分数+1、尾部保留 */
TEST(GameTest, SnakeMoveEatFood)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    snake.next_dir = DIR_RIGHT;

    Food foods[1] = { make_single_food(11, 10) };
    bool ate = false;
    int eaten_idx = -1;
    snake_move(&snake, foods, 1, &ate, &eaten_idx);

    EXPECT_TRUE(ate);
    EXPECT_EQ(eaten_idx, 0);
    EXPECT_EQ(snake.score, 1);

    DCListNode *tail = snake.body->prev;
    EXPECT_EQ(tail->data.x, 8);
    EXPECT_EQ(tail->data.y, 10);

    DCListDestroy(snake.body);
}

/* 左边撞墙 */
TEST(GameTest, WallCollisionLeft)
{
    Position pos = { -1, 5 };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

/* 右边撞墙 */
TEST(GameTest, WallCollisionRight)
{
    Position pos = { GRID_SIZE, 5 };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

/* 上边撞墙 */
TEST(GameTest, WallCollisionTop)
{
    Position pos = { 5, -1 };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

/* 下边撞墙 */
TEST(GameTest, WallCollisionBottom)
{
    Position pos = { 5, GRID_SIZE };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

/* 右下角撞墙 */
TEST(GameTest, WallCollisionCorner)
{
    Position pos = { GRID_SIZE, GRID_SIZE };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

/* 网格内坐标不撞墙 */
TEST(GameTest, WallCollisionFalse)
{
    Position pos = { 5, 5 };
    EXPECT_FALSE(wall_collided(pos, GRID_SIZE));
}

/* 边界上的坐标 (0,0) 和 (19,19) 在合法范围内 */
TEST(GameTest, WallCollisionEdgeOk)
{
    Position pos = { 0, 0 };
    EXPECT_FALSE(wall_collided(pos, GRID_SIZE));

    pos.x = GRID_SIZE - 1;
    pos.y = GRID_SIZE - 1;
    EXPECT_FALSE(wall_collided(pos, GRID_SIZE));
}

/* 直线蛇不会自碰 */
TEST(GameTest, SelfCollisionWhenNoOverlap)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    EXPECT_FALSE(self_collided(&snake));
    DCListDestroy(snake.body);
}

/* 构造环形蛇身使蛇头与尾部重叠，验证自碰检测为 true */
TEST(GameTest, SelfCollisionCircularBody)
{
    Snake snake = snake_create(10, 10, 1, DIR_RIGHT);
    Position p1 = { 10, 11 };
    Position p2 = { 11, 11 };
    Position p3 = { 11, 10 };
    Position p4 = { 10, 10 };
    DCListPushBack(snake.body, p1);
    DCListPushBack(snake.body, p2);
    DCListPushBack(snake.body, p3);
    DCListPushBack(snake.body, p4);

    DCListNode *head = snake.body->next;
    EXPECT_EQ(head->data.x, 10);
    EXPECT_EQ(head->data.y, 10);

    EXPECT_TRUE(self_collided(&snake));
    DCListDestroy(snake.body);
}

/* 一次性生成的 15 个食物均在网格内、不在蛇身上、彼此不重叠 */
TEST(GameTest, FoodAllNotOnSnake)
{
    Snake snake = snake_create(5, 5, 3, DIR_RIGHT);

    Food foods[FOOD_COUNT] = { 0 };
    generate_all_foods(foods, FOOD_COUNT, &snake, GRID_SIZE);

    for (int i = 0; i < FOOD_COUNT; i++)
    {
        EXPECT_TRUE(foods[i].active);
        EXPECT_GE(foods[i].pos.x, 0);
        EXPECT_LT(foods[i].pos.x, GRID_SIZE);
        EXPECT_GE(foods[i].pos.y, 0);
        EXPECT_LT(foods[i].pos.y, GRID_SIZE);
        EXPECT_FALSE(DCListContains(snake.body, foods[i].pos.x, foods[i].pos.y));
    }

    for (int i = 0; i < FOOD_COUNT; i++)
    {
        for (int j = i + 1; j < FOOD_COUNT; j++)
        {
            EXPECT_FALSE(foods[i].pos.x == foods[j].pos.x
                      && foods[i].pos.y == foods[j].pos.y);
        }
    }

    DCListDestroy(snake.body);
}

/* 重新生成一个食物后，新位置有效且不和其他食物及蛇重叠 */
TEST(GameTest, RegenerateFoodNotOnSnake)
{
    Snake snake = snake_create(5, 5, 3, DIR_RIGHT);

    Food foods[FOOD_COUNT] = { 0 };
    generate_all_foods(foods, FOOD_COUNT, &snake, GRID_SIZE);

    regenerate_food(&foods[0], &snake, foods, FOOD_COUNT, GRID_SIZE);

    EXPECT_TRUE(foods[0].active);
    EXPECT_FALSE(DCListContains(snake.body, foods[0].pos.x, foods[0].pos.y));

    for (int j = 1; j < FOOD_COUNT; j++)
    {
        EXPECT_FALSE(foods[0].pos.x == foods[j].pos.x
                  && foods[0].pos.y == foods[j].pos.y);
    }

    DCListDestroy(snake.body);
}

/* 验证速度随分数变化的公式：每 3 分减 0.02s，最低 0.06s */
TEST(GameTest, GetMoveInterval)
{
    EXPECT_FLOAT_EQ(get_move_interval(0), 0.2f);
    EXPECT_FLOAT_EQ(get_move_interval(1), 0.2f);
    EXPECT_FLOAT_EQ(get_move_interval(2), 0.2f);
    EXPECT_FLOAT_EQ(get_move_interval(3), 0.18f);
    EXPECT_FLOAT_EQ(get_move_interval(6), 0.16f);
    EXPECT_FLOAT_EQ(get_move_interval(21), 0.06f);
    EXPECT_FLOAT_EQ(get_move_interval(100), 0.06f);
}

/* 蛇在边界朝墙移动一步即死亡 */
TEST(GameTest, SnakeMoveIntoWall)
{
    Snake snake = snake_create(0, 0, 3, DIR_LEFT);
    snake.next_dir = DIR_LEFT;

    Food foods[1] = { make_single_food(-1, -1) };
    bool ate = false;
    int eaten_idx = -1;
    bool alive = snake_move(&snake, foods, 1, &ate, &eaten_idx);

    EXPECT_FALSE(alive);
    EXPECT_FALSE(snake.alive);

    DCListDestroy(snake.body);
}

/* 连续吃 2 个食物，分数累加 0→1→2 */
TEST(GameTest, ScoreIncrementOnEat)
{
    Snake snake = snake_create(5, 5, 3, DIR_RIGHT);
    snake.next_dir = DIR_RIGHT;
    EXPECT_EQ(snake.score, 0);

    Food foods1[1] = { make_single_food(6, 5) };
    snake_move(&snake, foods1, 1, NULL, NULL);
    EXPECT_EQ(snake.score, 1);

    snake.next_dir = DIR_RIGHT;
    Food foods2[1] = { make_single_food(7, 5) };
    snake_move(&snake, foods2, 1, NULL, NULL);
    EXPECT_EQ(snake.score, 2);

    DCListDestroy(snake.body);
}

/* 读取不存在的最高分文件应返回 0 */
TEST(GameTest, LoadHighScoreNonExistent)
{
    int score = load_high_score("__nonexistent_file__.dat");
    EXPECT_EQ(score, 0);
}

/* 保存最高分后能正确读取回来 */
TEST(GameTest, SaveAndLoadHighScore)
{
    const char *filename = "test_highscore.dat";
    save_high_score(filename, 42);

    int score = load_high_score(filename);
    EXPECT_EQ(score, 42);

    save_high_score(filename, 99);
    score = load_high_score(filename);
    EXPECT_EQ(score, 99);

    remove(filename);
}

/* 覆盖保存最高分，验证旧值被替换 */
TEST(GameTest, SaveHighScoreOverwrite)
{
    const char *filename = "test_highscore2.dat";
    save_high_score(filename, 10);
    save_high_score(filename, 25);

    int score = load_high_score(filename);
    EXPECT_EQ(score, 25);

    remove(filename);
}
