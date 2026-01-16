// MAGanttCanvas.cpp
// 甘特图画布 Widget 实现

#include "MAGanttCanvas.h"
#include "MASkillAllocationModel.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"

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
                
                // 计算位置和大小
                RenderData.Position.X = TimeStepToScreen(TimeStep);
                RenderData.Position.Y = RobotIndexToScreen(RobotIndex);
                RenderData.Size.X = TimeStepWidth - 4.0f; // 留一点间隙
                RenderData.Size.Y = RobotRowHeight - 4.0f;
                
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

    // 创建背景边框
    CanvasBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CanvasBackground"));
    CanvasBackground->SetBrushColor(CanvasBackgroundColor);
    WidgetTree->RootWidget = CanvasBackground;

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
    // 先绘制子 Widget
    int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, 
                                          LayerId, InWidgetStyle, bParentEnabled);

    // 绘制时间轴标题
    DrawTimelineHeader(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制机器人标签
    DrawRobotLabels(AllottedGeometry, OutDrawElements, LayerId + 1);

    // 绘制技能条 (包含技能名称文字，需要额外层)
    DrawSkillBars(AllottedGeometry, OutDrawElements, LayerId + 2);

    return MaxLayerId + 5;
}

void UMAGanttCanvas::DrawTimelineHeader(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 绘制时间步标签
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    
    for (int32 TimeStep : TimeStepOrder)
    {
        float X = TimeStepToScreen(TimeStep);
        
        // 只绘制可见的标签
        if (X < 0 || X > AllottedGeometry.GetLocalSize().X)
        {
            continue;
        }
        
        FString Label = FString::Printf(TEXT("T%d"), TimeStep);
        FVector2D TextPosition(X + 5.0f, 5.0f);
        FVector2D TextSize(TimeStepWidth - 10.0f, HeaderHeight - 10.0f);
        
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
    // 绘制机器人 ID 标签
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    
    for (int32 RobotIndex = 0; RobotIndex < RobotIdOrder.Num(); RobotIndex++)
    {
        float Y = RobotIndexToScreen(RobotIndex);
        
        // 只绘制可见的标签
        if (Y < 0 || Y > AllottedGeometry.GetLocalSize().Y)
        {
            continue;
        }
        
        const FString& RobotId = RobotIdOrder[RobotIndex];
        FVector2D TextPosition(5.0f, Y + RobotRowHeight / 2.0f - 7.0f);
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

void UMAGanttCanvas::DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    
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
        
        // 绘制技能条矩形 - ToPaintGeometry(LocalSize, LayoutTransform)
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(RenderData.Size), FSlateLayoutTransform(FVector2f(RenderData.Position))),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            RenderData.Color
        );
        
        // 在技能条上绘制技能名称
        FVector2D TextPosition(RenderData.Position.X + 4.0f, RenderData.Position.Y + RenderData.Size.Y / 2.0f - 7.0f);
        FVector2D TextSize(RenderData.Size.X - 8.0f, RenderData.Size.Y - 4.0f);
        
        // 使用深色文字以便在浅色背景上可见
        FLinearColor TextOnBarColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
        
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
                // 选中该技能条
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
    
    // 更新悬停状态
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
