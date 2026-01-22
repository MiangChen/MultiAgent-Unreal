// MAGanttCanvas.cpp
// 甘特图画布 Widget 实现

#include "MAGanttCanvas.h"
#include "MASkillAllocationModel.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateTypes.h"

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
    for (FMASkillBarRenderData& RenderData : SkillBarRenderData)
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
    if (!AllocationModel)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("RefreshFromModel: No model bound"));
        // Clear existing data to prevent stale rendering
        SkillBarRenderData.Empty();
        RobotIdOrder.Empty();
        TimeStepOrder.Empty();
        return;
    }

    // Check if model has valid data
    if (!AllocationModel->IsValidData())
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("RefreshFromModel: Model has no valid data"));
        SkillBarRenderData.Empty();
        RobotIdOrder.Empty();
        TimeStepOrder.Empty();
        return;
    }

    // 清空现有数据
    SkillBarRenderData.Empty();
    RobotIdOrder.Empty();
    TimeStepOrder.Empty();

    // 获取所有机器人 ID 和时间步
    RobotIdOrder = AllocationModel->GetAllRobotIds();
    TimeStepOrder = AllocationModel->GetAllTimeSteps();

    // Safety check: ensure we have data to render
    if (RobotIdOrder.Num() == 0 || TimeStepOrder.Num() == 0)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("RefreshFromModel: No robots or time steps to render"));
        return;
    }

    // 排序
    RobotIdOrder.Sort();
    TimeStepOrder.Sort();

    // 为所有技能生成渲染数据
    for (int32 TimeStep : TimeStepOrder)
    {
        // Safety check: skip invalid time steps
        if (TimeStep < 0)
        {
            UE_LOG(LogMAGanttCanvas, Warning, TEXT("RefreshFromModel: Skipping invalid time step %d"), TimeStep);
            continue;
        }

        for (int32 RobotIndex = 0; RobotIndex < RobotIdOrder.Num(); RobotIndex++)
        {
            const FString& RobotId = RobotIdOrder[RobotIndex];
            
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
                
                SkillBarRenderData.Add(RenderData);
            }
        }
    }

    UE_LOG(LogMAGanttCanvas, Log, TEXT("Refreshed from model: %d robots, %d time steps, %d skill bars"), 
           RobotIdOrder.Num(), TimeStepOrder.Num(), SkillBarRenderData.Num());
}

//=============================================================================
// 视图操作
//=============================================================================

void UMAGanttCanvas::SetScrollOffset(FVector2D Offset)
{
    // 限制滚动范围
    float MaxScrollX = FMath::Max(0.0f, TimeStepOrder.Num() * TimeStepWidth - 800.0f);
    float MaxScrollY = FMath::Max(0.0f, RobotIdOrder.Num() * RobotRowHeight - 600.0f);
    
    ScrollOffset.X = FMath::Clamp(Offset.X, 0.0f, MaxScrollX);
    ScrollOffset.Y = FMath::Clamp(Offset.Y, 0.0f, MaxScrollY);
}

void UMAGanttCanvas::ResetView()
{
    ScrollOffset = FVector2D::ZeroVector;
}

//=============================================================================
// 选择
//=============================================================================

void UMAGanttCanvas::SelectSkillBar(int32 TimeStep, const FString& RobotId)
{
    SelectedTimeStep = TimeStep;
    SelectedRobotId = RobotId;
    
    // 更新渲染数据中的选中状态
    for (FMASkillBarRenderData& RenderData : SkillBarRenderData)
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
    for (FMASkillBarRenderData& RenderData : SkillBarRenderData)
    {
        RenderData.bIsSelected = false;
    }
}

//=============================================================================
// 拖拽控制 (Requirements 8.1, 8.3)
//=============================================================================

void UMAGanttCanvas::SetDragEnabled(bool bEnabled)
{
    // 如果正在拖拽时禁用，先取消拖拽
    if (!bEnabled && DragState != EGanttDragState::Idle)
    {
        CancelDrag();
    }
    
    bDragEnabled = bEnabled;
    
    UE_LOG(LogMAGanttCanvas, Log, TEXT("Drag enabled: %s"), bEnabled ? TEXT("true") : TEXT("false"));
}

void UMAGanttCanvas::CancelDrag()
{
    if (DragState == EGanttDragState::Idle)
    {
        return;
    }
    
    UE_LOG(LogMAGanttCanvas, Log, TEXT("[取消] 拖拽操作已取消"));
    
    // 重置拖拽状态
    DragState = EGanttDragState::Idle;
    DragSource.Reset();
    CurrentDropTarget.Reset();
    DragPreview.Reset();
    MouseDownPosition = FVector2D::ZeroVector;
    
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

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("NativeConstruct"));
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
    return LabelWidth + TimeStep * TimeStepWidth - ScrollOffset.X;
}

float UMAGanttCanvas::RobotIndexToScreen(int32 RobotIndex) const
{
    return HeaderHeight + RobotIndex * RobotRowHeight - ScrollOffset.Y;
}

int32 UMAGanttCanvas::ScreenToTimeStep(float ScreenX) const
{
    float AdjustedX = ScreenX - LabelWidth + ScrollOffset.X;
    return FMath::FloorToInt(AdjustedX / TimeStepWidth);
}

int32 UMAGanttCanvas::ScreenToRobotIndex(float ScreenY) const
{
    float AdjustedY = ScreenY - HeaderHeight + ScrollOffset.Y;
    return FMath::FloorToInt(AdjustedY / RobotRowHeight);
}

//=============================================================================
// 槽位查询和验证 (Requirements 2.1, 2.2, 2.3, 6.1)
//=============================================================================

