# UMG Widget 点击穿透问题排查经验

## 问题描述

在 Edit 模式下：
1. 点击 UI 区域不会触发场景交互（如创建 POI）— 这是正确的
2. 但 UI 本身不响应点击（按钮不工作，文本框不获取焦点）
3. 对比：Modify 模式的 UI 工作正常

## 排查过程

### 第一阶段：错误的假设

最初假设问题是 Enhanced Input 消费了鼠标事件，导致 UMG 收不到点击。

尝试的方案：
```cpp
// 在 OnLeftClick 中检测 UI 并提前 return
void AMAPlayerController::OnLeftClick(const FInputActionValue& Value)
{
    if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
        {
            if (HUD->IsMouseOverEditWidget())
            {
                return;  // 希望让 UMG 处理点击
            }
        }
    }
    // ...
}
```

**结果**：无效。在 `OnLeftClick` 中 return 不会让 Widget 收到点击。

### 第二阶段：关键发现

观察到 **Modify 模式没有任何 UI 检测代码，但按钮点击正常工作**。

这说明：
1. UMG 按钮的 `OnClicked` 事件是独立于 Enhanced Input 的
2. Enhanced Input 处理鼠标事件不会阻止 UMG 接收点击
3. 问题不在于"谁先处理事件"

### 第三阶段：对比分析

对比 Edit 模式和 Modify 模式的差异：

| 方面 | Modify 模式 | Edit 模式 |
|------|-------------|-----------|
| Widget 数量 | 1 个 (ModifyWidget) | 2 个 (EditWidget + SceneListWidget) |
| SetWidgetToFocus | 有 | 最初没有，后来加上 |
| UI 检测 | 无 | 有 |

### 第四阶段：定位问题

通过逐步禁用功能，发现问题与 **多 Widget 同时显示** 有关：
- 禁用 SceneListWidget 后，EditWidget 的按钮可能恢复正常
- 两个 Widget 同时显示时，焦点管理出现问题

## 解决方案

### 方案 1：禁用多余的 Widget（当前采用）

```cpp
void AMAHUD::ShowEditWidget()
{
    // 只显示 EditWidget，不显示 SceneListWidget
    EditWidget->SetVisibility(ESlateVisibility::Visible);
    
    // 暂时禁用左侧 SceneListWidget
    // if (SceneListWidget)
    // {
    //     SceneListWidget->SetVisibility(ESlateVisibility::Visible);
    // }
    
    // 设置焦点
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(EditWidget->TakeWidget());
        // ...
    }
}
```

### 方案 2：合并 Widget（推荐长期方案）

将 SceneListWidget 的内容合并到 EditWidget 中，避免多 Widget 焦点问题。

### 方案 3：深入研究 UMG 焦点管理（复杂）

如果必须使用多个 Widget，需要研究：
- `SetWidgetToFocus` 的正确用法
- Widget 的 Z-Order 和焦点优先级
- `IsFocusable` 属性设置
- Slate 焦点路径 (Focus Path)

## 关键经验

### 1. UMG 和 Enhanced Input 是独立的

```
Enhanced Input 处理: IA_LeftClick → OnLeftClick()
UMG 处理: 鼠标点击 → Widget::OnMouseButtonDown → Button::OnClicked

两者并行工作，互不干扰
```

### 2. 在 OnLeftClick 中 return 不会帮助 UMG

错误理解：
> "如果我在 OnLeftClick 中 return，事件就会传递给 UMG"

正确理解：
> "Enhanced Input 和 UMG 各自独立处理鼠标事件，return 只是不执行你的代码"

### 3. 场景交互阻止应该在具体处理函数中

```cpp
// 正确做法：在具体处理函数中检测 UI
void AMAPlayerController::OnEditLeftClick()
{
    // 检测 UI，阻止场景交互
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        if (HUD->IsMouseOverEditWidget())
        {
            return;  // 不创建 POI，但 UMG 按钮仍然正常工作
        }
    }
    
    // 创建 POI 的代码...
}
```

### 4. 多 Widget 焦点问题

当同时显示多个 Widget 时：
- `SetWidgetToFocus` 只能设置一个焦点
- 其他 Widget 可能无法正常接收输入
- 建议：尽量使用单一 Widget，或仔细管理焦点

### 5. 参考工作正常的代码

当遇到 UI 问题时，找一个工作正常的类似功能对比：
- Modify 模式 vs Edit 模式
- 对比代码差异，逐步排除

## 相关文件

- `unreal_project/Source/MultiAgent/UI/MAHUD.cpp` - ShowEditWidget, IsMouseOverEditWidget
- `unreal_project/Source/MultiAgent/Input/MAPlayerController.cpp` - OnLeftClick, OnEditLeftClick
- `unreal_project/Source/MultiAgent/UI/MAEditWidget.cpp` - EditWidget 实现
- `unreal_project/Source/MultiAgent/UI/MAModifyWidget.cpp` - ModifyWidget 实现（参考）

## 调试技巧

### 添加日志

```cpp
UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnEditLeftClick: Mouse over UI = %s"), 
    HUD->IsMouseOverEditWidget() ? TEXT("true") : TEXT("false"));
```

### 检查鼠标位置

```cpp
float MouseX, MouseY;
PC->GetMousePosition(MouseX, MouseY);
UE_LOG(LogTemp, Log, TEXT("Mouse position: (%f, %f)"), MouseX, MouseY);
```

### 检查 Widget 可见性和焦点

```cpp
UE_LOG(LogTemp, Log, TEXT("EditWidget visible: %s, focusable: %s"),
    EditWidget->IsVisible() ? TEXT("true") : TEXT("false"),
    EditWidget->IsFocusable() ? TEXT("true") : TEXT("false"));
```
