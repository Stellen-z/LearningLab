#ifndef STATE_H
#define STATE_H

#include "game.h"

/* ── 状态回调类型 ── */
/* 每进入一个状态时调用一次（初始化/重置数据） */
typedef void (*StateEnter)(SnakeGame *game);
/* 每帧调用一次（处理输入 + 更新逻辑 + 渲染） */
typedef void (*StateUpdate)(SnakeGame *game);
/* 离开一个状态时调用一次（清理 / 保存数据） */
typedef void (*StateExit)(SnakeGame *game);

/* ── 状态处理器 ── */
/* 每个游戏状态对应一组回调函数 */
typedef struct {
    StateEnter enter;
    StateUpdate update;
    StateExit  exit;
} StateHandler;

/* ── API ── */
/* 注册一个状态处理器（全局数组，最多 5 种状态） */
void register_state(GameState state, StateHandler handler);
/* 切换游戏状态（自动调用旧 exit + 新 enter） */
void change_state(SnakeGame *game, GameState new_state);

/* 获取已注册的状态处理器指针（供 main 循环使用） */
StateHandler* get_handlers(void);

#endif
