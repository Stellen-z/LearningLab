# project.md — 贪吃蛇（Snack Game）

## 一、需求规格

### 1.1 游戏规则

- 蛇在 20×20 网格中移动，不能反向，不能撞墙，不能撞自己
- 食物随机出现在空位上，蛇吃到食物长度 +1，分数 +1
- 速度逐步递增：每吃 3 个食物，移动间隔缩短一档
- 蛇头碰到墙壁或自己的身体 → 游戏结束

### 1.2 网格与窗口

| 项目 | 规格 |
|------|------|
| **窗口尺寸** | 600 × 600 像素 |
| **网格** | 20 × 20，每格 30 × 30 像素 |
| **网格线** | 可见，浅灰色细线 |
| **帧率** | 60 FPS |

### 1.3 视觉设计

| 元素 | 颜色/样式 |
|------|----------|
| **背景** | #121212 深色 |
| **蛇头** | #0D47A1 深蓝，圆角方块，带眼睛（白色小圆点）+ 嘴巴弧线 |
| **蛇身** | #42A5F5 天蓝，圆角方块 |
| **食物** | #FFD740 金黄色方块 |
| **网格线** | 半透明灰色，1px |
| **文字** | 白色 |

### 1.4 初始状态

- 蛇初始长度 3 节，蛇头朝右，网格中央偏左位置
- 初始速度：每 200ms 移动一步
- 分数：0

### 1.5 游戏状态机

采用**状态机框架**管理游戏流程，每个状态有独立的回调函数：

```
                     ┌─── 按 B ───┐
                     ▼            │
  ┌──────┐  选"开始"  ┌─────────┐ │   ┌──────────┐
  │ MENU │ ────────→ │ PLAYING │ │   │HOW_TO_PLAY│
  │      │ ←──────── │   ↑↓    │ │   │           │
  │      │  死亡后   │   │ 空格 │ │   └───────────┘
  │      │  按任意键  │   ▼     │ │        ↑
  └──────┘           │ PAUSED  │─┘        │ 选"操作说明"
      ↑              └────┬────┘          └────────────┘
      │                   │ 撞墙/撞自己
      │              ┌────▼─────┐
      └──────────────│GAME_OVER │
                     └──────────┘
```

**状态回调接口**：

```c
typedef void (*StateEnter)(SnakeGame *game);
typedef void (*StateUpdate)(SnakeGame *game);
typedef void (*StateExit)(SnakeGame *game);

typedef struct {
    StateEnter enter;
    StateUpdate update;
    StateExit  exit;
} StateHandler;
```

- `enter` — 进入此状态时调用（初始化、重置）
- `update` — 每帧调用（处理输入 + 更新逻辑 + 渲染）
- `exit` — 离开此状态时调用（清理）

状态切换通过 `change_state(game, NEW_STATE)` 触发，自动调用旧状态的 `exit` 和新状态的 `enter`。

### 1.6 操作方式

| 按键 | 功能 |
|------|------|
| ↑ / W | 向上移动 |
| ↓ / S | 向下移动 |
| ← / A | 向左移动 |
| → / D | 向右移动 |
| 空格 / P | 暂停 / 继续 |
| Enter | 菜单确认 |
| B | 操作说明页返回菜单 |

**反向操作**：与当前方向相反的操作直接忽略。

### 1.7 音效

| 音效 | 触发时机 |
|------|---------|
| **吃食物音效** | 蛇吃到食物时播放 |
| **死亡音效** | 蛇撞墙或撞自身时播放 |
| **菜单切换音效** | 在菜单中切换选项或进入菜单时播放 |

使用 raylib 内置的 `LoadSound` / `PlaySound`，音频文件使用 `.wav` 格式。

### 1.8 历史最高分

- 本地文件 `highscore.dat` 存储历史最高分
- 游戏启动时读取，GAME_OVER 时若当前分数 > 历史最高分则更新
- 文件格式：单行纯文本，只存数字

---

## 二、架构设计

### 2.1 文件结构

