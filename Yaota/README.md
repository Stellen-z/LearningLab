# 妖塔 · 中文修仙 Roguelike

一个纯 C++17（零第三方依赖）的终端 Roguelike 小游戏：你是一名练气期散修，
闯一座随机生成的 30 层妖塔——斩妖、炼丹、夺宝、渡劫，登顶飞升。

```
    山  山  山  山  山  山  山  山  山  山  山  山
    ╔═══════════════════════════════════════╗
    ║              妖  塔                  ║
    ║        一 座 会 咬 人 的 塔          ║
    ╚═══════════════════════════════════════╝

    │山山山．．梯．．山山山山山．．．．．山山山│
    │山．．．．．．．山．．妖．．．山山山山山│
    │山．丹．．仙．．．．．．．．．妖．．．山│   ← 你是「仙」，妖怪按五行着色
    │山．．．．．．．．．山山山．．．．．山山│
    │山山山山．．坛．．山山山山山．．草．山山│   ← 坛=奇遇祭坛，草=灵草
```

## 构建

需要 MinGW-w64（g++）：

```bash
# 一条命令（多文件工程）
g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ \
    -finput-charset=UTF-8 -fexec-charset=UTF-8 -o yaota src/*.cpp

# 或者
./build.sh        # Git Bash / Linux
build.bat         # Windows CMD
```

**两个参数都不能省**（都是踩坑换来的，见下文"开发实录"）：
`-finput-charset=UTF-8`：源码是 UTF-8，MinGW 默认按系统码页解析会乱码报错；
`-static`：静态链接运行时，避免加载到 Git Bash 自带的旧版 libstdc++-6.dll
（该 DLL 与新版 GCC 的 ABI 不匹配，`ifstream` 构造直接段错误）。

### Visual Studio（VS2022 / VS2026）构建

所有源文件已带 **UTF-8 BOM**，MSVC 会自动识别编码，直接用你的 .sln/.vcxproj
重新生成就行（无需改项目属性）。命令行党也可以：

```bat
build_msvc.bat        :: 自动定位 vcvars64 并编译出 yaota_msvc.exe
```

如果你新建自己的 VS 工程，遇到满屏 `warning C4819` + 上百个语法错误
（错误信息里出现"鍚庡北鐮嶇"之类的乱码后缀），那就是源码被按 GBK 误读了：
要么保留 BOM，要么在 项目属性 → C/C++ → 命令行 里加 `/utf-8`。

## 玩法

| 按键 | 作用 | 按键 | 作用 |
|---|---|---|---|
| `w a s d` | 移动（走向妖怪=攻击） | `S`（大写） | 存档 |
| `g` | 拾取脚下 | `.` | 原地待一拍 |
| `i` | 背包（使用/装备/丢弃） | `m` | 妖怪图鉴 |
| `t` | 上楼梯 | `?` | 操作说明 |
| `b` | 渡劫突破（修为够时） | `q` | 退出 |
| `c` | 打坐（回灵力，小心偷袭） | `x` | 灵力爆发（耗15灵力重击周身） |
| `v` | 炼化奇物/炼材换灵石 | | |

**核心机制**
- **五行**：金克木、木克土、土克水、水克火、火克金。克制伤害 ×1.5，被克 ×0.7，
  攻击方生防守方 ×1.15。开局选的灵根决定你的攻击属性。
- **九境界**：练气→筑基→金丹→元婴→化神→炼虚→合体→大乘→渡劫。
  修为攒够按 `b` 渡劫，成功率随境界递减（失败重伤但留命，可再来）。
- **32 种妖怪**，4 类 AI：普通追击 / 凶悍（追更远打更狠）/ 守巢（只守房间）/
  胆小（半血逃跑）。
- **47 件物品**：法宝、法袍、丹药（7 种特效）、卷轴（6 种战术）、奇物、炼材。
- **34 条奇遇**：祭坛触发，多选项多结局，选择皆有代价。
- **Roguelike 要素**：随机地图（房间+走廊+环路）、迷雾视野（Bresenham 视线）、
  永久死亡、存档（`yaota_save.txt`，读档重生成当层地图）。

## 测试

自制零依赖测试框架（`tests/test_framework.h`），28 个用例 / 16000+ 断言：

```bash
./run_tests.sh       # 编译并运行全部用例
./run_coverage.sh    # gcov 分支覆盖率报告（自动关闭 -O2 防失真）
```

当前覆盖率（项目文件，行覆盖 / 分支执行率）：

