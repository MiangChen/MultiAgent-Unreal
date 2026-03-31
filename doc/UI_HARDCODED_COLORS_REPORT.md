# UI 硬编码颜色问题报告

本报告列出了 UI 代码中仍然存在的硬编码颜色问题，这些颜色没有使用 MAUITheme 主题系统。

## 修复进度

### 已修复 ✅

1. **MATaskPlannerWidget.cpp** - 任务规划工作台
   - TitleText, HintText, CloseText 使用 Theme 颜色
   - StatusLogLabel, JsonEditorLabel, UserInputLabel 使用 Theme->LabelTextColor
   - BackgroundOverlay 使用 Theme->OverlayColor
   - CloseButton hover/pressed 使用 Theme->DangerColor
   - ApplyTheme 方法已更新，会实际更新 UI 元素颜色

2. **MASkillAllocationViewer.cpp** - 技能分配查看器
   - TitleText, HintText, CloseText 使用 Theme 颜色
   - StatusLogLabel, JsonEditorLabel 使用 Theme->LabelTextColor
   - BackgroundOverlay 使用 Theme->OverlayColor
   - CloseButton hover/pressed 使用 Theme->DangerColor
   - 新增 ApplyTheme 方法

3. **MARightSidebarWidget.cpp** - 右侧边栏
   - CommandTitle, TaskGraphTitle, SkillListTitle, LogTitle 使用 Theme->TextColor 带 fallback

---

## 问题类型

### 类型 A: ApplyTheme 只更新变量，未更新 UI 元素
这类问题是 `ApplyTheme` 方法只更新了成员变量，但没有实际更新已创建的 UI 元素颜色。

### 类型 B: 直接使用 FLinearColor::White 或硬编码颜色值
这类问题是在 UI 构建时直接使用硬编码颜色，完全没有使用主题系统。

---

## 待修复问题详情

### ~~1. MATaskPlannerWidget.cpp (类型 A + B)~~ ✅ 已修复

---

### ~~2. MASkillAllocationViewer.cpp (类型 B)~~ ✅ 已修复

---

### ~~3. MARightSidebarWidget.cpp (类型 A + B)~~ ✅ 已修复

---

### 4. MASceneListWidget.cpp (类型 B)

**问题描述**: 空列表提示文字使用硬编码颜色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 309 | EmptyText (Goals 空列表) | (0.5, 0.5, 0.5) | Theme->HintTextColor |
| 366 | EmptyText (Zones 空列表) | (0.5, 0.5, 0.5) | Theme->HintTextColor |

---

### ~~5. MATaskGraphPreview.cpp (类型 B)~~ ✅ 已修复

**修复内容**:
- StatsText, EmptyText 使用成员变量 TextColor, HintTextColor
- DrawNode 中节点文字使用 Theme->InputTextColor
- ApplyTheme 方法完整实现，更新所有状态颜色和文本颜色

---

### 6. MANodePaletteWidget.cpp (类型 B)

**问题描述**: 按钮按下状态使用硬编码颜色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 182 | Button Pressed | (0.15, 0.15, 0.2) | Theme->SecondaryColor (darker) |
| 452 | Button Pressed | (0.15, 0.15, 0.2) | Theme->SecondaryColor (darker) |

---

### 7. MAEmergencyWidget.cpp (类型 B)

**问题描述**: 标题使用硬编码红色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 157 | TitleText | (1.0, 0.3, 0.3) 红色 | Theme->DangerColor |

---

### 8. MASimpleMainWidget.cpp (Legacy, 类型 B)

**问题描述**: 标题和提示文字使用硬编码颜色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 124 | TitleText | (0.9, 0.9, 1.0) | Theme->TextColor |
| 135 | HintText | (0.5, 0.5, 0.6) | Theme->HintTextColor |
| 168 | ResultLabel | (0.7, 0.7, 0.7) | Theme->SecondaryTextColor |

---

### ~~9. MANotificationWidget.cpp (类型 B)~~ ✅ 已修复

**修复内容**:
- NotificationText 使用 Theme->TextColor 带 fallback
- KeyHintText 使用 Theme->TextColor (带透明度调整) 带 fallback
- BuildUI 中使用主题字体

---

### 10. MAMiniMapWidget.cpp (类型 B)

