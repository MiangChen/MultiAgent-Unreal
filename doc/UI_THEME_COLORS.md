# UI 主题颜色配置指南

本文档说明 MAUITheme 主题系统中所有可自定义的颜色变量。

## 概述

UI 主题通过 `MAUITheme` 数据资产进行配置。你可以在 UE5 编辑器中创建 Data Asset 实例，修改颜色值来自定义整个 UI 的视觉风格。

**配置方式：**
1. 在 Content Browser 中右键 → Miscellaneous → Data Asset
2. 选择 `MAUITheme` 类
3. 编辑颜色属性
4. 在 MAUIManager 中加载该主题

---

## 颜色变量一览

### 基础颜色 (Colors)

| 变量名 | 默认值 | Hex | 用途 |
|--------|--------|-----|------|
| `PrimaryColor` | 蓝色 (0.2, 0.6, 1.0) | #33AAFF | 主要按钮、强调元素、链接 |
| `SecondaryColor` | 深灰 (0.3, 0.3, 0.35) | #4D4D59 | 次要按钮、边框、分隔线 |
| `DangerColor` | 红色 (1.0, 0.3, 0.3) | #FF4D4D | 危险操作、错误提示、删除按钮 |
| `SuccessColor` | 绿色 (0.3, 0.8, 0.4) | #4DCC66 | 成功提示、确认操作、完成状态 |
| `WarningColor` | 黄色 (1.0, 0.8, 0.2) | #FFCC33 | 警告提示、注意事项 |
| `BackgroundColor` | 深色透明 (0.15, 0.15, 0.18, 0.85) | #26262E | 面板背景、模态窗口背景 |
| `TextColor` | 白色 (1.0, 1.0, 1.0) | #FFFFFF | 主要文字颜色 |

---

### 模式颜色 (Colors|Mode)

用于不同操作模式的视觉区分，显示在 HUD 模式指示器上。

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `ModeSelectColor` | 绿色 (0, 1, 0) | Select 选择模式 |
| `ModeDeployColor` | 蓝色 (0.2, 0.6, 1.0) | Deployment 部署模式 |
| `ModeModifyColor` | 橙色 (1.0, 0.6, 0) | Modify 修改模式 |
| `ModeEditColor` | 蓝色 (0, 0.5, 1.0) | Edit 编辑模式 |

**影响组件：** MASelectionHUD 模式指示器、MAModifyWidget 标题

---

### 状态颜色 (Colors|Status)

用于任务和技能执行状态的显示。

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `StatusPendingColor` | 灰色 (0.4, 0.4, 0.4) | Pending 等待状态 |
| `StatusInProgressColor` | 黄色 (0.9, 0.8, 0.2) | InProgress 执行中状态 |
| `StatusCompletedColor` | 绿色 (0.2, 0.8, 0.3) | Completed 完成状态 |
| `StatusFailedColor` | 红色 (0.9, 0.2, 0.2) | Failed 失败状态 |

**影响组件：** MAGanttCanvas、MASkillListPreview、MAHUDWidget

---

### 文字颜色 (Colors|Text)

用于不同类型文字的显示。

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `SecondaryTextColor` | 浅灰白 (0.8, 0.8, 0.8) | 次要文字、说明文字 |
| `LabelTextColor` | 白色 (1.0, 1.0, 1.0) | 表单标签 |
| `HintTextColor` | 浅灰白 (0.7, 0.7, 0.7) | 提示文字、占位符 |
| `InputTextColor` | 白色 (1.0, 1.0, 1.0) | 输入框文字 |

**影响组件：** MAEditWidget、MAModifyWidget、MARightSidebarWidget

---

### 画布颜色 (Colors|Canvas)

用于 DAG 任务图和 Gantt 图画布。

| 变量名 | 默认值 | Hex | 用途 |
|--------|--------|-----|------|
| `CanvasBackgroundColor` | 很深的深色透明 (0.08, 0.08, 0.1, 0.9) | #14141A | 画布背景 |
| `GridLineColor` | 深灰 (0.15, 0.15, 0.2) | #262633 | 网格线 |
| `EdgeColor` | 灰色 (0.5, 0.5, 0.6) | #808099 | 节点连接边线 |
| `HighlightedEdgeColor` | 绿色 (0.3, 0.8, 0.3) | #4DCC4D | 高亮/选中边线 |

**影响组件：** MADAGCanvasWidget、MAGanttCanvas、MATaskNodeWidget

---

