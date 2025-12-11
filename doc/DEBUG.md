# 调试指南

## UI 系统调试

### 常见问题排查

#### 1. 按 Z 键无反应

**检查步骤**:
1. 确认 `MAGameMode` 使用了 `AMAHUD::StaticClass()`
2. 确认 `MAPlayerController` 绑定了 `IA_ToggleMainUI`
3. 检查 Output Log 是否有 `[PlayerController] ToggleMainUI called`

**相关日志**:
```
LogTemp: [PlayerController] ToggleMainUI called
LogMAHUD: Creating SimpleMainWidget...
LogMAHUD: SimpleMainWidget created successfully
```

#### 2. UI 创建但不可见

**检查步骤**:
1. 确认日志显示 `MainUI shown`
2. 检查 Widget 的锚点和位置设置
3. 确认 `AddToViewport(10)` 的 ZOrder 足够高

**调试代码**:
```cpp
// 在 MASimpleMainWidget::BuildUI() 中添加
UE_LOG(LogMASimpleWidget, Warning, TEXT("Widget Position: %s"), 
    *BorderSlot->GetPosition().ToString());
UE_LOG(LogMASimpleWidget, Warning, TEXT("Widget Size: %s"), 
    *BorderSlot->GetSize().ToString());
```

#### 3. 指令提交无响应

**检查步骤**:
1. 确认 `OnCommandSubmitted` 委托已绑定
2. 确认 `MACommSubsystem` 正确初始化
3. 检查 `OnPlannerResponse` 委托绑定

**相关日志**:
```
LogMASimpleWidget: Submitting command: 测试指令
LogMAHUD: Command submitted: 测试指令
LogMACommSubsystem: Generating mock response for: 测试指令
LogMAHUD: Planner response received: [模拟响应] 收到指令: 测试指令
```

### 调试工具

#### 1. Widget 调试器

在编辑器中使用 **Window → Developer Tools → Widget Reflector** 查看 Widget 层次结构。

#### 2. 输入调试

在 `MAPlayerController.cpp` 中添加调试日志：
```cpp
void AMAPlayerController::OnToggleMainUI(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnToggleMainUI called, Value: %f"), Value.Get<float>());
    // ... 原有代码
}
```

## 编译问题调试

### 常见编译错误

#### 1. 找不到头文件

**错误**: `fatal error: 'MASimpleMainWidget.h' file not found`

**解决**: 确认文件路径正确，重新生成项目文件：
```bash
./compile.sh
```

#### 2. 链接错误

**错误**: `undefined reference to UMASimpleMainWidget::StaticClass()`

**解决**: 确认 `.cpp` 文件包含了正确的头文件，检查 `GENERATED_BODY()` 宏。

#### 3. UMG 模块依赖

**错误**: `UEditableTextBox is not defined`

**解决**: 在 `MultiAgent.Build.cs` 中添加 UMG 依赖：
```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "UMG", "Slate", "SlateCore"
});
```

## 性能调试

### UI 性能监控

使用 UE5 的 Stat 命令监控 UI 性能：
```
stat fps          # 显示帧率
stat unit         # 显示各系统耗时
stat slate        # 显示 Slate UI 性能
```

## 日志配置

### 启用详细日志

在 `DefaultEngine.ini` 中添加：
```ini
[Core.Log]
LogMAHUD=Verbose
LogMASimpleWidget=Verbose
LogMACommSubsystem=Verbose
```

### 运行时日志过滤

在控制台中使用：
```
log LogMAHUD Verbose    # 启用 MAHUD 详细日志
log LogMAHUD Off        # 关闭 MAHUD 日志
```

## 常用调试命令

| 命令 | 功能 |
|------|------|
| `showdebug input` | 显示输入调试信息 |
| `widget.reflector` | 打开 Widget 反射器 |
| `stat slate` | 显示 Slate 性能统计 |
| `log list` | 列出所有日志类别 |
| `toggledebugcamera` | 切换调试相机 |