bool UMAGanttCanvas::GetSlotAtPosition(const FVector2D& Position, int32& OutTimeStep, FString& OutRobotId) const
{
    // 初始化输出参数
    OutTimeStep = -1;
    OutRobotId.Empty();

    // 检查是否在标签区（左侧机器人标签区域）
    if (Position.X < LabelWidth)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("GetSlotAtPosition: Position (%.1f, %.1f) is in label area"), 
               Position.X, Position.Y);
        return false;
    }

    // 检查是否在标题区（顶部时间步标题区域）
    if (Position.Y < HeaderHeight)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("GetSlotAtPosition: Position (%.1f, %.1f) is in header area"), 
               Position.X, Position.Y);
        return false;
    }

    // 计算时间步和机器人索引
    int32 TimeStep = ScreenToTimeStep(Position.X);
    int32 RobotIndex = ScreenToRobotIndex(Position.Y);

    // 验证时间步是否在有效范围内
    if (TimeStep < 0 || TimeStepOrder.Num() == 0)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("GetSlotAtPosition: TimeStep %d is out of range"), TimeStep);
        return false;
    }

    // 检查时间步是否存在于数据中
    if (!TimeStepOrder.Contains(TimeStep))
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("GetSlotAtPosition: TimeStep %d not found in data"), TimeStep);
        return false;
    }

    // 验证机器人索引是否在有效范围内
    if (RobotIndex < 0 || RobotIndex >= RobotIdOrder.Num())
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("GetSlotAtPosition: RobotIndex %d is out of range (0-%d)"), 
               RobotIndex, RobotIdOrder.Num() - 1);
        return false;
    }

    // 设置输出参数
    OutTimeStep = TimeStep;
    OutRobotId = RobotIdOrder[RobotIndex];

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("GetSlotAtPosition: Position (%.1f, %.1f) -> TimeStep=%d, RobotId=%s"), 
           Position.X, Position.Y, OutTimeStep, *OutRobotId);

    return true;
}

bool UMAGanttCanvas::IsSlotEmpty(int32 TimeStep, const FString& RobotId) const
{
    // 验证输入参数
    if (TimeStep < 0 || RobotId.IsEmpty())
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("IsSlotEmpty: Invalid parameters (TimeStep=%d, RobotId=%s)"), 
               TimeStep, *RobotId);
        return false;
    }

    // 检查是否绑定了数据模型
    if (!AllocationModel)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("IsSlotEmpty: No allocation model bound"));
        return true; // 没有模型时认为槽位为空
    }

    // 查询模型检查槽位是否有技能
    FMASkillAssignment Skill;
    bool bHasSkill = AllocationModel->FindSkill(TimeStep, RobotId, Skill);

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("IsSlotEmpty: TimeStep=%d, RobotId=%s -> %s"), 
           TimeStep, *RobotId, bHasSkill ? TEXT("Occupied") : TEXT("Empty"));

    return !bHasSkill;
}

bool UMAGanttCanvas::IsValidDropTarget(int32 TargetTimeStep, const FString& TargetRobotId) const
{
    // 验证输入参数
    if (TargetTimeStep < 0 || TargetRobotId.IsEmpty())
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("IsValidDropTarget: Invalid parameters (TimeStep=%d, RobotId=%s)"), 
               TargetTimeStep, *TargetRobotId);
        return false;
    }

    // 检查目标是否与拖拽源相同（排除拖拽源位置）
    if (DragState != EGanttDragState::Idle && DragSource.IsValid())
    {
        if (TargetTimeStep == DragSource.TimeStep && TargetRobotId == DragSource.RobotId)
        {
            UE_LOG(LogMAGanttCanvas, Verbose, TEXT("IsValidDropTarget: Target is same as drag source"));
            return false;
        }
    }

    // 检查目标槽位是否为空
    bool bIsEmpty = IsSlotEmpty(TargetTimeStep, TargetRobotId);

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("IsValidDropTarget: TimeStep=%d, RobotId=%s -> %s"), 
           TargetTimeStep, *TargetRobotId, bIsEmpty ? TEXT("Valid") : TEXT("Invalid (Occupied)"));

    return bIsEmpty;
}

//=============================================================================
// 吸附计算 (Requirements 3.1, 3.2, 3.4, 3.5)
//=============================================================================

FVector2D UMAGanttCanvas::GetSlotCenterPosition(int32 TimeStep, int32 RobotIndex) const
{
    // 计算槽位左上角位置
    float SlotX = TimeStepToScreen(TimeStep);
    float SlotY = RobotIndexToScreen(RobotIndex);
    
    // 返回槽位中心位置
    return FVector2D(
        SlotX + TimeStepWidth / 2.0f,
        SlotY + RobotRowHeight / 2.0f
    );
}

bool UMAGanttCanvas::IsWithinSnapRange(const FVector2D& Position, int32 TimeStep, int32 RobotIndex) const
{
    // 验证输入参数
    if (TimeStep < 0 || RobotIndex < 0 || RobotIndex >= RobotIdOrder.Num())
    {
        return false;
    }

    // 检查时间步是否存在于数据中
    if (!TimeStepOrder.Contains(TimeStep))
    {
        return false;
    }

    // 获取槽位中心位置
    FVector2D SlotCenter = GetSlotCenterPosition(TimeStep, RobotIndex);

    // 计算位置与槽位中心的距离
    float Distance = FVector2D::Distance(Position, SlotCenter);

    // 与 SnapRange 阈值比较
    bool bWithinRange = Distance <= SnapRange;

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("IsWithinSnapRange: Position (%.1f, %.1f) -> SlotCenter (%.1f, %.1f), Distance=%.1f, SnapRange=%.1f -> %s"),
           Position.X, Position.Y, SlotCenter.X, SlotCenter.Y, Distance, SnapRange,
           bWithinRange ? TEXT("Within") : TEXT("Outside"));

    return bWithinRange;
}

