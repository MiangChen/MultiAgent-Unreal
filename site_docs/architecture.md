# 架构

本页目标：
- 快速看懂项目目录结构
- 明确“最底层”与“最上层（UI）”
- 用 2D 分层图理解模块关系

## 1. 项目文件夹 Tree（高层）

```text
MultiAgent-Unreal/
├── config/                     # 仿真配置（map_type、navigation、server 等）
├── datasets/                   # 场景图与示例数据
├── scripts/                    # Mock backend 与工具脚本
├── site_docs/                  # 对外文档站点（GitHub Pages 源）
├── unreal_project/
│   ├── Config/                 # UE 项目配置
│   ├── Content/                # 资源资产
│   └── Source/
│       └── MultiAgent/
│           ├── Core/           # 核心系统（通信、管理器、类型、配置）
│           ├── Agent/          # 机器人角色、技能、StateTree
│           ├── Environment/    # 环境实体/特效/场景辅助
│           ├── Input/          # 输入绑定与 PlayerController
│           ├── UI/             # HUD、Modal、TaskGraph、SkillAllocation
│           └── Utils/          # 通用工具
├── mac_compile_and_start.sh
└── README.md
```

## 2. `Source/MultiAgent` 详细 Tree（核心）

```text
unreal_project/Source/MultiAgent/
├── Core/
│   ├── Types/
│   ├── Config/
│   ├── Comm/
│   ├── GameFlow/
│   └── Manager/
│       ├── scene_graph_ports/
│       ├── scene_graph_adapters/
│       ├── scene_graph_services/
│       └── ue_tools/
├── Agent/
│   ├── Character/
│   ├── Component/
│   │   └── Sensor/
│   ├── Skill/
│   │   ├── Impl/
│   │   └── Utils/
│   └── StateTree/
│       ├── Task/
│       └── Condition/
├── Environment/
│   ├── Entity/
│   ├── Effect/
│   └── Utils/
├── Input/
│   ├── Application/
│   ├── Domain/
│   └── Infrastructure/
├── UI/
│   ├── Core/
│   ├── HUD/
│   ├── Modal/
│   ├── Mode/
│   │   ├── Application/
│   │   ├── Domain/
│   │   └── Infrastructure/
│   ├── TaskGraph/
│   ├── SkillAllocation/
│   │   └── Gantt/
│   ├── Components/
│   └── Setup/
└── Utils/
```

## 3. 五层映射（按 L0~L4）

按你给出的 5 层模型，把当前代码先映射为：

| 层级 | 代码映射（当前） | 说明 |
|---|---|---|
| L0 Presentation | `UI/`（Widget/Modal/HUD 壳层）, `Input/` | 交互入口、HUD 壳层、按键鼠标事件 |
| L1 Application | `Core/Manager/`, `Core/GameFlow/`, `UI/HUD/Application/` | 用例编排、调度、Facade/协调逻辑 |
| L2 Domain | `Agent/`, `Environment/`, `Core/Types/` | 机器人/环境核心规则、状态与领域模型 |
| L3 Infrastructure | `Core/Comm/`, `Core/Config/`, `Core/Manager/scene_graph_adapters`, `Core/Manager/ue_tools` | HTTP/轮询、配置/文件、UE 适配器与外部依赖 |
| L4 Bootstrap | `MultiAgent.cpp` + Subsystem 初始化装配点 | 模块启动与依赖组装，不承载业务规则 |

当前判断：
- **最底层（业务意义）**：`L2 Domain`（`Agent/Environment/Core/Types`）
- **最上层（接近 UI）**：`L0 Presentation`（`UI/Input`）
- 备注：**目录位置不等于逻辑分层**，例如 `UI/HUD/Application` 物理上在 `UI/` 下，但逻辑职责属于 `L1 Application`（编排/协调）。

## 4. 2D 分层架构图（含跨层连接现状）

### 4.1 全局五层图（当前）

```mermaid
flowchart TB
    EXT["External Planner / Web"]

    subgraph L4["L4 Bootstrap"]
        BOOT["MultiAgent.cpp / Subsystem Wiring"]
    end

    subgraph L0["L0 Presentation"]
        UI["UI/"]
        INPUT["Input/"]
    end

    subgraph L1["L1 Application"]
        APP_MGR["Core/Manager/"]
        APP_FLOW["Core/GameFlow/"]
    end

    subgraph L2["L2 Domain"]
        DOM_AGENT["Agent/"]
        DOM_ENV["Environment/"]
        DOM_TYPES["Core/Types/"]
    end

    subgraph L3["L3 Infrastructure"]
        INF_COMM["Core/Comm/"]
        INF_CFG["Core/Config/"]
        INF_ADAPTER["scene_graph_adapters / ue_tools"]
    end

    BOOT --> UI
    BOOT --> APP_MGR
    BOOT --> INF_COMM

    INPUT --> UI
    UI --> APP_MGR
    APP_FLOW --> APP_MGR
    APP_MGR --> DOM_AGENT
    APP_MGR --> DOM_ENV
    APP_MGR --> DOM_TYPES

    APP_MGR --> INF_COMM
    APP_MGR --> INF_CFG
    APP_MGR --> INF_ADAPTER
    INF_COMM --> DOM_TYPES
    INF_ADAPTER --> DOM_TYPES
    EXT --> INF_COMM

    UI -. "cross-layer (current)" .-> INF_COMM
    UI -. "cross-layer (current)" .-> DOM_AGENT
    INPUT -. "cross-layer (current)" .-> DOM_AGENT
    INF_COMM -. "event callback (current)" .-> UI
```

