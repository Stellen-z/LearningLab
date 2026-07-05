#include "state.h"

/* 全局状态处理器数组（最多 5 种游戏状态） */
static StateHandler s_handlers[5] = { 0 };

/* 注册一个游戏状态的回调函数 */
void register_state(GameState state, StateHandler handler)
{
    s_handlers[state] = handler;
}

/* 切换当前游戏状态
 * 流程：同状态不切换 → 调用旧状态 exit → 置为新状态 → 调用新状态 enter */
void change_state(SnakeGame *game, GameState new_state)
{
    if (new_state == game->state)
        return;  /* 同状态无需切换 */

    StateHandler *old = &s_handlers[game->state];
    StateHandler *new = &s_handlers[new_state];

    if (old->exit)
        old->exit(game);       /* 旧状态清理 */

    game->state = new_state;

    if (new->enter)
        new->enter(game);      /* 新状态初始化 */
}

/* 获取处理器数组指针（供外部遍历） */
StateHandler* get_handlers(void)
{
    return s_handlers;
}