FGanttDropTarget UMAGanttCanvas::CalculateSnapTarget(const FVector2D& Position) const
{
    FGanttDropTarget Result;
    Result.Reset();

    // 检查是否有有效数据
    if (TimeStepOrder.Num() == 0 || RobotIdOrder.Num() == 0)
    {
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("CalculateSnapTarget: No data available"));
        return Result;
    }

    // 首先尝试获取当前位置对应的槽位
    int32 CurrentTimeStep = -1;
    FString CurrentRobotId;
    
    if (!GetSlotAtPosition(Position, CurrentTimeStep, CurrentRobotId))
    {
        // 位置不在有效数据区域内
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("CalculateSnapTarget: Position (%.1f, %.1f) not in valid data area"),
               Position.X, Position.Y);
        return Result;
    }

    // 获取当前机器人索引
    int32 CurrentRobotIndex = RobotIdOrder.IndexOfByKey(CurrentRobotId);
    if (CurrentRobotIndex == INDEX_NONE)
    {
        UE_LOG(LogMAGanttCanvas, Warning, TEXT("CalculateSnapTarget: RobotId %s not found in order list"), *CurrentRobotId);
        return Result;
    }

    // 查找最近的空白槽位
    float MinDistance = FLT_MAX;
    int32 BestTimeStep = -1;
    int32 BestRobotIndex = -1;
    FVector2D BestSnapPosition = FVector2D::ZeroVector;

    // 搜索范围：当前槽位及其相邻槽位（上下左右各一格）
    TArray<TPair<int32, int32>> SearchSlots;
    
    // 当前槽位
    SearchSlots.Add(TPair<int32, int32>(CurrentTimeStep, CurrentRobotIndex));
    
    // 相邻槽位（如果存在）
    int32 TimeStepIdx = TimeStepOrder.IndexOfByKey(CurrentTimeStep);
    if (TimeStepIdx != INDEX_NONE)
    {
        // 左侧槽位
        if (TimeStepIdx > 0)
        {
            SearchSlots.Add(TPair<int32, int32>(TimeStepOrder[TimeStepIdx - 1], CurrentRobotIndex));
        }
        // 右侧槽位
        if (TimeStepIdx < TimeStepOrder.Num() - 1)
        {
            SearchSlots.Add(TPair<int32, int32>(TimeStepOrder[TimeStepIdx + 1], CurrentRobotIndex));
        }
    }
    // 上方槽位
    if (CurrentRobotIndex > 0)
    {
        SearchSlots.Add(TPair<int32, int32>(CurrentTimeStep, CurrentRobotIndex - 1));
    }
    // 下方槽位
    if (CurrentRobotIndex < RobotIdOrder.Num() - 1)
    {
        SearchSlots.Add(TPair<int32, int32>(CurrentTimeStep, CurrentRobotIndex + 1));
    }

    // 遍历搜索槽位，找到最近的空白槽位
    for (const auto& Slot : SearchSlots)
    {
        int32 TestTimeStep = Slot.Key;
        int32 TestRobotIndex = Slot.Value;

        // 验证索引有效性
        if (TestRobotIndex < 0 || TestRobotIndex >= RobotIdOrder.Num())
        {
            continue;
        }

        const FString& TestRobotId = RobotIdOrder[TestRobotIndex];

        // 检查槽位是否为空（排除拖拽源）
        if (!IsValidDropTarget(TestTimeStep, TestRobotId))
        {
            continue;
        }

        // 计算距离
        FVector2D SlotCenter = GetSlotCenterPosition(TestTimeStep, TestRobotIndex);
        float Distance = FVector2D::Distance(Position, SlotCenter);

        // 检查是否在吸附范围内且是最近的
        if (Distance <= SnapRange && Distance < MinDistance)
        {
            MinDistance = Distance;
            BestTimeStep = TestTimeStep;
            BestRobotIndex = TestRobotIndex;
            BestSnapPosition = SlotCenter;
        }
    }

    // 如果找到了有效的吸附目标
    if (BestTimeStep >= 0 && BestRobotIndex >= 0)
    {
        Result.TimeStep = BestTimeStep;
        Result.RobotId = RobotIdOrder[BestRobotIndex];
        Result.bIsValid = true;
        Result.bIsSnapped = true;
        Result.SnapPosition = BestSnapPosition;

        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("CalculateSnapTarget: Found snap target TimeStep=%d, RobotId=%s, SnapPosition=(%.1f, %.1f)"),
               Result.TimeStep, *Result.RobotId, Result.SnapPosition.X, Result.SnapPosition.Y);
    }
    else
    {
        // 没有找到吸附目标，但位置在有效区域内
        // 返回当前槽位信息，但不吸附
        Result.TimeStep = CurrentTimeStep;
        Result.RobotId = CurrentRobotId;
        Result.bIsValid = IsValidDropTarget(CurrentTimeStep, CurrentRobotId);
        Result.bIsSnapped = false;
        Result.SnapPosition = FVector2D::ZeroVector;

        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("CalculateSnapTarget: No snap target found, current slot TimeStep=%d, RobotId=%s, Valid=%s"),
               Result.TimeStep, *Result.RobotId, Result.bIsValid ? TEXT("true") : TEXT("false"));
    }

    return Result;
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
    for (FMASkillBarRenderData& RenderData : SkillBarRenderData)
    {
        if (RenderData.TimeStep == TimeStep && RenderData.RobotId == RobotId)
        {
            RenderData.Status = Status;
            RenderData.Color = GetStatusColor(Status);
            bFound = true;
            UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Updated skill bar color: TimeStep=%d, RobotId=%s, Status=%d"), 
                   TimeStep, *RobotId, (int32)Status);
            break;
        }
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

    // 计算自适应布局（根据可用空间和数据量动态调整单元格大小）
    CalculateAdaptiveLayout(AllottedGeometry.GetLocalSize());

    // 绘制时间步分隔竖线（在技能条下方）
    DrawTimeStepGridLines(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制时间轴标题
    DrawTimelineHeader(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制机器人标签
    DrawRobotLabels(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制技能条 (包含技能名称文字，需要额外层)
    DrawSkillBars(AllottedGeometry, OutDrawElements, LayerId + 2);

    //=========================================================================
    // 拖拽视觉渲染 (Requirements 1.2, 1.3, 2.1)
    // 在拖拽状态下绘制拖拽相关的视觉元素
    // 图层顺序：占位符 -> 放置指示器 -> 拖拽预览（最上层）
    //=========================================================================
    if (DragState == EGanttDragState::Dragging)
    {
        // 绘制原始位置占位符（虚线边框）(Requirements 1.3)
        DrawDragSourcePlaceholder(AllottedGeometry, OutDrawElements, LayerId + 5);

        // 绘制放置指示器（有效/无效状态）(Requirements 2.1, 2.2, 2.4)
        DrawDropIndicator(AllottedGeometry, OutDrawElements, LayerId + 6);

        // 绘制拖拽预览（半透明技能块）(Requirements 1.2, 1.5)
        // 预览在最上层，确保始终可见
        DrawDragPreview(AllottedGeometry, OutDrawElements, LayerId + 8);
    }

    return MaxLayerId + 12;
}

void UMAGanttCanvas::CalculateAdaptiveLayout(const FVector2D& AvailableSize) const
{
    // 使用 mutable 或 const_cast 来修改布局参数（因为这是在 const 函数中）
    UMAGanttCanvas* MutableThis = const_cast<UMAGanttCanvas*>(this);
    
    // 计算可用于数据区域的空间（减去标签和标题区域）
    float DataAreaWidth = AvailableSize.X - LabelWidth;
    float DataAreaHeight = AvailableSize.Y - HeaderHeight;
    
    // 获取数据量
    int32 NumTimeSteps = TimeStepOrder.Num();
    int32 NumRobots = RobotIdOrder.Num();
    
    // 如果没有数据，使用默认值
    if (NumTimeSteps <= 0 || NumRobots <= 0)
    {
        MutableThis->TimeStepWidth = 120.0f;
        MutableThis->RobotRowHeight = 50.0f;
        return;
    }
    
    // 计算理想的单元格大小（使内容恰好填满可用空间）
    float IdealTimeStepWidth = DataAreaWidth / NumTimeSteps;
    float IdealRobotRowHeight = DataAreaHeight / NumRobots;
    
    // 限制在最小和最大范围内
    MutableThis->TimeStepWidth = FMath::Clamp(IdealTimeStepWidth, MinTimeStepWidth, MaxTimeStepWidth);
    MutableThis->RobotRowHeight = FMath::Clamp(IdealRobotRowHeight, MinRobotRowHeight, MaxRobotRowHeight);
    
    // 更新技能条渲染数据的位置和大小
    // 使用较大的间距使技能块之间有更多空隙
    const float SkillBarPadding = 10.0f;
    for (FMASkillBarRenderData& RenderData : MutableThis->SkillBarRenderData)
    {
        // 找到机器人索引
        int32 RobotIndex = RobotIdOrder.IndexOfByKey(RenderData.RobotId);
        if (RobotIndex != INDEX_NONE)
        {
            // 重新计算位置和大小（增加间距）
            RenderData.Position.X = MutableThis->TimeStepToScreen(RenderData.TimeStep) + SkillBarPadding / 2.0f;
            RenderData.Position.Y = MutableThis->RobotIndexToScreen(RobotIndex) + SkillBarPadding / 2.0f;
            RenderData.Size.X = MutableThis->TimeStepWidth - SkillBarPadding;
            RenderData.Size.Y = MutableThis->RobotRowHeight - SkillBarPadding;
        }
    }
}

void UMAGanttCanvas::DrawTimelineHeader(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 绘制时间步标签 - 居中显示
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    
    for (int32 TimeStep : TimeStepOrder)
    {
        float X = TimeStepToScreen(TimeStep);
        
        // 只绘制可见的标签
        if (X < 0 || X > AllottedGeometry.GetLocalSize().X)
        {
            continue;
        }
        
        FString Label = FString::Printf(TEXT("T%d"), TimeStep);
        
        // 计算文本居中位置
        // 假设每个字符约 8 像素宽，计算文本宽度
        float EstimatedTextWidth = Label.Len() * 8.0f;
        float CenteredX = X + (TimeStepWidth - EstimatedTextWidth) / 2.0f;
        float CenteredY = (HeaderHeight - 14.0f) / 2.0f;  // 14 is approximate font height
        
        FVector2D TextPosition(CenteredX, CenteredY);
        FVector2D TextSize(TimeStepWidth, HeaderHeight);
        
        // 使用正确的位置绘制文本 - ToPaintGeometry(LocalSize, LayoutTransform)
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            Label,
            FontInfo,
            ESlateDrawEffect::None,
            TextColor
        );
    }
}

void UMAGanttCanvas::DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 绘制机器人 ID 标签 - 与时间步标签字号统一为 12
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    
    for (int32 RobotIndex = 0; RobotIndex < RobotIdOrder.Num(); RobotIndex++)
    {
        float Y = RobotIndexToScreen(RobotIndex);
        
        // 只绘制可见的标签
        if (Y < 0 || Y > AllottedGeometry.GetLocalSize().Y)
        {
            continue;
        }
        
        const FString& RobotId = RobotIdOrder[RobotIndex];
        FVector2D TextPosition(5.0f, Y + RobotRowHeight / 2.0f - 8.0f);  // 调整垂直居中
        FVector2D TextSize(LabelWidth - 10.0f, RobotRowHeight - 4.0f);
        
        // 使用正确的位置绘制文本 - ToPaintGeometry(LocalSize, LayoutTransform)
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            RobotId,
            FontInfo,
            ESlateDrawEffect::None,
            TextColor
        );
    }
}