### 4.2 MAHUD 重构后内部图（当前）

```mermaid
flowchart LR
    PC["AMAPlayerController"]
    HUD["AMAHUD (orchestrator)"]
    UIM["UMAUIManager"]
    COMM["MACommSubsystem"]
    EDIT["UMAEditModeManager"]
    SCENE["UMASceneGraphManager"]
    PIP["MAPIPCameraManager"]

    LIFE["FMAHUDLifecycleCoordinator"]
    DELEGATE["FMAHUDDelegateCoordinator"]
    PANEL["FMAHUDPanelCoordinator"]
    VIEW["FMAHUDViewCoordinator"]
    WIDGET["FMAHUDWidgetCoordinator"]
    BACKEND["FMAHUDBackendCoordinator"]
    OVERLAY["FMAHUDOverlayCoordinator"]
    SCEDIT["FMAHUDSceneEditCoordinator"]

    PC --> HUD
    HUD --> LIFE
    HUD --> DELEGATE
    HUD --> PANEL
    HUD --> VIEW
    HUD --> WIDGET
    HUD --> BACKEND
    HUD --> OVERLAY
    HUD --> SCEDIT

    LIFE --> UIM
    DELEGATE --> UIM
    DELEGATE --> PC
    PANEL --> UIM
    VIEW --> UIM
    WIDGET --> UIM
    BACKEND --> COMM
    BACKEND --> UIM
    OVERLAY --> EDIT
    OVERLAY --> SCENE
    OVERLAY --> PIP
    SCEDIT --> EDIT
    SCEDIT --> SCENE
```

### 4.3 UIManager 分层图（当前）

```mermaid
flowchart LR
    HUD["AMAHUD / Controller"]
    UIM["UMAUIManager (L0 Facade)"]

    subgraph L1["L1 Application Coordinators"]
        LIFE["FMAUIWidgetLifecycleCoordinator"]
        REG["FMAUIWidgetRegistryCoordinator"]
        INTERACT["FMAUIWidgetInteractionCoordinator"]
        STATE["FMAUIStateModalCoordinator"]
        RUNTIME["FMAUIRuntimeEventCoordinator"]
        THEME["FMAUIThemeCoordinator"]
        NOTI["FMAUINotificationCoordinator"]
    end

    subgraph L3["L3 Infrastructure"]
        INPUT["FMAUIInputModeAdapter"]
    end

    HUD --> UIM
    UIM --> LIFE
    UIM --> REG
    UIM --> INTERACT
    UIM --> STATE
    UIM --> RUNTIME
    UIM --> THEME
    UIM --> NOTI
    UIM --> INPUT

    LIFE --> REG
    INTERACT --> INPUT
    STATE --> INPUT
    RUNTIME --> NOTI
    STATE --> NOTI
```

说明（本轮改动）：
- `CreateAllWidgets` 已拆成 `CreateHUD/CreatePlanner/CreateMode/CreatePanel/BindRuntimeEventSources` 五段，降低单函数复杂度。
- `UIManager` 新增受控写接口（`SetWidgetInstanceInternal`、`SetModalWidgetInternal`、`Register*Internal`、`Set*ThemeInternal`），用于收敛 coordinator 对私有字段的直接写入。
- `friend` 已清零；Coordinator 与 UIManager 的连接改为显式公开桥接接口（`SetInputMode*` + `On*` 委托回调）。
- `WidgetInteractionCoordinator` 已移除对 `UIManager` 私有字段直连，仅通过 `Get*` 接口访问状态。

### 4.4 Comm 协议层拆分图（当前）

```mermaid
flowchart LR
    subgraph L2["L2 Domain (协议模型)"]
        UMB["MACommTypes.h (Umbrella)"]
        D0["Domain/MACommBaseTypes.h"]
        D1["Domain/MACommTaskPlanTypes.h"]
        D2["Domain/MACommSkillTypes.h"]
        D3["Domain/MACommSceneTypes.h"]
        D4["Domain/MACommResponseTypes.h"]
    end

    subgraph L3["L3 Infrastructure (Codec)"]
        C0["MACommJsonCodec.h/.cpp (Facade)"]
        C1["MACommEnvelopeCodec.cpp"]
        C2["MACommBasicMessagesCodec.cpp"]
        C3["MACommTaskPlanCodec.cpp"]
        C4["MACommSkillCodec.cpp"]
        C5["MACommSceneAndResponseCodec.cpp"]
        C6["MACommTypeHelper.h/.cpp"]
    end

    subgraph L1["L1 Application 调用方"]
        O1["MACommOutbound.cpp"]
        O2["MACommInbound.cpp"]
        O3["MACommSubsystem.cpp"]
        O4["MAHUDBackendCoordinator.cpp"]
        O5["MACommandManager.cpp"]
    end

    UMB --> D0
    UMB --> D1
    UMB --> D2
    UMB --> D3
    UMB --> D4

    C0 --> D0
    C0 --> D1
    C0 --> D2
    C0 --> D3
    C0 --> D4
    C0 --> C1
    C0 --> C2
    C0 --> C3
    C0 --> C4
    C0 --> C5
    C1 --> C6

    O1 --> C0
    O2 --> C0
    O3 --> C0
    O4 --> C0
    O5 --> C0
```

