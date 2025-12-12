# Session 1 - WBP_MainWidget 配置指南

## 前置条件

1. 已编译项目（运行 `./startc.sh`）
2. 已创建 `WBP_MainWidget`（在 `Content/Sim/UI/` 目录下）

---

## 步骤 1：设置父类为 SimMainWidget

1. 在 Content Browser 中找到 `Content/Sim/UI/WBP_MainWidget`
2. **双击打开** Widget Blueprint
3. 点击顶部菜单 **File** → **Reparent Blueprint**
4. 在弹出的窗口中搜索 `SimMainWidget`
5. 选择它，点击 **Reparent**
6. 如果提示确认，点击 **Yes**

> 设置父类后，C++ 中定义的 BindWidget 属性会自动与蓝图中同名的控件绑定。

---

## 步骤 2：打开 Designer 视图

1. 在 Widget Blueprint 编辑器中，确保左上角选择的是 **Designer** 标签（不是 Graph）
2. 你会看到一个空白的画布，这就是 UI 设计区域

---

## 步骤 3：添加根容器

1. 在左侧 **Palette** 面板中（如果没看到，点击 Window → Palette）
2. 搜索 `Canvas Panel`
3. **拖拽** Canvas Panel 到中间的画布上
4. 这是根容器，所有其他控件都放在它里面

---

## 步骤 4：添加必需控件

### 4.1 添加 ResultTextBox（结果显示框）

1. 在 Palette 中搜索 `Multi Line Editable Text`（注意：不是 Text Box）
2. 拖拽到 Canvas Panel 上
3. **重要**：在右侧 Details 面板中，找到最上方的 **名称**，改为：`ResultTextBox`
4. 调整位置和大小：
   - Position X: `20`
   - Position Y: `50`
   - Size X: `400`
   - Size Y: `300`
5. 在 Details 面板中设置：
   - 展开 **Appearance** → **Text Style**
   - Is Read Only: ✓ 勾选（只读）


### 4.2 添加 InputTextBox（输入框）

1. 在 Palette 中搜索 `Editable Text`（注意：不是 Text Box，也不是 Multi Line）
2. 拖拽到 Canvas Panel 上
3. **重要**：改名为：`InputTextBox`
4. 调整位置和大小：
   - Position X: `20`
   - Position Y: `360`
   - Size X: `320`
   - Size Y: `30`
5. 在 Details 面板中设置：
   - 展开 **Content**
   - Hint Text: `请输入指令...`

### 4.3 添加 SendButton（发送按钮）

1. 在 Palette 中搜索 `Button`
2. 拖拽到 Canvas Panel 上
3. **重要**：改名为：`SendButton`
4. 调整位置和大小：
   - Position X: `350`
   - Position Y: `360`
   - Size X: `70`
   - Size Y: `30`
5. 给按钮添加文字（两种方法）：

   **方法A**：从 Palette 拖拽
   - 在 Palette 中搜索 `Text`
   - 拖拽到 Hierarchy 面板中的 `SendButton` 上（作为子控件）
   - 选中新添加的 Text，在 Details 中设置 Text 为 `发送`

   **方法B**：直接在 Details 中设置
   - 选中 SendButton
   - 在 Details 面板中找到 **Content** 部分
   - 如果有 Text 属性，直接设置为 `发送`

### 4.4 添加 StatusText（状态文本，可选）

1. 在 Palette 中搜索 `Text`（或 `Text Block`）
2. 拖拽到 Canvas Panel 上
3. **重要**：改名为：`StatusText`
4. 调整位置：
   - Position X: `20`
   - Position Y: `15`
   - Size X: `200`
   - Size Y: `25`
5. 在 Details 中设置默认文本：`就绪`

---

## 步骤 5：检查控件名称

这是最重要的一步！名称必须完全匹配 C++ 中的定义。

1. 在左侧 **Hierarchy** 面板中，你应该看到：
```
[Canvas Panel]
├── ResultTextBox
├── InputTextBox
├── SendButton
│   └── Text
└── StatusText
```

2. 确认每个控件的名称和类型：

| 名称 | 类型 |
|------|------|
| `ResultTextBox` | Multi Line Editable Text |
| `InputTextBox` | Editable Text |
| `SendButton` | Button |
| `StatusText` | Text Block（可选）|

> 如果名称不对，双击 Hierarchy 中的控件名可以重命名。

---

## 步骤 6：保存并编译

1. 点击左上角 **Compile** 按钮（蓝色齿轮图标）
2. 点击 **Save** 按钮
3. 如果没有错误，说明绑定成功

### 常见错误

如果编译时出现警告：
- `BindWidget property 'XXX' not found` - 说明控件名称不匹配，检查拼写
- 确保控件类型正确（比如 InputTextBox 必须是 Editable Text Box）

---

## 步骤 7：测试

1. 点击编辑器顶部的 **Play** 按钮（或按 Alt+P）
2. 游戏运行后，你应该能看到 UI 界面
3. 在输入框输入文字，按回车或点击发送
4. 观察结果框是否显示内容

---

## 布局参考图