void UMAGanttCanvas::DrawTimeStepGridLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 绘制时间步之间的细竖线分隔
    // 使用成员变量 GridLineColor（从 Theme 获取或使用 fallback 默认值）
    // 调整透明度使其更淡
    FLinearColor GridColor = GridLineColor;
    GridColor.A = 0.5f;
    
    // 获取可见区域
    FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
    
    // 计算数据区域的起始和结束 Y 坐标
    float DataStartY = HeaderHeight;
    float DataEndY = WidgetSize.Y;
    
    // 绘制每个时间步之间的竖线
    for (int32 i = 0; i < TimeStepOrder.Num(); i++)
    {
        int32 TimeStep = TimeStepOrder[i];
        float X = TimeStepToScreen(TimeStep);
        
        // 只绘制可见的竖线
        if (X < LabelWidth || X > WidgetSize.X)
        {
            continue;
        }
        
        // 绘制竖线（在时间步的左边缘）
        TArray<FVector2D> LinePoints;
        LinePoints.Add(FVector2D(X, DataStartY));
        LinePoints.Add(FVector2D(X, DataEndY));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            GridColor,
            true,
            1.0f  // 细线宽度
        );
    }
    
    // 绘制最后一个时间步的右边缘竖线
    if (TimeStepOrder.Num() > 0)
    {
        int32 LastTimeStep = TimeStepOrder.Last();
        float LastX = TimeStepToScreen(LastTimeStep) + TimeStepWidth;
        
        if (LastX >= LabelWidth && LastX <= WidgetSize.X)
        {
            TArray<FVector2D> LinePoints;
            LinePoints.Add(FVector2D(LastX, DataStartY));
            LinePoints.Add(FVector2D(LastX, DataEndY));
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId,
                AllottedGeometry.ToPaintGeometry(),
                LinePoints,
                ESlateDrawEffect::None,
                GridColor,
                true,
                1.0f
            );
        }
    }
}