### 4.5 MAEditWidget 分层图（当前）

```mermaid
flowchart LR
    HUD["AMAHUD / FMAHUDOverlayCoordinator"]
    STATECOORD["FMAEditWidgetStateCoordinator"]
    INDICATOR["FMAHUDEditModeIndicatorBuilder"]

    subgraph L0["L0 Presentation"]
        WIDGET["UMAEditWidget"]
    end

    subgraph L1["L1 Application"]
        COORD["FMAEditWidgetCoordinator"]
    end

    subgraph L2["L2 Domain"]
        MODEL["FMAEditWidgetViewModel"]
        STATE["FMAEditWidgetSelectionState"]
    end

    subgraph L3["L3 Infrastructure"]
        ADAPTER["FMAEditWidgetSceneGraphAdapter"]
        ACTION["FMAEditSceneActionAdapter"]
    end

    subgraph CORE["Existing Core Services"]
        EDITMODE["UMAEditModeManager"]
        SCENE["UMASceneGraphManager"]
        SCENEACT["FMAHUDSceneEditCoordinator"]
    end

    HUD --> COORD
    HUD --> INDICATOR
    COORD --> WIDGET
    COORD --> STATECOORD
    STATECOORD --> WIDGET
    WIDGET -->|confirm/delete/create/node-switch intents| HUD

    COORD --> MODEL
    COORD --> STATE
    COORD --> ADAPTER
    SCENEACT --> ACTION
    ADAPTER --> EDITMODE
    ADAPTER --> SCENE
    ACTION --> EDITMODE
    ACTION --> SCENE
    HUD --> SCENEACT
```

说明（本轮改动）：
- `UMAEditWidget` 已收缩为 `L0 Presentation`：只负责 `BuildUI`、渲染 `ViewModel`、发送用户 intent。
- 选择态、按钮显示规则、当前 node 标签与 JSON 展示内容已上提到 `FMAEditWidgetCoordinator + Domain Model`。
- `SceneGraphManager` 查询、Actor/Node 解析、JSON 文档校验已下沉到 `FMAEditWidgetSceneGraphAdapter`。
- `FMAEditWidgetStateCoordinator` 已补齐，使 edit 这条线也具备明确的 widget state apply/reset 角色，而不是由 coordinator 直接写 widget。
- `FMASceneSelectionDisplay` 已开始承接 `Edit/Modify` 共用的 selection 文案生成，避免在两个 flow 中继续复制“空选中/单选/多选”提示逻辑。
- `FMAHUDOverlayCoordinator` 现在持有 `FMAEditWidgetCoordinator`，统一处理“选择变化 / 场景图变化 / Widget intent”。
- `FMAHUDOverlayCoordinator` 内部也已补上统一的 `ResolveEditWidget + BindEditWidgetDelegates` helper，避免重复的 widget 获取和 delegate 绑定散落在多个入口函数里。
- `DrawEditModeIndicator` 中原本混在 `OverlayCoordinator` 里的 POI/Goal/Zone 列表拼装，已下沉到 `FMAHUDEditModeIndicatorBuilder`，让 `OverlayCoordinator` 更接近纯展示编排层。
- `FMAHUDSceneEditCoordinator` 的主要 edit 动作已下沉到 `FMAEditSceneActionAdapter`，HUD 侧只保留编排、通知和少量 UI 刷新。

### 4.6 MAModifyWidget 分层图（当前）

```mermaid
flowchart LR
    HUD["AMAHUD / FMAHUDSceneEditCoordinator"]
    PANEL["FMAHUDPanelCoordinator"]
    LIFE["FMAHUDModeWidgetLifecycleCoordinator"]
    RES["FMAHUDSceneActionResultCoordinator"]
    LISTSEL["FMAHUDSceneListSelectionCoordinator"]
    RESULT["FMASceneActionResult"]

    subgraph L0["L0 Presentation"]
        WIDGET["UMAModifyWidget"]
    end

    subgraph L1["L1 Application"]
        COORD["FMAModifyWidgetCoordinator"]
        STATECOORD["FMAModifyWidgetStateCoordinator"]
    end

    subgraph L2["L2 Domain"]
        SELVM["FMAModifySelectionViewModel"]
        PREVM["FMAModifyPreviewModel"]
        MODEL["FMAModifyWidgetModel"]
    end

    subgraph L3["L3 Infrastructure"]
        SGADAPTER["FMAModifyWidgetSceneGraphAdapter"]
        PARSER["FMAModifyWidgetInputParser"]
        NODEBUILDER["FMAModifyWidgetNodeBuilder"]
        ACTION["FMAModifySceneActionAdapter"]
    end

    subgraph CORE["Existing Core Services"]
        SCENE["UMASceneGraphManager"]
        GEO["FMAGeometryUtils"]
    end

    PANEL --> LIFE
    LIFE --> WIDGET
    HUD --> WIDGET
    HUD --> RES
    HUD --> LISTSEL
    WIDGET -->|single/multi modify confirm| HUD

    WIDGET --> COORD
    RES --> STATECOORD
    ACTION --> RESULT
    RES --> RESULT
    WIDGET --> MODEL
    WIDGET --> SGADAPTER
    COORD --> PARSER
    COORD --> NODEBUILDER
    COORD --> SGADAPTER
    STATECOORD --> WIDGET
    MODEL --> SELVM
    SGADAPTER --> PREVM
    HUD --> ACTION
    ACTION --> RES
    LISTSEL --> WIDGET

    PARSER --> SCENE
    SGADAPTER --> SCENE
    NODEBUILDER --> SCENE
    NODEBUILDER --> GEO
    ACTION --> SCENE
```

