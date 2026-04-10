// MAGanttCanvas.cpp
// 甘特图画布 Widget 实现

#include "MAGanttCanvas.h"
#include "../MASkillAllocationModel.h"
#include "../../Core/MAUITheme.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY(LogMAGanttCanvas);

//=============================================================================
// 构造函数
//=============================================================================

UMAGanttCanvas::UMAGanttCanvas(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// 主题
//=============================================================================

void UMAGanttCanvas::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("ApplyTheme: Theme is null, using fallback defaults"));
        return;
    }
    
    // 画布颜色
    CanvasBackgroundColor = Theme->CanvasBackgroundColor;
    GridLineColor = Theme->GridLineColor;
    
    // 文本颜色
    TextColor = Theme->TextColor;
    
    // 状态颜色
    PendingColor = Theme->StatusPendingColor;
    InProgressColor = Theme->StatusInProgressColor;
    CompletedColor = Theme->StatusCompletedColor;
    FailedColor = Theme->StatusFailedColor;
    
    // 交互颜色
    SelectionColor = Theme->SelectionColor;
    ValidDropColor = Theme->ValidDropColor;
    InvalidDropColor = Theme->InvalidDropColor;
    
    // 占位符颜色 - 使用 SecondaryTextColor 并调整透明度
    PlaceholderColor = Theme->SecondaryTextColor;
    PlaceholderColor.A = 0.8f;
    
    // 更新画布背景
    // 更新画布背景颜色 - 使用圆角效果
    if (CanvasBackground)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(CanvasBackground, CanvasBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 刷新技能条颜色
    for (FMASkillBarRenderData& RenderData : RenderSnapshot.Bars)
    {
        RenderData.Color = GetStatusColor(RenderData.Status);
    }
    
    UE_LOG(LogMAGanttCanvas, Log, TEXT("ApplyTheme: Theme applied successfully"));
}

//=============================================================================
// 数据绑定
//=============================================================================

void UMAGanttCanvas::BindToModel(UMASkillAllocationModel* Model)
{
    // 解绑旧模型
    if (AllocationModel)
    {
        AllocationModel->OnDataChanged.RemoveDynamic(this, &UMAGanttCanvas::OnModelDataChanged);
        AllocationModel->OnSkillStatusChanged.RemoveDynamic(this, &UMAGanttCanvas::OnSkillStatusChanged);
    }

    AllocationModel = Model;

    // 绑定新模型
    if (AllocationModel)
    {
        AllocationModel->OnDataChanged.AddDynamic(this, &UMAGanttCanvas::OnModelDataChanged);
        AllocationModel->OnSkillStatusChanged.AddDynamic(this, &UMAGanttCanvas::OnSkillStatusChanged);
        RefreshFromModel();
    }

    UE_LOG(LogMAGanttCanvas, Log, TEXT("Bound to model: %s"), AllocationModel ? TEXT("Valid") : TEXT("Null"));
}

