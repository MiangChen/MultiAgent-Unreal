# 文档整理方案

## 目标

让新接触仓库的 AI 或开发者能快速理解项目结构，准确找到开发功能所需的代码位置。

---

## 当前问题

### 1. 文档命名不一致

| 当前命名 | 问题 |
|---------|------|
| `20251220_jump.md` | 8位日期前缀 |
| `251221_esc.md` | 6位日期前缀 |
| `update_2025-12-13_xxx.md` | 带横线的日期 |
| `ARCHITECTURE.md` | 全大写，无日期 |

### 2. 文档分类混乱

- 功能设计、Bug 记录、API 变更、开发日志混在一起
- 没有明确的入口文档
- 新人不知道从哪里开始看

### 3. 内容重复/过时

- `SETUP_MAP_GUIDE.md` 中仍有已删除的 Town 地图
- `todo.md` 最后更新日期是 12-10
- 部分设计文档已实现但未标记状态

---

## 建议的文档结构

```
doc/
├── README.md                    # 【新增】文档索引入口
├── ARCHITECTURE.md              # 系统架构 (保留，核心文档)
├── KEYBINDINGS.md               # 按键说明 (保留)
├── UE5_API_CHANGES.md           # UE5 API 变更 (保留)
├── ue5_suggest_architecture.md  # UE5 架构建议
│
├── guide/                       # 开发指南
│   ├── DEBUG.md                 # 调试指南
│   ├── CODE_STYLE.md            # 编码规范 (从 prompt_code_style.md 重命名)
│   └── NEW_FEATURE_GUIDE.md     # 【新增】新功能开发指南
│
├── dev-completed/               # 已完成功能文档
│   ├── feature.md               # 功能特性汇总
│   ├── 2025-12-16_setup_map.md  # Setup 地图 (从 SETUP_MAP_GUIDE.md)
│   ├── 2025-12-20_jump.md       # 跳跃功能 (从 20251220_jump.md)
│   ├── 2025-12-20_direct_control.md  # 直接控制 (从 251220_direct_control.md)
│   ├── 2025-12-20_minimap.md    # 小地图 (从 20251220_minimap.md)
│   ├── 2025-12-21_pause_menu.md # 暂停菜单 (从 251221_esc.md)
│   └── ...
│
├── dev-planning/                # 规划中/设计阶段文档
│   ├── todo.md                  # TODO 列表
│   ├── 2025-12-20_scene_entity.md    # 场景实体设计 (从 20251220_scene.md)
│   ├── 2025-12-20_spawn_position.md  # 生成位置设计
│   └── ...
│
├── bug/                         # Bug 记录 (按日期拆分)
│   ├── 2025-12-09_static_mesh_actor.md      # Static Mesh vs Actor 混淆
│   ├── 2025-12-10_gameplay_tags.md          # Gameplay Tags 未注册
│   ├── 2025-12-10_patrol_stuck.md           # Patrol 路径点循环卡住
│   └── ...
│
└── asset/                       # 图片资源 (保留)
    └── ...
```

**说明：**
- 移除 `log/` - 开发日志可合并到对应 `dev-completed/` 文档的"开发历史"章节，或用 Git history 替代
- 移除 `reference/` - 内容已分散到其他文件夹
```

---

## 核心：新增 doc/README.md

这是最重要的改动，作为文档入口：

```markdown
# MultiAgent-Unreal 文档索引

## 🚀 快速开始

| 我想... | 看这个文档 |
|---------|-----------|
| 了解项目整体架构 | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 知道按键怎么用 | [KEYBINDINGS.md](KEYBINDINGS.md) |
| 开发一个新功能 | [guide/NEW_FEATURE_GUIDE.md](guide/NEW_FEATURE_GUIDE.md) |
| 调试问题 | [guide/DEBUG.md](guide/DEBUG.md) |

## 📁 代码定位速查

| 功能模块 | 代码位置 | 相关文档 |
|---------|---------|---------|
| Agent 生命周期 | `Core/MAAgentManager` | ARCHITECTURE.md §6 |
| 命令系统 (RTS) | `Core/MACommandManager` | ARCHITECTURE.md §2.3 |
| 选择/编组 | `Core/MASelectionManager` | ARCHITECTURE.md §2.3 |
| 编队系统 | `Core/MASquadManager` | dev-completed/feature.md |
| StateTree 任务 | `Agent/StateTree/Task/` | ARCHITECTURE.md §2.2 |
| GAS 技能 | `Agent/GAS/Ability/` | ARCHITECTURE.md §2.2 |
| 能力组件 | `Agent/Component/Capability/` | ARCHITECTURE.md §2.1 |
| 传感器 | `Agent/Component/Sensor/` | dev-completed/feature.md |
| UI 系统 | `UI/` | dev-completed/feature.md |
| 输入系统 | `Input/MAInputActions` | KEYBINDINGS.md |
| 环境实体 | `Environment/` | dev-completed/feature.md |

## 📋 文档分类