说明（本轮改动）：
- `UMAModifyWidget` 已不再自己做节点匹配、多选共享节点判断、JSON 预览拼装和编辑态 JSON 合成；这些已下沉到 `FMAModifyWidgetSceneGraphAdapter`。
- `FMAModifyWidgetModel` 负责把“当前选中 Actor 集合 + 是否命中现有节点”收敛成 `SelectionViewModel`，Widget 只做 UI 应用。
- `ParseAnnotationInput/ParseAnnotationInputV2` 和分类选择校验已下沉到 `FMAModifyWidgetInputParser`。
- `GetNextAvailableId`、默认属性组装、`GenerateSceneGraphNode*` 和几何/标签拼装已进一步下沉到 `FMAModifyWidgetNodeBuilder`。
- `GetNextAvailableId` 不再由 Widget 自己读 JSON 文件，而是统一复用 `UMASceneGraphManager::GetNextAvailableId()`。
- `OnConfirmButtonClicked` 的提交分流（解析输入、选择校验、决定 single/multi/edit 路径）已上提到 `FMAModifyWidgetCoordinator`，并由它直接组合 `InputParser + NodeBuilder + SceneGraphAdapter`。
- `MAModifyWidget` 本身已删除这些业务 wrapper，不再充当 application/infrastructure 的转发层。
- `FMAModifyWidgetStateCoordinator` 负责“把 selection / action result 应回 widget”，HUD 不再直接拼 `ClearSelection/SetSelectedActors/SetLabelText` 这类恢复细节。
- `FMAHUDModeWidgetLifecycleCoordinator` 已接管 edit/modify 面板 show/hide/reset 生命周期，`FMAHUDPanelCoordinator` 不再直接操作 widget 内部状态。
- `FMAHUDSceneListSelectionCoordinator` 已接管 scene-list 点击后的 Goal/Zone 选中解析，`FMAHUDSceneEditCoordinator` 不再直接查询 `EditModeManager/SceneGraphManager` 做二次解析。
- `FMASceneSelectionDisplay` 现在同时服务于 `Edit` 和 `Modify` 的 selection hint 生成，作为后续进一步统一 selection domain model 的落脚点。
- HUD 侧的 modify 保存/更新逻辑已下沉到 `FMAModifySceneActionAdapter`；edit/modify 两条线现在共享 `FMASceneActionResult`，并统一通过 `FMAHUDSceneActionResultCoordinator` 落到 HUD/UI。
- `FMAHUDSceneEditCoordinator` 现在更接近纯入口编排层。
- 因此 `MAModifyWidget + Modify flow` 已形成较完整的 `L0/L1/L2/L3` 分层；`Edit/Modify` 的结果模型、widget lifecycle、selection apply 模式都已完成第一阶段横向统一。

### 4.7 Input 分层图（当前）

```mermaid
flowchart LR
    subgraph L0["L0 Presentation"]
        PC["AMAPlayerController"]
        ACT["UMAInputActions"]
    end

    subgraph L1["L1 Application"]
        MODE["FMAMouseModeCoordinator"]
        HUDSHORT["FMAHUDShortcutCoordinator"]
        CMD["FMACommandInputCoordinator"]
        UTIL["FMAAgentUtilityInputCoordinator"]
        DEPLOY["FMADeploymentInputCoordinator"]
        RTS["FMARTSSelectionInputCoordinator"]
        CAMERA["FMACameraInputCoordinator"]
        SQUAD["FMASquadInputCoordinator"]
        MODIFY["FMAModifyInputCoordinator"]
        EDIT["FMAEditInputCoordinator"]
    end

    subgraph L2["L2 Domain"]
        MT["EMAMouseMode"]
        DEPLOYT["FMAPendingDeployment"]
    end

    subgraph L3["L3 Infrastructure"]
        HUDADAPTER["FMAHUDInputAdapter"]
        HL["FMAActorHighlightAdapter"]
    end

    subgraph CORE["Existing Core Services"]
        HUD["AMAHUD / UMAHUDStateManager"]
        SEL["UMASelectionManager"]
        CMDMGR["UMACommandManager"]
        SQMGR["UMASquadManager"]
        EDITMGR["UMAEditModeManager"]
        AGENTMGR["UMAAgentManager"]
        VIEW["UMAViewportManager"]
    end

    ACT --> PC
    PC --> MODE
    PC --> HUDSHORT
    PC --> CMD
    PC --> UTIL
    PC --> DEPLOY
    PC --> RTS
    PC --> CAMERA
    PC --> SQUAD
    PC --> MODIFY
    PC --> EDIT

    MODE --> MT
    DEPLOY --> DEPLOYT

    HUDSHORT --> HUDADAPTER
    MODIFY --> HL
    MODIFY --> HUDADAPTER
    EDIT --> HUDADAPTER

    HUDADAPTER --> HUD
    CMD --> CMDMGR
    UTIL --> AGENTMGR
    UTIL --> SEL
    DEPLOY --> AGENTMGR
    DEPLOY --> SEL
    RTS --> SEL
    CAMERA --> VIEW
    CAMERA --> AGENTMGR
    SQUAD --> SEL
    SQUAD --> SQMGR
    MODIFY --> SEL
    EDIT --> EDITMGR

    PC -. "remaining coupling" .-> CMDMGR
```