void UMAGanttCanvas::RefreshFromModel()
{
    bLayoutDirty = true;

    if (!AllocationModel)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("RefreshFromModel: No model bound"));
        // Clear existing data to prevent stale rendering
        RenderSnapshot.Reset();
        GridLayout.Reset();
        UpdateAdaptiveLayout(LastLayoutSize);
        return;
    }

    // Check if model has valid data
    if (!AllocationModel->IsValidData())
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("RefreshFromModel: Model has no valid data"));
        RenderSnapshot.Reset();
        GridLayout.Reset();
        UpdateAdaptiveLayout(LastLayoutSize);
        return;
    }

    // 清空现有数据
    RenderSnapshot.Reset();
    GridLayout.Reset();

    // 获取所有机器人 ID 和时间步
    RenderSnapshot.RobotOrder = AllocationModel->GetAllRobotIds();
    RenderSnapshot.TimeStepOrder = AllocationModel->GetAllTimeSteps();

    // Safety check: ensure we have data to render
    if (RenderSnapshot.RobotOrder.Num() == 0 || RenderSnapshot.TimeStepOrder.Num() == 0)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("RefreshFromModel: No robots or time steps to render"));
        UpdateAdaptiveLayout(LastLayoutSize);
        return;
    }

    // 排序
    RenderSnapshot.RobotOrder.Sort();
    RenderSnapshot.TimeStepOrder.Sort();
    RenderSnapshot.RebuildOrderIndices();
    GridLayout.SetTimeStepOrder(RenderSnapshot.TimeStepOrder);

    // 为所有技能生成渲染数据
    for (int32 TimeStep : RenderSnapshot.TimeStepOrder)
    {
        // Safety check: skip invalid time steps
        if (TimeStep < 0)
        {
            UE_LOG(LogMAGanttCanvas, Warning, TEXT("RefreshFromModel: Skipping invalid time step %d"), TimeStep);
            continue;
        }

        for (int32 RobotIndex = 0; RobotIndex < RenderSnapshot.RobotOrder.Num(); RobotIndex++)
        {
            const FString& RobotId = RenderSnapshot.RobotOrder[RobotIndex];
            
            // Safety check: skip empty robot IDs
            if (RobotId.IsEmpty())
            {
                UE_LOG(LogMAGanttCanvas, Warning, TEXT("RefreshFromModel: Skipping empty robot ID at index %d"), RobotIndex);
                continue;
            }
            
            // 查找该时间步和机器人的技能
            FMASkillAssignment Skill;
            if (AllocationModel->FindSkill(TimeStep, RobotId, Skill))
            {
                FMASkillBarRenderData RenderData;
                RenderData.TimeStep = TimeStep;
                RenderData.RobotId = RobotId;
                RenderData.SkillName = Skill.SkillName;
                RenderData.ParamsJson = Skill.ParamsJson;
                RenderData.Status = Skill.Status;
                
                // 计算位置和大小（使用较大的间距）
                const float SkillBarPadding = 10.0f;
                RenderData.Position.X = TimeStepToScreen(TimeStep) + SkillBarPadding / 2.0f;
                RenderData.Position.Y = RobotIndexToScreen(RobotIndex) + SkillBarPadding / 2.0f;
                RenderData.Size.X = TimeStepWidth - SkillBarPadding;
                RenderData.Size.Y = RobotRowHeight - SkillBarPadding;
                
                // 设置颜色
                RenderData.Color = GetStatusColor(Skill.Status);
                
                RenderSnapshot.Bars.Add(RenderData);
            }
        }
    }

    RenderSnapshot.RebuildSlotIndex();

    UE_LOG(LogMAGanttCanvas, Log, TEXT("Refreshed from model: %d robots, %d time steps, %d skill bars"), 
           RenderSnapshot.RobotOrder.Num(), RenderSnapshot.TimeStepOrder.Num(), RenderSnapshot.Bars.Num());

    UpdateAdaptiveLayout(LastLayoutSize);
}

//=============================================================================
// 视图操作
//=============================================================================

void UMAGanttCanvas::SetScrollOffset(FVector2D Offset)
{
    const FVector2D OldOffset = ScrollOffset;

    // 限制滚动范围
    float MaxScrollX = FMath::Max(0.0f, RenderSnapshot.TimeStepOrder.Num() * TimeStepWidth - 800.0f);
    float MaxScrollY = FMath::Max(0.0f, RenderSnapshot.RobotOrder.Num() * RobotRowHeight - 600.0f);
    
    ScrollOffset.X = FMath::Clamp(Offset.X, 0.0f, MaxScrollX);
    ScrollOffset.Y = FMath::Clamp(Offset.Y, 0.0f, MaxScrollY);

    if (!ScrollOffset.Equals(OldOffset, KINDA_SMALL_NUMBER))
    {
        bLayoutDirty = true;
        UpdateAdaptiveLayout(LastLayoutSize);
    }
}

