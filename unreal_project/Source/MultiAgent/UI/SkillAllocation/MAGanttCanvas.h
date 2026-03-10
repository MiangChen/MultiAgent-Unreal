// MAGanttCanvas.h
// 甘特图画布 Widget - 可视化技能分配的甘特图组件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"
#include "Gantt/MAGanttGridLayout.h"
#include "Gantt/MAGanttDragStateMachine.h"
#include "Gantt/MAGanttPainter.h"
#include "Gantt/MAGanttRenderSnapshot.h"
#include "MAGanttCanvas.generated.h"

class UMASkillAllocationModel;
class UMAUITheme;
class UBorder;
class UCanvasPanel;

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAGanttCanvas, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/** 拖拽完成委托 - 当技能块成功移动到新位置时广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGanttDragCompleted, 
    int32, SourceTimeStep, 
    const FString&, SourceRobotId,
    int32, TargetTimeStep, 
    const FString&, TargetRobotId);

/** 拖拽取消委托 - 当拖拽操作被取消时广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGanttSkillDragCancelled);

/** 拖拽被阻止委托 - 当执行期间尝试拖拽时广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGanttDragBlocked);

/** 拖拽开始委托 - 当拖拽操作开始时广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGanttDragStarted,
    const FString&, SkillName,
    int32, TimeStep,
    const FString&, RobotId);

/** 拖拽失败委托 - 当拖拽操作因无效目标失败时广播 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGanttDragFailed);

//=============================================================================
// UMAGanttCanvas - 甘特图画布 Widget
//=============================================================================

/**
 * 甘特图画布 Widget
 * 
 * 可视化技能分配的甘特图组件，支持：
 * - 时间步横轴，机器人纵轴
 * - 技能条渲染（不同状态不同颜色）
 * - 鼠标悬停显示工具提示
 * - 横向和纵向滚动
 * - 技能条选择
 */
