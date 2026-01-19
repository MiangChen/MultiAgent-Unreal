# UI 系统架构文档

## 概述

本项目的 UI 系统采用 **混合架构**：
- **HUD 层** (`AHUD`) - 用于 Canvas 直接绘制（文字指示器、通知、调试信息）
- **Widget 层** (`UUserWidget`) - 用于 UMG Widget（交互式面板、输入框、按钮）

两层通过 `MAHUD` 和 `MAUIManager` 协调管理。

---

## 目录结构

```
UI/
├── Core/                     # 核心基础设施
│   ├── MAUIManager           # Widget 管理器 (生命周期 + 可见性)
│   ├── MAUITheme             # 主题系统 (颜色、字体、动画)
│   ├── MAHUDTypes.h          # 类型定义 (状态枚举、通知类型)
│   └── MAHUDStateManager     # HUD 状态机管理器
│
├── HUD/                      # HUD 层 (Canvas 绘制)
│   ├── MAHUD                 # 主 HUD 管理器
│   ├── MASelectionHUD        # 框选绘制基类
│   ├── MAHUDWidget           # HUD Widget
│   └── MAMainHUDWidget       # 主 HUD Widget (集成小地图、通知、侧边栏)
│
├── Modal/                    # 模态窗口
│   ├── MABaseModalWidget     # 模态基类 (淡入淡出动画、按钮布局)
│   ├── MATaskGraphModal      # 任务图模态 (详细可视化 + 编辑)
│   ├── MASkillAllocationModal      # 技能列表模态 (甘特图 + 编辑)
│   └── MAEmergencyModal      # 紧急事件模态 (事件详情 + 干预)
│
├── Components/               # 可复用组件
│   ├── MAStyledButton        # 样式化按钮 (主题支持 + 微交互)
│   ├── MARightSidebarWidget  # 右侧边栏 (命令输入 + 预览 + 日志)
│   ├── MANotificationWidget  # 通知组件 (滑入动画 + 按键提示)
│   ├── MAMiniMapWidget       # 小地图 Widget
│   ├── MAMiniMapManager      # 小地图管理器
│   ├── MAContextMenuWidget   # 右键菜单
│   ├── MADirectControlIndicator # 直接控制指示器
│   ├── MASkillListPreview    # 技能列表预览 (紧凑视图)
│   └── MATaskGraphPreview    # 任务图预览 (紧凑视图)
│
├── TaskGraph/                # 任务图系统 (Model + Canvas)
│   ├── MATaskGraphModel      # 任务图数据模型
│   ├── MADAGCanvasWidget     # DAG 画布 (节点拖拽/连线)
│   ├── MATaskNodeWidget      # 任务节点 Widget
│   ├── MANodePaletteWidget   # 节点工具栏
│   └── MATaskPlannerWidget   # 任务规划工作台 (主容器)
│
├── SkillAllocation/                # 技能分配系统 (Model + Canvas)
│   ├── MASkillAllocationModel  # 技能分配数据模型
│   ├── MAGanttCanvas           # 甘特图画布
│   └── MASkillAllocationViewer # 技能分配查看器 (主容器)
│
├── Mode/                     # 模式相关 Widget
│   ├── MAModifyWidget        # Modify 模式面板 (场景标注)
│   ├── MAEditWidget          # Edit 模式面板 (场景编辑)
│   ├── MAEmergencyWidget     # 突发事件详情面板
│   ├── MASceneListWidget     # 场景列表面板 (Goal/Zone)
│   └── MAModifyTypes.h       # 标注类型定义
│
├── Setup/                    # 设置相关
│   ├── MASetupHUD            # 设置 HUD
│   └── MASetupWidget         # 设置 Widget
│
├── Legacy/                   # 遗留代码 (待清理)
│   └── MASimpleMainWidget    # 简单主界面
│
└── README.md                 # 本文档
```

---