说明（本轮改动）：
- `AMAPlayerController` 已从输入 God Object 收缩为壳层 + 路由层：当前约 `1905 -> 464 LOC`。
- `EMAMouseMode` 与 `FMAPendingDeployment` 已下沉到 `Input/Domain/MAInputTypes.h`，不再内嵌在 `PlayerController` 头文件中。
- `SetupInputComponent` 已拆成 `BindPointerActions / BindGameplayActions / BindHUDActions` 三段，明确属于 `L4 bootstrap` 的输入绑定装配职责。
- `FMAHUDShortcutCoordinator` 已接管 `CheckTask/CheckSkill/CheckDecision` 与右侧面板快捷键，不再让 `PlayerController` 直穿 `HUD -> UIManager -> HUDStateManager`。
- `FMAMouseModeCoordinator` 已统一 `Select / Deployment / Modify / Edit` 模式切换，去掉重复的 enter/exit 副作用。
- `FMAAgentUtilityInputCoordinator` 已接管调试/生成占位输入和选中单位跳跃，`PlayerController` 不再承载这些杂项快捷键的具体行为。
- `FMARTSSelectionInputCoordinator` 已接管 RTS 选择框生命周期、单击/框选判定、HUD 框选同步，以及中键导航逻辑。
- `FMACameraInputCoordinator` 已接管右键旋转、相机切换、返回观察者、拍照和 TCP 推流切换。
- `FMASquadInputCoordinator` 已接管控制组、编队切换、创建/解散 Squad 的快捷键流程。
- `FMAModifyInputCoordinator` 与 `FMAEditInputCoordinator` 已接管 Modify/Edit 场景点击行为；Actor 高亮递归逻辑已下沉到 `FMAActorHighlightAdapter`。
- `FMADeploymentInputCoordinator` 已接管部署队列、进入/退出部署模式、框选投放、地面投影与 spawn 流程；`PlayerController` 不再直接持有这段大逻辑。
- 旧的无效输入概念已清理：`IA_ToggleMainUI`、`IA_ToggleSkillAllocationViewer`、`IA_StartPatrol`、`IA_StartCoverage`、`IA_StartAvoid` 已从 `InputActions` 和 `PlayerController` 里移除。
- 当前还保留在 `PlayerController` 的主要职责是输入绑定入口、Subsystem 初始化，以及少量 `CommandManager` façade 调用；主交互流已经进入 `Application` 层。

## 5. 控制论目标架构（长期）

这部分不是“当前实现”，而是后续重构的长期 target。

核心思想：
- 前向通道严格逐层：`L1 -> L2 -> L3 -> L4 -> L5`
- 反馈通道也严格逐层：`L5 -> FB54 -> L4 -> FB43 -> L3 -> FB32 -> L2 -> FB21 -> L1`
- 不允许跳层前向依赖，也不允许跳层 feedback
- `feedback` 只传标准化 `DTO / Snapshot / Result / ViewModel`
- `Composition Root / Bootstrap` 负责装配，但不承载业务规则
- 只有真正触达 `L5` 的运行时动作，才要求走完整 `FB54 -> FB43 -> FB32 -> FB21` 链；纯 `L2/L3` 编排或 UI 状态变更，可从最近层级直接产出 `FB21`

### 5.1 控制论五层图（目标）

```mermaid
flowchart LR
    CR["Composition Root / Bootstrap"]

    L1["L1 Interface / Input / UI"]
    L2["L2 Controller / Orchestration"]
    L3["L3 Domain / Policy / State"]
    L4["L4 Adapter / Actuator / Sensor"]
    L5["L5 Plant / UE Runtime / Managers / Agents"]

    FB21["Feedback21\nViewModel / Notice / UIState"]
    FB32["Feedback32\nStateChange / DecisionResult"]
    FB43["Feedback43\nExecutionResult / Observation"]
    FB54["Feedback54\nPlantState / SensorSnapshot"]

    CR --> L1
    CR --> L2
    CR --> L4

    L1 --> L2
    L2 --> L3
    L3 --> L4
    L4 --> L5

    L5 --> FB54 --> L4
    L4 --> FB43 --> L3
    L3 --> FB32 --> L2
    L2 --> FB21 --> L1
```

### 5.2 文件夹映射（目标）

| 控制层 | 目标 folder | 说明 |
|---|---|---|
| `L1` | `Input/`、`UI/` | 输入入口、HUD、Widget、Web/UI 交互入口 |
| `L2` | `Core/Interaction/Application/` | 编排、router、workflow、mode controller |
| `L3` | `Core/Interaction/Domain/` | 规则、状态、policy、snapshot、intent |
| `L4` | `Core/Interaction/Infrastructure/` | Unreal adapter、sensor、actuator、query bridge |
| `L5` | `Core/Manager/`、`Agent/`、`Environment/` | 真实运行系统、Subsystem、Agent、场景与 UE runtime |
| `FB` | `Core/Interaction/Feedback/` | 标准化反馈 DTO：`FB21/FB32/FB43/FB54` |
| `CR` | `Core/Interaction/Bootstrap/` | 显式组装入口，如 `MAInteractionBootstrap` |

### 5.3 规则清单（目标）