```
┌────────────────────────────────────────────┐
│ StatusText: 就绪                            │  ← 顶部状态栏
├────────────────────────────────────────────┤
│                                            │
│  ┌──────────────────────────────────────┐  │
│  │                                      │  │
│  │  ResultTextBox                       │  │  ← 结果显示区
│  │  (Multi Line Editable Text Box)      │  │     占据大部分空间
│  │                                      │  │
│  │                                      │  │
│  └──────────────────────────────────────┘  │
│                                            │
│  ┌─────────────────────────┐ ┌──────────┐  │
│  │ InputTextBox            │ │SendButton│  │  ← 底部输入区
│  └─────────────────────────┘ └──────────┘  │
└────────────────────────────────────────────┘
```

---

## 快捷键提示

- **Ctrl+S**: 保存
- **F7**: 编译
- **Alt+P**: 运行游戏
- **Esc**: 停止游戏
- **Ctrl+Z**: 撤销
- **Delete**: 删除选中的控件

---

## 下一步

完成 WBP_MainWidget 配置后，确保 BP_SimHUD 中的 MainWidgetClass 指向 WBP_MainWidget。

如果之前已经设置过，应该不需要再改。


好的，现在在gamemode的管理上出现了问题。
接下来我把需求计划和遇到的问题详细告诉你，你要帮我管理gamemode。

我对机器人操控的要求是，当处于topdown视角的时候，既能用wasd来操纵，又能用鼠标点击地图上的点来使其移动。BP_SimGameMode只能实现wasd移动，不能用鼠标点击移动，而uidevGameMode能同时实现这两点。我希望BP_SimGameMode也能实现这点。

然后这个UI里面的输入文本框会和移动的WASD产生冲突，我希望这个UI在不用的时候应该隐藏起来，隐藏起来的时候WASD让人物移动，然后当我按下C键的时候UI显示，时候WASD控制的移动被禁用，可以在UI输入框里输入文本

现在请你思考应该在哪些地方修改代码

---


## 新增功能：C 键切换 UI + 点击移动

### 功能说明

| 按键 | 功能 |
|------|------|
| **C** | 切换主 UI 显示/隐藏 |
| **WASD** | UI 隐藏时：控制角色移动；UI 显示时：在输入框输入文字 |
| **鼠标左键** | UI 隐藏时：点击地面移动角色；UI 显示时：点击 UI 控件 |

### 需要在虚幻编辑器中配置

#### 1. 创建新的 Input Action

在 `Content/Sim/Input/` 目录下创建：

| 资产名 | 类型 | Value Type |
|--------|------|------------|
| `IA_ToggleMainUI` | Input Action | Digital (Bool) |
| `IA_SetDestination` | Input Action | Digital (Bool) |

#### 2. 配置 Input Mapping Context

打开 `IMC_Default`，添加映射：

| Input Action | 按键 |
|--------------|------|
| `IA_ToggleMainUI` | C |
| `IA_SetDestination` | 鼠标左键 |

#### 3. 配置 BP_SimPlayerController

打开 `Content/Sim/Blueprints/BP_SimPlayerController`，在 Details 面板中设置：

| 属性 | 值 |
|------|-----|
| `ToggleMainUIAction` | `IA_ToggleMainUI` |
| `SetDestinationClickAction` | `IA_SetDestination` |
| `FXCursor` | 可选，设置点击特效（可以从 uidevPlayerController 复制） |
| `ShortPressThreshold` | `0.2`（默认值） |

### 行为说明

1. **游戏启动时**：主 UI 默认隐藏，可以用 WASD 和鼠标点击移动
2. **按 C 键**：显示主 UI，WASD 移动被禁用，可以在输入框输入文字
3. **再按 C 键**：隐藏主 UI，恢复 WASD 和点击移动功能



LogSimHUD: MainWidget created (hidden by default, press C to show)
LogSimHUD: SemanticMapWidget created
LogSimHUD: GanttChartWidget created
LogSimHUD: RealMapWidget created
LogSimHUD: PropertyPanelWidget created
LogSimHUD: EventPopupWidget created
LogSimHUD: Controller events bound
LogSimHUD: SimHUD initialized
LogSimHUD: Showing PropertyPanel for: StaticMeshActor_34 at X=1216.192 Y=1850.997 Z=-0.000
LogSimHUD: OnMainUIToggled called: bVisible=1, MainWidget=Valid
LogSimHUD: MainWidget visibility set to Visible
LogSimHUD: Input focus set to MainWidget input box
LogSimHUD: MainUI visibility set to: 1
LogSimHUD: Toggled MainUI visibility

现在我描述一下我程序运行之后的情况：
当我按下c的时候UI会出现，光标自动位于文本输入框中，我可以输入文本，但是当我鼠标点击界面其他位置的时候，光标会消失，即使我再次点击文本输入框光标也不会出现，我一定要再按一次c键然后关闭ui，光标才会重新刷出来。
按Alt不会有任何反应。
现在请你把所有按Alt关闭UI的逻辑都给删除，然后解决这个即使我再次点击文本输入框光标也不会出现的问题。



现在我们先不做阶段三机器人与视角的内容，先做这个：

阶段四：语义地图系统（3-5天，最复杂）

目标：根据 JSON 渲染语义地图

SimSemanticMapManager - 先生成简单几何体

SimCaptureManager - 俯视摄像机

SimSemanticMapWidget - 地图 UI 容器

SimPropertyPanelWidget - 点击元素显示属性

验收标准：JSON → 生成几何体 → 显示在 UI 地图上 → 点击显示属性



现在请你先做SimSemanticMapManager - 先生成简单几何体，做完告诉我写了哪几部分，作用是什么，我还要做什么。能在C++里完成就尽量不要在虚幻里操作