**问题描述**: 图标颜色使用硬编码白色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 199 | IconColor (default) | FLinearColor::White | Theme->TextColor |
| 206 | IconColor (selected) | FLinearColor::White | Theme->SelectionColor |

---

### 11. MASetupWidget.cpp (类型 B)

**问题描述**: ComboBox 选中文字使用硬编码白色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 176 | SceneComboBox SelectedTextColor | FLinearColor::White | Theme->TextColor |
| 240 | AgentTypeComboBox SelectedTextColor | FLinearColor::White | Theme->TextColor |

---

### 12. MAStyledButton.cpp (类型 B)

**问题描述**: 动画状态使用硬编码白色

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 43 | TintColor (初始) | FLinearColor::White | 保持白色 (用于 tint) |
| 498 | TintColor (hover) | FLinearColor::White | 保持白色 (用于 tint) |
| 516 | TintColor (normal) | FLinearColor::White | 保持白色 (用于 tint) |

**注意**: MAStyledButton 的 TintColor 用于颜色叠加，保持白色是正确的。

---

### 13. MAHUDWidget.cpp (类型 B)

**问题描述**: 静态颜色常量使用硬编码值

| 行号 | 元素 | 当前颜色 | 应使用 |
|------|------|----------|--------|
| 21 | EditMode | (0.3, 0.6, 1.0) | Theme->ModeEditColor |
| 22 | POI | (0.3, 0.8, 0.3) | Theme->SuccessColor |
| 23 | Goal | (1.0, 0.4, 0.4) | Theme->DangerColor |
| 24 | Zone | (0.3, 0.8, 1.0) | Theme->PrimaryColor |

---

## 统计摘要

| 文件 | 硬编码颜色数量 | 状态 |
|------|---------------|------|
| MATaskPlannerWidget.cpp | 10 | ✅ 已修复 |
| MASkillAllocationViewer.cpp | 5 | ✅ 已修复 |
| MARightSidebarWidget.cpp | 4 | ✅ 已修复 |
| MATaskGraphPreview.cpp | 6 | ✅ 已修复 |
| MANotificationWidget.cpp | 2 | ✅ 已修复 |
| MASceneListWidget.cpp | 2 | 待修复 |
| MANodePaletteWidget.cpp | 2 | 待修复 |
| MAEmergencyWidget.cpp | 1 | 待修复 |
| MASimpleMainWidget.cpp | 3 | 待修复 (Legacy) |
| MAMiniMapWidget.cpp | 2 | 待修复 |
| MASetupWidget.cpp | 2 | 待修复 |
| MAHUDWidget.cpp | 4 | 待修复 |

**已修复**: 27 处 (高优先级 + 中优先级部分完成)
**待修复**: 约 16 处 (中低优先级)

---

## 修复优先级

### 高优先级 (用户可见的主要界面) ✅ 全部完成
1. ~~MATaskPlannerWidget.cpp - 任务规划工作台~~ ✅
2. ~~MASkillAllocationViewer.cpp - 技能分配查看器~~ ✅
3. ~~MARightSidebarWidget.cpp - 右侧边栏~~ ✅

### 中优先级 ✅ 部分完成
4. ~~MATaskGraphPreview.cpp - 任务图预览~~ ✅
5. ~~MANotificationWidget.cpp - 通知组件~~ ✅
6. MAHUDWidget.cpp - HUD 状态颜色

### 低优先级 (待修复)
7. 其他组件

---

## 修复模式

对于类型 A 问题 (ApplyTheme 未更新 UI)，需要：

```cpp
// 1. 在头文件中添加 TextBlock 引用
UPROPERTY()
UTextBlock* StatusLogLabel;

// 2. 在 BuildUI 中保存引用
StatusLogLabel = WidgetTree->ConstructWidget<UTextBlock>(...);

// 3. 在 ApplyTheme 中更新颜色
void UMATaskPlannerWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme) return;
    
    // 更新标签颜色
    if (StatusLogLabel)
    {
        StatusLogLabel->SetColorAndOpacity(FSlateColor(Theme->LabelTextColor));
    }
    // ... 其他元素
}
```

对于类型 B 问题 (直接硬编码)，需要：
1. 添加 Theme 检查
2. 使用 Theme 变量或 fallback 默认值