| 规则 | 内容 |
|---|---|
| 前向规则 | 只允许 `L1 -> L2 -> L3 -> L4 -> L5` |
| 反馈规则 | 只允许逐层 feedback，不允许 `L5 -> L2`、`L4 -> L1` 这类回跳 |
| 依赖规则 | `L1` 不得直接依赖 `L3/L4/L5`；`L3` 不得直接依赖 `L5` |
| 载荷规则 | feedback 中禁止传 `UObject* / AActor* / Manager*`，只传 DTO/snapshot/result |
| 装配规则 | `Bootstrap` 可见全局，但只做构造、注入、绑定，不写业务判断 |
| 命名规则 | 文件名应体现职责：`*UseCase`、`*State`、`*Policy`、`*Adapter`、`*Feedback` |

### 5.4 后续重构任务表（Target Backlog）

| 阶段 | 任务 | 目标 folder / 模块 | 完成标准 | 当前状态 |
|---|---|---|---|---|
| `T1` | 建立控制论目录骨架 | `Core/Interaction/{Application,Domain,Infrastructure,Feedback}` | 目录存在，职责说明写入 docs | 已完成 |
| `T2` | 将 `Input/Application` 上移到 `Core/Interaction/Application` | `Input/`、`Core/Interaction/Application/` | `Input/` 只剩 `PlayerController`、`InputActions` 等入口壳层 | 已完成 |
| `T3` | 将 `Input/Domain` 收敛为交互域模型 | `Core/Interaction/Domain/` | `MouseModeState`、`DeploymentQueue`、`SelectionSnapshot`、`CommandIntent` 等进入统一 domain | 已完成 |
| `T4` | 将 `Input/Infrastructure` 扩展为统一 Unreal bridge | `Core/Interaction/Infrastructure/` | `HUD / Agent / Selection / Squad / Camera / Edit / HitTest` 全部通过 adapter 访问 | 已完成 |
| `T5` | 建立 `FB21 / FB32 / FB43 / FB54` 反馈 DTO | `Core/Interaction/Feedback/` | UI/ViewModel、StateChange、ExecutionResult、PlantSnapshot 反馈类型稳定 | 已完成 |
| `T6` | HUD/UI 改为只消费 `FB21` 结果 | `UI/`、`UI/HUD/Application/` | UI 不再直接查询深层 manager，统一消费上游反馈结果 | 已完成 |
| `T7` | SceneEdit / Modify / Input 三条线统一进入同一交互通道 | `UI/Mode/`、`Input/`、`Core/Interaction/` | 不再各自维护一套独立交互编排模型 | 已完成 |
| `T8` | 建立架构守卫 | CI / 静态检查 / include 规则 | 禁止新增跳层 include 和跳层 feedback | 已完成 |

### 5.5 当前实现状态（2026-03-09）

本页 `T1-T8` 对应的控制论 backlog 已完成到当前仓库的目标形态，当前实现具备以下特征：

1. `Input` 主线已经收敛成：
   - `L1`: `Input/MAPlayerController.*`
   - `L2`: `Core/Interaction/Application/*`
   - `L3`: `Core/Interaction/Domain/*`
   - `L4`: `Core/Interaction/Infrastructure/*`
   - `CR`: `Core/Interaction/Bootstrap/MAInteractionBootstrap.*`
2. `PlayerController` 已不再持有裸的 `CurrentMouseMode / PreviousMouseMode / DeploymentQueue / HighlightedActors`，统一收敛到：
   - `FMAMouseModeState`
   - `FMADeploymentQueueState`
   - `FMAModifySelectionState`
3. `UI/HUD/Application` 与 `UI/Mode/Application` 已去掉直接 `GetWorld/GetSubsystem` 查询，改为通过：
   - `UI/HUD/Infrastructure/MAHUDEditRuntimeAdapter.*`
   - `UI/HUD/Infrastructure/MAHUDBackendRuntimeAdapter.*`
   - `UI/HUD/Infrastructure/MAHUDSceneActionRuntimeAdapter.*`
   - `UI/Mode/Infrastructure/MAEditWidgetRuntimeAdapter.*`
4. `EditWidget` 的业务动作已统一改为 `ActionRequest -> SceneEditCoordinator` 通道，不再保留旧的 HUD 直执行业务回调链。
5. `FB21` 已是统一的 UI 反馈通道；命令派发链路已落地 `FB54 -> FB43 -> FB32 -> FB21`。其余纯编排动作按本节规则允许在最近层级直接产出 `FB21`，不再为了“凑层级”伪造 `L5` 观测。
6. `scripts/check_interaction_architecture.py` 已升级为架构守卫，当前会阻止：
   - `Application` 层重新出现 `GetWorld/GetSubsystem` 等运行时查询
   - `Application` 层重新 include `Manager / Agent / Environment`
   - `Domain` 层重新引入 `UWorld / AActor / Manager` 等运行时类型
   - `PlayerController` 重新出现旧的裸状态字段

### 5.6 当前实现类图（2026-03-09）

