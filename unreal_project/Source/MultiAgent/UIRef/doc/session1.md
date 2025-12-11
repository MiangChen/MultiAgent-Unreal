# 阶段二：常驻 UI + 通信 - 开发总结

## 目标
打通数据流：输入框输入文字 → Subsystem 处理 → 显示框显示结果

## 完成的 C++ 文件

### 1. SimTypes.h / .cpp - 数据结构定义
**作用**：定义所有的结构体和枚举，是整个系统的数据基础

| 类型 | 说明 |
|------|------|
| `ERobotType` | 机器人类型（人形/机器狗/小车） |
| `ERobotStatus` | 机器人状态（空闲/执行中/暂停等） |
| `ESkillType` | 原子技能类型（导航/操纵/感知/广播） |
| `ESkillStatus` | 技能执行状态 |
| `ETaskStatus` | 任务状态 |
| `ESimEventType` | 仿真事件类型 |
| `FAtomSkill` | 原子技能数据 |
| `FRobotData` | 机器人完整数据 |
| `FTaskData` | 任务数据 |
| `FSemanticObject` | 语义地图对象 |
| `FMapData` | 地图数据 |
| `FSimEvent` | 仿真事件 |
| `FPlannerResponse` | 规划器响应 |

### 2. SimCommSubsystem.h / .cpp - 通信子系统
**作用**：核心通信模块，负责与外部规划器通信

| 功能 | 说明 |
|------|------|
| `SendNaturalLanguageCommand()` | 发送自然语言指令 |
| `RequestMapData()` | 请求地图数据 |
| `RequestRobotStatus()` | 请求机器人状态 |
| `GenerateMockResponse()` | 生成模拟响应（调试用） |
| `OnPlannerResponse` | 规划器响应委托 |
| `OnMapDataReceived` | 地图数据接收委托 |
| `OnRobotStatusUpdated` | 机器人状态更新委托 |
| `OnSimEventTriggered` | 仿真事件触发委托 |

### 3. SimMainWidget.h / .cpp - 主界面 Widget
**作用**：输入框 + 文本显示框的 C++ 基类

| 控件绑定 | 说明 |
|----------|------|
| `InputTextBox` | 输入框 (UEditableText) |
| `SendButton` | 发送按钮 (UButton) |
| `ResultTextBox` | 结果显示框 (UMultiLineEditableText) |
| `StatusText` | 状态文本 (UTextBlock, 可选) |

| 功能 | 说明 |
|------|------|
| `SetResultText()` / `AppendResultText()` | 设置/追加结果文本 |
| `FocusInputBox()` | 聚焦到输入框 |
| `AddLogEntry()` | 添加带时间戳的日志 |
| `OnCommandSubmitted` | 指令提交委托 |

### 4. SimGameInstance.h / .cpp - 全局访问入口
**作用**：游戏的全局单例，跨关卡持续存在

| 配置 | 说明 |
|------|------|
| `PlannerServerURL` | 规划器服务器地址 |
| `bUseMockData` | 是否使用模拟数据 |
| `bDebugMode` | 调试模式 |

| 功能 | 说明 |
|------|------|
| `GetCommSubsystem()` | 获取通信子系统 |
| `GetSimGameInstance()` | 静态获取实例 |
| `StartSimulation()` / `PauseSimulation()` | 仿真控制 |

### 5. SimPlayerController.h / .cpp - 玩家控制器
**作用**：处理输入、UI 切换、点击检测

| 功能 | 说明 |
|------|------|
| WASD 移动 | 俯视角移动控制 |
| 鼠标点击移动 | 点击地面移动到目标点 |
| C 键切换 UI | 显示/隐藏主界面 |
| M/G/R 键 | 切换语义地图/甘特图/真实地图 |
| `OnRefocusMainUI` | UI 显示时点击屏幕重新聚焦 |

### 6. SimHUD.h / .cpp - HUD 管理器
**作用**：创建和管理所有 UI Widget

| Widget | 说明 |
|--------|------|
| `MainWidget` | 主界面（输入框+显示框） |
| `SemanticMapWidget` | 语义地图 |
| `GanttChartWidget` | 甘特图 |
| `PropertyPanelWidget` | 属性面板 |
| `EventPopupWidget` | 事件弹窗 |
| `RealMapWidget` | 真实地图 |

