#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

Snake snake_create(int head_x, int head_y, int length, Direction dir)
{
    Snake snake;
    snake.body = DCListInit();
    snake.dir = dir;
    snake.next_dir = dir;
    snake.score = 0;
    snake.alive = true;

    int dx = 0, dy = 0;
    switch (dir)
    {
    case DIR_UP:    dy = -1; break;
    case DIR_DOWN:  dy = 1;  break;
    case DIR_LEFT:  dx = -1; break;
    case DIR_RIGHT: dx = 1;  break;
    }

    for (int i = 0; i < length; i++)
    {
        Position pos = { head_x - i * dx, head_y - i * dy };
        DCListPushBack(snake.body, pos);
    }

    return snake;
}

bool snake_move(Snake *snake, Position food_pos, bool *ate)
{
    if (snake == NULL || snake->body == NULL || !snake->alive)
        return false;

    snake->dir = snake->next_dir;

    DCListNode *headNode = snake->body->next;
    int head_x = headNode->data.x;
    int head_y = headNode->data.y;

    switch (snake->dir)
    {
    case DIR_UP:    head_y--; break;
    case DIR_DOWN:  head_y++; break;
    case DIR_LEFT:  head_x--; break;
    case DIR_RIGHT: head_x++; break;
    }

    Position new_head_pos = { head_x, head_y };
    DCListPushFront(snake->body, new_head_pos);

    if (wall_collided(new_head_pos, GRID_SIZE))
    {
        snake->alive = false;
        return false;
    }

    if (new_head_pos.x == food_pos.x && new_head_pos.y == food_pos.y)
    {
        if (ate != NULL) *ate = true;
        snake->score++;
    }
    else
    {
        if (ate != NULL) *ate = false;
        DCListPopBack(snake->body);
    }

    if (self_collided(snake))
    {
        snake->alive = false;
        return false;
    }

    return true;
}

bool wall_collided(Position pos, int grid_size)
{
    return pos.x < 0 || pos.x >= grid_size
        || pos.y < 0 || pos.y >= grid_size;
}

bool self_collided(Snake *snake)
{
    if (snake == NULL || snake->body == NULL || DCListIsEmpty(snake->body))
        return false;

    DCListNode *headNode = snake->body->next;
    DCListNode *check = headNode->next;

    while (check != snake->body)
    {
        if (headNode->data.x == check->data.x
         && headNode->data.y == check->data.y)
            return true;
        check = check->next;
    }

    return false;
}

void generate_food(Position *food_pos, Snake *snake, int grid_size)
{
    if (food_pos == NULL || snake == NULL || snake->body == NULL)
        return;

    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    int max_attempts = grid_size * grid_size;
    int attempts = 0;

    do
    {
        food_pos->x = rand() % grid_size;
        food_pos->y = rand() % grid_size;
        attempts++;
    }
    while (DCListContains(snake->body, food_pos->x, food_pos->y)
        && attempts < max_attempts);
}

float get_move_interval(int score)
{
    float interval = 0.2f - (score / 3) * 0.02f;
    if (interval < 0.06f)
        interval = 0.06f;
    return interval;
}

int load_high_score(const char *filename)
{
    if (filename == NULL) return 0;

    FILE *fp = NULL;
    errno_t err = fopen_s(&fp, filename, "r");
    if (err != 0 || fp == NULL) return 0;

    int score = 0;
    if (fscanf_s(fp, "%d", &score) != 1)
        score = 0;

    fclose(fp);
    return score;
}

void save_high_score(const char *filename, int score)
{
    if (filename == NULL) return;

    FILE *fp = NULL;
    errno_t err = fopen_s(&fp, filename, "w");
    if (err != 0 || fp == NULL) return;

    fprintf_s(fp, "%d", score);
    fclose(fp);
}