void UMAGanttCanvas::DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 圆角半径 - 使用较大的圆角使技能块更加圆润
    const float CornerRadius = 14.0f;
    
    // 技能条间距 - 用于计算技能条大小时的边距
    const float SkillBarPadding = 10.0f;
    
    // 绘制所有技能条
    for (const FMASkillBarRenderData& RenderData : SkillBarRenderData)
    {
        // 只绘制可见的技能条
        if (RenderData.Position.X + RenderData.Size.X < 0 || RenderData.Position.X > AllottedGeometry.GetLocalSize().X)
        {
            continue;
        }
        if (RenderData.Position.Y + RenderData.Size.Y < 0 || RenderData.Position.Y > AllottedGeometry.GetLocalSize().Y)
        {
            continue;
        }
        
        // 绘制技能条圆角矩形
        FSlateRoundedBoxBrush RoundedBrush(RenderData.Color, CornerRadius);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(RenderData.Size), FSlateLayoutTransform(FVector2f(RenderData.Position))),
            &RoundedBrush,
            ESlateDrawEffect::None,
            RenderData.Color
        );
        
        // 在技能条上绘制技能名称 - 自适应字号并居中显示
        // 根据技能条大小计算合适的字号
        float BarHeight = RenderData.Size.Y;
        int32 FontSize = FMath::Clamp(static_cast<int32>(BarHeight * 0.4f), 10, 16);
        FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
        
        // 计算文本居中位置
        float EstimatedTextWidth = RenderData.SkillName.Len() * (FontSize * 0.6f);
        float EstimatedTextHeight = FontSize * 1.2f;
        
        float CenteredX = RenderData.Position.X + (RenderData.Size.X - EstimatedTextWidth) / 2.0f;
        float CenteredY = RenderData.Position.Y + (RenderData.Size.Y - EstimatedTextHeight) / 2.0f;
        
        // 确保文本不会超出技能条边界
        CenteredX = FMath::Max(CenteredX, RenderData.Position.X + 2.0f);
        
        FVector2D TextPosition(CenteredX, CenteredY);
        FVector2D TextSize(RenderData.Size.X - 4.0f, RenderData.Size.Y - 4.0f);
        
        // 使用深色文字以便在浅色背景上可见
        // 从 Theme 获取 InputTextColor，或使用 fallback 默认值
        FLinearColor TextOnBarColor = Theme ? Theme->InputTextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
        
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            RenderData.SkillName,
            FontInfo,
            ESlateDrawEffect::None,
            TextOnBarColor
        );
        
        // 如果选中，绘制边框
        if (RenderData.bIsSelected)
        {
            TArray<FVector2D> BorderPoints;
            BorderPoints.Add(RenderData.Position);
            BorderPoints.Add(RenderData.Position + FVector2D(RenderData.Size.X, 0));
            BorderPoints.Add(RenderData.Position + RenderData.Size);
            BorderPoints.Add(RenderData.Position + FVector2D(0, RenderData.Size.Y));
            BorderPoints.Add(RenderData.Position);
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(),
                BorderPoints,
                ESlateDrawEffect::None,
                SelectionColor,
                true,
                2.0f
            );
        }
    }
}

//=============================================================================
// 拖拽视觉渲染 (Requirements 1.2, 1.3, 1.5, 2.1, 2.2, 2.4)
//=============================================================================

void UMAGanttCanvas::DrawDragPreview(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 仅在拖拽状态下绘制预览 (Requirements 1.2)
    if (DragState != EGanttDragState::Dragging)
    {
        return;
    }

    // 检查预览数据是否有效
    if (DragPreview.Size.X <= 0 || DragPreview.Size.Y <= 0)
    {
        return;
    }

    // 检查预览是否在可见区域内
    FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
    if (DragPreview.Position.X + DragPreview.Size.X < 0 || DragPreview.Position.X > WidgetSize.X ||
        DragPreview.Position.Y + DragPreview.Size.Y < 0 || DragPreview.Position.Y > WidgetSize.Y)
    {
        return;
    }

    // 圆角半径 - 与技能条保持一致
    const float CornerRadius = 14.0f;

    // 绘制半透明技能块预览圆角矩形 (Requirements 1.2)
    FSlateRoundedBoxBrush RoundedBrush(DragPreview.Color, CornerRadius);
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2f(DragPreview.Size), FSlateLayoutTransform(FVector2f(DragPreview.Position))),
        &RoundedBrush,
        ESlateDrawEffect::None,
        DragPreview.Color
    );

    // 绘制预览边框（使用稍深的颜色）
    FLinearColor BorderColor = DragPreview.Color;
    BorderColor.A = FMath::Min(1.0f, DragPreview.Color.A + 0.3f);
    
    TArray<FVector2D> BorderPoints;
    BorderPoints.Add(DragPreview.Position);
    BorderPoints.Add(DragPreview.Position + FVector2D(DragPreview.Size.X, 0));
    BorderPoints.Add(DragPreview.Position + DragPreview.Size);
    BorderPoints.Add(DragPreview.Position + FVector2D(0, DragPreview.Size.Y));
    BorderPoints.Add(DragPreview.Position);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        BorderColor,
        true,
        2.0f
    );

    // 在预览上绘制技能名称 (Requirements 1.5)
    if (!DragPreview.SkillName.IsEmpty())
    {
        FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
        
        FVector2D TextPosition(DragPreview.Position.X + 4.0f, DragPreview.Position.Y + DragPreview.Size.Y / 2.0f - 7.0f);
        FVector2D TextSize(DragPreview.Size.X - 8.0f, DragPreview.Size.Y - 4.0f);
        
        // 使用深色文字以便在浅色背景上可见
        // 从 Theme 获取 InputTextColor，或使用 fallback 默认值
        FLinearColor TextOnBarColor = Theme ? Theme->InputTextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
        
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            DragPreview.SkillName,
            FontInfo,
            ESlateDrawEffect::None,
            TextOnBarColor
        );
    }

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("DrawDragPreview: Position=(%.1f, %.1f), Size=(%.1f, %.1f), Skill=%s"),
           DragPreview.Position.X, DragPreview.Position.Y, DragPreview.Size.X, DragPreview.Size.Y, *DragPreview.SkillName);
}

