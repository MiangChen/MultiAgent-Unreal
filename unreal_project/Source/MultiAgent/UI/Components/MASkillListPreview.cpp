// MASkillListPreview.cpp
// 技能列表预览组件实现 - 使用 NativePaint 绘制简化的甘特图

#include "MASkillListPreview.h"
#include "../Core/MAUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

//=============================================================================
// 日志类别
//=============================================================================
DEFINE_LOG_CATEGORY_STATIC(LogMASkillListPreview, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMASkillListPreview::UMASkillListPreview(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMASkillListPreview::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMASkillListPreview::NativeConstruct()
{
    Super::NativeConstruct();
}

TSharedRef<SWidget> UMASkillListPreview::RebuildWidget()
{
    BuildUI();
    return Super::RebuildWidget();
}

int32 UMASkillListPreview::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                                        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                                        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // 缓存几何信息用于鼠标检测
    CachedGeometry = AllottedGeometry;
    
    // 先调用父类绘制
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    
    // 如果有数据，绘制甘特图
    if (bHasData && SkillBarRenderData.Num() > 0)
    {
        DrawGantt(AllottedGeometry, OutDrawElements, LayerId);
        
        // 绘制工具提示 (在最上层)
        if (HoveredBarIndex >= 0 && HoveredBarIndex < SkillBarRenderData.Num())
        {
            DrawTooltip(AllottedGeometry, OutDrawElements, LayerId + 10);
        }
    }
    
    return LayerId;
}

//=============================================================================
// UI 构建
//=============================================================================

void UMASkillListPreview::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMASkillListPreview, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }
    
    // 创建根 SizeBox
    RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
    if (!RootSizeBox)
    {
        UE_LOG(LogMASkillListPreview, Error, TEXT("BuildUI: Failed to create RootSizeBox"));
        return;
    }
    RootSizeBox->SetMinDesiredHeight(PreviewHeight);
    
    // 设置为根 Widget
    WidgetTree->RootWidget = RootSizeBox;
    
    // 创建内容边框
    ContentBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ContentBorder"));
    if (!ContentBorder)
    {
        return;
    }
    ContentBorder->SetBrushColor(BackgroundColor);
    ContentBorder->SetPadding(FMargin(8.0f));
    
    // 创建垂直布局容器
    UVerticalBox* ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentVBox"));
    if (!ContentVBox)
    {
        return;
    }
    
    // 创建统计信息文本
    StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(TEXT("No data")));
        StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
        
        FSlateFontInfo StatsFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
        StatsText->SetFont(StatsFont);
        
        UVerticalBoxSlot* StatsSlot = ContentVBox->AddChildToVerticalBox(StatsText);
        StatsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }
    
    // 创建空状态提示 (当没有数据时显示)
    EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
    if (EmptyText)
    {
        EmptyText->SetText(FText::FromString(TEXT("Waiting for skill list...")));
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
        EmptyText->SetJustification(ETextJustify::Center);
        
        FSlateFontInfo EmptyFont = FCoreStyle::GetDefaultFontStyle("Italic", 10);
        EmptyText->SetFont(EmptyFont);
        
        UVerticalBoxSlot* EmptySlot = ContentVBox->AddChildToVerticalBox(EmptyText);
        EmptySlot->SetHorizontalAlignment(HAlign_Center);
        EmptySlot->SetVerticalAlignment(VAlign_Center);
        EmptySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    
    // 组装层级
    ContentBorder->SetContent(ContentVBox);
    RootSizeBox->SetContent(ContentBorder);
}

//=============================================================================
// 预览更新
//=============================================================================