### 核心文档
- [ARCHITECTURE.md](ARCHITECTURE.md) - 系统架构、模块设计、类继承
- [KEYBINDINGS.md](KEYBINDINGS.md) - 所有按键操作说明
- [UE5_API_CHANGES.md](UE5_API_CHANGES.md) - UE5 废弃 API 迁移指南

### 开发指南
- [guide/NEW_FEATURE_GUIDE.md](guide/NEW_FEATURE_GUIDE.md) - 新功能开发流程
- [guide/DEBUG.md](guide/DEBUG.md) - 调试技巧
- [guide/CODE_STYLE.md](guide/CODE_STYLE.md) - 编码规范

### 功能文档
- [dev-completed/feature.md](dev-completed/feature.md) - 已实现功能汇总

### 参考资料
- [bug/](bug/) - Bug 记录与解决方案 (按日期分文件)
- [dev-planning/todo.md](dev-planning/todo.md) - 待开发功能列表
```

---

## 核心：新增 NEW_FEATURE_GUIDE.md

帮助 AI/开发者快速定位代码：

```markdown
# 新功能开发指南

## 功能类型 → 代码位置

### 1. 添加新的 Agent 命令 (如巡逻、跟随)

```
1. MACommandManager.h    - 添加 EMACommand 枚举
2. MACommandManager.cpp  - 实现 SendCommand() 分支
3. MAPlayerController    - 绑定快捷键
4. StateTree Task        - 实现具体行为 (Agent/StateTree/Task/)
5. KEYBINDINGS.md        - 更新按键文档
```

### 2. 添加新的 Agent 类型 (如新无人机)

```
1. Agent/Character/      - 创建新 Character 类
2. MAAgentInterfaces.h   - 实现需要的 Interface
3. MATypes.h             - 添加 EMAAgentType 枚举
4. MASetupWidget.cpp     - 添加到 AvailableAgentTypes
5. config/SimConfig.json - 添加配置 (可选)
```

### 3. 添加新的 GAS 技能

```
1. Agent/GAS/Ability/    - 创建 GA_XXX.h/cpp
2. MAGameplayTags.cpp    - 注册新 Tag
3. MAAbilitySystemComponent - 注册 Ability
4. DefaultGameplayTags.ini  - 配置 Tag
```

### 4. 添加新的 StateTree Task

```
1. Agent/StateTree/Task/ - 创建 MASTTask_XXX.h/cpp
2. 在 UE 编辑器中添加到 StateTree Asset
```

### 5. 添加新的 UI 界面

```
1. UI/                   - 创建 Widget 类
2. MAHUD.cpp             - 管理 Widget 显示
3. MAPlayerController    - 绑定切换快捷键
4. MAInputActions        - 添加 Input Action
```

### 6. 添加新的传感器

```
1. Agent/Component/Sensor/ - 创建 Sensor Component
2. MACharacter             - 添加 Sensor 管理
3. config/SimConfig.json   - 配置传感器参数
```

### 7. 添加新的环境实体 (如新的交互物)

```
1. Environment/          - 创建 Actor 类
2. 在 UE 编辑器中放置到地图
```
```

---

## 执行步骤

### Phase 1: 基础整理 (建议先做)

1. [ ] 创建 `doc/README.md` 作为入口
2. [ ] 更新 `SETUP_MAP_GUIDE.md` 移除 Town
3. [ ] 更新 `todo.md` 日期
4. [ ] 重命名 `251221_esc.md` → `251221_pause_menu.md`

### Phase 2: 目录重组 (可选)

1. [ ] 创建 `guide/`, `dev-completed/`, `dev-planning/`, `bug/` 子目录
2. [ ] 移动文件到对应目录
3. [ ] 更新所有文档中的相对路径引用

### Phase 3: 内容完善 (可选)

1. [ ] 创建 `guide/NEW_FEATURE_GUIDE.md`
2. [ ] 为每个功能文档添加 "状态" 标记 (已实现/开发中/规划中)
3. [ ] 统一日期格式为 `YYYY-MM-DD`

---

## 文件命名规范 (建议)

| 类型 | 格式 | 示例 |
|-----|------|------|
| 核心文档 | `UPPER_CASE.md` | `ARCHITECTURE.md` |
| 功能文档 | `YYYY-MM-DD_name.md` | `2025-12-20_jump.md` |
| 设计文档 | `YYYY-MM-DD_name.md` | `2025-12-20_scene_entity.md` |
| 开发日志 | `YYYY-MM-DD_topic.md` | `2025-12-13_capability.md` |
| Bug 记录 | `YYYY-MM-DD_bug_name.md` | `2025-12-10_gameplay_tags.md` |

---

## 优先级建议

**高优先级** (立即改善 AI 理解):
- 创建 `doc/README.md` 入口文档
- 创建 `guide/NEW_FEATURE_GUIDE.md`

**中优先级** (改善组织):
- 目录重组
- 文件重命名

**低优先级** (锦上添花):
- 统一日期格式
- 添加状态标记