void UMAGanttCanvas::DrawDragSourcePlaceholder(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 仅在拖拽状态下绘制占位符 (Requirements 1.3)
    if (DragState != EGanttDragState::Dragging)
    {
        return;
    }

    // 检查拖拽源是否有效
    if (!DragSource.IsValid())
    {
        return;
    }

    // 获取原始位置和大小（使用较大的间距）
    const float SkillBarPadding = 10.0f;
    FVector2D PlaceholderPosition = DragSource.OriginalPosition;
    FVector2D PlaceholderSize(TimeStepWidth - SkillBarPadding, RobotRowHeight - SkillBarPadding);

    // 检查占位符是否在可见区域内
    FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
    if (PlaceholderPosition.X + PlaceholderSize.X < 0 || PlaceholderPosition.X > WidgetSize.X ||
        PlaceholderPosition.Y + PlaceholderSize.Y < 0 || PlaceholderPosition.Y > WidgetSize.Y)
    {
        return;
    }

    // 绘制虚线边框 (Requirements 1.3)
    // 由于 Slate 不直接支持虚线，我们使用多个短线段模拟虚线效果
    const float DashLength = 8.0f;
    const float GapLength = 4.0f;
    const float TotalLength = DashLength + GapLength;

    // 顶边
    float CurrentX = PlaceholderPosition.X;
    float EndX = PlaceholderPosition.X + PlaceholderSize.X;
    while (CurrentX < EndX)
    {
        float SegmentEnd = FMath::Min(CurrentX + DashLength, EndX);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(CurrentX, PlaceholderPosition.Y));
        DashPoints.Add(FVector2D(SegmentEnd, PlaceholderPosition.Y));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            PlaceholderColor,
            true,
            2.0f
        );
        CurrentX += TotalLength;
    }

    // 底边
    CurrentX = PlaceholderPosition.X;
    float BottomY = PlaceholderPosition.Y + PlaceholderSize.Y;
    while (CurrentX < EndX)
    {
        float SegmentEnd = FMath::Min(CurrentX + DashLength, EndX);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(CurrentX, BottomY));
        DashPoints.Add(FVector2D(SegmentEnd, BottomY));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            PlaceholderColor,
            true,
            2.0f
        );
        CurrentX += TotalLength;
    }

    // 左边
    float CurrentY = PlaceholderPosition.Y;
    float EndY = PlaceholderPosition.Y + PlaceholderSize.Y;
    while (CurrentY < EndY)
    {
        float SegmentEnd = FMath::Min(CurrentY + DashLength, EndY);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(PlaceholderPosition.X, CurrentY));
        DashPoints.Add(FVector2D(PlaceholderPosition.X, SegmentEnd));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            PlaceholderColor,
            true,
            2.0f
        );
        CurrentY += TotalLength;
    }

    // 右边
    CurrentY = PlaceholderPosition.Y;
    float RightX = PlaceholderPosition.X + PlaceholderSize.X;
    while (CurrentY < EndY)
    {
        float SegmentEnd = FMath::Min(CurrentY + DashLength, EndY);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(RightX, CurrentY));
        DashPoints.Add(FVector2D(RightX, SegmentEnd));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            PlaceholderColor,
            true,
            2.0f
        );
        CurrentY += TotalLength;
    }

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("DrawDragSourcePlaceholder: Position=(%.1f, %.1f), Size=(%.1f, %.1f)"),
           PlaceholderPosition.X, PlaceholderPosition.Y, PlaceholderSize.X, PlaceholderSize.Y);
}