```
SnackGame/
├── project.md
├── SnackGame.slnx
├── SnackGame/
│   ├── SnackGame.vcxproj
│   ├── main.c                    # 入口，raylib 主循环
│   ├── state.h / state.c         # 状态机框架（回调注册+切换）
│   ├── resource/
│   │   ├── eat.wav
│   │   ├── die.wav
│   │   └── menu.wav
│   ├── list.h / list.c           # 带头结点双向循环链表（通用）
│   ├── game.h / game.c           # 游戏核心逻辑
│   ├── render.h / render.c       # 界面渲染
│   ├── test/
│   │   ├── test_list.cpp         # 链表 gtest 测试
│   │   └── test_game.cpp         # 游戏逻辑 gtest 测试
│   └── highscore.dat
```

### 2.2 模块依赖

```
main.c
  ├── state.h → 注册/切换状态回调
  │     ├── render.h → 调用 raylib API
  │     └── game.h   → 调用 list.h
  ├── game.h   → 调用 list.h
  ├── list.h   → 纯 C 数据结构，无依赖
  └── raylib   → 第三方图形库
```

### 2.3 蛇的链表映射

```
哨兵 ←→ [蛇头] ←→ [节2] ←→ [节3] ←→ ... ←→ [蛇尾] ←→ 哨兵
 head      head->next                              head->prev
```

- 哨兵结点 `head` 不存储蛇身数据，`head->next` 指向蛇头，`head->prev` 指向蛇尾
- 蛇移动 = 头插新坐标 + 条件尾删，均为 O(1) 操作

---

## 三、数据结构设计

### 3.1 坐标 & 蛇身节点数据类型

```c
// list.h
typedef struct Position {
    int x, y;
} Position;

typedef struct ListNode {
    Position pos;
    struct ListNode *prev;
    struct ListNode *next;
} ListNode;

typedef struct {
    ListNode *head;    // 哨兵结点，head->next/prev 指向首/尾
    int length;        // 实际节点数（不含哨兵）
} LinkedList;
```

### 3.2 食物数据结构

```c
// game.h
typedef struct {
    Position pos;
    bool active;
} Food;
```

### 3.3 历史最高分记录

```c
// game.h
int  load_high_score(const char *filename);
void save_high_score(const char *filename, int score);
```

### 3.4 SnakeGame 核心结构体

```c
// game.h
typedef enum {
    DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} Direction;

typedef enum {
    MENU, PLAYING, PAUSED, GAME_OVER, HOW_TO_PLAY
} GameState;

typedef struct {
    LinkedList *body;
    Direction dir;
    Direction next_dir;
    int score;
    bool alive;
} Snake;

typedef struct {
    Snake snake;
    Food food;
    GameState state;
    int selected_menu;

    // 计时与速度
    float move_timer;
    float move_interval;

    // 音效
    Sound eat_sound;
    Sound die_sound;
    Sound menu_sound;

    // 最高分
    int high_score;
    bool high_score_updated;
} SnakeGame;
```

---

## 四、Phase 分解

> **R** = 界面（手动确认），**G** = 游戏逻辑（gtest + 审核）

### Phase R1 — raylib 环境搭建 & 空窗口

- [x] 下载/配置 raylib，vcxproj 设置包含目录、库目录、链接器输入
- [x] `main.c` 实现 `InitWindow(600, 600, "Snake Game")` + 主循环 `while (!WindowShouldClose())`
- [x] `ClearBackground(#121212)` + `BeginDrawing`/`EndDrawing`
- [x] 设置目标帧率 60fps
- [x] **手动确认**：弹出 600×600 深色窗口，无崩溃

### Phase R2 — 网格 & 蛇身原型绘制

- [x] `render.h / render.c`：
  - [x] `DrawGameGrid()` — 20×20 网格线（为避免与 raylib 的 `DrawGrid(int slices, float spacing)` 冲突，改名为 `DrawGameGrid`）
  - [x] `DrawSnake(LinkedList *body, int dir)` — 蛇头深蓝圆角方块 + 眼睛嘴巴 + 蛇身天蓝圆角方块
  - [x] `DrawFood(Position pos)` — 食物金黄色方块
- [x] `main.c` 中写死一个 3 节静态蛇绘制
- [x] **手动确认**：网格 + 蛇头表情 + 蛇身可见

### Phase G1 — 蛇移动逻辑 & 测试