```mermaid
classDiagram
    class MAInteractionBootstrap {
      +InitializePlayerController()
    }

    class AMAPlayerController {
      +ApplyFeedback()
      +GetCurrentMouseMode()
    }

    class FMAMouseModeState
    class FMADeploymentQueueState
    class FMAModifySelectionState

    class FMACommandInputCoordinator
    class FMAMouseModeCoordinator
    class FMADeploymentInputCoordinator
    class FMARTSSelectionInputCoordinator
    class FMACameraInputCoordinator
    class FMASquadInputCoordinator
    class FMAHUDShortcutCoordinator
    class FMAEditInputCoordinator
    class FMAModifyInputCoordinator

    class FMAInteractionRuntimeAdapter
    class FMAHUDInputAdapter
    class FMAActorHighlightAdapter

    class FMAFeedback54
    class FMAFeedback43
    class FMAFeedback32
    class FMAFeedback21Batch
    class FMAFeedbackPipeline
    class FMAFeedback21Applier

    class AMAHUD
    class FMAHUDOverlayCoordinator
    class FMAHUDSceneEditCoordinator
    class FMAHUDSceneActionResultCoordinator
    class FMAHUDBackendCoordinator
    class FMAHUDEditRuntimeAdapter
    class FMAHUDBackendRuntimeAdapter
    class FMAHUDSceneActionRuntimeAdapter

    class UMAEditWidget
    class FMAEditWidgetCoordinator
    class FMAEditWidgetRuntimeAdapter
    class FMAEditWidgetActionRequest
    class UMAUIManager

    MAInteractionBootstrap --> AMAPlayerController : initialize

    AMAPlayerController *-- FMAMouseModeState
    AMAPlayerController *-- FMADeploymentQueueState
    AMAPlayerController *-- FMAModifySelectionState

    AMAPlayerController *-- FMACommandInputCoordinator
    AMAPlayerController *-- FMAMouseModeCoordinator
    AMAPlayerController *-- FMADeploymentInputCoordinator
    AMAPlayerController *-- FMARTSSelectionInputCoordinator
    AMAPlayerController *-- FMACameraInputCoordinator
    AMAPlayerController *-- FMASquadInputCoordinator
    AMAPlayerController *-- FMAHUDShortcutCoordinator
    AMAPlayerController *-- FMAEditInputCoordinator
    AMAPlayerController *-- FMAModifyInputCoordinator

    AMAPlayerController --> FMAFeedback21Applier : ApplyFeedback

    FMACommandInputCoordinator --> FMAInteractionRuntimeAdapter
    FMACommandInputCoordinator --> FMAFeedbackPipeline

    FMAMouseModeCoordinator --> FMAInteractionRuntimeAdapter
    FMADeploymentInputCoordinator --> FMAInteractionRuntimeAdapter
    FMARTSSelectionInputCoordinator --> FMAInteractionRuntimeAdapter
    FMACameraInputCoordinator --> FMAInteractionRuntimeAdapter
    FMASquadInputCoordinator --> FMAInteractionRuntimeAdapter
    FMAEditInputCoordinator --> FMAInteractionRuntimeAdapter
    FMAModifyInputCoordinator --> FMAInteractionRuntimeAdapter
    FMAModifyInputCoordinator --> FMAActorHighlightAdapter

    FMAInteractionRuntimeAdapter --> FMAFeedback54 : emits
    FMAFeedbackPipeline --> FMAFeedback43 : build
    FMAFeedbackPipeline --> FMAFeedback32 : build
    FMAFeedbackPipeline --> FMAFeedback21Batch : build

    AMAHUD *-- FMAHUDOverlayCoordinator
    AMAHUD *-- FMAHUDSceneEditCoordinator
    AMAHUD *-- FMAHUDSceneActionResultCoordinator
    AMAHUD *-- FMAHUDBackendCoordinator
    AMAHUD --> UMAUIManager

    FMAHUDOverlayCoordinator --> FMAHUDEditRuntimeAdapter
    FMAHUDOverlayCoordinator --> FMAEditWidgetCoordinator
    FMAHUDOverlayCoordinator --> UMAEditWidget : resolve/bind

    FMAEditWidgetCoordinator --> FMAEditWidgetRuntimeAdapter
    FMAEditWidgetCoordinator --> FMAEditWidgetActionRequest

    FMAHUDSceneEditCoordinator --> FMAHUDSceneActionRuntimeAdapter
    FMAHUDSceneEditCoordinator --> FMAHUDSceneActionResultCoordinator
    FMAHUDBackendCoordinator --> FMAHUDBackendRuntimeAdapter
    FMAHUDSceneActionResultCoordinator --> FMAFeedback21Applier : ApplyToHUD
```

### 5.7 当前实现 folder 图（2026-03-09）

```mermaid
flowchart TB
    subgraph CR["Composition Root"]
        BOOT["Core/Interaction/Bootstrap/"]
    end

    subgraph L1["L1 Entry / Presentation"]
        INPUT["Input/"]
        UIHUD["UI/HUD/"]
        UIMODE["UI/Mode/"]
        UICORE["UI/Core/"]
    end

    subgraph L2["L2 Application"]
        INTAPP["Core/Interaction/Application/"]
        HUDAPP["UI/HUD/Application/"]
        MODEAPP["UI/Mode/Application/"]
    end

    subgraph L3["L3 Domain / Feedback"]
        INTDOMAIN["Core/Interaction/Domain/"]
        FEEDBACK["Core/Interaction/Feedback/"]
        HUDDOMAIN["UI/HUD/Domain/"]
        MODEDOMAIN["UI/Mode/Domain/"]
    end

    subgraph L4["L4 Infrastructure"]
        INTINFRA["Core/Interaction/Infrastructure/"]
        HUDINFRA["UI/HUD/Infrastructure/"]
        MODEINFRA["UI/Mode/Infrastructure/"]
    end

    subgraph L5["L5 Plant / Runtime"]
        MANAGER["Core/Manager/"]
        AGENT["Agent/"]
        ENV["Environment/"]
        COMM["Core/Comm/"]
    end

    BOOT --> INPUT
    BOOT --> INTAPP
    BOOT --> INTINFRA

    INPUT --> INTAPP

    UICORE --> UIHUD
    UICORE --> UIMODE
    UIHUD --> HUDAPP
    UIMODE --> MODEAPP

    INTAPP --> INTDOMAIN
    INTAPP --> FEEDBACK
    INTAPP --> INTINFRA

    HUDAPP --> HUDDOMAIN
    HUDAPP --> HUDINFRA
    HUDAPP --> MODEAPP
    HUDAPP --> FEEDBACK

    MODEAPP --> MODEDOMAIN
    MODEAPP --> MODEINFRA

    INTINFRA --> INTDOMAIN
    INTINFRA --> FEEDBACK
    INTINFRA --> MANAGER
    INTINFRA --> AGENT
    INTINFRA --> ENV
    INTINFRA --> COMM

    HUDINFRA --> MANAGER
    HUDINFRA --> AGENT
    HUDINFRA --> ENV
    HUDINFRA --> COMM

    MODEINFRA --> MANAGER
    MODEINFRA --> AGENT
    MODEINFRA --> ENV
```

