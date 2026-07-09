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
| **窗口尺寸** | 840 × 600 像素（左侧 600px 游戏区 + 右侧 240px 信息栏） |
| **网格** | 20 × 20，每格 30 × 30 像素 |
| **网格线** | 可见，浅灰色细线 |
| **帧率** | 60 FPS |

### 1.3 视觉设计

| 元素 | 颜色/样式 |
|------|----------|
| **背景** | #121212 深色 |
| **蛇头** | #0D47A1 深蓝，圆角方块，带白色眼球 + 黑色瞳孔 + 嘴巴弧线（表情随方向变化） |
| **蛇身** | #42A5F5 天蓝，圆角方块 |
| **食物** | 彩色正圆形（半径 13px），15 种随机配色：红/橙/金/绿/青/蓝/紫/粉/薄荷/黄/青柠/海蓝/珊瑚/浅蓝/薰衣草 |
| **网格线** | #555555 中灰半透明（56% 不透明度），1px |
| **侧边栏背景** | #1A1A1A 暗黑 |
| **全局字体** | Verdana 96px（全部界面：菜单/侧边栏/说明页/PAUSED/GAME_OVER） |
| **窗口整体** | 840×600：左侧 600px 20×20 游戏网格 + 右侧 240px 信息侧边栏 |

### 1.4 初始状态

- 蛇初始长度 3 节，蛇头朝右，网格中央偏左位置 (10,10)
- 初始速度：每 200ms 移动一步
- 分数：0
- 启动后先进入主菜单（Play / How to Play / Exit）

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
| R | 死亡后重玩（R3 独立模式） |
| ESC | 退出游戏 |

**反向操作**：与当前方向相反的操作直接忽略。

### 1.7 音效

| 音效 | 触发时机 |
|------|---------|
| **吃食物音效** | 蛇吃到食物时播放 |
| **死亡音效** | 蛇撞墙或撞自身时播放 |
| **菜单切换音效** | 在菜单中切换选项或进入菜单时播放 |

使用 raylib 内置的 `LoadSound` / `PlaySound`，音频文件使用 `.wav` 格式。
音效由项目自带的 `tools/gen_wav.ps1` 脚本生成（纯代码合成，无外部依赖）：
- `eat.wav`：800Hz 正弦波 100ms，短促清脆
- `die.wav`：300→80Hz 降频锯齿波 500ms，低沉碰撞
- `menu.wav`：1200Hz 正弦波 50ms，快速滴答

### 1.8 历史最高分

- 本地文件 `highscore.dat` 存储历史最高分
- 游戏启动时读取，GAME_OVER 时若当前分数 > 历史最高分则更新
- 文件格式：单行纯文本，只存数字

---

## 二、架构设计

### 2.1 文件结构（实际实现）

```
SnackGame/
├── project.md
├── SnakeGame/
│   ├── SnakeGame.slnx
│   ├── SnakeGame/
│   │   ├── SnakeGame.vcxproj
│   │   ├── main.c                    # 入口 → 状态机主循环（handlers[state].update()）
│   │   ├── state.h / state.c         # 状态机框架（回调注册+切换，含 gtest）
│   │   ├── DCList.h / DCList.c       # 带头结点双向循环链表（存储 Position 坐标）
│   │   ├── game.h / game.c           # 游戏核心逻辑
│   │   ├── render.h / render.c       # 界面渲染（菜单/说明页/网格/蛇/食物/侧边栏）
│   │   ├── resource/
│   │   │   ├── eat.wav               # 吃食物音效
│   │   │   ├── die.wav               # 死亡音效
│   │   │   └── menu.wav              # 菜单切换音效
│   │   ├── tools/
│   │   │   └── gen_wav.ps1           # WAV 音效生成脚本
│   │   ├── test/
│   │   │   ├── test_list.cpp         # 链表 gtest 测试（9 用例）
│   │   │   ├── test_game.cpp         # 游戏逻辑 gtest 测试（22 用例）
│   │   │   ├── test_state.cpp        # 状态机 gtest 测试（6 用例）
│   │   │   ├── SnakeGame_Test.vcxproj # 测试独立项目
│   │   │   └── googletest/           # gtest 1.15.2 源码（内嵌编译）
│   │   ├── packages.config           # NuGet 依赖：raylib 5.5.0
│   │   └── highscore.dat             # 运行生成，不计入版本控制
```

### 2.2 模块依赖

