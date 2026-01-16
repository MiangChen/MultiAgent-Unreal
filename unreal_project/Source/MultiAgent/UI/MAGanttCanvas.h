// MAGanttCanvas.h
// 甘特图画布 Widget - 可视化技能分配的甘特图组件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Core/Types/MATaskGraphTypes.h"
#include "MAGanttCanvas.generated.h"

class UMASkillAllocationModel;
class UBorder;
class UCanvasPanel;

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAGanttCanvas, Log, All);

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

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                              const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                              int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

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

    /** 获取状态颜色 */
    FLinearColor GetStatusColor(ESkillExecutionStatus Status) const;

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

    /** 绑定的数据模型 */
    UPROPERTY()
    UMASkillAllocationModel* AllocationModel;

    /** 技能条渲染数据 */
    TArray<FMASkillBarRenderData> SkillBarRenderData;

    /** 机器人 ID 顺序 */
    TArray<FString> RobotIdOrder;

    /** 时间步顺序 */
    TArray<int32> TimeStepOrder;

    //=========================================================================
    // 视图状态
    //=========================================================================

    /** 滚动偏移 */
    FVector2D ScrollOffset = FVector2D::ZeroVector;

    //=========================================================================
    // 布局常量 (放大 1.5 倍)
    //=========================================================================

    /** 时间步列宽 */
    float TimeStepWidth = 150.0f;

    /** 机器人行高 */
    float RobotRowHeight = 60.0f;

    /** 标题栏高度 */
    float HeaderHeight = 45.0f;

    /** 标签区域宽度 */
    float LabelWidth = 180.0f;

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
    // 颜色配置
    //=========================================================================

    /** 画布背景颜色 */
    FLinearColor CanvasBackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 网格线颜色 */
    FLinearColor GridLineColor = FLinearColor(0.15f, 0.15f, 0.2f, 1.0f);

    /** 文本颜色 */
    FLinearColor TextColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);

    /** Pending 状态颜色 (灰色) */
    FLinearColor PendingColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

    /** InProgress 状态颜色 (黄色) */
    FLinearColor InProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);

    /** Completed 状态颜色 (绿色) */
    FLinearColor CompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

    /** Failed 状态颜色 (红色) */
    FLinearColor FailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);

    /** 选中边框颜色 */
    FLinearColor SelectionColor = FLinearColor(0.3f, 0.7f, 1.0f, 1.0f);
};