void UMAGanttCanvas::ResetView()
{
    if (!ScrollOffset.IsZero())
    {
        ScrollOffset = FVector2D::ZeroVector;
        bLayoutDirty = true;
        UpdateAdaptiveLayout(LastLayoutSize);
    }
}

//=============================================================================
// 选择
//=============================================================================

void UMAGanttCanvas::SelectSkillBar(int32 TimeStep, const FString& RobotId)
{
    SelectedTimeStep = TimeStep;
    SelectedRobotId = RobotId;
    
    // 更新渲染数据中的选中状态
    for (FMASkillBarRenderData& RenderData : RenderSnapshot.Bars)
    {
        RenderData.bIsSelected = (RenderData.TimeStep == TimeStep && RenderData.RobotId == RobotId);
    }
    
    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Selected skill bar: TimeStep=%d, RobotId=%s"), TimeStep, *RobotId);
}

void UMAGanttCanvas::ClearSelection()
{
    SelectedTimeStep = -1;
    SelectedRobotId.Empty();
    
    // 清除所有选中状态
    for (FMASkillBarRenderData& RenderData : RenderSnapshot.Bars)
    {
        RenderData.bIsSelected = false;
    }
}

//=============================================================================
// 拖拽控制
//=============================================================================

bool UMAGanttCanvas::IsDragging() const
{
    return DragStateMachine.IsDragging();
}

bool UMAGanttCanvas::IsDragEnabled() const
{
    return DragStateMachine.IsDragEnabled();
}

void UMAGanttCanvas::SetDragEnabled(bool bEnabled)
{
    // 如果正在拖拽时禁用，先取消拖拽
    if (!bEnabled && DragStateMachine.IsDragActive())
    {
        CancelDrag();
    }

    DragStateMachine.SetDragEnabled(bEnabled);
    
    UE_LOG(LogMAGanttCanvas, Log, TEXT("Drag enabled: %s"), bEnabled ? TEXT("true") : TEXT("false"));
}

void UMAGanttCanvas::CancelDrag()
{
    if (!DragStateMachine.CancelDrag())
    {
        return;
    }
    
    UE_LOG(LogMAGanttCanvas, Log, TEXT("[Cancel] Drag operation cancelled"));

    // 广播取消事件
    OnSkillDragCancelled.Broadcast();
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAGanttCanvas::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAGanttCanvas::NativeConstruct()
{
    Super::NativeConstruct();

    bLayoutDirty = true;
    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("NativeConstruct"));
}

void UMAGanttCanvas::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateAdaptiveLayout(MyGeometry.GetLocalSize());
}

TSharedRef<SWidget> UMAGanttCanvas::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    if (!WidgetTree->RootWidget)
    {
        BuildUI();
    }

    return Super::RebuildWidget();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMAGanttCanvas::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAGanttCanvas, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("BuildUI: Starting UI construction..."));

    // 在构建 UI 之前先从主题获取颜色
    if (!Theme)
    {
        Theme = NewObject<UMAUITheme>();
    }
    CanvasBackgroundColor = Theme->CanvasBackgroundColor;

    // 创建背景边框 - 使用圆角效果
    CanvasBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CanvasBackground"));
    MARoundedBorderUtils::ApplyRoundedCorners(CanvasBackground, CanvasBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    WidgetTree->RootWidget = CanvasBackground;
    
    // 启用裁剪 - 确保内容不会溢出边界
    CanvasBackground->SetClipping(EWidgetClipping::ClipToBoundsAlways);

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("BuildUI: UI construction completed"));
}

//=============================================================================
// 坐标转换
//=============================================================================

float UMAGanttCanvas::TimeStepToScreen(int32 TimeStep) const
{
    return GridLayout.TimeStepToScreen(TimeStep, LabelWidth, TimeStepWidth, ScrollOffset.X);
}