```
main.c
  ├── state.h → 注册/切换状态回调
  │     ├── render.h → 调用 raylib API
  │     └── game.h   → 调用 DCList.h
  ├── game.h   → 调用 DCList.h
  ├── DCList.h → 纯 C 数据结构（Position {x,y} 为数据负载）
  └── raylib   → 第三方图形库
```

### 2.3 蛇的链表映射（DCList 实现）

蛇身用 `DCListNode *body` 存储（哨兵结点即为链表标识），`body->next` 指向蛇头，`body->prev` 指向蛇尾。

```
哨兵(body) ←→ [蛇头] ←→ [节2] ←→ [节3] ←→ ... ←→ [蛇尾] ←→ 哨兵(body)
              body->next                              body->prev
```

- 哨兵结点 `body` 的 `data` 为 `{-1, -1}`（标记值），不存储蛇身数据
- 蛇移动（`snake_move`）= `DCListPushFront` 头插新坐标（O(1)）+ 条件 `DCListPopBack` 尾删（O(1)）
- 碰撞/食物检测通过 `DCListContains` 遍历坐标判定

---

## 三、数据结构设计

### 3.1 坐标 & 链表节点数据类型（DCList 实现）

`Position {x,y}` 定义在 `DCList.h` 中作为 `DCLDataType`（数据负载类型），链表节点 `DCListNode` 存储 `Position data`。

```c
// DCList.h
typedef struct {
    int x, y;
} Position;

typedef Position DCLDataType;

typedef struct DCListNode {
    DCLDataType data;              // 存储坐标 (x,y)
    struct DCListNode* prev;       // 前驱结点指针
    struct DCListNode* next;       // 后继结点指针
} DCListNode;

// 核心 API（直接操作 DCListNode* 哨兵）
DCListNode* DCListInit(void);
void        DCListDestroy(DCListNode* L);
void        DCListPushFront(DCListNode* L, DCLDataType x);
void        DCListPushBack(DCListNode* L, DCLDataType x);
void        DCListPopBack(DCListNode* L);
void        DCListPopFront(DCListNode* L);
bool        DCListIsEmpty(DCListNode* L);
bool        DCListContains(DCListNode* L, int x, int y);
```

### 3.2 食物数据结构

