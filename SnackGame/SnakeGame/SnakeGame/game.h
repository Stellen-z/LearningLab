#ifndef GAME_H
#define GAME_H

#include "DCList.h"
#include "raylib.h"
#include <stdbool.h>

#define GRID_SIZE 20
#define CELL_SIZE 30
#define INITIAL_LENGTH 3

typedef enum {
    DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} Direction;

typedef enum {
    MENU, PLAYING, PAUSED, GAME_OVER, HOW_TO_PLAY
} GameState;

typedef struct {
    DCListNode *body;
    Direction dir;
    Direction next_dir;
    int score;
    bool alive;
} Snake;

typedef struct {
    Position pos;
    bool active;
} Food;

typedef struct {
    Snake snake;
    Food food;
    GameState state;
    int selected_menu;
    float move_timer;
    float move_interval;
    Sound eat_sound;
    Sound die_sound;
    Sound menu_sound;
    int high_score;
    bool high_score_updated;
} SnakeGame;

Snake snake_create(int head_x, int head_y, int length, Direction dir);
bool  snake_move(Snake *snake, Position food_pos, bool *ate);
bool  wall_collided(Position pos, int grid_size);
bool  self_collided(Snake *snake);
void  generate_food(Position *food_pos, Snake *snake, int grid_size);
float get_move_interval(int score);
int   load_high_score(const char *filename);
void  save_high_score(const char *filename, int score);

#endif
