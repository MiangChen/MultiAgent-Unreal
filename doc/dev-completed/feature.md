# 功能特性文档

## Robot 共用参数 (CARLA 风格)

本项目采用 CARLA 风格架构，参数存储在 Robot 上，多个 Task/Ability 共用。

### MARobotDogCharacter 属性

| 属性 | 类型 | 默认值 | 用途 |
|------|------|--------|------|
| `ScanRadius` | float | 200 | 感知范围（多任务复用） |
| `FollowTarget` | AMACharacter* | null | 跟随目标 |
| `CoverageArea` | AActor* | null | 覆盖区域 |
| `PatrolPath` | AMAPatrolPath* | null | 巡逻路径 |

### ScanRadius 复用

| Task | 用途 |
|------|------|
| MA Follow | 与目标保持的距离 |
| MA Coverage | 路径间距 + 到达判定半径 |
| MA Patrol | 到达路径点判定半径 |

### 设计好处

- 参数集中管理，改一处全生效
- 运行时可动态修改
- 符合物理意义（感知范围）

---

## 区域覆盖 (Coverage)

### 概述

区域覆盖功能让机器人按"割草机"模式扫描指定区域，确保完整覆盖。

### CARLA 风格设计

参数存储在 Robot 上，Task 从 Robot 获取：

```cpp
// Robot 属性
Robot->CoverageArea;   // 覆盖区域
Robot->ScanRadius;     // 扫描半径（复用）
```

### 组件

- `MACoverageArea` - 定义覆盖区域边界（Spline）
- `MASTTask_Coverage` - StateTree Task，执行覆盖任务

### 路径生成

1. 从 `Robot->GetCoverageArea()` 获取覆盖区域
2. 使用 `Robot->ScanRadius` 计算路径间距
3. NavMesh 投影确保所有点可达
4. 投影失败时尝试4个方向偏移（1x, 2x, 3x 半径）

### 可视化

- 橙色线：区域边界
- 绿色点和线：实际执行路径（任务开始后显示）
- 绿色球：起点
- 红色球：终点

### 使用方式

```cpp
// 1. 设置覆盖区域（必须）
Robot->SetCoverageArea(CoverageAreaActor);

// 2. 发送命令
ASC->AddLooseGameplayTag("Command.Coverage");
// 或按 K 键
```

### StateTree 配置

MA Coverage Task 无需配置参数，自动从 Robot 获取

---

## 跟随 (Follow)

### 概述

任何 RobotDog 都可以执行跟随命令，持续跟随指定目标。

### CARLA 风格设计

参数存储在 Robot 上，Task 从 Robot 获取：

```cpp
// Robot 属性
Robot->FollowTarget;   // 跟随目标
Robot->ScanRadius;     // 跟随距离（复用）
```

### 使用方式

```cpp
// 1. 设置跟随目标（必须）
Robot->SetFollowTarget(HumanCharacter);

// 2. 发送命令
ASC->AddLooseGameplayTag("Command.Follow");
// 或按 F 键
```

### StateTree 配置

MA Follow Task 无需配置参数，自动从 Robot 获取

---

## 充电 (Charge)

### 概述

机器人自动查找最近的充电站，导航过去并渐进式充电。

### CARLA 风格设计

Charge 比较特殊：
- 自动查找最近的充电站（不需要预设）
- `AcceptanceRadius` 从 `ChargingStation->InteractionRadius` 获取

```cpp
// 充电站属性
ChargingStation->InteractionRadius;  // 交互范围（到达判定）
```

### 渐进式充电

充电不是瞬间完成，而是渐进式恢复：
- 默认充电速率：每秒 20% 电量
- 更新间隔：0.5 秒
- 离开充电站范围会中断充电

```cpp
// GA_Charge 属性
ChargeRatePerSecond = 20.f;    // 每秒恢复 20% 电量
ChargeUpdateInterval = 0.5f;   // 每 0.5 秒更新一次
```

### 使用方式

```cpp
// 直接发送命令，自动查找最近充电站
ASC->AddLooseGameplayTag("Command.Charge");
// 或按 H 键
// 或低电量时自动触发
```

### StateTree 配置

MA Charge Task 无需配置参数，自动查找充电站并获取其 InteractionRadius

---

## 充电站 Actor (ChargingStation)

### 碰撞设计