### 7. SimGameMode.h / .cpp - 游戏模式
**作用**：设置默认的 Controller、HUD、Pawn

```cpp
PlayerControllerClass = ASimPlayerController::StaticClass();
HUDClass = ASimHUD::StaticClass();
```

---

## 虚幻编辑器操作

### 1. 创建 Widget Blueprint
1. Content Browser → 右键 → User Interface → Widget Blueprint
2. 命名为 `WBP_MainWidget`
3. 设置父类为 `SimMainWidget`
4. 添加控件并命名（必须与 C++ 中的 BindWidget 名称一致）：
   - `InputTextBox` - Editable Text
   - `SendButton` - Button
   - `ResultTextBox` - Multi-Line Editable Text
   - `StatusText` - Text Block (可选)

### 2. 创建 HUD Blueprint
1. Content Browser → 右键 → Blueprint Class → 选择 `SimHUD`
2. 命名为 `BP_SimHUD`
3. 打开蓝图，在 Details 面板设置：
   - `MainWidgetClass` = `WBP_MainWidget`
   - 其他 Widget Class 暂时留空

### 3. 创建 GameMode Blueprint
1. Content Browser → 右键 → Blueprint Class → 选择 `SimGameMode`
2. 命名为 `BP_SimGameMode`
3. 打开蓝图，确认设置：
   - `PlayerControllerClass` = `SimPlayerController`
   - `HUDClass` = `BP_SimHUD`

### 4. 设置项目默认
1. Edit → Project Settings → Maps & Modes
2. 设置：
   - `Default GameMode` = `BP_SimGameMode`
   - `Game Instance Class` = `SimGameInstance`

### 5. 配置 Input Actions (Enhanced Input)
1. Content Browser → 右键 → Input → Input Action
2. 创建以下 Input Actions：
   - `IA_Move` - Axis2D (WASD)
   - `IA_Click` - Bool (鼠标左键)
   - `IA_SetDestination` - Bool (鼠标左键长按)
   - `IA_ToggleMainUI` - Bool (C 键)
   - `IA_ToggleSemanticMap` - Bool (M 键)
   - `IA_ToggleGanttChart` - Bool (G 键)
   - `IA_ToggleRealMap` - Bool (R 键)

3. 创建 Input Mapping Context：
   - `IMC_Default` - 绑定上述所有 Actions

4. 在 `BP_SimPlayerController` 中设置：
   - `DefaultMappingContext` = `IMC_Default`
   - 各个 Action 属性指向对应的 Input Action

---

## 关键问题解决记录

### 问题1：UI 显示后立即隐藏
**原因**：`OnToggleUI.Broadcast(FName("MainUI"))` 导致 `ToggleMainUI` 被调用两次
**解决**：移除重复的广播调用

### 问题2：点击屏幕后输入框失焦，无法恢复
**原因**：`FInputModeGameAndUI` 模式下点击 Widget 外部会转移焦点到游戏世界
**解决**：添加 `OnRefocusMainUI` 委托，UI 显示时点击屏幕自动重新聚焦到输入框

### 问题3：枚举解析函数编译错误
**原因**：`FindEnumValueByNameString` 在 UE5.5 中已弃用
**解决**：改用 `GetValueByName` 函数

---

## 文件结构
```
Source/uidev/
├── SimTypes.h / .cpp          # 数据结构定义
├── SimCommSubsystem.h / .cpp  # 通信子系统
├── SimMainWidget.h / .cpp     # 主界面 Widget
├── SimGameInstance.h / .cpp   # 全局访问入口
├── SimPlayerController.h / .cpp # 玩家控制器
├── SimHUD.h / .cpp            # HUD 管理器
├── SimGameMode.h / .cpp       # 游戏模式
└── uidev.Build.cs             # 构建配置（已添加 UMG, JSON, HTTP 等模块）
```

---

## 验收标准 ✓
- [x] 按 C 键显示/隐藏主 UI
- [x] 输入框可以输入文字
- [x] 按回车或点击发送按钮提交指令
- [x] 通信子系统处理指令并返回模拟响应
- [x] 结果显示在文本框中
- [x] 点击屏幕任意位置可重新聚焦到输入框
- [x] WASD 移动和鼠标点击移动正常工作