### 交互颜色 (Colors|Interaction)

用于选中、高亮、拖放等交互状态。

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `SelectionColor` | 蓝色 (0.3, 0.7, 1.0) | 选中状态 |
| `HighlightColor` | 深色半透明 (0.2, 0.2, 0.3, 0.95) | 悬停高亮 |
| `ValidDropColor` | 绿色半透明 (0.2, 0.8, 0.3, 0.5) | 有效拖放目标 |
| `InvalidDropColor` | 红色半透明 (0.9, 0.2, 0.2, 0.5) | 无效拖放目标 |

**影响组件：** MATaskNodeWidget、MAGanttCanvas、MANodePaletteWidget

---

### 输入框颜色 (Colors|Input)

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `InputBackgroundColor` | 很深的深色透明 (0.1, 0.1, 0.12, 0.9) | 输入框背景 |

**影响组件：** MAEditWidget、MAModifyWidget、MARightSidebarWidget

---

### 遮罩颜色 (Colors|Overlay)

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `OverlayColor` | 黑色半透明 (0, 0, 0, 0.5) | 模态窗口遮罩背景 |
| `ShadowColor` | 黑色半透明 (0, 0, 0, 0.5) | 文字阴影、投影 |

**影响组件：** MABaseModalWidget、MAHUDWidget

---

### Agent 类型颜色 (Colors|Agent)

用于小地图和 Agent 列表中不同类型机器人的显示。

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `AgentHumanoidColor` | 黄色 (1.0, 0.8, 0.2) | Humanoid 类型 Agent |
| `AgentUAVColor` | 蓝色 (0.2, 0.6, 1.0) | UAV/FixedWingUAV 类型 Agent |
| `AgentUGVColor` | 橙色 (0.8, 0.4, 0.1) | UGV 类型 Agent |
| `AgentQuadrupedColor` | 绿色 (0.2, 1.0, 0.4) | Quadruped 类型 Agent |
| `AgentSelectedColor` | 白色 (1.0, 1.0, 1.0) | 选中的 Agent 高亮 |

**影响组件：** MAMiniMapWidget

---

### 小地图颜色 (Colors|MiniMap)

用于小地图的背景和相机指示器。

| 变量名 | 默认值 | 用途 |
|--------|--------|------|
| `MiniMapBackgroundColor` | 深色半透明 (0.1, 0.1, 0.15, 0.85) | 小地图背景 |
| `MiniMapCameraColor` | 红色 (1.0, 0.2, 0.2) | 相机位置和视野指示器 |

**影响组件：** MAMiniMapWidget

---

## 颜色值格式

所有颜色使用 `FLinearColor` 格式，值范围 0.0 ~ 1.0：

```cpp
FLinearColor(R, G, B, A)
// R = 红色分量
// G = 绿色分量  
// B = 蓝色分量
// A = 透明度 (1.0 = 不透明, 0.0 = 完全透明)
```

**常用颜色参考：**
- 纯白: `(1.0, 1.0, 1.0, 1.0)`
- 纯黑: `(0.0, 0.0, 0.0, 1.0)`
- 纯红: `(1.0, 0.0, 0.0, 1.0)`
- 纯绿: `(0.0, 1.0, 0.0, 1.0)`
- 纯蓝: `(0.0, 0.0, 1.0, 1.0)`
- 50% 透明黑: `(0.0, 0.0, 0.0, 0.5)`

---

## 组件颜色映射

### HUD 层

| 组件 | 使用的颜色变量 |
|------|---------------|
| MASelectionHUD | ModeSelectColor, ModeDeployColor, ModeModifyColor, ModeEditColor, PrimaryColor, SecondaryTextColor |
| MAHUDWidget | DangerColor, SuccessColor, WarningColor, PrimaryColor, ShadowColor |

### Mode 层

| 组件 | 使用的颜色变量 |
|------|---------------|
| MAEditWidget | PrimaryColor, HintTextColor, DangerColor, LabelTextColor, InputTextColor, InputBackgroundColor |
| MAModifyWidget | ModeModifyColor, HintTextColor, CanvasBackgroundColor, SuccessColor, InputTextColor |
| MASceneListWidget | PrimaryColor, DangerColor, SecondaryTextColor |
| MAEmergencyWidget | BackgroundColor, DangerColor, CanvasBackgroundColor, InputTextColor, SecondaryColor, InputBackgroundColor, TextColor |

### TaskGraph 层

