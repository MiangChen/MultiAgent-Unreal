| 文件名 | Value Type | 绑定按键 | |--------|-----------|---------| 
| IA_Move | Axis2D (Vector2D) | W/A/S/D |
 | IA_Click | Digital (Bool) | 鼠标左键 | 
| IA_ToggleSemanticMap | Digital (Bool) | M | 
| IA_ToggleGanttChart | Digital (Bool) | G | 
| IA_ToggleRealMap | Digital (Bool) | R |


| 属性名 | 用途 | 默认显示 | 
|--------|------|----------| 
| MainWidgetClass | 输入框+文本显示 | 常驻显示 | 
| SemanticMapWidgetClass | 语义地图 | 隐藏 |
| GanttChartWidgetClass | 甘特图 | 隐藏 | 
| PropertyPanelWidgetClass | 属性面板 | 隐藏 | 
| EventPopupWidgetClass | 事件弹窗 | 隐藏 |
| RealMapWidgetClass | 真实地图 | 隐藏 |


我已经创建了完整的数据结构定义文件。以下是各部分的作用说明：

枚举类型 (Enums)
| 枚举 | 作用 |
|------|------| 
| EAtomSkillType | 原子技能类型：导航/操纵/感知/广播/等待/自定义 | 
| ESkillStatus | 技能执行状态：等待/执行中/完成/失败/取消 | 
| ERobotType | 机器人类型：人形/机器狗/轮式小车/无人机 |
| ERobotStatus | 机器人状态：空闲/工作中/移动中/故障/充电/离线 |
| ESemanticObjectType | 语义对象类型：道路/交叉口/建筑物/车辆/行人/任务点等 | 
| ETaskPriority | 任务优先级：低/普通/高/紧急 | 
| ETaskStatus | 任务状态：等待/已分配/进行中/完成/失败/取消 | 
| ESimEventType | 事件类型：信息/警告/错误/决策点/紧急事件 | 
| ECommandType | 发送给规划器的命令类型 | 
| EResponseType | 规划器响应类型 |


结构体 (Structs)
| 结构体 | 作用 | 
|--------|------| 
| FAtomSkill | 原子技能数据：ID、类型、状态、进度、目标位置等 | 
| FRobotData | 机器人数据：ID、类型、位置、电量、当前技能、技能队列 | 
| FSemanticObject | 语义对象：ID、类型、位置、多边形顶点、颜色、属性 | 
| FSemanticMapData | 语义地图：边界、所有对象列表 | 
| FTaskData | 任务数据：ID、优先级、分配的机器人、子技能列表、进度 | 
| FDecisionOption | 决策选项：用于事件弹窗的按钮 | 
| FSimEvent | 仿真事件：类型、描述、关联机器人、决策选项 | 
| FPlannerCommand | 发送给规划器的命令 | 
| FPlannerResponse | 规划器返回的响应 | 
| FGanttChartEntry | 甘特图条目：开始时间、持续时间、进度、颜色 | 
| FGanttChartRow | 甘特图行：一个机器人的所有任务条目 |


委托 (Delegates)
| 委托 | 触发时机 | 
|------|----------| 
| FOnRobotStatusChanged | 机器人状态变化时 | | FOnTaskStatusChanged | 任务状态变化时 | | FOnSemanticMapUpdated | 语义地图更新时 | | FOnSimEventTriggered | 事件触发时 | | FOnPlannerResponse | 收到规划器响应时 | | FOnConnectionStatusChanged | 连接状态变化时 |


辅助函数库 (USimTypesHelper)
提供蓝图可调用的转换函数：

各种枚举转中文字符串
各种类型对应的默认颜色


| 需求 | 对应数据结构 | 
|------|-------------|
| 原子技能（导航/操纵/感知/广播） | EAtomSkillType, FAtomSkill | 
| 机器人类型（人形/机器狗/小车） | ERobotType, FRobotData | 
| 机器人状态和技能队列 | FRobotData.SkillQueue, ERobotStatus | 
| 语义地图对象（道路/交叉口/建筑物/车辆/行人/任务点/机器人） | ESemanticObjectType, FSemanticObject | 
| 语义地图数据 | FSemanticMapData | 
| 任务系统 | FTaskData, ETaskStatus, ETaskPriority | 
| 甘特图数据 | FGanttChartEntry, FGanttChartRow | 
| 事件弹窗（决策点/紧急事件） | FSimEvent, FDecisionOption, ESimEventType | 
| 与规划器通信 | FPlannerCommand, FPlannerResponse | 
| 属性面板（附加属性） | FSemanticObject.Properties (TMap) | 
| 仿真控制 | FSimulationControl, ESimulationState | 
| 事件广播委托 | 6个 DECLARE_DYNAMIC_MULTICAST_DELEGATE | 
| 类型转字符串/颜色辅助函数 | USimTypesHelper |


| 模块 | 功能 | 
|------|------| 
| 连接管理 | Connect(), Disconnect(), IsConnected() - 管理与规划器的连接状态 | 
| 命令发送 | SendNaturalLanguageCommand(), SendCommand(), SendDecisionResponse() - 发送指令给规划器 | 
| 数据访问 | GetAllRobots(), GetRobotByID(), GetAllTasks(), GetSemanticMap(), GetGanttChartData() - 获取缓存数据 | 
| 事件委托 | 6个委托供UI绑定：OnPlannerResponse, OnRobotStatusChanged, OnTaskStatusChanged, OnSemanticMapUpdated, OnSimEventTriggered, OnConnectionStatusChanged | 
| 模拟数据 | GenerateMockRobots(), GenerateMockSemanticMap(), GenerateMockPlanResponse(), TriggerMockEvent() - 开发测试用 | 
| JSON处理 | 解析和序列化函数 - 为将来真实通信做准备 | 
| 甘特图生成 | RebuildGanttChartData() - 自动从任务数据生成甘特图 |