void UMAGanttCanvas::DrawDropIndicator(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 仅在拖拽状态下绘制放置指示器 (Requirements 2.1)
    if (DragState != EGanttDragState::Dragging)
    {
        return;
    }

    // 检查是否有有效的放置目标
    if (!CurrentDropTarget.HasTarget())
    {
        return;
    }

    // 获取目标槽位的机器人索引
    int32 RobotIndex = RobotIdOrder.IndexOfByKey(CurrentDropTarget.RobotId);
    if (RobotIndex == INDEX_NONE)
    {
        return;
    }

    // 计算目标槽位的位置和大小（使用较大的间距）
    const float SkillBarPadding = 10.0f;
    FVector2D IndicatorPosition(TimeStepToScreen(CurrentDropTarget.TimeStep) + SkillBarPadding / 2.0f, 
                                 RobotIndexToScreen(RobotIndex) + SkillBarPadding / 2.0f);
    FVector2D IndicatorSize(TimeStepWidth - SkillBarPadding, RobotRowHeight - SkillBarPadding);

    // 检查指示器是否在可见区域内
    FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
    if (IndicatorPosition.X + IndicatorSize.X < 0 || IndicatorPosition.X > WidgetSize.X ||
        IndicatorPosition.Y + IndicatorSize.Y < 0 || IndicatorPosition.Y > WidgetSize.Y)
    {
        return;
    }

    // 根据有效性选择颜色 (Requirements 2.4)
    FLinearColor IndicatorColor = CurrentDropTarget.bIsValid ? ValidDropColor : InvalidDropColor;

    // 绘制半透明背景填充
    FLinearColor FillColor = IndicatorColor;
    FillColor.A = 0.2f;
    
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2f(IndicatorSize), FSlateLayoutTransform(FVector2f(IndicatorPosition))),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FillColor
    );

    // 绘制边框 (Requirements 2.1, 2.2)
    TArray<FVector2D> BorderPoints;
    BorderPoints.Add(IndicatorPosition);
    BorderPoints.Add(IndicatorPosition + FVector2D(IndicatorSize.X, 0));
    BorderPoints.Add(IndicatorPosition + IndicatorSize);
    BorderPoints.Add(IndicatorPosition + FVector2D(0, IndicatorSize.Y));
    BorderPoints.Add(IndicatorPosition);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        IndicatorColor,
        true,
        3.0f  // 使用较粗的边框以便更明显
    );

    // 如果是有效目标且已吸附，绘制额外的吸附指示
    if (CurrentDropTarget.bIsValid && CurrentDropTarget.bIsSnapped)
    {
        // 绘制内部高亮边框
        FVector2D InnerOffset(4.0f, 4.0f);
        FVector2D InnerPosition = IndicatorPosition + InnerOffset;
        FVector2D InnerSize = IndicatorSize - InnerOffset * 2.0f;

        TArray<FVector2D> InnerBorderPoints;
        InnerBorderPoints.Add(InnerPosition);
        InnerBorderPoints.Add(InnerPosition + FVector2D(InnerSize.X, 0));
        InnerBorderPoints.Add(InnerPosition + InnerSize);
        InnerBorderPoints.Add(InnerPosition + FVector2D(0, InnerSize.Y));
        InnerBorderPoints.Add(InnerPosition);

        FLinearColor SnapHighlightColor = ValidDropColor;
        SnapHighlightColor.A = 0.8f;

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            InnerBorderPoints,
            ESlateDrawEffect::None,
            SnapHighlightColor,
            true,
            1.5f
        );
    }

    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("DrawDropIndicator: TimeStep=%d, RobotId=%s, Valid=%s, Snapped=%s"),
           CurrentDropTarget.TimeStep, *CurrentDropTarget.RobotId,
           CurrentDropTarget.bIsValid ? TEXT("true") : TEXT("false"),
           CurrentDropTarget.bIsSnapped ? TEXT("true") : TEXT("false"));
}

//=============================================================================
// 鼠标交互
//=============================================================================