| 组件 | 使用的颜色变量 |
|------|---------------|
| MATaskNodeWidget | BackgroundColor, SelectionColor, HighlightColor, GridLineColor, PrimaryColor, SecondaryColor, SuccessColor, TextColor, SecondaryTextColor |
| MADAGCanvasWidget | CanvasBackgroundColor, EdgeColor, HighlightedEdgeColor |
| MANodePaletteWidget | BackgroundColor, SecondaryColor, HighlightColor, TextColor |
| MATaskPlannerWidget | BackgroundColor, CanvasBackgroundColor, TextColor, PrimaryColor |

### SkillAllocation 层

| 组件 | 使用的颜色变量 |
|------|---------------|
| MAGanttCanvas | CanvasBackgroundColor, GridLineColor, TextColor, StatusPendingColor, StatusInProgressColor, StatusCompletedColor, StatusFailedColor, SelectionColor, ValidDropColor, InvalidDropColor |
| MASkillListPreview | CanvasBackgroundColor, GridLineColor, SecondaryTextColor, TextColor, HighlightColor, HintTextColor, InputTextColor, StatusPendingColor, StatusInProgressColor, StatusCompletedColor, StatusFailedColor |
| MATaskGraphPreview | BackgroundColor, EdgeColor, SecondaryTextColor, HintTextColor, TextColor, InputTextColor, HighlightColor, StatusPendingColor, StatusInProgressColor, StatusCompletedColor, StatusFailedColor |
| MASkillAllocationViewer | OverlayColor, BackgroundColor, CanvasBackgroundColor, LabelTextColor, HintTextColor, SecondaryTextColor, DangerColor, InputTextColor, SecondaryColor, InputBackgroundColor, PrimaryColor |

### Components 层

| 组件 | 使用的颜色变量 |
|------|---------------|
| MAContextMenuWidget | BackgroundColor, SecondaryColor, HighlightColor, TextColor, SecondaryTextColor |
| MADirectControlIndicator | SuccessColor |
| MARightSidebarWidget | TextColor, InputTextColor, DangerColor, SecondaryTextColor |
| MAMiniMapWidget | MiniMapBackgroundColor, MiniMapCameraColor, AgentHumanoidColor, AgentUAVColor, AgentUGVColor, AgentQuadrupedColor, AgentSelectedColor |
| MANotificationWidget | BackgroundColor, PrimaryColor, SuccessColor, WarningColor, DangerColor, TextColor |

### Modal 层

| 组件 | 使用的颜色变量 |
|------|---------------|
| MABaseModalWidget | OverlayColor, TextColor, DangerColor, SecondaryTextColor |
| MAEmergencyModal | SecondaryColor, TextColor, InputBackgroundColor, InputTextColor, CanvasBackgroundColor |
| MATaskGraphModal | SecondaryColor, TextColor, InputTextColor |
| MASkillAllocationModal | SecondaryColor, TextColor, InputTextColor |

---

## 主题切换

主题可以在运行时切换：

```cpp
// 获取 UIManager
UMAUIManager* UIManager = /* ... */;

// 加载新主题
UMAUITheme* NewTheme = LoadObject<UMAUITheme>(nullptr, TEXT("/Game/UI/Themes/DarkTheme"));
UIManager->LoadTheme(NewTheme);

// 主题会自动分发到所有 Widget
```

你也可以订阅主题变更事件：

```cpp
UIManager->OnThemeChanged.AddDynamic(this, &UMyWidget::OnThemeChanged);

void UMyWidget::OnThemeChanged(UMAUITheme* NewTheme)
{
    // 处理主题变更
}
```

---

## 创建自定义主题

1. 在 Content Browser 中：右键 → Miscellaneous → Data Asset
2. 选择 `MAUITheme` 作为类
3. 命名为 `DA_MyCustomTheme`
4. 在 Details 面板中编辑各颜色属性
5. 保存资产

**推荐主题命名：**
- `DA_Theme_Default` - 默认主题
- `DA_Theme_Dark` - 深色主题
- `DA_Theme_Light` - 浅色主题
- `DA_Theme_HighContrast` - 高对比度主题

---

## 注意事项

1. **Alpha 值**：所有颜色的 Alpha 值必须 > 0，否则验证会失败
2. **Fallback**：如果主题未加载，Widget 会使用硬编码的默认值
3. **实时更新**：修改主题后需要调用 `DistributeThemeToWidgets()` 才能生效
4. **编辑器预览**：在编辑器中修改 Data Asset 后，需要重新运行才能看到效果