void UMASkillListPreview::UpdatePreview(const FMASkillAllocationData& Data)
{
    CurrentData = Data;
    bHasData = Data.Data.Num() > 0;
    HoveredBarIndex = -1;  // 重置悬浮状态
    
    // 获取机器人和时间步列表
    RobotIds = CurrentData.GetAllRobotIds();
    TimeSteps = CurrentData.GetAllTimeSteps();
    
    // 更新统计信息
    if (StatsText)
    {
        if (bHasData)
        {
            FString StatsStr = FString::Printf(TEXT("%d robots, %d time steps"), 
                RobotIds.Num(), TimeSteps.Num());
            StatsText->SetText(FText::FromString(StatsStr));
            StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
        }
        else
        {
            StatsText->SetText(FText::FromString(TEXT("No data")));
            StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
        }
    }
    
    // 更新空状态提示可见性
    if (EmptyText)
    {
        EmptyText->SetVisibility(bHasData ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    
    // 预计算技能条渲染数据
    SkillBarRenderData.Empty();
    
    if (bHasData)
    {
        for (int32 RobotIndex = 0; RobotIndex < RobotIds.Num(); ++RobotIndex)
        {
            const FString& RobotId = RobotIds[RobotIndex];
            
            for (int32 TimeStepIndex = 0; TimeStepIndex < TimeSteps.Num(); ++TimeStepIndex)
            {
                int32 TimeStep = TimeSteps[TimeStepIndex];
                
                // 查找该时间步该机器人的技能分配
                if (const FMATimeStepData* TimeStepData = CurrentData.Data.Find(TimeStep))
                {
                    if (const FMASkillAssignment* Skill = TimeStepData->RobotSkills.Find(RobotId))
                    {
                        FMAPreviewSkillBarData BarData;
                        BarData.RobotId = RobotId;
                        BarData.SkillName = Skill->SkillName;
                        BarData.TimeStep = TimeStep;
                        BarData.RobotIndex = RobotIndex;
                        BarData.Status = Skill->Status;
                        // 位置将在 CalculateLayout 中计算
                        
                        SkillBarRenderData.Add(BarData);
                    }
                }
            }
        }
    }
    
    UE_LOG(LogMASkillListPreview, Log, TEXT("UpdatePreview: %d robots, %d time steps, %d skill bars"), 
        RobotIds.Num(), TimeSteps.Num(), SkillBarRenderData.Num());
}

void UMASkillListPreview::ClearPreview()
{
    CurrentData = FMASkillAllocationData();
    bHasData = false;
    SkillBarRenderData.Empty();
    RobotIds.Empty();
    TimeSteps.Empty();
    HoveredBarIndex = -1;
    
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(TEXT("No data")));
        StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    }
    
    if (EmptyText)
    {
        EmptyText->SetVisibility(ESlateVisibility::Visible);
    }
}

void UMASkillListPreview::UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    if (!bHasData)
    {
        return;
    }
    
    // 更新 CurrentData 中的状态
    if (FMATimeStepData* TimeStepData = CurrentData.Data.Find(TimeStep))
    {
        if (FMASkillAssignment* Skill = TimeStepData->RobotSkills.Find(RobotId))
        {
            Skill->Status = NewStatus;
        }
    }
    
    // 更新渲染数据中的状态
    for (FMAPreviewSkillBarData& Bar : SkillBarRenderData)
    {
        if (Bar.TimeStep == TimeStep && Bar.RobotId == RobotId)
        {
            Bar.Status = NewStatus;
            UE_LOG(LogMASkillListPreview, Verbose, TEXT("UpdateSkillStatus: TimeStep=%d, RobotId=%s, NewStatus=%d"),
                TimeStep, *RobotId, static_cast<int32>(NewStatus));
            break;
        }
    }
    
    // 触发重绘
    Invalidate(EInvalidateWidgetReason::Paint);
}

//=============================================================================
// 布局计算
//=============================================================================

void UMASkillListPreview::CalculateLayout(const FGeometry& AllottedGeometry)
{
    if (!bHasData || SkillBarRenderData.Num() == 0)
    {
        return;
    }
    
    FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    float AvailableWidth = CanvasSize.X - LeftMargin - RightMargin;
    float AvailableHeight = CanvasSize.Y - TopMargin - BottomMargin;
    
    // 计算每个时间步的宽度
    float TimeStepWidth = (TimeSteps.Num() > 0) ? AvailableWidth / TimeSteps.Num() : AvailableWidth;
    
    // 计算每个机器人的行高
    float ActualRowHeight = (RobotIds.Num() > 0) ? FMath::Min(RowHeight, AvailableHeight / RobotIds.Num()) : RowHeight;
    
    // 更新技能条位置
    for (FMAPreviewSkillBarData& Bar : SkillBarRenderData)
    {
        // 查找时间步索引
        int32 TimeStepIndex = TimeSteps.IndexOfByKey(Bar.TimeStep);
        if (TimeStepIndex == INDEX_NONE)
        {
            continue;
        }
        
        float X = LeftMargin + TimeStepIndex * TimeStepWidth + BarPadding;
        float Y = TopMargin + Bar.RobotIndex * ActualRowHeight + BarPadding;
        float Width = TimeStepWidth - BarPadding * 2;
        float Height = ActualRowHeight - BarPadding * 2;
        
        Bar.Position = FVector2D(X, Y);
        Bar.Size = FVector2D(Width, Height);
    }
}

//=============================================================================
// 绘制
//=============================================================================

void UMASkillListPreview::DrawGantt(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 需要在绘制前计算布局
    const_cast<UMASkillListPreview*>(this)->CalculateLayout(AllottedGeometry);
    
    // 绘制网格线
    DrawGrid(AllottedGeometry, OutDrawElements, LayerId);
    
    // 绘制机器人标签
    DrawRobotLabels(AllottedGeometry, OutDrawElements, LayerId + 1);
    
    // 绘制技能条
    DrawSkillBars(AllottedGeometry, OutDrawElements, LayerId + 2);
}