float UMAGanttCanvas::RobotIndexToScreen(int32 RobotIndex) const
{
    return GridLayout.RobotIndexToScreen(RobotIndex, HeaderHeight, RobotRowHeight, ScrollOffset.Y);
}

int32 UMAGanttCanvas::ScreenToTimeStep(float ScreenX) const
{
    return GridLayout.ScreenToTimeStep(ScreenX, LabelWidth, TimeStepWidth, ScrollOffset.X);
}

int32 UMAGanttCanvas::ScreenToRobotIndex(float ScreenY) const
{
    return GridLayout.ScreenToRobotIndex(ScreenY, HeaderHeight, RobotRowHeight, ScrollOffset.Y);
}

//=============================================================================
// 模型事件处理
//=============================================================================

void UMAGanttCanvas::OnModelDataChanged()
{
    RefreshFromModel();
}

void UMAGanttCanvas::OnSkillStatusChanged(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus Status)
{
    // Validate inputs
    if (TimeStep < 0)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("OnSkillStatusChanged: Invalid TimeStep %d, ignoring"), TimeStep);
        return;
    }
    
    if (RobotId.IsEmpty())
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("OnSkillStatusChanged: Empty RobotId, ignoring"));
        return;
    }

    // 更新对应技能条的颜色
    bool bFound = false;
    if (FMASkillBarRenderData* RenderData = RenderSnapshot.FindMutableBarAtSlot(TimeStep, RobotId))
    {
        RenderData->Status = Status;
        RenderData->Color = GetStatusColor(Status);
        bFound = true;
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Updated skill bar color: TimeStep=%d, RobotId=%s, Status=%d"),
               TimeStep, *RobotId, (int32)Status);
    }
    
    if (!bFound)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("OnSkillStatusChanged: Skill bar not found for TimeStep=%d, RobotId=%s"), 
               TimeStep, *RobotId);
    }
}

//=============================================================================
// 绘制
//=============================================================================

FLinearColor UMAGanttCanvas::GetStatusColor(ESkillExecutionStatus Status) const
{
    switch (Status)
    {
        case ESkillExecutionStatus::Pending:
            return PendingColor;
        case ESkillExecutionStatus::InProgress:
            return InProgressColor;
        case ESkillExecutionStatus::Completed:
            return CompletedColor;
        case ESkillExecutionStatus::Failed:
            return FailedColor;
        default:
            return PendingColor;
    }
}

