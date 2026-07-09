# 🐍 Snake Game — Vibe Coding 小项目

> 一个使用 **raylib + C** 从头构建的贪吃蛇游戏，全程 AI 辅助开发（Vibe Coding）。

## 🎮 游戏截图

```
┌──────────────────────────────┬──────────┐
│  🟦🐍                        │  SCORE   │
│      🟢  🟡  🔴  🟣  🟠     │    42    │
│    🟤  🟢  🟡  🔴  🟣       │          │
│       🟠  🟤  🟢  🟡        │ HIGH SCORE│
│                              │    99    │
│              🟡              │          │
│                              │  SPEED   │
│                              │ Level 3  │
│                              │──────────│
│   20×20 Grid  600×600px      │WASD→Move │
│                              │Space→Pause│
│                              │B→Back    │
│                              │ESC→Exit  │
└──────────────────────────────┴──────────┘
```

## 📦 技术栈

| 层 | 技术 |
|----|------|
| 语言 | C (核心逻辑) + C++ (gtest) |
| 图形 | [raylib 5.5.0](https://www.raylib.com/) |
| 测试 | [Google Test 1.15.2](https://github.com/google/googletest) |
| 音效 | 代码合成 WAV（`tools/gen_wav.ps1`） |
| 构建 | MSBuild / Visual Studio 2022 |
| 字体 | Verdana 96px（系统字体） |

## 🚀 快速开始

### 1. 下载运行

从 [Releases](https://github.com/Stellen-z/LearningLab/releases) 下载 `SnakeGame_v1.0.0.zip`，解压后双击 `SnakeGame.exe` 即可。

### 2. 从源码编译

```bash
# 克隆仓库
git clone https://github.com/Stellen-z/LearningLab.git
cd LearningLab/SnackGame/SnakeGame/SnakeGame

# 用 VS2022 打开 SnakeGame.vcxproj
# NuGet 会自动恢复 raylib 依赖
# 编译 Debug x64 或 Release x64
```

### 3. 运行测试

```bash
# 编译并运行测试项目
cd test
MSBuild SnakeGame_Test.vcxproj /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\SnakeGame_Test.exe
```

## 🎯 功能特性

- 🎨 **状态机驱动**：MENU → PLAYING ↔ PAUSED → GAME_OVER 完整流程
- 🍎 **15 个彩色圆形食物**：常驻网格，随机配色
- 🐍 **蛇头表情**：眼睛 + 瞳孔 + 嘴巴弧线，跟随方向
- 📊 **侧边信息栏**：实时分数 / 最高分 / 速度等级 / 操作提示
- 🔊 **代码合成音效**：吃食物 / 死亡 / 菜单切换
- 🧪 **41 个 gtest 用例**：链表 / 游戏逻辑 / 状态机全覆盖
- 🌍 **全局 Verdana 字体**：告别像素风

## 🕹️ 操作说明

| 按键 | 功能 |
|------|------|
| `WASD` / `↑↓←→` | 移动蛇 |
| `Space` / `P` | 暂停/继续 |
| `Enter` | 菜单确认 |
| `B` | 返回菜单 |
| `ESC` | 退出 |

## 📊 测试覆盖率

| 文件 | 覆盖率 |
|------|--------|
| `state.c` | 100% |
| `game.c` | 95.5% |
| `DCList.c` | 76.3% |

## 🏗️ 项目结构

```
SnackGame/
├── main.c              # 入口 + 5 状态机回调
├── state.h / state.c   # 状态机框架
├── DCList.h / DCList.c # 双向循环链表
├── game.h / game.c     # 游戏核心逻辑
├── render.h / render.c # 渲染（菜单/网格/蛇/食物/侧边栏）
├── resource/           # 音效文件（.wav）
├── tools/              # 音效生成脚本
├── test/               # gtest 测试（41 用例）
└── project.md          # 完整设计文档
```

## 🤖 AI 开发说明

本项目采用 **Vibe Coding** 方式开发——需求描述给 AI，AI 输出代码，人工验证确认。总计约 3000 行 C/C++ 代码，41 个自动化测试用例。

---

**License**: MIT