/*
* test_state.cpp — 状态机框架测试
* 测试 state.h/c 的回调注册、状态切换、enter/exit 顺序、
* update 路由、NULL 安全性和完整流转流程。
*/

#include <gtest/gtest.h>

extern "C" {
#include "state.h"
#include "game.h"
}

/*
* 辅助计数器：用于验证回调函数的调用状态。
* g_enter/update/exit_count：被调用次数
* g_enter/exit_order：调用顺序（用于验证 exit 先于 enter）
* g_call_order：全局递增序号
*/
static int g_enter_count = 0;
static int g_update_count = 0;
static int g_exit_count = 0;
static int g_call_order = 0;
static int g_enter_order = 0;
static int g_exit_order = 0;

/* 重置所有计数器到 0（每个测试用例调用一次） */
static void reset_counters(void)
{
    g_enter_count = 0;
    g_update_count = 0;
    g_exit_count = 0;
    g_call_order = 0;
    g_enter_order = 0;
    g_exit_order = 0;
}

/* 三个测试用的回调函数，每次调用递增对应计数器并记录顺序 */
static void test_enter(SnakeGame *game) { (void)game; g_enter_count++; g_enter_order = ++g_call_order; }
static void test_update(SnakeGame *game) { (void)game; g_update_count++; }
static void test_exit(SnakeGame *game)  { (void)game; g_exit_count++; g_exit_order = ++g_call_order; }

/* MENU→PLAYING 切换时，旧状态 exit 和新状态 enter 各被调用一次 */
TEST(StateTest, ChangeStateCallsExitAndEnter)
{
    reset_counters();

    StateHandler hA = { test_enter, test_update, test_exit };
    StateHandler hB = { test_enter, test_update, test_exit };
    register_state(MENU, hA);
    register_state(PLAYING, hB);

    SnakeGame game = { 0 };
    game.state = MENU;
    change_state(&game, PLAYING);

    EXPECT_EQ(g_exit_count, 1);
    EXPECT_EQ(g_enter_count, 1);
}

/* 旧 exit 在新 enter 之前调用（验证顺序正确） */
TEST(StateTest, ExitBeforeEnter)
{
    reset_counters();

    StateHandler hA = { test_enter, test_update, test_exit };
    StateHandler hB = { test_enter, test_update, test_exit };
    register_state(MENU, hA);
    register_state(PLAYING, hB);

    SnakeGame game = { 0 };
    game.state = MENU;
    change_state(&game, PLAYING);

    EXPECT_LT(g_exit_order, g_enter_order);
}

/* 切换到当前状态不触发任何回调 */
TEST(StateTest, SameStateNoop)
{
    reset_counters();

    StateHandler hA = { test_enter, test_update, test_exit };
    register_state(MENU, hA);

    SnakeGame game = { 0 };
    game.state = MENU;
    change_state(&game, MENU);

    EXPECT_EQ(g_exit_count, 0);
    EXPECT_EQ(g_enter_count, 0);
}

/* 用例 4：update 正确路由到当前状态的处理器 */
TEST(StateTest, UpdateRoutesCorrectly)
{
    reset_counters();

    StateHandler hA = { test_enter, test_update, test_exit };
    StateHandler hB = { test_enter, test_update, test_exit };
    register_state(MENU, hA);
    register_state(PLAYING, hB);

    SnakeGame game = { 0 };
    game.state = MENU;
    change_state(&game, PLAYING);

    /* B 的 update 应该被调 */
    StateHandler *handlers = get_handlers();
    handlers[PLAYING].update(&game);

    EXPECT_EQ(g_update_count, 1);
}

/* 用例 5：exit 为 NULL 的 handler 不会崩溃 */
TEST(StateTest, NullHandlerSafe)
{
    reset_counters();

    StateHandler hA = { test_enter, test_update, NULL };  /* exit = NULL */
    StateHandler hB = { test_enter, test_update, NULL };
    register_state(MENU, hA);
    register_state(PLAYING, hB);

    SnakeGame game = { 0 };
    game.state = MENU;
    change_state(&game, PLAYING);  /* 不应崩溃 */

    EXPECT_EQ(g_enter_count, 1);
}

/* 用例 6：完整状态流转：MENU→PLAYING→GAME_OVER→MENU */
TEST(StateTest, FullCycle)
{
    reset_counters();

    StateHandler hMenu     = { test_enter, test_update, test_exit };
    StateHandler hPlaying   = { test_enter, test_update, test_exit };
    StateHandler hGameOver  = { test_enter, test_update, test_exit };
    register_state(MENU, hMenu);
    register_state(PLAYING, hPlaying);
    register_state(GAME_OVER, hGameOver);

    SnakeGame game = { 0 };
    game.state = MENU;

    change_state(&game, PLAYING);
    EXPECT_EQ(g_exit_count, 1);
    EXPECT_EQ(g_enter_count, 1);

    change_state(&game, GAME_OVER);
    EXPECT_EQ(g_exit_count, 2);

    change_state(&game, MENU);
    EXPECT_EQ(g_exit_count, 3);
    EXPECT_EQ(g_enter_count, 3);
}