```c
// game.h
#define FOOD_COUNT 15          /* 网格常驻食物数 */
typedef struct {
    Position pos;              /* 食物在网格中的坐标 */
    Color color;               /* 食物颜色（15 种随机配色之一） */
    bool active;               /* 是否有效 */
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
    DCListNode *body;             // 链表哨兵（body->next=蛇头，body->prev=蛇尾）
    Direction dir;                 // 当前移动方向
    Direction next_dir;            // 下一次移动方向（键盘缓冲）
    int score;                     // 当前得分
    bool alive;                    // 是否存活
} Snake;

typedef struct {
    Snake snake;                     /* 蛇的数据 */
    Food foods[FOOD_COUNT];          /* 15 个食物 */

    float move_timer;                /* 移动累计计时器（每秒累加 GetFrameTime()） */
    float move_interval;             /* 移动触发间隔（秒） */

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

**链表 API（DCList.h/c 替代原 list.h/c）**：
- [x] `DCListNode* DCListInit()`
- [x] `void DCListPushFront(DCListNode *L, DCLDataType x)`
- [x] `void DCListPushBack(DCListNode *L, DCLDataType x)`
- [x] `void DCListPopBack(DCListNode *L)`
- [x] `void DCListPopFront(DCListNode *L)`
- [x] `bool DCListIsEmpty(DCListNode *L)`
- [x] `bool DCListContains(DCListNode *L, int x, int y)`
- [x] `void DCListDestroy(DCListNode *L)`

**游戏逻辑（game.h/c）**：
- [x] `Snake snake_create(int head_x, int head_y, int length, Direction dir)`
- [x] `bool snake_move(Snake *snake, Food *foods, int food_count, bool *ate, int *eaten_idx)` — 头插新坐标 + 条件尾删 + 多食物碰撞
- [x] `bool wall_collided(Position pos, int grid_size)`
- [x] `bool self_collided(Snake *snake)` — 新蛇头坐标是否与蛇身重叠（不含自身）
- [x] `void generate_all_foods(Food *foods, int food_count, Snake *snake, int grid_size)` — 批量生成多个食物
- [x] `float get_move_interval(int score)` — 基础 0.2s，每 3 分减 0.02s，最低 0.06s

**gtest 测试**：
- [x] 链表创建/头插/尾删/头删/判空/包含/销毁（9 用例）
- [x] 蛇创建后长度 3，坐标正确（2 用例：DIR_RIGHT + DIR_UP）
- [x] 蛇移动方向正确，头结点更新
- [x] 吃食物长度 +1，尾部不变
- [x] 不吃食物长度不变，尾部变更
- [x] 撞墙检测返回 true（5 个方向 + 边界 OK + 对角线）
- [x] 撞自身检测返回 true（无重叠 + 环形身体）
- [x] 食物生成位置不在蛇身上（50 次随机验证）
- [x] **提交测试用例给我审核** → 确认后运行 → 30/30 PASSED

### Phase R3 — 蛇动画 & 键盘控制

- [x] `main.c` 整合 `snake_move` 驱动动画：`move_timer += GetFrameTime()`，达标时触发移动
- [x] 键盘绑定：↑/W ↓/S ←/A →/D → 更新 `snake->next_dir`（缓冲方向）
- [x] 反向操作忽略
- [x] 空格/P → 暂停
- [x] 侧边栏实时更新分数/最高分/速度等级
- [x] 死亡后半透明遮罩 + "GAME OVER" + R 键重玩
- [x] ESC 退出
- [x] **手动确认**：蛇可控制移动，方向正确

### Phase G2 — 食物管理 & 历史最高分 & 测试

- [x] `generate_food()` 完善实现（随机生成 + 惰性种子 + 最大尝试次数）
- [x] `load_high_score()` / `save_high_score()` 文件读写（带错误处理）
- [x] **gtest 测试**：
  - [x] 食物生成位置合法（50 次验证）
  - [x] 历史最高分读写正确（3 用例：读取不存在文件、保存并读取、覆盖保存）
  - [x] **提交测试用例给我审核** → 确认后运行 → 已合并入 30 用例，全部 PASSED

### Phase R4 — 状态机框架 & 音效 & 全流程整合

**状态机框架（state.h/c）**：
- [x] `typedef void (*StateEnter)(SnakeGame*) / StateUpdate / StateExit`
- [x] `void register_state(GameState state, StateHandler handler)`
- [x] `void change_state(SnakeGame *game, GameState new_state)` — 调用旧状态 exit + 新状态 enter
- [x] 全局状态处理器数组 `StateHandler handlers[5]`

**各状态实现**：

| 状态 | enter | update | exit |
|------|-------|--------|------|
| `MENU` | 重置 `selected_menu = 0` | 绘制菜单 + ↑↓切换 + Enter 确认 + 菜单音效 | — |
| `HOW_TO_PLAY` | — | 绘制说明文字 + 按 B 返回 + 菜单音效 | — |
| `PLAYING` | 重置蛇、食物、计时器 | 方向输入 → `snake_move` → 碰撞 → 吃食物 + 音效 → 计分 → `BeginDrawing/EndDrawing` | — |
| `PAUSED` | — | 渲染游戏画面 + 半透明遮罩 + "PAUSED" + 空格/P 恢复 + `BeginDrawing/EndDrawing` | — |
| `GAME_OVER` | 播放死亡音效 + 更新最高分 | 渲染结束画面 + "GAME OVER" + 任意键回菜单 + `BeginDrawing/EndDrawing` | 保存最高分 |

**渲染完善**：
- [x] `DrawSidebar(int score, int high_score, float interval)` — 右侧 240px 信息栏（SCORE / HIGH SCORE / SPEED / 操作提示）
- [x] 暂停遮罩 + "PAUSED"（在 `paused_update` 中内联实现）
- [x] 死亡遮罩 + "GAME OVER"（在 `gameover_update` 中内联实现）
- [x] `DrawMenu(int selected)` — 标题 "SNAKE GAME" + 三个选项（选中项高亮）
- [x] `DrawHowToPlay()` — 操作说明文字

**音效整合**：
- [x] `InitAudioDevice()` / `LoadSound`（3 个 .wav 由 `tools/gen_wav.ps1` 生成）
- [x] 吃食物 → `PlaySound(eat_sound)`
- [x] 死亡 → `PlaySound(die_sound)`
- [x] 菜单切换/选项变更 → `PlaySound(menu_sound)`
- [x] 退出时 `UnloadSound` + `CloseAudioDevice`

**主循环结构（main.c）**：
```c
while (!WindowShouldClose()) {
    handlers[game.state].update(&game);
}
```

**gtest 测试**：
- [x] `test/test_state.cpp` 6 个用例（exit/enter 顺序、同状态跳过、update 路由、NULL 安全、完整流转）
- [x] 回归测试 41/41 PASSED

- [x] **手动确认**：MENU→PLAYING→PAUSED→PLAYING→GAME_OVER→MENU→HOW_TO_PLAY→MENU→EXIT 完整流程可走通，音效正常

### Phase R5 — 最终润色

- [x] 蛇头表情跟随方向旋转（眼睛/嘴巴方向随 DIR 变化）
- [x] 食物不生成在蛇身上验证（generate_food 循环检查）
- [x] `high_score_updated` 标志位处理
- [x] 代码全面添加详细中文注释
- [x] 回归测试：gtest 41 用例全部通过
- [x] 菜单+说明页+PAUSED+GAME_OVER 字体统一为 Verdana，全窗口 840px 居中
- [x] 食物改为彩色正圆形（15 色 DrawCircle）
- [x] 代码风格统一（缩进/命名/宏定义）
- [x] 最终手动游玩测试（走通全流程 + 连续玩 5+ 分钟无崩溃）

---

## 五、验收标准

| # | 验收项 | Phase | 测试方式 | 状态 |
|---|--------|-------|---------|------|
| 1 | 窗口 840×600 深色背景启动 | R1 | 手动 | ✅ |
| 2 | 20×20 网格线正确显示 | R2 | 手动 | ✅ |
| 3 | 蛇头圆角方块 + 表情（眼睛+瞳孔+嘴巴弧线） | R2 | 手动 | ✅ |
| 4 | 蛇身天蓝圆角方块，与蛇头颜色区分 | R2 | 手动 | ✅ |
| 5 | 链表 API 全部正确 | G1 | gtest | ✅ |
| 6 | 蛇创建长度=3，坐标正确 | G1 | gtest | ✅ |
| 7 | 蛇移动方向正确，头结点更新 | G1 | gtest | ✅ |
| 8 | 吃食物长度+1，尾部保留 | G1 | gtest | ✅ |
| 9 | 不吃食物长度不变，尾部删除 | G1 | gtest | ✅ |
| 10 | 撞墙检测正确 | G1 | gtest | ✅ |
| 11 | 撞自身检测正确 | G1 | gtest | ✅ |
| 12 | 方向键 + WASD 均可控制 | R3 | 手动 | ✅ |
| 13 | 反向操作忽略 | R3 | 手动 | ✅ |
| 14 | 空格/P 暂停正常 | R3 | 手动 | ✅ |
| 15 | 食物生成位置不在蛇身上 | G2 | gtest | ✅ |
| 16 | 历史最高分读写正确 | G2 | gtest | ✅ |
| 17 | 状态机框架 enter/update/exit 正确切换 | R4 | gtest | ✅ |
| 18 | MENU → HOW_TO_PLAY → 返回 MENU | R4 | 手动 | ✅ |
| 19 | MENU 选"退出"关闭窗口 | R4 | 手动 | ✅ |
| 20 | PLAYING 按空格进入 PAUSED，再按恢复 | R4 | 手动 | ✅ |
| 21 | 撞墙/撞自身 → GAME_OVER | R4 | 手动 | ✅ |
| 22 | GAME_OVER 显示文字+遮罩，任意键回 MENU | R4 | 手动 | ✅ |
| 23 | 食物彩色正圆形绘制正确（15 色随机 + DrawCircle） | R2 | 手动 | ✅ |
| 24 | 侧边栏分数 + 历史最高分实时显示 | R2+ | 手动 | ✅ |
| 25 | 吃食物音效播放 | R4 | 手动 | ✅ |
| 26 | 死亡音效播放 | R4 | 手动 | ✅ |
| 27 | 菜单切换音效播放 | R4 | 手动 | ✅ |
| 28 | 吃食物蛇变长分数+1 | R3 | 手动 | ✅ |
| 29 | 速度随分数提升（每 3 分减 0.02s） | R3 | 手动 | ✅ |
| 30 | 蛇头表情随方向旋转 | R5 | 手动 | ✅ |
| 31 | 所有代码有详细中文注释 | R5 | 代码审查 | ✅ |

---

## 六、开发流程

1. 按 Phase 顺序（R1 → R2 → G1 → R3 → G2 → R4 → R5）逐步实现
2. **R 类 Phase**：实现后手动启动游戏确认，通过后进入下一 Phase
3. **G 类 Phase**：实现后将测试用例提交给我审核，我确认后再运行 gtest，全部通过后进入下一 Phase
4. R 和 G 交替进行，确保界面和逻辑可以联动测试
5. 遇到设计问题随时沟通调整