int32 UMAGanttCanvas::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                   const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                   int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // 检查父 Widget 是否可见，如果不可见则不绘制
    if (!IsVisible() || GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden)
    {
        return LayerId;
    }

    // 先绘制子 Widget
    int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, 
                                          LayerId, InWidgetStyle, bParentEnabled);

    // 绘制时间步分隔竖线（在技能条下方）
    DrawTimeStepGridLines(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制时间轴标题
    DrawTimelineHeader(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制机器人标签
    DrawRobotLabels(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制技能条 (包含技能名称文字，需要额外层)
    DrawSkillBars(AllottedGeometry, OutDrawElements, LayerId + 2);

    //=========================================================================
    // 拖拽视觉渲染
    // 在拖拽状态下绘制拖拽相关的视觉元素
    // 图层顺序：占位符 -> 放置指示器 -> 拖拽预览（最上层）
    //=========================================================================
    if (DragStateMachine.IsDragging())
    {
        // 绘制原始位置占位符（虚线边框）
        DrawDragSourcePlaceholder(AllottedGeometry, OutDrawElements, LayerId + 5);

        // 绘制放置指示器（有效/无效状态）
        DrawDropIndicator(AllottedGeometry, OutDrawElements, LayerId + 6);

        // 绘制拖拽预览（半透明技能块）
        // 预览在最上层，确保始终可见
        DrawDragPreview(AllottedGeometry, OutDrawElements, LayerId + 8);
    }

    return MaxLayerId + 12;
}

void UMAGanttCanvas::UpdateAdaptiveLayout(const FVector2D& AvailableSize)
{
    const bool bSizeValid = AvailableSize.X > KINDA_SMALL_NUMBER && AvailableSize.Y > KINDA_SMALL_NUMBER;
    const bool bSizeChanged = !AvailableSize.Equals(LastLayoutSize, KINDA_SMALL_NUMBER);
    if (!bLayoutDirty && !bSizeChanged)
    {
        return;
    }

    if (!bSizeValid)
    {
        return;
    }

    LastLayoutSize = AvailableSize;
    bLayoutDirty = false;

    // 计算可用于数据区域的空间（减去标签和标题区域）
    const float DataAreaWidth = FMath::Max(0.0f, AvailableSize.X - LabelWidth);
    const float DataAreaHeight = FMath::Max(0.0f, AvailableSize.Y - HeaderHeight);
    
    // 获取数据量
    const int32 NumTimeSteps = RenderSnapshot.TimeStepOrder.Num();
    const int32 NumRobots = RenderSnapshot.RobotOrder.Num();
    
    // 如果没有数据，使用默认值
    if (NumTimeSteps <= 0 || NumRobots <= 0)
    {
        TimeStepWidth = 120.0f;
        RobotRowHeight = 50.0f;
        return;
    }
    
    // 计算理想的单元格大小（使内容恰好填满可用空间）
    const float IdealTimeStepWidth = DataAreaWidth / NumTimeSteps;
    const float IdealRobotRowHeight = DataAreaHeight / NumRobots;
    
    // 限制在最小和最大范围内
    TimeStepWidth = FMath::Clamp(IdealTimeStepWidth, MinTimeStepWidth, MaxTimeStepWidth);
    RobotRowHeight = FMath::Clamp(IdealRobotRowHeight, MinRobotRowHeight, MaxRobotRowHeight);
    
    // 更新技能条渲染数据的位置和大小
    // 使用较大的间距使技能块之间有更多空隙
    const float SkillBarPadding = 10.0f;
    for (FMASkillBarRenderData& RenderData : RenderSnapshot.Bars)
    {
        // 找到机器人索引
        const int32 RobotIndex = RenderSnapshot.FindRobotIndex(RenderData.RobotId);
        if (RobotIndex != INDEX_NONE)
        {
            // 重新计算位置和大小（增加间距）
            RenderData.Position.X = TimeStepToScreen(RenderData.TimeStep) + SkillBarPadding * 0.5f;
            RenderData.Position.Y = RobotIndexToScreen(RobotIndex) + SkillBarPadding * 0.5f;
            RenderData.Size.X = TimeStepWidth - SkillBarPadding;
            RenderData.Size.Y = RobotRowHeight - SkillBarPadding;
        }
    }
}

FMAGanttPainterContext UMAGanttCanvas::MakePainterContext(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements) const
{
    return {
        AllottedGeometry,
        OutDrawElements,
        RenderSnapshot.TimeStepOrder,
        RenderSnapshot.RobotOrder,
        RenderSnapshot.Bars,
        GridLayout,
        LabelWidth,
        HeaderHeight,
        TimeStepWidth,
        RobotRowHeight,
        ScrollOffset,
        GridLineColor,
        TextColor,
        SelectionColor,
        ValidDropColor,
        InvalidDropColor,
        PlaceholderColor,
        Theme ? Theme->InputTextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f)
    };
}

FMAGanttPainterDragVisual UMAGanttCanvas::MakeDragVisual() const
{
    return DragStateMachine.GetDragVisualState();
}

FMAGanttDragStateContext UMAGanttCanvas::MakeDragStateContext() const
{
    FMAGanttDragStateContext Context;
    Context.RenderSnapshot = &RenderSnapshot;
    Context.GridLayout = &GridLayout;
    Context.LabelWidth = LabelWidth;
    Context.HeaderHeight = HeaderHeight;
    Context.TimeStepWidth = TimeStepWidth;
    Context.RobotRowHeight = RobotRowHeight;
    Context.SnapRange = SnapRange;
    Context.PendingColor = PendingColor;
    Context.InProgressColor = InProgressColor;
    Context.CompletedColor = CompletedColor;
    Context.FailedColor = FailedColor;
    Context.ScrollOffset = ScrollOffset;

    return Context;
}

bool UMAGanttCanvas::TryCommitDrop(const FGanttDragSource& DragSource, const FGanttDropTarget& FinalTarget)
{
    if (!FinalTarget.bIsValid || !FinalTarget.HasTarget())
    {
        UE_LOG(LogMAGanttCanvas, Log, TEXT("[Failed] Cannot drop: target slot is occupied or invalid"));
        OnDragFailed.Broadcast();
        return false;
    }

    if (!AllocationModel)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("Drop failed: No allocation model bound"));
        return false;
    }

    const bool bDropSuccess = AllocationModel->MoveSkill(
        DragSource.TimeStep, DragSource.RobotId,
        FinalTarget.TimeStep, FinalTarget.RobotId
    );

    if (bDropSuccess)
    {
        UE_LOG(LogMAGanttCanvas, Log, TEXT("[Success] Skill moved: %s from T%d to T%d"),
               *DragSource.SkillName, DragSource.TimeStep, FinalTarget.TimeStep);
        OnDragCompleted.Broadcast(
            DragSource.TimeStep, DragSource.RobotId,
            FinalTarget.TimeStep, FinalTarget.RobotId
        );
        return true;
    }

    UE_LOG(LogMAGanttCanvas, Log, TEXT("[Failed] Cannot drop: target slot is occupied"));
    OnDragFailed.Broadcast();
    return false;
}