```
AMAChargingStation
├── MeshComponent (视觉)
│   ├── SetCanEverAffectNavigation(false) - 不影响 NavMesh
│   └── SetCollisionEnabled(NoCollision) - 不阻挡移动
│
└── InteractionSphere (检测)
    ├── 半径由 InteractionRadius 属性控制（默认 300）
    └── Overlap 模式 - 可穿过但触发事件
```

### 可配置属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `InteractionRadius` | 300 | 交互范围，也用于 Charge Task 的到达判定 |

### 设计原因

| 问题 | 解决方案 |
|------|----------|
| 充电站挖掉 NavMesh，导致无法导航 | `SetCanEverAffectNavigation(false)` |
| 充电站阻挡机器人移动 | `SetCollisionEnabled(NoCollision)` |
| 需要检测机器人进入范围 | `InteractionSphere` 使用 Overlap 模式 |

---

## 巡逻 (Patrol)

### 概述

机器人循环巡逻指定路径点。

### CARLA 风格设计

参数存储在 Robot 上，Task 从 Robot 获取：

```cpp
// Robot 属性
Robot->PatrolPath;     // 巡逻路径
Robot->ScanRadius;     // 到达判定半径（复用）
```

### 使用方式

```cpp
// 1. 设置巡逻路径（必须）
Robot->SetPatrolPath(PatrolPathActor);

// 2. 发送命令
ASC->AddLooseGameplayTag("Command.Patrol");
// 或按 G 键
```

### StateTree 配置

MA Patrol Task 无需配置参数，自动从 Robot 获取

---

## 巡逻路径 Actor (PatrolPath)

### 可视化功能

运行时显示：
- 黄色球体标记每个路径点
- 青色线条连接路径点
- 方向箭头显示巡逻方向
- 绿色圆圈标记起点
- 每个点上方显示编号 (1, 2, 3...)

### 可配置属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `bShowPathAtRuntime` | true | 是否在运行时显示 |
| `PathColor` | Cyan | 路径线颜色 |
| `WaypointColor` | Yellow | 路径点颜色 |
| `WaypointSize` | 30 | 路径点大小 |
| `PathThickness` | 3 | 线条粗细 |
| `bShowWaypointNumbers` | true | 是否显示编号 |


---

## UI 指令输入系统

### 概述

集成了 UIRef 项目的 UI 系统，提供自然语言指令输入界面，支持与外部规划器通信。

### 系统架构

```
MASimpleMainWidget (纯 C++ UI)
    ├── 输入框 (EditableTextBox)
    ├── 发送按钮 (Button)
    └── 结果显示区 (MultiLineEditableTextBox)
            │
            ▼
MACommSubsystem (通信子系统)
    ├── SendNaturalLanguageCommand()
    ├── GenerateMockPlanResponse()
    └── OnPlannerResponse 委托
```

### 核心特性

| 特性 | 说明 |
|------|------|
| **Z 键切换** | 显示/隐藏指令输入界面 |
| **自动聚焦** | UI 显示时自动聚焦到输入框 |
| **回车提交** | 输入框支持回车键快速提交 |
| **模拟响应** | 开发阶段使用模拟数据测试 |
| **中英文支持** | 支持中英文指令输入 |
| **RTS 兼容** | UI 显示时仍支持 RTS 快捷键 |

### 使用方式

1. **显示界面**: 按 Z 键显示指令输入界面
2. **输入指令**: 在输入框中输入自然语言指令
3. **提交指令**: 按回车键或点击发送按钮
4. **查看结果**: 在结果显示区查看系统响应
5. **隐藏界面**: 再次按 Z 键隐藏界面

### 技术实现

- **纯 C++ 实现**: 不依赖蓝图，使用 `WidgetTree` 动态创建 UI
- **继承兼容**: `MAHUD` 继承自 `MASelectionHUD`，保留选择框功能
- **委托通信**: 使用 UE5 委托系统实现组件间通信
- **输入模式**: UI 显示时切换为 `FInputModeGameAndUI` 模式

### 配置选项

通过 `MAGameInstance` 配置：

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `PlannerServerURL` | `http://localhost:8080` | 规划器服务器地址 |
| `bUseMockData` | `true` | 是否使用模拟数据 |
| `bDebugMode` | `true` | 是否启用调试模式 |

### 扩展性