void UMASkillListPreview::DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    float AvailableWidth = CanvasSize.X - LeftMargin - RightMargin;
    float AvailableHeight = CanvasSize.Y - TopMargin - BottomMargin;
    
    // 绘制水平网格线 (机器人行分隔)
    float ActualRowHeight = (RobotIds.Num() > 0) ? FMath::Min(RowHeight, AvailableHeight / RobotIds.Num()) : RowHeight;
    
    for (int32 i = 0; i <= RobotIds.Num(); ++i)
    {
        float Y = TopMargin + i * ActualRowHeight;
        TArray<FVector2D> LinePoints;
        LinePoints.Add(FVector2D(LeftMargin, Y));
        LinePoints.Add(FVector2D(CanvasSize.X - RightMargin, Y));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            GridLineColor,
            true,
            1.0f
        );
    }
    
    // 绘制垂直网格线 (时间步分隔)
    float TimeStepWidth = (TimeSteps.Num() > 0) ? AvailableWidth / TimeSteps.Num() : AvailableWidth;
    
    for (int32 i = 0; i <= TimeSteps.Num(); ++i)
    {
        float X = LeftMargin + i * TimeStepWidth;
        TArray<FVector2D> LinePoints;
        LinePoints.Add(FVector2D(X, TopMargin));
        LinePoints.Add(FVector2D(X, TopMargin + RobotIds.Num() * ActualRowHeight));
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            GridLineColor,
            true,
            1.0f
        );
    }
}

void UMASkillListPreview::DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    float AvailableHeight = CanvasSize.Y - TopMargin - BottomMargin;
    float ActualRowHeight = (RobotIds.Num() > 0) ? FMath::Min(RowHeight, AvailableHeight / RobotIds.Num()) : RowHeight;
    
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 8);
    
    for (int32 i = 0; i < RobotIds.Num(); ++i)
    {
        float Y = TopMargin + i * ActualRowHeight + ActualRowHeight / 2 - 6;
        
        // 截断机器人 ID
        FString DisplayId = RobotIds[i];
        if (DisplayId.Len() > 8)
        {
            DisplayId = DisplayId.Left(6) + TEXT("..");
        }
        
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2D(LeftMargin - 5, ActualRowHeight), FSlateLayoutTransform(FVector2D(5, Y))),
            FText::FromString(DisplayId),
            FontInfo,
            ESlateDrawEffect::None,
            TextColor
        );
    }
}

void UMASkillListPreview::DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    for (int32 i = 0; i < SkillBarRenderData.Num(); ++i)
    {
        const FMAPreviewSkillBarData& Bar = SkillBarRenderData[i];
        
        // 绘制技能条背景
        FLinearColor BarColor = GetStatusColor(Bar.Status);
        
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(Bar.Size, FSlateLayoutTransform(Bar.Position)),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            BarColor
        );
        
        // 如果是悬浮的技能条，绘制高亮
        if (i == HoveredBarIndex)
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(Bar.Size, FSlateLayoutTransform(Bar.Position)),
                FCoreStyle::Get().GetBrush("WhiteBrush"),
                ESlateDrawEffect::None,
                HoverHighlightColor
            );
        }
        
        // 如果空间足够，绘制技能名称
        if (Bar.Size.X > 30 && Bar.Size.Y > 10)
        {
            FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 7);
            
            // 截断技能名称
            FString DisplayName = Bar.SkillName;
            if (DisplayName.Len() > 8)
            {
                DisplayName = DisplayName.Left(6) + TEXT("..");
            }
            
            FVector2D TextPos = Bar.Position + FVector2D(2, 2);
            
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(Bar.Size - FVector2D(4, 4), FSlateLayoutTransform(TextPos)),
                FText::FromString(DisplayName),
                FontInfo,
                ESlateDrawEffect::None,
                FLinearColor::White
            );
        }
    }
}

//=============================================================================
// 辅助方法
//=============================================================================

FLinearColor UMASkillListPreview::GetStatusColor(ESkillExecutionStatus Status) const
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

int32 UMASkillListPreview::FindSkillBarAtPosition(const FVector2D& LocalPosition) const
{
    for (int32 i = 0; i < SkillBarRenderData.Num(); ++i)
    {
        const FMAPreviewSkillBarData& Bar = SkillBarRenderData[i];
        
        // 检查点是否在技能条矩形内
        if (LocalPosition.X >= Bar.Position.X && 
            LocalPosition.X <= Bar.Position.X + Bar.Size.X &&
            LocalPosition.Y >= Bar.Position.Y && 
            LocalPosition.Y <= Bar.Position.Y + Bar.Size.Y)
        {
            return i;
        }
    }
    
    return -1;  // 未找到
}

