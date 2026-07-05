#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
* 创建一条蛇
* 蛇在网格中的初始位置为 (head_x, head_y)，由 length 节组成。
* 蛇身朝指定方向延伸——新节点从蛇头朝反方向排列，使整条蛇沿 dir 方向展开。
* 例如 dir=DIR_RIGHT 时，蛇头在 (10,10)，后面两节分别在 (9,10) 和 (8,10)。
*
* @param head_x   蛇头初始 x 坐标
* @param head_y   蛇头初始 y 坐标
* @param length   蛇的初始长度（≥ 1）
* @param dir      蛇的初始朝向
* @return         初始化完成的 Snake 结构体
*/
Snake snake_create(int head_x, int head_y, int length, Direction dir)
{
    Snake snake;
    snake.body = DCListInit();        /* 创建空链表作为蛇身 */
    snake.dir = dir;
    snake.next_dir = dir;             /* next_dir 初始与 dir 一致 */
    snake.score = 0;
    snake.alive = true;

    /* 根据朝向确定每个新节的偏移量（逆行排列蛇身） */
    int dx = 0, dy = 0;
    switch (dir)
    {
    case DIR_UP:    dy = -1; break;   /* 向上延伸 → 节点 y 增大（反方向） */
    case DIR_DOWN:  dy = 1;  break;
    case DIR_LEFT:  dx = -1; break;
    case DIR_RIGHT: dx = 1;  break;   /* 向右延伸 → 节点 x 减小（反方向） */
    }

    for (int i = 0; i < length; i++)
    {
        Position pos = { head_x - i * dx, head_y - i * dy };
        DCListPushBack(snake.body, pos); /* 从头到尾依次尾插 */
    }

    return snake;
}

/*
* 驱动蛇移动一步
* 执行流程：
*   1. 根据 next_dir 计算新蛇头坐标
*   2. 将新蛇头插入链表头部
*   3. 碰撞检测（撞墙 → 死亡）
*   4. 判断是否吃到食物（是 → score+1，尾部保留；否 → 尾删）
*   5. 自碰检测（撞自己 → 死亡）
*
* @param snake       蛇的指针
* @param food_pos    当前食物的位置
* @param ate         [出参] 是否吃到食物（true=吃到，false=没吃到）
* @return            本步移动后蛇是否存活
*/
bool snake_move(Snake *snake, Position food_pos, bool *ate)
{
    if (snake == NULL || snake->body == NULL || !snake->alive)
        return false;

    /* 置入方向为当前方向（next_dir 在外部由键盘设定） */
    snake->dir = snake->next_dir;

    /* 从链表第一个有效结点（蛇头）获取当前坐标 */
    DCListNode *headNode = snake->body->next;
    int head_x = headNode->data.x; 
    int head_y = headNode->data.y;

    /* 根据方向计算新蛇头坐标 */
    switch (snake->dir)
    {
    case DIR_UP:    head_y--; break;
    case DIR_DOWN:  head_y++; break;
    case DIR_LEFT:  head_x--; break;
    case DIR_RIGHT: head_x++; break;
    }

    Position new_head_pos = { head_x, head_y };
    DCListPushFront(snake->body, new_head_pos);  /* O(1) 头插新坐标 */

    /* 撞墙检测 */
    if (wall_collided(new_head_pos, GRID_SIZE))
    {
        snake->alive = false;
        return false;
    }

    /* 吃食物 / 正常移动 */
    if (new_head_pos.x == food_pos.x && new_head_pos.y == food_pos.y)
    {
        if (ate != NULL) *ate = true;
        snake->score++;              /* 吃到 → 分数+1，不删尾 → 变长 */
    }
    else
    {
        if (ate != NULL) *ate = false;
        DCListPopBack(snake->body);  /* O(1) 没吃到 → 删尾，保持长度不变 */
    }

    /* 自碰检测（新蛇头是否与蛇身重叠） */
    if (self_collided(snake))
    {
        snake->alive = false;
        return false;
    }

    return true;
}

/*
* 检测坐标是否超出网格边界（墙壁碰撞）
* 网格坐标有效范围是 [0, grid_size-1]
* @param pos         要检测的坐标
* @param grid_size   网格大小（每维的格数）
* @return            true=撞墙，false=在网格内
*/
bool wall_collided(Position pos, int grid_size)
{
    return pos.x < 0 || pos.x >= grid_size
        || pos.y < 0 || pos.y >= grid_size;
}

/*
* 检测蛇头是否与蛇身发生自碰
* 从蛇头之后的第2个结点开始，逐一与蛇头坐标比对
* （跳过蛇头自身，即 headNode 本身）
* @param snake   蛇的指针
* @return        true=撞自身，false=未撞
*/
bool self_collided(Snake *snake)
{
    if (snake == NULL || snake->body == NULL || DCListIsEmpty(snake->body))
        return false;

    DCListNode *headNode = snake->body->next;     /* 蛇头结点 */
    DCListNode *check = headNode->next;           /* 从第2节开始检查 */

    while (check != snake->body)
    {
        if (headNode->data.x == check->data.x
         && headNode->data.y == check->data.y)
            return true;
        check = check->next;
    }

    return false;
}

/*
* 在空白位置随机生成食物
* 循环生成随机坐标，直到坐标不在蛇身上为止（避免食物和蛇重叠）。
* 设有最大尝试次数 grid_size^2，防止极端情况（蛇几乎占满网格）时死循环。
* 首次调用时会用 time(NULL) 初始化随机种子。
*
* @param food_pos   [出参] 生成的食物位置
* @param snake      蛇的指针（用于判断碰撞）
* @param grid_size  网格大小
*/
void generate_food(Position *food_pos, Snake *snake, int grid_size)
{
    if (food_pos == NULL || snake == NULL || snake->body == NULL)
        return;

    /* 惰性初始化随机种子（只执行一次） */
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

/*
* 根据当前分数计算移动间隔（速度）
* 基础间隔 0.2 秒，每吃 3 个食物减 0.02 秒，最低 0.06 秒。
* 例如：分数 0~2 → 200ms，分数 3~5 → 180ms，分数 ≥ 21 → 60ms。
*
* @param score   当前分数
* @return        当前速度对应的移动间隔（秒）
*/
float get_move_interval(int score)
{
    float interval = 0.2f - (score / 3) * 0.02f;
    if (interval < 0.06f)
        interval = 0.06f;      /* 速度有下限，不会无限加速 */
    return interval;
}

/*
* 从文件中读取历史最高分
* 文件格式为单行纯数字，若文件不存在或格式错误则返回 0。
*
* @param filename   最高分文件路径
* @return           读取到的最高分，失败返回 0
*/
int load_high_score(const char *filename)
{
    if (filename == NULL) return 0;

    FILE *fp = NULL;
    errno_t err = fopen_s(&fp, filename, "r");
    if (err != 0 || fp == NULL) return 0;  /* 文件不存在，首次运行 */

    int score = 0;
    if (fscanf_s(fp, "%d", &score) != 1)
        score = 0;                         /* 读取失败，按 0 处理 */

    fclose(fp);
    return score;
}

/*
* 将历史最高分保存到文件
* 覆盖写入，文件只存一个整数。
*
* @param filename   最高分文件路径
* @param score      要保存的得分
*/
void save_high_score(const char *filename, int score)
{
    if (filename == NULL) return;

    FILE *fp = NULL;
    errno_t err = fopen_s(&fp, filename, "w");
    if (err != 0 || fp == NULL) return;    /* 写入失败，静默忽略 */

    fprintf_s(fp, "%d", score);
    fclose(fp);
}