| 文件 | 行 | 分支 | 说明 |
|---|---|---|---|
| dungeon.cpp | 100% | 99% | 地图生成 |
| player.cpp | 100% | 100% | 成长/渡劫 |
| save.cpp | 100% | 100% | 存档 |
| render.cpp | 59% | 55% | 其余为菜单/暂停等交互件，由脚本灌入冒烟覆盖 |
| game.cpp | 56% | 37% | 同上（主循环/背包菜单等交互壳） |
| events_data.cpp | 93% | 61% | 大量文案静态数据 |

测试架构：`GamePeer`（`src/game.h`）以友元身份把非交互玩法逻辑暴露给单测；
交互流程用脚本灌入 stdin 的方式做端到端冒烟（`tests/smoke.cpp`）。

## 项目结构

```
Yaota/
├── yaota_all.cpp      # 单文件合集版（= src/ 全部按依赖序拼接，直接编译可玩）
├── src/               # 多文件工程（开发版，推荐从这里改）
│   ├── types.h        # 五行/境界/颜色/编码工具 —— 世界观常量
│   ├── rng.h          # mt19937 全局随机源
│   ├── item.h + items_data.cpp       # 物品图鉴（47）
│   ├── monster.h + monsters_data.cpp # 妖怪图鉴（32）
│   ├── events.h + events_data.cpp    # 奇遇事件（34）
│   ├── dungeon.h/.cpp # 随机地图生成
│   ├── player.h/.cpp  # 玩家成长
│   ├── combat.h       # 战斗公式（纯函数）
│   ├── render.h/.cpp  # 汉字渲染/菜单/HUD
│   ├── save.h/.cpp    # 存档
│   ├── game.h/.cpp    # 总控（回合循环/AI/背包/事件结算）
│   └── main.cpp       # 入口（标题画面/创建角色）
├── tests/             # 测试框架 + 全量用例 + 冒烟脚本
├── build.sh/.bat、run_tests.sh、run_coverage.sh
└── README.md
```

改完 `src/` 后同步单文件版：
```bash
for f in src/*.h src/*.cpp; do sed -e '/^#pragma once/d' -e '/^#include /d' "$f"; done >> yaota_all.cpp
#（头部统一放 include，段落顺序见文件内分隔注释）
```

## 开发实录：测试抓到的 4 个真 bug

这个项目是"先写代码、后补测试"的活教材——补测试当天就抓出 4 个真 bug：

1. **`saveExists()` 恒真**：用 `ifstream.good()` 判断存档存在。该 MinGW 运行时下
   打开失败 `good()` 仍是 true，标题界面永远显示"读取存档"。改用 `is_open()`。
2. **stdin EOF 死循环**：脚本灌输入耗尽后 `getline` 恒返空串，主循环空输入
   `continue` 无限重绘——一次刷出 6.9GB 输出文件。`promptLine` 现在检测 EOF 干净退出。
3. **运行时 DLL 错配段错误**：编译用 D:\VsCode 的 GCC 15，运行时却加载 Git Bash
   自带的旧版 `libstdc++-6.dll`，ABI 不匹配，`ifstream` 构造即段错误。`-static` 解决。
4. **玩家出生在墙里**：`setupFloor` 生成两次地图、用第一次的房间中心当出生点——
   第二次重生成后那里可能是山岩。`game_stairs_next_floor` 用例抓出，改为单次生成
   + 贴脸妖怪挪远。
5. **MSVC 满屏语法错误**：VS2026 按 GBK（代码页 936）读 UTF-8 无 BOM 源码，
   中文注释吞换行、字符串字面量断裂，C4819 之后雪崩出几百条错误。
   修复：全部源文件加 UTF-8 BOM（g++ 与 MSVC 通吃）。附赠同款：
   `cmd.exe` 解析批处理也用 GBK——.bat 里的 UTF-8 中文注释同样会炸，
   所以 `build_msvc.bat` 全 ASCII。

## 给 C++ 学习者的导览

有 C + 数据结构基础、正在学 C++ 的话，推荐按这个顺序读源码：

1. `types.h` —— `enum class`、`inline` 函数、表驱动设计（境界表/克制矩阵）
2. `rng.h` —— 单例模式（Meyers' Singleton）、模板成员函数
3. `item.h` —— 结构体聚合初始化（图鉴数据全是花括号列表，改数据不用改逻辑）
4. `dungeon.cpp` —— 二维网格、随机算法、`std::vector` 当万能容器
5. `combat.h` —— 纯函数设计（同样的输入只依赖显式参数，最好测）
6. `player.cpp` —— RAII 之外的"值语义"：Player 拷来拷去不操心内存
7. `save.cpp` —— 流式 I/O（`ifstream/ofstream`）、序列化
8. `game.cpp` —— 状态机式的主循环、lambda（`tryStep`）、友元（`GamePeer`）
9. `tests/` —— 宏注册测试用例、断言统计、gcov 分支覆盖率工作流