## 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        MAHUD (HUD 管理器)                        │
│  继承: AMASelectionHUD → AHUD                                   │
│                                                                 │
│  职责:                                                          │
│  - DrawHUD() Canvas 绘制 (指示器、通知、场景标签)                  │
│  - 事件绑定和协调 (PlayerController, Manager 委托)                │
│  - 委托 Widget 管理给 UIManager                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ 持有
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    MAUIManager (Widget 管理器)                   │
│                                                                 │
│  职责:                                                          │
│  - 创建所有 Widget 实例                                          │
│  - 管理 Widget 可见性 (Show/Hide/Toggle)                         │
│  - 控制 Widget ZOrder                                           │
│  - 处理输入模式切换 (Game/UI)                                    │
│  - 集成 HUDStateManager 处理状态转换                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ 管理
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Widget 实例                              │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │TaskPlanner  │  │SkillViewer │  │ Emergency   │              │
│  │  Widget     │  │   Widget    │  │  Widget     │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │  Modify     │  │   Edit      │  │ SceneList   │              │
│  │  Widget     │  │  Widget     │  │  Widget     │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │DirectControl│  │ TaskGraph   │  │ SkillList   │              │
│  │ Indicator   │  │   Modal     │  │   Modal     │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 核心类说明

### Core/MAUIManager

**职责**:
1. **Widget 生命周期** - 创建和持有所有 Widget 实例
2. **可见性管理** - Show/Hide/Toggle + ZOrder 控制
3. **输入模式** - Game/UI 混合模式切换
4. **焦点管理** - SetWidgetFocus
5. **状态管理** - 集成 HUDStateManager 处理 UI 状态转换

### Core/MAHUDStateManager

**职责**:
1. **状态机管理** - NormalHUD, NotificationPending, ReviewModal, EditingModal
2. **输入处理** - Z/N/X 键触发模态窗口
3. **状态转换验证** - 确保合法的状态流转
4. **委托通知** - OnStateChanged 事件

### HUD/MAHUD

**继承**: `AMASelectionHUD` → `AHUD`

**职责**:
1. **Canvas 绘制** - `DrawHUD()` 中绘制指示器、通知、场景标签
2. **事件协调** - 绑定 PlayerController、Manager 委托
3. **Widget 控制** - 通过 UIManager 代理

### Modal/MABaseModalWidget

**职责**:
1. **统一布局** - 标题、内容区、按钮容器
2. **动画支持** - 淡入淡出动画
3. **按钮回调** - Confirm/Reject/Edit 委托
4. **主题支持** - ApplyTheme 虚方法

---

## 渲染层级

**重要**: Canvas 绘制 (DrawHUD) 始终在 Widget 之上渲染。

```
渲染顺序 (从下到上):
1. 游戏世界
2. UMG Widget (按 ZOrder 排序)
3. Canvas 绘制 (DrawHUD)
```

---

## Widget ZOrder

| Widget | ZOrder | 说明 |
|--------|--------|------|
| DirectControl | 0 | 最底层指示器 |
| TaskPlanner | 1 | 主工作台 |
| SkillAllocation | 2 | 技能查看器 |
| Emergency | 3 | 突发事件 |
| Modify | 4 | Modify 面板 |
| Edit | 5 | Edit 面板 |
| SceneList | 6 | 场景列表 |
| Modal Windows | 10+ | 模态窗口 (最高层) |

---

## 添加新 Widget 步骤

1. **创建 Widget 类** - 在对应目录下创建 .h/.cpp
2. **在 MAUIManager 中注册**:
   - 添加 `EMAWidgetType` 枚举值
   - 添加成员变量和 Getter
   - 在 `CreateAllWidgets()` 中创建实例
   - 在 `GetWidgetZOrder()` 中设置 ZOrder
3. **在 MAHUD 中添加控制方法** (可选):
   - Show/Hide/Toggle 方法
   - 事件绑定
4. **绑定委托** (如需要):
   - 在 `BindWidgetDelegates()` 中绑定

---

## 注意事项

1. **Canvas vs Widget**: 
   - 简单文字/指示器 → Canvas (DrawHUD)
   - 交互式 UI → Widget (UMG)

2. **输入模式**:
   - Widget 显示时需要 `SetInputModeGameAndUI`
   - Widget 隐藏时恢复 `SetInputModeGameOnly`

3. **Widget 可见性**:
   - 使用 `SetVisibility()` 而非 `AddToViewport/RemoveFromViewport`
   - 所有 Widget 在 BeginPlay 时创建，通过可见性控制显示

4. **Include 路径**:
   - 跨目录引用使用相对路径 (如 `../Core/MAUITheme.h`)
   - 同目录引用直接使用文件名
