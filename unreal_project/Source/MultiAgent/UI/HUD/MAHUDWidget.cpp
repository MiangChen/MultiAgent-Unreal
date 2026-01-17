// MAHUDWidget.cpp
// HUD Widget 实现

#include "MAHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDWidget, Log, All);

//=============================================================================
// 颜色常量
//=============================================================================
namespace MAHUDColors
{
    static const FLinearColor Emergency = FLinearColor::Red;
    static const FLinearColor EditMode = FLinearColor(0.3f, 0.6f, 1.0f);    // 蓝色
    static const FLinearColor POI = FLinearColor(0.3f, 0.8f, 0.3f);         // 绿色
    static const FLinearColor Goal = FLinearColor(1.0f, 0.4f, 0.4f);        // 红色
    static const FLinearColor Zone = FLinearColor(0.3f, 0.8f, 1.0f);        // 青色
    static const FLinearColor Success = FLinearColor::Green;
    static const FLinearColor Warning = FLinearColor::Yellow;
    static const FLinearColor Error = FLinearColor::Red;
    static const FLinearColor Outline = FLinearColor::Black;
}

//=============================================================================
// 构造函数
//=============================================================================

UMAHUDWidget::UMAHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootCanvas(nullptr)
    , EmergencyText(nullptr)
    , EditModeText(nullptr)
    , POIListText(nullptr)
    , GoalListText(nullptr)
    , ZoneListText(nullptr)
    , NotificationText(nullptr)
    , CurrentFadeAlpha(1.0f)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAHUDWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAHUDWidget::NativeDestruct()
{
    // 清理 Timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(NotificationTimerHandle);
        World->GetTimerManager().ClearTimer(FadeTimerHandle);
    }

    Super::NativeDestruct();
}

TSharedRef<SWidget> UMAHUDWidget::RebuildWidget()
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    // 构建 UI
    if (!WidgetTree->RootWidget)
    {
        BuildUI();
    }

    return Super::RebuildWidget();
}


//=============================================================================
// BuildUI - 构建 UI 布局
//=============================================================================

void UMAHUDWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAHUDWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMAHUDWidget, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAHUDWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    //=========================================================================
    // Emergency 指示器 - 右上角
    //=========================================================================
    EmergencyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmergencyText"));
    EmergencyText->SetText(FText::FromString(TEXT("Emergency Event")));
    EmergencyText->SetColorAndOpacity(FSlateColor(MAHUDColors::Emergency));

    // 设置字体
    FSlateFontInfo EmergencyFont = EmergencyText->GetFont();
    EmergencyFont.Size = 24;
    EmergencyText->SetFont(EmergencyFont);

    // 设置描边
    EmergencyText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    EmergencyText->SetShadowColorAndOpacity(MAHUDColors::Outline);

    UCanvasPanelSlot* EmergencySlot = RootCanvas->AddChildToCanvas(EmergencyText);
    EmergencySlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));  // 右上角
    EmergencySlot->SetAlignment(FVector2D(1.0f, 0.0f));
    EmergencySlot->SetPosition(FVector2D(-30.0f, 30.0f));
    EmergencySlot->SetAutoSize(true);

    // 默认隐藏
    EmergencyText->SetVisibility(ESlateVisibility::Collapsed);

    //=========================================================================
    // Edit 模式指示器 - 右上角 (Emergency 下方)
    //=========================================================================
    EditModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EditModeText"));
    EditModeText->SetText(FText::FromString(TEXT("Mode: Edit (M)")));
    EditModeText->SetColorAndOpacity(FSlateColor(MAHUDColors::EditMode));

    FSlateFontInfo EditModeFont = EditModeText->GetFont();
    EditModeFont.Size = 18;
    EditModeText->SetFont(EditModeFont);

    EditModeText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    EditModeText->SetShadowColorAndOpacity(MAHUDColors::Outline);

    UCanvasPanelSlot* EditModeSlot = RootCanvas->AddChildToCanvas(EditModeText);
    EditModeSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));  // 右上角
    EditModeSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    EditModeSlot->SetPosition(FVector2D(-30.0f, 60.0f));
    EditModeSlot->SetAutoSize(true);

    // 默认隐藏
    EditModeText->SetVisibility(ESlateVisibility::Collapsed);

    //=========================================================================
    // POI 列表 - 左下角
    //=========================================================================
    POIListText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("POIListText"));
    POIListText->SetText(FText::FromString(TEXT("")));
    POIListText->SetColorAndOpacity(FSlateColor(MAHUDColors::POI));

    FSlateFontInfo POIFont = POIListText->GetFont();
    POIFont.Size = 12;
    POIListText->SetFont(POIFont);

    POIListText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    POIListText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));

    UCanvasPanelSlot* POISlot = RootCanvas->AddChildToCanvas(POIListText);
    POISlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));  // 左下角
    POISlot->SetAlignment(FVector2D(0.0f, 1.0f));
    POISlot->SetPosition(FVector2D(20.0f, -30.0f));
    POISlot->SetAutoSize(true);

    // 默认隐藏
    POIListText->SetVisibility(ESlateVisibility::Collapsed);

    //=========================================================================
    // Goal 列表 - 左下角 (POI 上方)
    //=========================================================================
    GoalListText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalListText"));
    GoalListText->SetText(FText::FromString(TEXT("")));
    GoalListText->SetColorAndOpacity(FSlateColor(MAHUDColors::Goal));

    FSlateFontInfo GoalFont = GoalListText->GetFont();
    GoalFont.Size = 12;
    GoalListText->SetFont(GoalFont);

    GoalListText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    GoalListText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));

    UCanvasPanelSlot* GoalSlot = RootCanvas->AddChildToCanvas(GoalListText);
    GoalSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));  // 左下角
    GoalSlot->SetAlignment(FVector2D(0.0f, 1.0f));
    GoalSlot->SetPosition(FVector2D(20.0f, -48.0f));
    GoalSlot->SetAutoSize(true);

    // 默认隐藏
    GoalListText->SetVisibility(ESlateVisibility::Collapsed);

    //=========================================================================
    // Zone 列表 - 左下角 (Goal 上方)
    //=========================================================================
    ZoneListText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneListText"));
    ZoneListText->SetText(FText::FromString(TEXT("")));
    ZoneListText->SetColorAndOpacity(FSlateColor(MAHUDColors::Zone));

    FSlateFontInfo ZoneFont = ZoneListText->GetFont();
    ZoneFont.Size = 12;
    ZoneListText->SetFont(ZoneFont);

    ZoneListText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    ZoneListText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));

    UCanvasPanelSlot* ZoneSlot = RootCanvas->AddChildToCanvas(ZoneListText);
    ZoneSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));  // 左下角
    ZoneSlot->SetAlignment(FVector2D(0.0f, 1.0f));
    ZoneSlot->SetPosition(FVector2D(20.0f, -66.0f));
    ZoneSlot->SetAutoSize(true);

    // 默认隐藏
    ZoneListText->SetVisibility(ESlateVisibility::Collapsed);

    //=========================================================================
    // 通知消息 - 底部中央 (75% 高度)
    //=========================================================================
    NotificationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotificationText"));
    NotificationText->SetText(FText::FromString(TEXT("")));
    NotificationText->SetColorAndOpacity(FSlateColor(MAHUDColors::Success));
    NotificationText->SetJustification(ETextJustify::Center);

    FSlateFontInfo NotificationFont = NotificationText->GetFont();
    NotificationFont.Size = 18;
    NotificationText->SetFont(NotificationFont);

    NotificationText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    NotificationText->SetShadowColorAndOpacity(MAHUDColors::Outline);

    UCanvasPanelSlot* NotificationSlot = RootCanvas->AddChildToCanvas(NotificationText);
    NotificationSlot->SetAnchors(FAnchors(0.5f, 0.75f, 0.5f, 0.75f));  // 底部中央 75% 高度
    NotificationSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    NotificationSlot->SetPosition(FVector2D(0.0f, 0.0f));
    NotificationSlot->SetAutoSize(true);

    // 默认隐藏
    NotificationText->SetVisibility(ESlateVisibility::Collapsed);

    UE_LOG(LogMAHUDWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}


//=============================================================================
// Emergency 指示器
//=============================================================================

void UMAHUDWidget::ShowEmergencyIndicator()
{
    if (EmergencyText)
    {
        EmergencyText->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogMAHUDWidget, Log, TEXT("Emergency indicator shown"));
    }
}

void UMAHUDWidget::HideEmergencyIndicator()
{
    if (EmergencyText)
    {
        EmergencyText->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAHUDWidget, Log, TEXT("Emergency indicator hidden"));
    }
}

bool UMAHUDWidget::IsEmergencyIndicatorVisible() const
{
    if (EmergencyText)
    {
        return EmergencyText->GetVisibility() == ESlateVisibility::Visible;
    }
    return false;
}

//=============================================================================
// Edit 模式指示器
//=============================================================================

void UMAHUDWidget::SetEditModeVisible(bool bVisible)
{
    ESlateVisibility Visibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

    if (EditModeText)
    {
        EditModeText->SetVisibility(Visibility);
    }
    if (POIListText)
    {
        POIListText->SetVisibility(Visibility);
    }
    if (GoalListText)
    {
        GoalListText->SetVisibility(Visibility);
    }
    if (ZoneListText)
    {
        ZoneListText->SetVisibility(Visibility);
    }

    UE_LOG(LogMAHUDWidget, Log, TEXT("Edit mode visibility set to: %s"), bVisible ? TEXT("Visible") : TEXT("Hidden"));
}

