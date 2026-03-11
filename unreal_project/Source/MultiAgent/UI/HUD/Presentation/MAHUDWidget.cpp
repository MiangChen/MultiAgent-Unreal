// MAHUDWidget.cpp
// HUD Widget 实现

#include "MAHUDWidget.h"
#include "../../Core/MAUITheme.h"
#include "../Infrastructure/MAHUDWidgetRuntimeAdapter.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDWidget, Log, All);

//=============================================================================
// 颜色常量
//=============================================================================
namespace MAHUDColors
{
    static const FLinearColor EditMode = FLinearColor(0.3f, 0.6f, 1.0f);    // 蓝色
    static const FLinearColor POI = FLinearColor(0.3f, 0.8f, 0.3f);         // 绿色
    static const FLinearColor Goal = FLinearColor(1.0f, 0.4f, 0.4f);        // 红色
    static const FLinearColor Zone = FLinearColor(0.3f, 0.8f, 1.0f);        // 青色
    static const FLinearColor Success = FLinearColor::Green;
    static const FLinearColor Warning = FLinearColor::Yellow;
    static const FLinearColor Error = FLinearColor::Red;
    static const FLinearColor Outline = FLinearColor::Black;
}

namespace
{
const FMAHUDWidgetRuntimeAdapter& HUDWidgetRuntimeAdapter()
{
    static const FMAHUDWidgetRuntimeAdapter Adapter;
    return Adapter;
}

FLinearColor ResolveNotificationColor(const UMAUITheme* Theme, const EMAHUDWidgetNotificationSeverity Severity)
{
    switch (Severity)
    {
    case EMAHUDWidgetNotificationSeverity::Error:
        return Theme ? Theme->DangerColor : MAHUDColors::Error;
    case EMAHUDWidgetNotificationSeverity::Warning:
        return Theme ? Theme->WarningColor : MAHUDColors::Warning;
    case EMAHUDWidgetNotificationSeverity::Info:
    default:
        return Theme ? Theme->SuccessColor : MAHUDColors::Success;
    }
}
}

//=============================================================================
// 构造函数
//=============================================================================

UMAHUDWidget::UMAHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , Theme(nullptr)
    , RootCanvas(nullptr)
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
    HUDWidgetRuntimeAdapter().ClearNotificationTimers(this);

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
    // Edit 模式指示器 - 右上角
    //=========================================================================
    EditModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EditModeText"));
    EditModeText->SetText(FText::FromString(TEXT("Mode: Edit (M)")));
    // 使用 Theme 颜色（带 fallback）
    FLinearColor EditModeColor = Theme ? Theme->ModeEditColor : MAHUDColors::EditMode;
    EditModeText->SetColorAndOpacity(FSlateColor(EditModeColor));

    FSlateFontInfo EditModeFont = EditModeText->GetFont();
    EditModeFont.Size = 18;
    EditModeText->SetFont(EditModeFont);

    EditModeText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    // 使用 Theme 阴影颜色（带 fallback）
    FLinearColor EditModeShadowColor = Theme ? Theme->ShadowColor : MAHUDColors::Outline;
    EditModeText->SetShadowColorAndOpacity(EditModeShadowColor);

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
    // 使用 Theme 颜色（带 fallback）
    FLinearColor POIColor = Theme ? Theme->SuccessColor : MAHUDColors::POI;
    POIListText->SetColorAndOpacity(FSlateColor(POIColor));

    FSlateFontInfo POIFont = POIListText->GetFont();
    POIFont.Size = 12;
    POIListText->SetFont(POIFont);

    POIListText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    // 使用 Theme 阴影颜色（带 fallback）
    FLinearColor POIShadowColor = Theme ? Theme->ShadowColor : FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    POIListText->SetShadowColorAndOpacity(POIShadowColor);

    UCanvasPanelSlot* POISlot = RootCanvas->AddChildToCanvas(POIListText);
    POISlot->SetAnchors(FAnchors(0.5f, 0.85f, 0.5f, 0.85f));  // 屏幕中下方 (85%高度)
    POISlot->SetAlignment(FVector2D(0.5f, 0.5f));
    POISlot->SetPosition(FVector2D(0.0f, 0.0f));
    POISlot->SetAutoSize(true);

    // 默认隐藏
    POIListText->SetVisibility(ESlateVisibility::Collapsed);

    //=========================================================================
    // Goal 列表 - 左下角 (POI 上方)
    //=========================================================================
    GoalListText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalListText"));
    GoalListText->SetText(FText::FromString(TEXT("")));
    // 使用 Theme 颜色（带 fallback）
    FLinearColor GoalColor = Theme ? Theme->DangerColor : MAHUDColors::Goal;
    GoalListText->SetColorAndOpacity(FSlateColor(GoalColor));

    FSlateFontInfo GoalFont = GoalListText->GetFont();
    GoalFont.Size = 12;
    GoalListText->SetFont(GoalFont);

    GoalListText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    // 使用 Theme 阴影颜色（带 fallback）
    FLinearColor GoalShadowColor = Theme ? Theme->ShadowColor : FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    GoalListText->SetShadowColorAndOpacity(GoalShadowColor);

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
    // 使用 Theme 颜色（带 fallback）
    FLinearColor ZoneColor = Theme ? Theme->PrimaryColor : MAHUDColors::Zone;
    ZoneListText->SetColorAndOpacity(FSlateColor(ZoneColor));

    FSlateFontInfo ZoneFont = ZoneListText->GetFont();
    ZoneFont.Size = 12;
    ZoneListText->SetFont(ZoneFont);

    ZoneListText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    // 使用 Theme 阴影颜色（带 fallback）
    FLinearColor ZoneShadowColor = Theme ? Theme->ShadowColor : FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    ZoneListText->SetShadowColorAndOpacity(ZoneShadowColor);

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
    // 使用 Theme 颜色（带 fallback）
    FLinearColor NotificationColor = Theme ? Theme->SuccessColor : MAHUDColors::Success;
    NotificationText->SetColorAndOpacity(FSlateColor(NotificationColor));
    NotificationText->SetJustification(ETextJustify::Center);

    FSlateFontInfo NotificationFont = NotificationText->GetFont();
    NotificationFont.Size = 18;
    NotificationText->SetFont(NotificationFont);

    NotificationText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    // 使用 Theme 阴影颜色（带 fallback）
    FLinearColor NotificationShadowColor = Theme ? Theme->ShadowColor : MAHUDColors::Outline;
    NotificationText->SetShadowColorAndOpacity(NotificationShadowColor);

    UCanvasPanelSlot* NotificationSlot = RootCanvas->AddChildToCanvas(NotificationText);
    NotificationSlot->SetAnchors(FAnchors(0.5f, 0.75f, 0.5f, 0.75f));  // 底部中央 75% 高度
    NotificationSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    NotificationSlot->SetPosition(FVector2D(0.0f, 0.0f));
    NotificationSlot->SetAutoSize(true);

    // 默认隐藏
    NotificationText->SetVisibility(ESlateVisibility::Collapsed);

    UE_LOG(LogMAHUDWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}