FReply UMAGanttCanvas::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        
        // 检查是否点击在技能条上
        for (const FMASkillBarRenderData& RenderData : SkillBarRenderData)
        {
            FVector2D BarMin = RenderData.Position;
            FVector2D BarMax = RenderData.Position + RenderData.Size;
            
            if (LocalPosition.X >= BarMin.X && LocalPosition.X <= BarMax.X &&
                LocalPosition.Y >= BarMin.Y && LocalPosition.Y <= BarMax.Y)
            {
                // 检查拖拽是否启用 (Requirements 8.1, 8.2)
                if (bDragEnabled)
                {
                    // 设置潜在拖拽状态 (Requirements 1.1)
                    DragState = EGanttDragState::Potential;
                    
                    // 记录鼠标按下位置
                    MouseDownPosition = LocalPosition;
                    
                    // 记录拖拽源信息
                    DragSource.TimeStep = RenderData.TimeStep;
                    DragSource.RobotId = RenderData.RobotId;
                    DragSource.SkillName = RenderData.SkillName;
                    DragSource.ParamsJson = RenderData.ParamsJson;
                    DragSource.Status = RenderData.Status;
                    DragSource.OriginalPosition = RenderData.Position;
                    
                    UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Potential drag started: TimeStep=%d, RobotId=%s, Skill=%s"),
                           DragSource.TimeStep, *DragSource.RobotId, *DragSource.SkillName);
                    
                    // 捕获鼠标以接收后续事件
                    return FReply::Handled().CaptureMouse(TakeWidget());
                }
                else
                {
                    // 拖拽被禁用（可能正在执行中）(Requirements 8.2)
                    UE_LOG(LogMAGanttCanvas, Log, TEXT("[警告] 执行期间无法修改技能分配"));
                    
                    // 广播拖拽被阻止事件，通知 Viewer 显示警告日志
                    OnDragBlocked.Broadcast();
                }
                
                // 选中该技能条 (Requirements 1.4 - 点击但未拖拽时执行选择)
                SelectSkillBar(RenderData.TimeStep, RenderData.RobotId);
                return FReply::Handled();
            }
        }
        
        // 点击空白区域，取消选中
        ClearSelection();
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply UMAGanttCanvas::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    
    //=========================================================================
    // 拖拽状态处理 (Requirements 1.1, 2.5, 3.1)
    //=========================================================================
    
    // 处理潜在拖拽状态 - 检查是否超过拖拽阈值
    if (DragState == EGanttDragState::Potential)
    {
        float MoveDistance = FVector2D::Distance(LocalPosition, MouseDownPosition);
        
        if (MoveDistance >= DragThreshold)
        {
            // 超过阈值，从 Potential 转换到 Dragging 状态 (Requirements 1.1)
            DragState = EGanttDragState::Dragging;
            
            // 初始化拖拽预览（使用较大的间距）
            const float SkillBarPadding = 10.0f;
            DragPreview.Position = LocalPosition;
            DragPreview.Size = FVector2D(TimeStepWidth - SkillBarPadding, RobotRowHeight - SkillBarPadding);
            DragPreview.Color = GetStatusColor(DragSource.Status);
            DragPreview.Color.A = DragPreviewAlpha;
            DragPreview.SkillName = DragSource.SkillName;
            
            // 记录拖拽开始日志 (Requirements 7.1)
            UE_LOG(LogMAGanttCanvas, Log, TEXT("[拖拽] 开始移动技能: %s (T%d, %s)"),
                   *DragSource.SkillName, DragSource.TimeStep, *DragSource.RobotId);
            
            // 广播拖拽开始事件（用于状态日志记录）(Requirements 7.1)
            OnDragStarted.Broadcast(DragSource.SkillName, DragSource.TimeStep, DragSource.RobotId);
            
            return FReply::Handled();
        }
        
        // 未超过阈值，继续等待
        return FReply::Handled();
    }
    
    // 处理拖拽中状态 - 更新预览位置和吸附目标
    if (DragState == EGanttDragState::Dragging)
    {
        // 计算预览位置（以鼠标为中心）
        FVector2D PreviewCenter = LocalPosition;
        FVector2D PreviewHalfSize = DragPreview.Size * 0.5f;
        
        // 计算吸附目标 (Requirements 3.1)
        CurrentDropTarget = CalculateSnapTarget(LocalPosition);
        
        // 根据吸附状态更新预览位置 (Requirements 2.5)
        if (CurrentDropTarget.bIsSnapped)
        {
            // 吸附到槽位中心
            DragPreview.Position = CurrentDropTarget.SnapPosition - PreviewHalfSize;
        }
        else
        {
            // 跟随鼠标
            DragPreview.Position = PreviewCenter - PreviewHalfSize;
        }
        
        UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Drag update: Position=(%.1f, %.1f), Snapped=%s, Valid=%s"),
               DragPreview.Position.X, DragPreview.Position.Y,
               CurrentDropTarget.bIsSnapped ? TEXT("true") : TEXT("false"),
               CurrentDropTarget.bIsValid ? TEXT("true") : TEXT("false"));
        
        return FReply::Handled();
    }
    
    //=========================================================================
    // 非拖拽状态 - 更新悬停状态
    //=========================================================================
    
    HoveredTimeStep = -1;
    HoveredRobotId.Empty();
    
    for (const FMASkillBarRenderData& RenderData : SkillBarRenderData)
    {
        FVector2D BarMin = RenderData.Position;
        FVector2D BarMax = RenderData.Position + RenderData.Size;
        
        if (LocalPosition.X >= BarMin.X && LocalPosition.X <= BarMax.X &&
            LocalPosition.Y >= BarMin.Y && LocalPosition.Y <= BarMax.Y)
        {
            HoveredTimeStep = RenderData.TimeStep;
            HoveredRobotId = RenderData.RobotId;
            break;
        }
    }
    
    return FReply::Unhandled();
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
        FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        
        //=========================================================================
        // 处理拖拽释放 (Requirements 4.1, 4.3, 4.4)
        //=========================================================================
        
        if (DragState == EGanttDragState::Potential)
        {
            // 未超过拖拽阈值，执行选择操作 (Requirements 1.4)
            SelectSkillBar(DragSource.TimeStep, DragSource.RobotId);
            
            // 重置拖拽状态
            DragState = EGanttDragState::Idle;
            DragSource.Reset();
            MouseDownPosition = FVector2D::ZeroVector;
            
            UE_LOG(LogMAGanttCanvas, Verbose, TEXT("Click detected (no drag), selected skill bar"));
            
            return FReply::Handled().ReleaseMouseCapture();
        }
        
        if (DragState == EGanttDragState::Dragging)
        {
            // 计算最终放置目标
            FGanttDropTarget FinalTarget = CalculateSnapTarget(LocalPosition);
            
            bool bDropSuccess = false;
            
            // 验证放置目标 (Requirements 4.1)
            if (FinalTarget.bIsValid && FinalTarget.HasTarget())
            {
                // 有效目标 - 尝试更新数据
                if (AllocationModel)
                {
                    // 调用模型的 MoveSkill 方法
                    bDropSuccess = AllocationModel->MoveSkill(
                        DragSource.TimeStep, DragSource.RobotId,
                        FinalTarget.TimeStep, FinalTarget.RobotId
                    );
                    
                    if (bDropSuccess)
                    {
                        // 记录成功日志 (Requirements 7.2)
                        UE_LOG(LogMAGanttCanvas, Log, TEXT("[成功] 技能已移动: %s 从 T%d 到 T%d"),
                               *DragSource.SkillName, DragSource.TimeStep, FinalTarget.TimeStep);
                        
                        // 广播拖拽完成事件（用于数据持久化）
                        OnDragCompleted.Broadcast(
                            DragSource.TimeStep, DragSource.RobotId,
                            FinalTarget.TimeStep, FinalTarget.RobotId
                        );
                    }
                    else
                    {
                        // 记录失败日志 (Requirements 7.4)
                        UE_LOG(LogMAGanttCanvas, Log, TEXT("[失败] 无法放置: 目标槽位已被占用"));
                        
                        // 广播拖拽失败事件（用于状态日志记录）(Requirements 7.4)
                        OnDragFailed.Broadcast();
                    }
                }
                else
                {
                    UE_LOG(LogMAGanttCanvas, Warning, TEXT("Drop failed: No allocation model bound"));
                }
            }
            else
            {
                // 无效目标 (Requirements 4.3, 4.4)
                UE_LOG(LogMAGanttCanvas, Log, TEXT("[失败] 无法放置: 目标槽位已被占用或无效"));
                
                // 广播拖拽失败事件（用于状态日志记录）(Requirements 7.4)
                OnDragFailed.Broadcast();
            }
            
            // 重置拖拽状态
            DragState = EGanttDragState::Idle;
            DragSource.Reset();
            CurrentDropTarget.Reset();
            DragPreview.Reset();
            MouseDownPosition = FVector2D::ZeroVector;
            
            return FReply::Handled().ReleaseMouseCapture();
        }
    }
    
    return FReply::Unhandled();
}

FReply UMAGanttCanvas::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    //=========================================================================
    // 处理 Escape 键取消拖拽 (Requirements 4.5)
    //=========================================================================
    
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (DragState != EGanttDragState::Idle)
        {
            // 取消拖拽操作
            CancelDrag();
            
            // 释放鼠标捕获
            return FReply::Handled().ReleaseMouseCapture();
        }
    }
    
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