void UMAHUDWidget::UpdatePOIList(const TArray<FString>& POIInfos)
{
    if (!POIListText)
    {
        return;
    }

    if (POIInfos.Num() == 0)
    {
        POIListText->SetText(FText::FromString(TEXT("")));
        return;
    }

    // 格式化 POI 列表: "POI: [1](x,y,z) [2](x,y,z) ..."
    FString POIText = TEXT("POI: ");
    for (const FString& Info : POIInfos)
    {
        POIText += Info + TEXT(" ");
    }

    POIListText->SetText(FText::FromString(POIText));
    UE_LOG(LogMAHUDWidget, Verbose, TEXT("POI list updated: %d items"), POIInfos.Num());
}

void UMAHUDWidget::UpdateGoalList(const TArray<FString>& GoalInfos)
{
    if (!GoalListText)
    {
        return;
    }

    if (GoalInfos.Num() == 0)
    {
        GoalListText->SetText(FText::FromString(TEXT("")));
        return;
    }

    // 格式化 Goal 列表: "Goal: [label](x,y,z) ..."
    FString GoalText = TEXT("Goal: ");
    for (const FString& Info : GoalInfos)
    {
        GoalText += Info + TEXT(" ");
    }

    GoalListText->SetText(FText::FromString(GoalText));
    UE_LOG(LogMAHUDWidget, Verbose, TEXT("Goal list updated: %d items"), GoalInfos.Num());
}

void UMAHUDWidget::UpdateZoneList(const TArray<FString>& ZoneInfos)
{
    if (!ZoneListText)
    {
        return;
    }

    if (ZoneInfos.Num() == 0)
    {
        ZoneListText->SetText(FText::FromString(TEXT("")));
        return;
    }

    // 格式化 Zone 列表: "Zone: [label] ..."
    FString ZoneText = TEXT("Zone: ");
    for (const FString& Info : ZoneInfos)
    {
        ZoneText += Info + TEXT(" ");
    }

    ZoneListText->SetText(FText::FromString(ZoneText));
    UE_LOG(LogMAHUDWidget, Verbose, TEXT("Zone list updated: %d items"), ZoneInfos.Num());
}

//=============================================================================
// 通知消息
//=============================================================================

void UMAHUDWidget::ShowNotification(const FString& Message, bool bIsError, bool bIsWarning)
{
    if (!NotificationText)
    {
        return;
    }

    // 清除之前的 Timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(NotificationTimerHandle);
        World->GetTimerManager().ClearTimer(FadeTimerHandle);
    }

    // 设置消息文本
    NotificationText->SetText(FText::FromString(Message));

    // 设置颜色
    FLinearColor NotificationColor;
    if (bIsError)
    {
        NotificationColor = MAHUDColors::Error;
    }
    else if (bIsWarning)
    {
        NotificationColor = MAHUDColors::Warning;
    }
    else
    {
        NotificationColor = MAHUDColors::Success;
    }
    NotificationText->SetColorAndOpacity(FSlateColor(NotificationColor));

    // 重置透明度
    CurrentFadeAlpha = 1.0f;
    NotificationText->SetRenderOpacity(1.0f);

    // 显示通知
    NotificationText->SetVisibility(ESlateVisibility::Visible);

    // 启动淡出 Timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            NotificationTimerHandle,
            this,
            &UMAHUDWidget::StartNotificationFadeOut,
            NotificationDisplayDuration,
            false
        );
    }

    UE_LOG(LogMAHUDWidget, Log, TEXT("ShowNotification: %s (Error=%d, Warning=%d)"),
        *Message, bIsError, bIsWarning);
}

void UMAHUDWidget::StartNotificationFadeOut()
{
    UE_LOG(LogMAHUDWidget, Verbose, TEXT("Starting notification fade out"));

    // 启动淡出动画 Timer (每帧更新)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FadeTimerHandle,
            this,
            &UMAHUDWidget::OnFadeTick,
            0.016f,  // ~60 FPS
            true     // 循环
        );
    }
}

void UMAHUDWidget::OnFadeTick()
{
    // 计算淡出进度
    float FadeStep = 0.016f / NotificationFadeDuration;  // 每帧减少的透明度
    CurrentFadeAlpha -= FadeStep;

    if (CurrentFadeAlpha <= 0.0f)
    {
        CurrentFadeAlpha = 0.0f;
        OnNotificationFadeComplete();
        return;
    }

    // 更新透明度
    if (NotificationText)
    {
        NotificationText->SetRenderOpacity(CurrentFadeAlpha);
    }
}

void UMAHUDWidget::OnNotificationFadeComplete()
{
    UE_LOG(LogMAHUDWidget, Verbose, TEXT("Notification fade complete"));

    // 清除 Timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FadeTimerHandle);
    }

    // 隐藏通知
    if (NotificationText)
    {
        NotificationText->SetVisibility(ESlateVisibility::Collapsed);
        NotificationText->SetRenderOpacity(1.0f);  // 重置透明度
    }

    CurrentFadeAlpha = 1.0f;
}