### 5.8 当前结论

1. 以本页定义的控制论五层规则为准，当前仓库已经达到本轮目标形态，可作为后续重构的稳定基线。
2. 后续工作不再是“补齐骨架”，而是针对具体业务继续复用这套骨架，例如把更多 runtime 动作接入完整 feedback 链，或进一步纯化 `UI/Mode/Domain`。
3. 如果未来继续演进，默认要求是：先复用本页已有 `L1-L5 + Feedback + Bootstrap + Guard` 结构，而不是再引入新的平行交互通道。

## 6. 重构建议

### 6.1 总体原则

- 图中的**实线**是目标依赖方向（推荐长期保留）。
- 图中的**虚线**是当前项目里已存在的跨层直连（本页保留展示，不做粉饰）。
- 如果后续要做“严格五层”，优先消除 `L0 -> L2/L3` 的直连，把入口收敛到 `L1`。
- 如果后续进入“控制论重构阶段”，则以本页 `5.x` 的 `L1-L5 + Feedback` 为目标，而不是继续停留在当前 `L0-L4` 的过渡架构。

### 6.2 优先级清单（建议）

| 优先级 | 主题 | 涉及模块 | 重构动作 | 目标收益 |
|---|---|---|---|---|
| P0 | UI 入口解耦（持续） | `UI/HUD/MAHUD.cpp` + `UI/HUD/Application/*` + `UI/Core/MAUIManager.*` | MAHUD 与 UIManager 均已完成 Coordinator 化；UIManager 已完成 `friend` 清零、显式回调桥接与私有字段直连清理 | 降低 UI 变更连锁影响 |
| P0 | 调度核心收口 | `Core/Manager/MASceneGraphManager.cpp` | 保持外部 API 稳定，内部继续通过 ports/adapters 隔离实现细节 | 防止回耦，稳定跨模块调用 |
| P0 | 导航状态显式化 | `Agent/Component/MANavigationService*.cpp` | 将 Pause/Resume/Cleanup 等状态切换统一到 transition helper | 降低分支散落与遗漏风险 |
| P0 | `Utils` 拆层 | `Utils/MAPathPlanner*` `Utils/MAFlightController*` | 领域规则下沉到 `L2 Domain`，UE `World/Trace/Overlap` 访问上提到 `L3 Adapter` | 提升可测试性与可替换性 |
| P1 | 画布交互解耦 | `UI/SkillAllocation/MAGanttCanvas.cpp` | 将 `NativeOnMouseDown/Move/Up` 收敛到 DragController，Canvas 仅做编排 | 降低 UI 复杂度，便于迭代 |
| P1 | 通信类型治理（已完成第一阶段） | `Core/Comm/MACommTypes.*` | 已完成 Domain 拆头 + Codec 下沉；下一步可补 DTO 校验与版本字段 | 减少前后端联调破坏 |
| P1 | 架构守卫 | CI / 静态检查 | 增加 include 与层间依赖规则，禁止新增 `L0 -> L2/L3` 直连 | 防止重构成果回退 |

通信类型治理（当前进展，2026-03-08）：
- `MACommTypes.h` 已改为 umbrella，仅聚合 `Domain/MAComm*Types.h` 五个子头；原单体头中的模型方法声明已移除。
- 原 `MACommTypes.cpp` 已删除；编解码能力下沉到 `Core/Comm/Infrastructure/Codec/*.cpp`（Envelope / Basic / TaskPlan / Skill / Scene+Response / TypeHelper）。
- `LogMACommTypes` 与消息枚举映射策略集中到 `MACommTypeHelper.cpp`，避免重复定义与分散维护。
- `MACommOutbound` / `MACommSubsystem` / `MACommInbound` / `MAHUDBackendCoordinator` 已统一通过 `MACommJsonCodec` 门面访问序列化逻辑。
- `MACommandManager` 中 `SkillList` 时间步查询已改为 `MACommJsonCodec::FindSkillListTimeStep`，保持模型层“纯数据结构”定位。

### 6.3 执行顺序建议

1. 先做全部 P0（控制复杂度和耦合主风险）。
2. 再做 P1（提高演进效率和长期稳定性）。
3. 每完成一个主题，更新本页架构图中的连线关系，保持文档与代码同步。