// Edit 模式指示器
//=============================================================================

void UMAHUDWidget::ApplyEditModeViewModel(const FMAHUDWidgetEditModeViewModel& ViewModel)
{
    if (EditModeText)
    {
        EditModeText->SetVisibility(ViewModel.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    ApplyTextBlockState(POIListText, ViewModel.POIText, ViewModel.bVisible && !ViewModel.POIText.IsEmpty());
    ApplyTextBlockState(GoalListText, ViewModel.GoalText, ViewModel.bVisible && !ViewModel.GoalText.IsEmpty());
    ApplyTextBlockState(ZoneListText, ViewModel.ZoneText, ViewModel.bVisible && !ViewModel.ZoneText.IsEmpty());
}

//=============================================================================
// 通知消息
//=============================================================================

void UMAHUDWidget::ApplyNotificationModel(const FMAHUDWidgetNotificationModel& NotificationModel)
{
    if (!NotificationText)
    {
        return;
    }

    HUDWidgetRuntimeAdapter().ClearNotificationTimers(this);

    // 设置消息文本
    NotificationText->SetText(FText::FromString(NotificationModel.Message));

    const FLinearColor NotificationColor = ResolveNotificationColor(Theme, NotificationModel.Severity);
    NotificationText->SetColorAndOpacity(FSlateColor(NotificationColor));

    // 重置透明度
    CurrentFadeAlpha = 1.0f;
    NotificationText->SetRenderOpacity(1.0f);

    // 显示通知
    NotificationText->SetVisibility(ESlateVisibility::Visible);

    HUDWidgetRuntimeAdapter().ScheduleNotificationFadeOut(this, NotificationDisplayDuration);

    UE_LOG(LogMAHUDWidget, Log, TEXT("ApplyNotificationModel: %s"), *NotificationModel.Message);
}

void UMAHUDWidget::StartNotificationFadeOut()
{
    UE_LOG(LogMAHUDWidget, Verbose, TEXT("Starting notification fade out"));

    HUDWidgetRuntimeAdapter().ScheduleFadeTick(this, 0.016f);
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

    HUDWidgetRuntimeAdapter().ClearNotificationTimers(this);

    // 隐藏通知
    if (NotificationText)
    {
        NotificationText->SetVisibility(ESlateVisibility::Collapsed);
        NotificationText->SetRenderOpacity(1.0f);  // 重置透明度
    }

    CurrentFadeAlpha = 1.0f;
}

//=============================================================================
// 主题系统
//=============================================================================

void UMAHUDWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme) return;

    // 更新 Edit 模式指示器颜色
    if (EditModeText)
    {
        EditModeText->SetColorAndOpacity(FSlateColor(Theme->ModeEditColor));
        EditModeText->SetShadowColorAndOpacity(Theme->ShadowColor);
    }

    // 更新 POI 列表颜色
    if (POIListText)
    {
        POIListText->SetColorAndOpacity(FSlateColor(Theme->SuccessColor));
        POIListText->SetShadowColorAndOpacity(Theme->ShadowColor);
    }

    // 更新 Goal 列表颜色
    if (GoalListText)
    {
        GoalListText->SetColorAndOpacity(FSlateColor(Theme->DangerColor));
        GoalListText->SetShadowColorAndOpacity(Theme->ShadowColor);
    }

    // 更新 Zone 列表颜色
    if (ZoneListText)
    {
        ZoneListText->SetColorAndOpacity(FSlateColor(Theme->PrimaryColor));
        ZoneListText->SetShadowColorAndOpacity(Theme->ShadowColor);
    }

    // 更新通知消息颜色（默认使用成功色，实际颜色在 ShowNotification 中根据类型设置）
    if (NotificationText)
    {
        NotificationText->SetColorAndOpacity(FSlateColor(Theme->SuccessColor));
        NotificationText->SetShadowColorAndOpacity(Theme->ShadowColor);
    }

    UE_LOG(LogMAHUDWidget, Log, TEXT("Theme applied to HUD Widget"));
}

void UMAHUDWidget::ApplyTextBlockState(UTextBlock* TextBlock, const FString& Text, bool bVisible)
{
    if (!TextBlock)
    {
        return;
    }

    TextBlock->SetText(FText::FromString(Text));
    TextBlock->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
