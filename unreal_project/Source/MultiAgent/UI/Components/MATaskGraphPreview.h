// MATaskGraphPreview.h
// 任务图预览组件 - 在右侧边栏显示紧凑的 DAG 可视化
// 纯 C++ 实现，使用 NativePaint 绘制简化的 DAG 图
// 支持状态颜色显示和鼠标悬浮提示

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Shared/Types/MATaskGraphTypes.h"
#include "MATaskGraphPreview.generated.h"

class UVerticalBox;
class UTextBlock;
class UBorder;
class USizeBox;
class UMAUITheme;

//=============================================================================
// 节点渲染数据
//=============================================================================

/** 预览节点渲染数据 */
USTRUCT()
struct FMAPreviewNodeRenderData
{
    GENERATED_BODY()

    /** 节点 ID */
    FString NodeId;

    /** 节点描述 */
    FString Description;

    /** 节点位置 */
    FString Location;

    /** 节点位置 (画布坐标) */
    FVector2D Position;

    /** 节点大小 */
    FVector2D Size;

    /** 执行状态 */
    EMATaskExecutionStatus Status;

    /** 层级 (用于布局) */
    int32 Level;

    /** 层内索引 (用于布局) */
    int32 IndexInLevel;

    FMAPreviewNodeRenderData() 
        : Position(FVector2D::ZeroVector)
        , Size(60.0f, 30.0f)
        , Status(EMATaskExecutionStatus::Pending)
        , Level(0)
        , IndexInLevel(0) 
    {}
};

/** 预览边渲染数据 */
USTRUCT()
struct FMAPreviewEdgeRenderData
{
    GENERATED_BODY()

    /** 起点位置 */
    FVector2D StartPoint;

    /** 终点位置 */
    FVector2D EndPoint;

    /** 边类型 */
    FString EdgeType;

    FMAPreviewEdgeRenderData() : StartPoint(FVector2D::ZeroVector), EndPoint(FVector2D::ZeroVector) {}
};

//=============================================================================
// MATaskGraphPreview 类
//=============================================================================

/**
 * 任务图预览 Widget
 * 
 * 在右侧边栏显示紧凑的 DAG 可视化：
 * - 使用 NativePaint 绘制简化的 DAG 图
 * - 节点颜色表示执行状态（灰色=未执行，黄色=执行中，绿色=成功，红色=失败）
 * - 鼠标悬浮显示详细信息
 * - 自动布局节点
 * - 支持主题化
 * 
 */
UCLASS()
class MULTIAGENT_API UMATaskGraphPreview : public UUserWidget
{
    GENERATED_BODY()

public:
    UMATaskGraphPreview(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 预览更新
    //=========================================================================

    /**
     * 更新预览内容
     * @param Data 任务图数据
     */
    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdatePreview(const FMATaskGraphData& Data);

    /**
     * 清空预览
     */
    UFUNCTION(BlueprintCallable, Category = "Preview")
    void ClearPreview();

    /**
     * 检查是否有数据
     * @return true 如果有任务图数据
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

    /** 当前任务图数据 */
    FMATaskGraphData CurrentData;

    /** 节点渲染数据 */
    TArray<FMAPreviewNodeRenderData> NodeRenderData;

    /** 边渲染数据 */
    TArray<FMAPreviewEdgeRenderData> EdgeRenderData;

    /** 悬浮的节点索引 (-1 表示无悬浮) */
    int32 HoveredNodeIndex = -1;

    /** 上次鼠标位置 */
    FVector2D LastMousePosition = FVector2D::ZeroVector;

    /** 缓存的几何信息 */
    mutable FGeometry CachedGeometry;

    //=========================================================================
    // 布局常量
    //=========================================================================

    /** 节点宽度 */
    float NodeWidth = 50.0f;

    /** 节点高度 */
    float NodeHeight = 24.0f;

    /** 节点水平间距 */
    float NodeHSpacing = 20.0f;

    /** 节点垂直间距 */
    float NodeVSpacing = 15.0f;

    /** 顶部边距 (为统计信息留空间) */
    float TopMargin = 25.0f;

    /** 左边距 */
    float LeftMargin = 10.0f;

    //=========================================================================
    // 颜色配置 (状态颜色)
    //=========================================================================

    /** 背景颜色 */
    FLinearColor BackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 边颜色 */
    FLinearColor EdgeColor = FLinearColor(0.5f, 0.5f, 0.6f, 0.8f);

    /** Pending 状态颜色 (灰色) */
    FLinearColor PendingColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

    /** InProgress 状态颜色 (黄色) */
    FLinearColor InProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);

    /** Completed 状态颜色 (绿色) */
    FLinearColor CompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);

    /** Failed 状态颜色 (红色) */
    FLinearColor FailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);

    /** 悬浮高亮颜色 */
    FLinearColor HoverHighlightColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);

    /** 工具提示背景颜色 */
    FLinearColor TooltipBgColor = FLinearColor(0.15f, 0.15f, 0.18f, 0.95f);

    /** 工具提示文本颜色 */
    FLinearColor TooltipTextColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);

    /** 统计文本颜色 */
    FLinearColor TextColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);

    /** 提示文本颜色 */
    FLinearColor HintTextColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 计算节点布局 */
    void CalculateLayout(const FGeometry& AllottedGeometry);

    /** 绘制 DAG 图 */
    void DrawDAG(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制节点 */
    void DrawNode(const FMAPreviewNodeRenderData& Node, int32 NodeIndex, const FGeometry& AllottedGeometry, 
                  FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制边 */
    void DrawEdge(const FMAPreviewEdgeRenderData& Edge, const FGeometry& AllottedGeometry, 
                  FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制箭头 */
    void DrawArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color,
                   const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制工具提示 */
    void DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 获取状态颜色 */
    FLinearColor GetStatusColor(EMATaskExecutionStatus Status) const;

    /** 查找鼠标位置下的节点索引 */
    int32 FindNodeAtPosition(const FVector2D& LocalPosition) const;
};
