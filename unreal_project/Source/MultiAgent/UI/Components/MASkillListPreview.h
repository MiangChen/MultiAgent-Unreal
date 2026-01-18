// MASkillListPreview.h
// 技能列表预览组件 - 在右侧边栏显示紧凑的甘特图可视化
// 纯 C++ 实现，使用 NativePaint 绘制简化的甘特图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MASkillListPreview.generated.h"

class UVerticalBox;
class UTextBlock;
class UBorder;
class USizeBox;
class UMAUITheme;

//=============================================================================
// 技能条渲染数据
//=============================================================================

/** 预览技能条渲染数据 */
USTRUCT()
struct FMAPreviewSkillBarData
{
    GENERATED_BODY()

    /** 机器人 ID */
    FString RobotId;

    /** 技能名称 */
    FString SkillName;

    /** 时间步 */
    int32 TimeStep;

    /** 机器人索引 (用于 Y 坐标计算) */
    int32 RobotIndex;

    /** 状态 */
    ESkillExecutionStatus Status;

    /** 位置 */
    FVector2D Position;

    /** 大小 */
    FVector2D Size;

    FMAPreviewSkillBarData() : TimeStep(0), RobotIndex(0), Status(ESkillExecutionStatus::Pending), 
                               Position(FVector2D::ZeroVector), Size(FVector2D::ZeroVector) {}
};

//=============================================================================
// MASkillListPreview 类
//=============================================================================

/**
 * 技能列表预览 Widget
 * 
 * 在右侧边栏显示紧凑的甘特图可视化：
 * - 使用 NativePaint 绘制简化的甘特图
 * - 横轴为时间步，纵轴为机器人
 * - 不同状态使用不同颜色
 * - 支持主题化
 * 
 * Requirements: 11.5
 */
UCLASS()
class MULTIAGENT_API UMASkillListPreview : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASkillListPreview(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 预览更新
    //=========================================================================

    /**
     * 更新预览内容
     * @param Data 技能分配数据
     */
    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdatePreview(const FMASkillAllocationData& Data);

    /**
     * 更新单个技能的执行状态
     * 用于执行过程中实时更新显示
     * @param TimeStep 时间步
     * @param RobotId 机器人 ID
     * @param NewStatus 新状态
     */
    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    /**
     * 清空预览
     */
    UFUNCTION(BlueprintCallable, Category = "Preview")
    void ClearPreview();

    /**
     * 检查是否有数据
     * @return true 如果有技能分配数据
     */
    UFUNCTION(BlueprintPure, Category = "Preview")
    bool HasData() const { return bHasData; }

    //=========================================================================
    // 主题应用
    //=========================================================================

    /**
     * 应用主题
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    void ApplyTheme(UMAUITheme* InTheme);

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
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 预览高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "50.0", ClampMax = "300.0"))
    float PreviewHeight = 150.0f;

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 SizeBox */
    UPROPERTY()
    USizeBox* RootSizeBox;

    /** 内容边框 */
    UPROPERTY()
    UBorder* ContentBorder;

    /** 统计信息文本 */
    UPROPERTY()
    UTextBlock* StatsText;

    /** 空状态提示文本 */
    UPROPERTY()
    UTextBlock* EmptyText;

    //=========================================================================
    // 状态
    //=========================================================================

    /** 是否有数据 */
    bool bHasData = false;

    /** 当前技能分配数据 */
    FMASkillAllocationData CurrentData;

    /** 技能条渲染数据 */
    TArray<FMAPreviewSkillBarData> SkillBarRenderData;

    /** 机器人 ID 列表 */
    TArray<FString> RobotIds;

    /** 时间步列表 */
    TArray<int32> TimeSteps;

    //=========================================================================
    // 布局常量
    //=========================================================================

    /** 顶部边距 (为统计信息留空间) */
    float TopMargin = 25.0f;

    /** 左边距 (为机器人标签留空间) */
    float LeftMargin = 60.0f;

    /** 右边距 */
    float RightMargin = 10.0f;

    /** 底部边距 */
    float BottomMargin = 5.0f;

    /** 行高 */
    float RowHeight = 20.0f;

    /** 条形间距 */
    float BarPadding = 2.0f;

    //=========================================================================
    // 颜色配置
    //=========================================================================

    /** 背景颜色 */
    FLinearColor BackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 网格线颜色 */
    FLinearColor GridLineColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.5f);

    /** 文本颜色 */
    FLinearColor TextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

    /** Pending 状态颜色 (灰色) */
    FLinearColor PendingColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

    /** InProgress 状态颜色 (黄色) */
    FLinearColor InProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);

    /** Completed 状态颜色 (绿色) */
    FLinearColor CompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

    /** Failed 状态颜色 (红色) */
    FLinearColor FailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 计算布局 */
    void CalculateLayout(const FGeometry& AllottedGeometry);

    /** 绘制甘特图 */
    void DrawGantt(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制网格线 */
    void DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制机器人标签 */
    void DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制时间轴标签 */
    void DrawTimeLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制技能条 */
    void DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 获取状态颜色 */
    FLinearColor GetStatusColor(ESkillExecutionStatus Status) const;

    /** 查找鼠标位置下的技能条索引 */
    int32 FindSkillBarAtPosition(const FVector2D& LocalPosition) const;

    /** 绘制工具提示 */
    void DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    //=========================================================================
    // 悬浮状态
    //=========================================================================

    /** 悬浮的技能条索引 (-1 表示无悬浮) */
    int32 HoveredBarIndex = -1;

    /** 上次鼠标位置 */
    FVector2D LastMousePosition = FVector2D::ZeroVector;

    /** 缓存的几何信息 */
    mutable FGeometry CachedGeometry;

    /** 工具提示背景颜色 */
    FLinearColor TooltipBgColor = FLinearColor(0.15f, 0.15f, 0.18f, 0.95f);

    /** 工具提示文本颜色 */
    FLinearColor TooltipTextColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);

    /** 悬浮高亮颜色 */
    FLinearColor HoverHighlightColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
};