UCLASS()
class MULTIAGENT_API UMAGanttCanvas : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAGanttCanvas(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 数据绑定
    //=========================================================================

    /** 绑定到数据模型 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas")
    void BindToModel(UMASkillAllocationModel* Model);

    /** 从模型刷新画布 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas")
    void RefreshFromModel();

    /** 获取绑定的模型 */
    UFUNCTION(BlueprintPure, Category = "GanttCanvas")
    UMASkillAllocationModel* GetModel() const { return AllocationModel; }

    //=========================================================================
    // 主题
    //=========================================================================

    /** 应用主题样式 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas|Theme")
    void ApplyTheme(UMAUITheme* InTheme);

    /** 获取当前主题 */
    UFUNCTION(BlueprintPure, Category = "GanttCanvas|Theme")
    UMAUITheme* GetTheme() const { return Theme; }

    //=========================================================================
    // 视图操作
    //=========================================================================

    /** 设置滚动偏移 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas")
    void SetScrollOffset(FVector2D Offset);

    /** 获取滚动偏移 */
    UFUNCTION(BlueprintPure, Category = "GanttCanvas")
    FVector2D GetScrollOffset() const { return ScrollOffset; }

    /** 重置视图 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas")
    void ResetView();

    //=========================================================================
    // 选择
    //=========================================================================

    /** 选中技能条 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas")
    void SelectSkillBar(int32 TimeStep, const FString& RobotId);

    /** 取消选中 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas")
    void ClearSelection();

    //=========================================================================
    // 拖拽控制
    //=========================================================================

    /** 检查是否正在拖拽 */
    UFUNCTION(BlueprintPure, Category = "GanttCanvas|Drag")
    bool IsDragging() const;

    /** 设置拖拽启用状态 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas|Drag")
    void SetDragEnabled(bool bEnabled);

    /** 检查拖拽是否启用 */
    UFUNCTION(BlueprintPure, Category = "GanttCanvas|Drag")
    bool IsDragEnabled() const;

    /** 取消当前拖拽操作 */
    UFUNCTION(BlueprintCallable, Category = "GanttCanvas|Drag")
    void CancelDrag();

    //=========================================================================
    // 拖拽事件委托
    //=========================================================================

    /** 拖拽完成事件 - 当技能块成功移动到新位置时广播 */
    UPROPERTY(BlueprintAssignable, Category = "GanttCanvas|Drag")
    FOnGanttDragCompleted OnDragCompleted;

    /** 拖拽取消事件 - 当拖拽操作被取消时广播 */
    UPROPERTY(BlueprintAssignable, Category = "GanttCanvas|Drag")
    FOnGanttSkillDragCancelled OnSkillDragCancelled;

    /** 拖拽被阻止事件 - 当执行期间尝试拖拽时广播 */
    UPROPERTY(BlueprintAssignable, Category = "GanttCanvas|Drag")
    FOnGanttDragBlocked OnDragBlocked;

    /** 拖拽开始事件 - 当拖拽操作开始时广播 */
    UPROPERTY(BlueprintAssignable, Category = "GanttCanvas|Drag")
    FOnGanttDragStarted OnDragStarted;

    /** 拖拽失败事件 - 当拖拽操作因无效目标失败时广播 */
    UPROPERTY(BlueprintAssignable, Category = "GanttCanvas|Drag")
    FOnGanttDragFailed OnDragFailed;

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                              const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                              int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    //=========================================================================
    // 绘制
    //=========================================================================

    /** 绘制时间轴标题 */
    void DrawTimelineHeader(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制机器人标签 */
    void DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制技能条 */
    void DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制时间步分隔竖线 */
    void DrawTimeStepGridLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 获取状态颜色 */
    FLinearColor GetStatusColor(ESkillExecutionStatus Status) const;

    //=========================================================================
    // 拖拽视觉渲染
    //=========================================================================

    /**
     * 绘制拖拽预览
     * 在鼠标位置绘制半透明的技能块预览，显示技能名称
     * @param AllottedGeometry Widget 几何信息
     * @param OutDrawElements 绘制元素列表
     * @param LayerId 图层 ID
     */
    void DrawDragPreview(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /**
     * 绘制拖拽源占位符
     * 在原始位置绘制虚线边框，表示技能块的原始位置
     * @param AllottedGeometry Widget 几何信息
     * @param OutDrawElements 绘制元素列表
     * @param LayerId 图层 ID
     */
    void DrawDragSourcePlaceholder(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /**
     * 绘制放置指示器
     * 在当前放置目标位置绘制有效/无效放置指示
     * 使用绿色边框表示可放置，红色边框表示不可放置
     * @param AllottedGeometry Widget 几何信息
     * @param OutDrawElements 绘制元素列表
     * @param LayerId 图层 ID
     */
    void DrawDropIndicator(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    //=========================================================================
    // 坐标转换
    //=========================================================================

    /** 时间步转换为屏幕 X 坐标 */
    float TimeStepToScreen(int32 TimeStep) const;

    /** 机器人索引转换为屏幕 Y 坐标 */
    float RobotIndexToScreen(int32 RobotIndex) const;

    /** 屏幕 X 坐标转换为时间步 */
    int32 ScreenToTimeStep(float ScreenX) const;

    /** 屏幕 Y 坐标转换为机器人索引 */
    int32 ScreenToRobotIndex(float ScreenY) const;

    //=========================================================================
    // 模型事件处理
    //=========================================================================

    /** 模型数据变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    /** 技能状态变更回调 */
    UFUNCTION()
    void OnSkillStatusChanged(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus Status);

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 画布背景 */
    UPROPERTY()
    UBorder* CanvasBackground;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    /** 绑定的数据模型 */
    UPROPERTY()
    UMASkillAllocationModel* AllocationModel;

    /** 渲染快照（Bars/RobotOrder/TimeStepOrder/SlotIndexMap） */
    FMAGanttRenderSnapshot RenderSnapshot;

    /** 网格布局与时间步索引映射 */
    FMAGanttGridLayout GridLayout;

    //=========================================================================
    // 视图状态
    //=========================================================================

    /** 滚动偏移 */
    FVector2D ScrollOffset = FVector2D::ZeroVector;

    //=========================================================================
    // 布局常量 (可动态调整以适应内容)
    //=========================================================================

    /** 时间步列宽 (基础值，会根据内容自适应) */
    float TimeStepWidth = 120.0f;

    /** 机器人行高 (基础值，会根据内容自适应) */
    float RobotRowHeight = 50.0f;

    /** 标题栏高度 */
    float HeaderHeight = 35.0f;

    /** 标签区域宽度 */
    float LabelWidth = 120.0f;

    /** 最小时间步列宽 */
    float MinTimeStepWidth = 80.0f;

    /** 最大时间步列宽 */
    float MaxTimeStepWidth = 200.0f;

    /** 最小机器人行高 */
    float MinRobotRowHeight = 35.0f;

    /** 最大机器人行高 */
    float MaxRobotRowHeight = 80.0f;

    /** 根据可用空间和数据量更新自适应布局 */
    void UpdateAdaptiveLayout(const FVector2D& AvailableSize);

    /** 构造绘制上下文 */
    FMAGanttPainterContext MakePainterContext(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements) const;

    /** 构造拖拽视觉状态 */
    FMAGanttPainterDragVisual MakeDragVisual() const;

    /** 构造拖拽状态机输入上下文 */
    FMAGanttDragStateContext MakeDragStateContext() const;

    /** 提交拖拽放置到模型并广播结果 */
    bool TryCommitDrop(const FGanttDragSource& DragSource, const FGanttDropTarget& FinalTarget);

    //=========================================================================
    // 交互状态
    //=========================================================================

    /** 选中的时间步 */
    int32 SelectedTimeStep = -1;

    /** 选中的机器人 ID */
    FString SelectedRobotId;

    /** 悬停的时间步 */
    int32 HoveredTimeStep = -1;

    /** 悬停的机器人 ID */
    FString HoveredRobotId;

    //=========================================================================
    // 颜色配置 (从 Theme 获取，以下为 fallback 默认值)
    //=========================================================================

    /** 画布背景颜色 (fallback: Theme->CanvasBackgroundColor) */
    FLinearColor CanvasBackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 网格线颜色 (fallback: Theme->GridLineColor) */
    FLinearColor GridLineColor = FLinearColor(0.15f, 0.15f, 0.2f, 1.0f);

    /** 文本颜色 (fallback: Theme->TextColor) */
    FLinearColor TextColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);

    /** Pending 状态颜色 (fallback: Theme->StatusPendingColor) */
    FLinearColor PendingColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

    /** InProgress 状态颜色 (fallback: Theme->StatusInProgressColor) */
    FLinearColor InProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);

    /** Completed 状态颜色 (fallback: Theme->StatusCompletedColor) */
    FLinearColor CompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

    /** Failed 状态颜色 (fallback: Theme->StatusFailedColor) */
    FLinearColor FailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);

    /** 选中边框颜色 (fallback: Theme->SelectionColor) */
    FLinearColor SelectionColor = FLinearColor(0.3f, 0.7f, 1.0f, 1.0f);

    //=========================================================================
    // 拖拽状态
    //=========================================================================

    /** 拖拽状态机 */
    FMAGanttDragStateMachine DragStateMachine;

    //=========================================================================
    // 拖拽配置常量
    //=========================================================================

    /** 吸附范围（像素）- 进入此范围内自动吸附到槽位 */
    float SnapRange = 20.0f;

    /** 有效放置指示颜色 (fallback: Theme->ValidDropColor) */
    FLinearColor ValidDropColor = FLinearColor(0.2f, 0.8f, 0.3f, 0.5f);

    /** 无效放置指示颜色 (fallback: Theme->InvalidDropColor) */
    FLinearColor InvalidDropColor = FLinearColor(0.9f, 0.2f, 0.2f, 0.5f);

    /** 占位符边框颜色 (fallback: Theme->SecondaryTextColor with alpha) */
    FLinearColor PlaceholderColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.8f);

    /** 上一次用于布局计算的 Widget 尺寸 */
    FVector2D LastLayoutSize = FVector2D(-1.0f, -1.0f);

    /** 布局是否需要重新计算 */
    bool bLayoutDirty = true;
};