void UMAGanttCanvas::DrawTimelineHeader(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawTimelineHeader(MakePainterContext(AllottedGeometry, OutDrawElements), LayerId);
}

void UMAGanttCanvas::DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawRobotLabels(MakePainterContext(AllottedGeometry, OutDrawElements), LayerId);
}

void UMAGanttCanvas::DrawTimeStepGridLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawTimeStepGridLines(MakePainterContext(AllottedGeometry, OutDrawElements), LayerId);
}

void UMAGanttCanvas::DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawSkillBars(MakePainterContext(AllottedGeometry, OutDrawElements), LayerId);
}

//=============================================================================
// 拖拽视觉渲染
//=============================================================================

void UMAGanttCanvas::DrawDragPreview(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawDragPreview(MakePainterContext(AllottedGeometry, OutDrawElements), MakeDragVisual(), LayerId);
}

void UMAGanttCanvas::DrawDragSourcePlaceholder(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawDragSourcePlaceholder(MakePainterContext(AllottedGeometry, OutDrawElements), MakeDragVisual(), LayerId);
}

void UMAGanttCanvas::DrawDropIndicator(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FMAGanttPainter::DrawDropIndicator(MakePainterContext(AllottedGeometry, OutDrawElements), MakeDragVisual(), LayerId);
}

//=============================================================================
// 鼠标交互
//=============================================================================