//=============================================================================
// 鼠标事件处理
//=============================================================================

FReply UMASkillListPreview::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 获取本地鼠标位置
    FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    LastMousePosition = LocalPos;
    
    // 查找悬浮的技能条
    int32 NewHoveredIndex = FindSkillBarAtPosition(LocalPos);
    
    if (NewHoveredIndex != HoveredBarIndex)
    {
        HoveredBarIndex = NewHoveredIndex;
        // 触发重绘
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UMASkillListPreview::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    // 清除悬浮状态
    if (HoveredBarIndex >= 0)
    {
        HoveredBarIndex = -1;
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    
    Super::NativeOnMouseLeave(InMouseEvent);
}

void UMASkillListPreview::DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (HoveredBarIndex < 0 || HoveredBarIndex >= SkillBarRenderData.Num())
    {
        return;
    }
    
    const FMAPreviewSkillBarData& Bar = SkillBarRenderData[HoveredBarIndex];
    
    // 构建工具提示文本
    FString StatusStr;
    switch (Bar.Status)
    {
    case ESkillExecutionStatus::Pending:
        StatusStr = TEXT("Pending");
        break;
    case ESkillExecutionStatus::InProgress:
        StatusStr = TEXT("In Progress");
        break;
    case ESkillExecutionStatus::Completed:
        StatusStr = TEXT("Completed");
        break;
    case ESkillExecutionStatus::Failed:
        StatusStr = TEXT("Failed");
        break;
    }
    
    TArray<FString> TooltipLines;
    TooltipLines.Add(FString::Printf(TEXT("Robot: %s"), *Bar.RobotId));
    TooltipLines.Add(FString::Printf(TEXT("Skill: %s"), *Bar.SkillName));
    TooltipLines.Add(FString::Printf(TEXT("Time Step: %d"), Bar.TimeStep));
    TooltipLines.Add(FString::Printf(TEXT("Status: %s"), *StatusStr));
    
    // 计算工具提示大小
    float TooltipWidth = 150.0f;
    float LineHeight = 14.0f;
    float TooltipHeight = TooltipLines.Num() * LineHeight + 10.0f;
    float TooltipPadding = 5.0f;
    
    // 计算工具提示位置 (在技能条上方)
    FVector2D TooltipPos = Bar.Position + FVector2D(0, -TooltipHeight - 5);
    
    // 确保工具提示不超出边界
    FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    if (TooltipPos.X + TooltipWidth > CanvasSize.X - 5)
    {
        TooltipPos.X = CanvasSize.X - TooltipWidth - 5;
    }
    if (TooltipPos.X < 5)
    {
        TooltipPos.X = 5;
    }
    if (TooltipPos.Y < 5)
    {
        // 如果上方空间不够，显示在下方
        TooltipPos.Y = Bar.Position.Y + Bar.Size.Y + 5;
    }
    
    // 绘制工具提示背景
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2D(TooltipWidth, TooltipHeight), FSlateLayoutTransform(TooltipPos)),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        TooltipBgColor
    );
    
    // 绘制工具提示边框
    TArray<FVector2D> BorderPoints;
    BorderPoints.Add(TooltipPos);
    BorderPoints.Add(TooltipPos + FVector2D(TooltipWidth, 0));
    BorderPoints.Add(TooltipPos + FVector2D(TooltipWidth, TooltipHeight));
    BorderPoints.Add(TooltipPos + FVector2D(0, TooltipHeight));
    BorderPoints.Add(TooltipPos);
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        FLinearColor(0.3f, 0.3f, 0.35f, 1.0f),
        true,
        1.0f
    );
    
    // 绘制工具提示文本
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    
    for (int32 i = 0; i < TooltipLines.Num(); ++i)
    {
        FVector2D TextPos = TooltipPos + FVector2D(TooltipPadding, TooltipPadding + i * LineHeight);
        
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2D(TooltipWidth - TooltipPadding * 2, LineHeight), FSlateLayoutTransform(TextPos)),
            FText::FromString(TooltipLines[i]),
            FontInfo,
            ESlateDrawEffect::None,
            TooltipTextColor
        );
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMASkillListPreview::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        return;
    }
    
    // 应用背景颜色
    BackgroundColor = Theme->BackgroundColor;
    BackgroundColor.R -= 0.02f;
    BackgroundColor.G -= 0.02f;
    BackgroundColor.B -= 0.02f;
    
    if (ContentBorder)
    {
        ContentBorder->SetBrushColor(BackgroundColor);
    }
}