**链表 API（list.h/c）**：
- [ ] `LinkedList* ll_create()`
- [ ] `ListNode* ll_create_node(int x, int y)`
- [ ] `void ll_push_front(LinkedList *list, int x, int y)`
- [ ] `void ll_push_back(LinkedList *list, int x, int y)`
- [ ] `void ll_pop_back(LinkedList *list)`
- [ ] `bool ll_is_empty(LinkedList *list)`
- [ ] `bool ll_contains(LinkedList *list, int x, int y)`
- [ ] `void ll_destroy(LinkedList *list)`

**游戏逻辑（game.h/c）**：
- [ ] `Snake snake_create(int head_x, int head_y, int length, Direction dir)`
- [ ] `bool snake_move(Snake *snake, Position food_pos, bool *ate)` — 头插新坐标 + 条件尾删
- [ ] `bool wall_collided(Position pos, int grid_size)`
- [ ] `bool self_collided(Snake *snake)` — 新蛇头坐标是否与蛇身重叠（不含自身）
- [ ] `void generate_food(Position *food_pos, Snake *snake, int grid_size)`
- [ ] `float get_move_interval(int score)` — 基础 0.2s，每 3 分减 0.02s，最低 0.06s

**gtest 测试**：
- [ ] 链表创建/头插/尾删/判空/包含/销毁
- [ ] 蛇创建后长度 3，坐标正确
- [ ] 蛇移动方向正确，头结点更新
- [ ] 吃食物长度 +1，尾部不变
- [ ] 不吃食物长度不变，尾部变更
- [ ] 撞墙检测返回 true
- [ ] 撞自身检测返回 true
- [ ] 食物生成位置不在蛇身上
- [ ] **提交测试用例给我审核** → 确认后运行

### Phase R3 — 蛇动画 & 键盘控制

- [ ] `main.c` 整合 `snake_move` 驱动动画：`move_timer += GetFrameTime()`，达标时触发移动
- [ ] 键盘绑定：↑/W ↓/S ←/A →/D → 更新 `snake->next_dir`（缓冲方向）
- [ ] 反向操作忽略
- [ ] 空格/P → 切换暂停（设置 `game_state = PAUSED`，后续状态机管理）
- [ ] **手动确认**：蛇可控制移动，方向正确

### Phase G2 — 食物管理 & 历史最高分 & 测试

- [ ] `generate_food()` 完善实现
- [ ] `load_high_score()` / `save_high_score()` 文件读写
- [ ] **gtest 测试**：
  - [ ] 食物生成位置合法
  - [ ] 连吃 3 个食物位置均不重复且在网格内
  - [ ] 历史最高分读写正确
  - [ ] **提交测试用例给我审核** → 确认后运行

### Phase R4 — 状态机框架 & 音效 & 全流程整合

**状态机框架（state.h/c）**：
- [ ] `typedef void (*StateEnter)(SnakeGame*) / StateUpdate / StateExit`
- [ ] `void register_state(GameState state, StateHandler handler)`
- [ ] `void change_state(SnakeGame *game, GameState new_state)` — 调用旧状态 exit + 新状态 enter
- [ ] 全局状态处理器数组 `StateHandler handlers[5]`

**各状态实现**：

| 状态 | enter | update | exit |
|------|-------|--------|------|
| `MENU` | 重置 `selected_menu = 0` | 绘制菜单 + ↑↓切换 + Enter 确认 + 菜单音效 | — |
| `HOW_TO_PLAY` | — | 绘制说明文字 + 按 B 返回 | — |
| `PLAYING` | 重置蛇、食物、分数、计时器 | 驱动蛇移动 + 碰撞检测 + 吃食物 + 计分 | — |
| `PAUSED` | — | 绘制暂停画面 + 空格/P 继续 | — |
| `GAME_OVER` | 更新最高分 + 死亡音效 | 绘制结束画面 + 任意键回菜单 | 保存最高分 |

**渲染完善**：
- [ ] `DrawScore(int score, int high_score)` — 左上角分数 + 历史最高分
- [ ] `DrawPause()` — 半透明遮罩 + "PAUSED"
- [ ] `DrawGameOver(int score, int high_score)` — "GAME OVER" + 分数 + 任意键提示
- [ ] `DrawMenu(int selected)` — 标题 "Snake Game" + 三个选项（高亮选中）
- [ ] `DrawHowToPlay()` — 操作说明文字

