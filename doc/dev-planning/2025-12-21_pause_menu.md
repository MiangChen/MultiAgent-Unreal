# Tab 暂停菜单功能设计

## 需求背景

目前 tab 键在 Editor 模式下会退出当前界面，希望改为类似仿真的暂停菜单体验。

## 功能设计

### 暂停菜单界面

```
┌─────────────────────────────┐
│                             │
│         PAUSED              │
│                             │
│      [ 继续仿真 ]           │
│      [ 按键说明 ]           │
│      [ 设置 ]               │
│      [ 返回主菜单 ]         │
│      [ 退出仿真 ]           │
│                             │
└─────────────────────────────┘
```

### 按键行为

| 状态 | tab 行为 |
|------|----------|
| 正常仿真 | 打开暂停菜单，暂停仿真 |
| 暂停菜单中 | 关闭菜单，恢复仿真 |
|                |                        |
| 其他 UI 打开时 | 关闭当前 UI |

### 菜单选项功能

1. **继续仿真**：关闭菜单，恢复仿真
2. **按键说明**：显示所有快捷键（可从 KEYBINDINGS.md 生成）
3. **设置**：
   - 音量控制
   - 鼠标灵敏度
   - 画质设置（可选）
4. **返回主菜单**：返回 Setup 界面或主菜单
5. **退出仿真**：退出应用程序

## 技术实现

### 新增文件

- `MAPauseMenuWidget.h/cpp` - 暂停菜单 Widget
- `MAKeybindingsWidget.h/cpp` - 按键说明 Widget（可选）
- `MASettingsWidget.h/cpp` - 设置界面 Widget（可选）

### 修改文件

- `MAInputActions.cpp` - 添加 tab 键绑定
- `MAPlayerController.cpp` - 处理暂停逻辑
- `MAHUD.cpp` - 管理暂停菜单显示

### 核心代码逻辑

```cpp
// MAPlayerController.cpp
void AMAPlayerController::OntabapePressed()
{

    
    // 1. 如果有其他 UI 打开，关闭它
    if (IsAnyUIOpen())
    {
        CloseCurrentUI();
        return;
    }
    
    // 2. 切换暂停状态
    TogglePauseMenu();
}

void AMAPlayerController::TogglePauseMenu()
{
    if (bIsPaused)
    {
        // 恢复仿真
        UGameplayStatics::SetGamePaused(GetWorld(), false);
        HidePauseMenu();
        SetInputMode(FInputModeGameAndUI());
    }
    else
    {
        // 暂停仿真
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        ShowPauseMenu();
        SetInputMode(FInputModeUIOnly());
    }
    bIsPaused = !bIsPaused;
}
```

### Widget 结构

```cpp
// MAPauseMenuWidget
UCLASS()
class UMAPauseMenuWidget : public UUserWidget
{
    // 按钮
    UButton* ResumeButton;
    UButton* KeybindingsButton;
    UButton* SettingsButton;
    UButton* MainMenuButton;
    UButton* QuitButton;
    
    // 回调
    void OnResumeClicked();
    void OnKeybindingsClicked();
    void OnSettingsClicked();
    void OnMainMenuClicked();
    void OnQuitClicked();
};
```

## UI 样式建议

- 半透明蓝色背景遮罩
- 显示在最上层
- 居中显示菜单面板
- 按钮悬停高亮效果
- 简洁的字体和配色（与现有 UI 风格一致）

## 优先级

1. **P0**：基础暂停/恢复功能
2. **P1**：按键说明界面
3. **P2**：设置界面
4. **P3**：返回主菜单功能

## 相关文件

- `doc/KEYBINDINGS.md` - 按键说明数据源
- `MAHUD.cpp/h` - HUD 管理
- `MAPlayerController.cpp/h` - 输入处理
