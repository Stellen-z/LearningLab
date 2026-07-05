#include <gtest/gtest.h>

extern "C" {
#include "game.h"
#include "DCList.h"
}

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

TEST(GameTest, SnakeMoveRight)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    snake.next_dir = DIR_RIGHT;

    Position food = { -1, -1 };
    bool ate = false;
    bool alive = snake_move(&snake, food, &ate);

    EXPECT_TRUE(alive);
    EXPECT_FALSE(ate);

    DCListNode *head = snake.body->next;
    EXPECT_EQ(head->data.x, 11);
    EXPECT_EQ(head->data.y, 10);

    DCListDestroy(snake.body);
}

TEST(GameTest, SnakeMoveNoEatLengthUnchanged)
{
    Snake snake = snake_create(5, 5, 3, DIR_DOWN);
    snake.next_dir = DIR_DOWN;

    Position food = { 99, 99 };
    bool ate = true;
    snake_move(&snake, food, &ate);

    EXPECT_FALSE(ate);
    DCListDestroy(snake.body);
}

TEST(GameTest, SnakeMoveEatFood)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    snake.next_dir = DIR_RIGHT;

    Position food = { 11, 10 };
    bool ate = false;
    snake_move(&snake, food, &ate);

    EXPECT_TRUE(ate);
    EXPECT_EQ(snake.score, 1);

    DCListNode *tail = snake.body->prev;
    EXPECT_EQ(tail->data.x, 8);
    EXPECT_EQ(tail->data.y, 10);

    DCListDestroy(snake.body);
}

TEST(GameTest, WallCollisionLeft)
{
    Position pos = { -1, 5 };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, WallCollisionRight)
{
    Position pos = { GRID_SIZE, 5 };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, WallCollisionTop)
{
    Position pos = { 5, -1 };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, WallCollisionBottom)
{
    Position pos = { 5, GRID_SIZE };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, WallCollisionCorner)
{
    Position pos = { GRID_SIZE, GRID_SIZE };
    EXPECT_TRUE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, WallCollisionFalse)
{
    Position pos = { 5, 5 };
    EXPECT_FALSE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, WallCollisionEdgeOk)
{
    Position pos = { 0, 0 };
    EXPECT_FALSE(wall_collided(pos, GRID_SIZE));

    pos.x = GRID_SIZE - 1;
    pos.y = GRID_SIZE - 1;
    EXPECT_FALSE(wall_collided(pos, GRID_SIZE));
}

TEST(GameTest, SelfCollisionWhenNoOverlap)
{
    Snake snake = snake_create(10, 10, 3, DIR_RIGHT);
    EXPECT_FALSE(self_collided(&snake));
    DCListDestroy(snake.body);
}

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

TEST(GameTest, FoodNotOnSnake)
{
    Snake snake = snake_create(5, 5, 3, DIR_RIGHT);
    for (int i = 0; i < 50; i++)
    {
        Position food = { -1, -1 };
        generate_food(&food, &snake, GRID_SIZE);

        EXPECT_GE(food.x, 0);
        EXPECT_LT(food.x, GRID_SIZE);
        EXPECT_GE(food.y, 0);
        EXPECT_LT(food.y, GRID_SIZE);

        EXPECT_FALSE(DCListContains(snake.body, food.x, food.y));
    }
    DCListDestroy(snake.body);
}

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

TEST(GameTest, SnakeMoveIntoWall)
{
    Snake snake = snake_create(0, 0, 3, DIR_LEFT);
    snake.next_dir = DIR_LEFT;

    Position food = { -1, -1 };
    bool ate = false;
    bool alive = snake_move(&snake, food, &ate);

    EXPECT_FALSE(alive);
    EXPECT_FALSE(snake.alive);

    DCListDestroy(snake.body);
}

TEST(GameTest, ScoreIncrementOnEat)
{
    Snake snake = snake_create(5, 5, 3, DIR_RIGHT);
    snake.next_dir = DIR_RIGHT;
    EXPECT_EQ(snake.score, 0);

    Position food = { 6, 5 };
    snake_move(&snake, food, NULL);
    EXPECT_EQ(snake.score, 1);

    food.x = 7; food.y = 5;
    snake.next_dir = DIR_RIGHT;
    snake_move(&snake, food, NULL);
    EXPECT_EQ(snake.score, 2);

    DCListDestroy(snake.body);
}

TEST(GameTest, LoadHighScoreNonExistent)
{
    int score = load_high_score("__nonexistent_file__.dat");
    EXPECT_EQ(score, 0);
}

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

TEST(GameTest, SaveHighScoreOverwrite)
{
    const char *filename = "test_highscore2.dat";
    save_high_score(filename, 10);
    save_high_score(filename, 25);

    int score = load_high_score(filename);
    EXPECT_EQ(score, 25);

    remove(filename);
}