- 支持连接外部规划器服务
- 可扩展指令类型和响应格式
- 支持多语言本地化
- 可添加语音输入功能


---

## 突发事件系统 (Emergency Event)

### 概述

突发事件系统用于处理 Agent 在执行任务过程中发现的异常情况。系统提供事件触发、通知显示、详情查看和用户交互功能。

### 系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        突发事件系统                                          │
│                                                                             │
│  触发方式:                                                                   │
│  ├── 键盘模拟: "-" 键 → ToggleEvent() → 默认 0 号机器狗                      │
│  └── Agent 自动: TriggerEventFromAgent(Agent) → 指定 Agent                   │
│                                                                             │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐       │
│  │ EmergencyManager│ ──► │     MAHUD       │ ──► │ EmergencyWidget │       │
│  │ - SourceAgent   │     │ - 红色指示器    │     │ - 相机画面      │       │
│  │ - bIsEventActive│     │ - Widget 管理   │     │ - 操作按钮      │       │
│  └─────────────────┘     └─────────────────┘     │ - 输入框        │       │
│                                                   └─────────────────┘       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 核心特性

| 特性 | 说明 |
|------|------|
| **"-" 键触发** | 切换事件状态（触发/结束） |
| **"X" 键详情** | 显示/隐藏详情界面（仅事件激活时有效） |
| **红色指示器** | 右上角显示 "突发事件" 文字 |
| **实时视频流** | 显示触发事件 Agent 的相机画面 |
| **解耦设计** | 视频流与 Agent 编号完全解耦 |

### API 设计

```cpp
// 键盘模拟触发（使用默认 Agent）
MA_SUBS.EmergencyManager->ToggleEvent();

// Agent 主动报告事件（未来 AI 自动触发）
MA_SUBS.EmergencyManager->TriggerEventFromAgent(ReportingAgent);

// 查询状态
bool bActive = MA_SUBS.EmergencyManager->IsEventActive();
AMACharacter* Agent = MA_SUBS.EmergencyManager->GetSourceAgent();
```

### 使用方式

1. **触发事件**: 按 "-" 键，右上角显示红色 "突发事件" 指示器
2. **查看详情**: 按 "X" 键，显示详情界面（相机画面 + 操作按钮）
3. **关闭详情**: 再按 "X" 键，隐藏详情界面
4. **结束事件**: 再按 "-" 键，指示器消失

### 详情界面布局

```
┌─────────────────────────────────────────────────────────────┐
│  突发事件详情                                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌────────────┐  │
│  │                 │  │ 信息显示区域    │  │ 扩大搜索范围│  │
│  │   相机画面      │  │                 │  ├────────────┤  │
│  │   (实时视频)    │  │ 按 X 键关闭     │  │ 忽略并返回 │  │
│  │                 │  │ 按 - 键结束事件 │  ├────────────┤  │
│  └─────────────────┘  └─────────────────┘  │ 切换灭火任务│  │
│                                             └────────────┘  │
│  ┌─────────────────────────────────────────┐  ┌────────┐   │
│  │ 输入指令或消息...                        │  │  发送  │   │
│  └─────────────────────────────────────────┘  └────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 扩展性

- **任意 Agent 触发**: 调用 `TriggerEventFromAgent(Agent)` 即可
- **自定义事件类型**: 未来可扩展 `FMAEmergencyEvent` 结构体
- **按钮功能**: 当前为占位符，可绑定实际命令

---

## 任务规划工作台 (Task Planner Workbench)

### 概述

任务规划工作台是一个可视化的 DAG (有向无环图) 编辑器，用于创建、编辑和管理任务规划图。支持从规划器后端接收任务 DAG 并可视化展示，也支持手动编辑和发送自然语言指令。

### 系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MATaskPlannerWidget (主容器)                              │
│  ┌─────────────────────┬───────────────────────────────────────────────────┐│
│  │   Left Panel        │         Right Panel                               ││
│  │  ┌───────────────┐  │  ┌─────────────────────────────────────────────┐ ││
│  │  │ Status Log    │  │  │     DAG Canvas (拓扑排序布局)               │ ││
│  │  │ (只读日志)    │  │  │  ┌─────────┐  ┌─────────┐                  │ ││
│  │  └───────────────┘  │  │  │ Node A  │──│ Node B  │                  │ ││
│  │  ┌───────────────┐  │  │  └─────────┘  └─────────┘                  │ ││
│  │  │ JSON Editor   │  │  │       │                                     │ ││
│  │  │ (可编辑)      │  │  │       ▼                                     │ ││
│  │  └───────────────┘  │  │  ┌─────────┐                                │ ││
│  │  ┌───────────────┐  │  │  │ Node C  │                                │ ││
│  │  │ Update Button │  │  │  └─────────┘                                │ ││
│  │  └───────────────┘  │  └─────────────────────────────────────────────┘ ││
│  │  ┌───────────────┐  │  ┌─────────────────────────────────────────────┐ ││
│  │  │ 指令输入框    │  │  │     Node Palette (工具栏)                   │ ││
│  │  └───────────────┘  │  │  [Navigate] [Patrol] [Custom]               │ ││
│  │  ┌───────────────┐  │  └─────────────────────────────────────────────┘ ││
│  │  │ 发送指令按钮  │  │                                                  ││
│  │  └───────────────┘  │                                                  ││
│  └─────────────────────┴───────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 |
|------|------|
| **MATaskPlannerWidget** | 主容器，管理布局和子组件 |
| **MADAGCanvasWidget** | DAG 画布，渲染节点和边，支持平移/缩放/拖拽 |
| **MATaskNodeWidget** | 任务节点 Widget，显示任务类型、参数、状态 |
| **MANodePaletteWidget** | 节点工具栏，提供可拖拽的节点模板 |
| **MATaskGraphModel** | 数据模型，管理任务图数据和 JSON 序列化 |

### 拓扑排序布局

DAG 画布使用 **Kahn 算法** 进行拓扑排序，实现分层布局：

```
算法流程:
1. 构建邻接表和入度表
2. 将所有入度为 0 的节点加入队列 (第 0 层)
3. BFS 遍历，计算每个节点的层级
4. 按层级分组节点
5. 同层节点水平排列，层与层之间垂直排列