| 模块 | 功能 | 
|------|------| 
| UI 控件绑定 | InputTextBox (输入框), SendButton (发送按钮), ResultTextBox (结果显示), StatusText (状态文本) | 
| 公共接口 | SetResultText(), AppendResultText(), ClearResultText(), SetStatusText(), GetInputText(), ClearInputText(), FocusInputBox(), AddLogEntry() | 
| 事件委托 | OnCommandSubmitted - 当用户提交指令时触发 | 
| 自动绑定 | 自动绑定 SimCommSubsystem 的事件，接收规划响应、连接状态变化、仿真事件 | 
| 命令历史 | 保存最近 50 条命令历史 |


| 控件类型 | 名称 | 用途 | 
|---------|------|------| 
| Editable Text Box | InputTextBox | 用户输入指令 | 
| Button | SendButton | 发送按钮 | 
| Multi Line Editable Text Box | ResultTextBox | 显示规划结果 | 
| Text Block (可选) | StatusText | 显示状态 |


| GameMode | 来源 | 作用 | 
|----------|------|------| 
| BP_SimGameMode | 我们创建的 | 你的仿真平台专用，配置了 SimHUD 和 SimPlayerController | 
| SimGameMode | C++ 基类 | BP_SimGameMode 的父类 | 
| uidevGameMode | 项目模板 | 项目创建时自带的，可能是俯视角模板 | 
| GameMode | 引擎基类 | UE 默认 GameMode | 
| GameModeBase | 引擎基类 | 更轻量的 GameMode 基类 | 
| FunctionalTestGameMode | 引擎测试 | 用于自动化测试，不用管 | 
| RenderToTexture_Game | 引擎示例 | 渲染相关示例，不用管 |


| 文件 | 修改内容 | 
|------|----------| 
| SimPlayerController.h | 添加 C 键切换、点击移动、UI 状态变量 | 
| SimPlayerController.cpp | 实现点击移动、UI 切换、移动禁用逻辑 | 
| SimHUD.h | 添加 ToggleMainUI() 和 OnMainUIToggled() |
| SimHUD.cpp | 主 UI 默认隐藏，实现切换逻辑 |


| 功能 | 说明 | 
|------|------| 
| 全局配置 | PlannerServerURL - 规划器服务器地址bUseMockData - 是否使用模拟数据bDebugMode - 调试模式 | 
| 全局状态 | bSimulationRunning - 仿真是否运行中SimulationTime - 仿真时间 | 
| 便捷访问 | GetCommSubsystem() - 获取通信子系统GetSimGameInstance() - 静态函数获取实例 | 
| 仿真控制 | StartSimulation() / PauseSimulation() / StopSimulation() / ResetSimulation() | 
| 事件委托 | OnSimulationStateChanged - 仿真状态变化时广播 |


| 功能模块 | 说明 |
|----------|------| 
| 地图加载 | LoadMapData() - 加载地图数据并生成几何体LoadMapFromJSON() - 从 JSON 字符串解析并加载ClearMap() - 清除当前地图 | 
| 对象管理 | AddSemanticObject() - 添加单个语义对象RemoveSemanticObject() - 移除对象UpdateSemanticObject() - 更新对象 | 
| 查询功能 | GetSemanticObjectByID() - 根据 ID 获取对象GetSemanticObjectByActor() - 根据 Actor 获取对象GetObjectsByType() - 获取指定类型的所有对象 | 
| 几何体生成 | CreateRectangleMesh() - 矩形平面（道路、区域）CreateBoxMesh() - 立方体（建筑物）CreateCylinderMesh() - 圆柱体（任务点）CreateCircleMesh() - 圆形平面（交叉口） | 
| 测试功能 | GenerateTestMap() - 生成测试地图（包含道路、建筑、任务点） |



| 特性 | MAMainWidget | MASimpleMainWidget | 
|------|--------------|-------------------| 
| 实现方式 | 依赖蓝图 (BindWidget) | 纯 C++ 动态创建 | 
| UI 控件 | 需要在蓝图中绑定 | 代码中用 WidgetTree 创建 | 
| 使用场景 | 需要 WBP_MAMainWidget 蓝图 | 无需蓝图，开箱即用 | 
| 当前状态 | 冗余 - 不再使用 | 活跃 - 当前使用 |


| 功能 | 操作 | 预期结果 | 通过 | 
|------|------|----------|------| 
| 进入 Agent View | Tab | 切换视角，显示 HUD | ☐ | 
| W 前进 | 按住 W | Agent 向前移动 | ☐ | 
| S 后退 | 按住 S | Agent 向后移动 | ☐ | 
| A 左移 | 按住 A | Agent 向左移动 | ☐ | 
| D 右移 | 按住 D | Agent 向右移动 | ☐ | 
| 松开停止 | 松开 WASD | Agent 停止 | ☐ | 
| 鼠标水平 | 左右移动鼠标 | Agent 旋转 | ☐ | 
| 鼠标垂直 | 上下移动鼠标 | 相机俯仰变化 | ☐ | 
| Drone 上升 | Space | Drone 升高 | ☐ | 
| Drone 下降 | Ctrl | Drone 降低 | ☐ | 
| 非 Drone 垂直 | Space/Ctrl | 无反应 | ☐ | 
| RTS 命令免疫 | G/F/K | 不响应命令 | ☐ | 
| 退出 Agent View | 0 | 返回 Spectator，HUD 消失 | ☐ |