**音效整合**：
- [ ] `InitAudioDevice()` / `LoadSound`
- [ ] 吃食物 → `PlaySound(eat_sound)`
- [ ] 死亡 → `PlaySound(die_sound)`
- [ ] 菜单切换 → `PlaySound(menu_sound)`
- [ ] 退出时 `UnloadSound` + `CloseAudioDevice`

**主循环结构（main.c）**：
```c
while (!WindowShouldClose()) {
    handlers[game.state].update(&game);
}
```

- [ ] **手动确认**：完整走通菜单→游戏→死亡→回菜单全流程，音效正常

### Phase R5 — 最终润色

- [ ] 蛇头表情跟随方向旋转（眼睛/嘴巴方向随 DIR 变化）
- [ ] 食物不生成在蛇身上验证（边缘 case）
- [ ] `high_score_updated` 标志位处理
- [ ] 代码全面添加详细中文注释
- [ ] 代码风格统一
- [ ] 回归测试：gtest 全部通过
- [ ] 最终手动游玩测试

---

## 五、验收标准

| # | 验收项 | Phase | 测试方式 |
|---|--------|-------|---------|
| 1 | 窗口 600×600 深色背景启动 | R1 | 手动 |
| 2 | 20×20 网格线正确显示 | R2 | 手动 |
| 3 | 蛇头圆角方块 + 表情（眼睛嘴巴） | R2 | 手动 |
| 4 | 蛇身天蓝圆角方块，与蛇头颜色区分 | R2 | 手动 |
| 5 | 链表 API 全部正确 | G1 | gtest |
| 6 | 蛇创建长度=3，坐标正确 | G1 | gtest |
| 7 | 蛇移动方向正确，头结点更新 | G1 | gtest |
| 8 | 吃食物长度+1，尾部保留 | G1 | gtest |
| 9 | 不吃食物长度不变，尾部删除 | G1 | gtest |
| 10 | 撞墙检测正确 | G1 | gtest |
| 11 | 撞自身检测正确 | G1 | gtest |
| 12 | 方向键 + WASD 均可控制 | R3 | 手动 |
| 13 | 反向操作忽略 | R3 | 手动 |
| 14 | 空格/P 暂停正常 | R3 | 手动 |
| 15 | 食物生成位置不在蛇身上 | G2 | gtest |
| 16 | 历史最高分读写正确 | G2 | gtest |
| 17 | 状态机框架 enter/update/exit 正确切换 | R4 | 手动 |
| 18 | MENU → HOW_TO_PLAY → 返回 MENU | R4 | 手动 |
| 19 | MENU 选"退出"关闭窗口 | R4 | 手动 |
| 20 | PLAYING 按空格进入 PAUSED，再按恢复 | R4 | 手动 |
| 21 | 撞墙/撞自身 → GAME_OVER | R4 | 手动 |
| 22 | GAME_OVER 显示文字+分数+最高分，任意键回 MENU | R4 | 手动 |
| 23 | 食物金黄色方块绘制正确 | R4 | 手动 |
| 24 | 左上角分数 + 历史最高分实时显示 | R4 | 手动 |
| 25 | 吃食物音效播放 | R4 | 手动 |
| 26 | 死亡音效播放 | R4 | 手动 |
| 27 | 菜单切换音效播放 | R4 | 手动 |
| 28 | 吃食物蛇变长分数+1 | R4 | 手动 |
| 29 | 速度随分数提升 | R4 | 手动 |
| 30 | 蛇头表情随方向旋转 | R5 | 手动 |
| 31 | 所有代码有详细中文注释 | R5 | 代码审查 |

---

## 六、开发流程

1. 按 Phase 顺序（R1 → R2 → G1 → R3 → G2 → R4 → R5）逐步实现
2. **R 类 Phase**：实现后手动启动游戏确认，通过后进入下一 Phase
3. **G 类 Phase**：实现后将测试用例提交给我审核，我确认后再运行 gtest，全部通过后进入下一 Phase
4. R 和 G 交替进行，确保界面和逻辑可以联动测试
5. 遇到设计问题随时沟通调整