布局效果:
Layer 0:    [Node A]  [Node B]
                │         │
                ▼         ▼
Layer 1:    [Node C]  [Node D]
                │
                ▼
Layer 2:    [Node E]
```

### 画布交互

| 操作 | 功能 |
|------|------|
| 左键拖拽空白区域 | 平移画布 |
| 滚轮 | 缩放画布 (0.25x ~ 2.0x) |
| 左键点击节点 | 选中节点 |
| 左键拖拽节点 | 移动节点 |
| 从输出端口拖拽到输入端口 | 创建边 |
| Delete/Backspace | 删除选中的节点或边 |
| 双击节点 | 编辑节点 |
| 左键点击边 | 选中边 |

### 使用方式

1. **显示工作台**: 按 Z 键
2. **查看任务图**: 从规划器后端接收的 DAG 会自动显示在画布上
3. **手动编辑**: 在 JSON 编辑器中修改，点击 "更新任务图" 按钮
4. **发送指令**: 在指令输入框中输入自然语言，点击 "发送指令" 按钮
5. **隐藏工作台**: 再按 Z 键

### JSON 格式

```json
{
  "description": "巡逻任务规划",
  "nodes": [
    {
      "task_id": "node_1",
      "task_type": "navigate",
      "display_name": "导航到起点",
      "parameters": { "target_x": "100", "target_y": "200" },
      "status": "pending"
    }
  ],
  "edges": [
    { "from_node_id": "node_1", "to_node_id": "node_2", "edge_type": "sequential" }
  ]
}
```

### 数据流程

```
规划器后端 → MACommSubsystem::OnTaskPlanReceived
                    │
                    ▼
            MAHUD::OnTaskPlanReceived()
                    │
                    ▼
            TaskPlannerWidget->LoadTaskGraph(Data)
                    │
                    ├── GraphModel->LoadFromData(Data)
                    ├── SyncJsonEditorFromModel()
                    └── DAGCanvas->RefreshFromModel()
                            │
                            ├── ClearAllNodes()
                            ├── CreateNode() for each node
                            ├── CreateEdge() for each edge
                            └── AutoLayoutNodes() (拓扑排序)
```

### 扩展性

- **自定义节点类型**: 在 NodePalette 中添加新模板
- **自定义边类型**: 支持 sequential, conditional, parallel 等
- **与后端集成**: 通过 MACommSubsystem 发送/接收任务图