FReply UMAGanttCanvas::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        const FMAGanttDragMouseDownResult DownResult = DragStateMachine.HandleMouseButtonDown(
            LocalPosition, MakeDragStateContext());

        if (!DownResult.bHandled)
        {
            return FReply::Unhandled();
        }

        if (DownResult.bDragBlocked)
        {
            UE_LOG(LogMAGanttCanvas, Log, TEXT("[Warning] Cannot modify skill allocation during execution"));
            OnDragBlocked.Broadcast();
        }

        if (DownResult.bClearSelection)
        {
            ClearSelection();
        }

        if (DownResult.bSelectSkill)
        {
            SelectSkillBar(DownResult.SelectedTimeStep, DownResult.SelectedRobotId);
        }

        if (DownResult.bPotentialDragStarted)
        {
            const FGanttDragSource& DragSource = DownResult.DragSource;
            UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Potential drag started: TimeStep=%d, RobotId=%s, Skill=%s"),
                   DragSource.TimeStep, *DragSource.RobotId, *DragSource.SkillName);
        }

        if (DownResult.bCaptureMouse)
        {
            return FReply::Handled().CaptureMouse(TakeWidget());
        }

        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply UMAGanttCanvas::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    const FMAGanttDragMouseMoveResult MoveResult = DragStateMachine.HandleMouseMove(
        LocalPosition, MakeDragStateContext());

    if (MoveResult.bDragStarted)
    {
        const FGanttDragSource& DragSource = MoveResult.DragSource;
        UE_LOG(LogMAGanttCanvas, Log, TEXT("[Drag] Start moving skill: %s (T%d, %s)"),
               *DragSource.SkillName, DragSource.TimeStep, *DragSource.RobotId);
        OnDragStarted.Broadcast(DragSource.SkillName, DragSource.TimeStep, DragSource.RobotId);
    }

    if (MoveResult.bDragUpdated)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Drag update: Position=(%.1f, %.1f), Snapped=%s, Valid=%s"),
               MoveResult.DragPreview.Position.X, MoveResult.DragPreview.Position.Y,
               MoveResult.DropTarget.bIsSnapped ? TEXT("true") : TEXT("false"),
               MoveResult.DropTarget.bIsValid ? TEXT("true") : TEXT("false"));
    }

    if (MoveResult.bHasHoverUpdate)
    {
        HoveredTimeStep = -1;
        HoveredRobotId.Empty();
        if (MoveResult.bHoverSkill)
        {
            HoveredTimeStep = MoveResult.HoveredTimeStep;
            HoveredRobotId = MoveResult.HoveredRobotId;
        }
    }

    return MoveResult.bHandled ? FReply::Handled() : FReply::Unhandled();
}

FReply UMAGanttCanvas::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 滚动
    float WheelDelta = InMouseEvent.GetWheelDelta();
    FVector2D NewOffset = ScrollOffset;
    
    // 垂直滚动
    NewOffset.Y -= WheelDelta * 40.0f;
    
    SetScrollOffset(NewOffset);
    
    return FReply::Handled();
}

void UMAGanttCanvas::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void UMAGanttCanvas::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    
    // 清除悬停状态
    HoveredTimeStep = -1;
    HoveredRobotId.Empty();
}

FReply UMAGanttCanvas::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        const FMAGanttDragMouseUpResult UpResult = DragStateMachine.HandleMouseButtonUp(
            LocalPosition, MakeDragStateContext());

        if (!UpResult.bHandled)
        {
            return FReply::Unhandled();
        }

        if (UpResult.bSelectSkill)
        {
            SelectSkillBar(UpResult.SelectedTimeStep, UpResult.SelectedRobotId);
            UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Click detected (no drag), selected skill bar"));
        }

        if (UpResult.bHasDropAttempt)
        {
            const FGanttDragSource& DragSource = UpResult.DragSource;
            const FGanttDropTarget& FinalTarget = UpResult.DropTarget;
            TryCommitDrop(DragSource, FinalTarget);
        }

        if (UpResult.bReleaseMouse)
        {
            return FReply::Handled().ReleaseMouseCapture();
        }

        return FReply::Handled();
    }
    
    return FReply::Unhandled();
}

FReply UMAGanttCanvas::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    //=========================================================================
    // 处理 Escape 键取消拖拽
    //=========================================================================
    
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (DragStateMachine.IsDragActive())
        {
            // 取消拖拽操作
            CancelDrag();
            
            // 释放鼠标捕获
            return FReply::Handled().ReleaseMouseCapture();
        }
    }
    
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
