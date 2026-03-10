// MANotificationWidget.cpp
// 通知组件实现

#include "MANotificationWidget.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

//=============================================================================
// 构造函数
//=============================================================================

UMANotificationWidget::UMANotificationWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 默认不可见
    SetVisibility(ESlateVisibility::Collapsed);
}

//=============================================================================
// 通知控制
//=============================================================================

void UMANotificationWidget::ShowNotification(EMANotificationType Type)
{
    Coordinator.BeginShow(NotificationState, Type);
    if (NotificationState.Type == EMANotificationType::None)
    {
        HideNotification();
        return;
    }

    // 更新内容
    UpdateNotificationContent();

    // 显示 Widget
    SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Log, TEXT("[Notification] Showing notification type: %d"), static_cast<int32>(Type));
}

void UMANotificationWidget::HideNotification()
{
    Coordinator.BeginHide(NotificationState);

    UE_LOG(LogTemp, Log, TEXT("[Notification] Hiding notification"));
}

//=============================================================================
// 消息获取
//=============================================================================

FString UMANotificationWidget::GetNotificationMessage(EMANotificationType Type)
{
    return FMANotificationCoordinator().GetNotificationMessage(Type);
}

FString UMANotificationWidget::GetKeyHint(EMANotificationType Type)
{
    return FMANotificationCoordinator().GetKeyHint(Type);
}

//=============================================================================
// 主题应用
//=============================================================================

void UMANotificationWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;

    if (!Theme)
    {
        return;
    }

    // 应用圆角和背景颜色
    if (NotificationBorder)
    {
        FLinearColor BgColor = GetBackgroundColorForType(NotificationState.Type);
        MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
            NotificationBorder,
            Theme,
            EMARoundedElementType::Panel
        );
        NotificationBorder->SetBrushColor(BgColor);
    }

    // 应用文本样式
    if (NotificationText)
    {
        NotificationText->SetFont(Theme->BodyFont);
        NotificationText->SetColorAndOpacity(Theme->TextColor);
    }

    if (KeyHintText)
    {
        KeyHintText->SetFont(Theme->BodyFont);
        // 按键提示使用稍微暗一点的颜色
        FLinearColor HintColor = Theme->TextColor;
        HintColor.A = 0.8f;
        KeyHintText->SetColorAndOpacity(HintColor);
    }
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMANotificationWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMANotificationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Log, TEXT("[Notification] Widget NativeConstruct"));
}

void UMANotificationWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void UMANotificationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (NotificationState.bAnimating)
    {
        UpdateAnimation(InDeltaTime);
    }
}

TSharedRef<SWidget> UMANotificationWidget::RebuildWidget()
{
    BuildUI();
    return Super::RebuildWidget();
}

//=============================================================================
// 内部方法
//=============================================================================

void UMANotificationWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("[Notification] BuildUI: WidgetTree is null"));
        return;
    }
    
    // 创建根 Canvas
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogTemp, Error, TEXT("[Notification] BuildUI: Failed to create RootCanvas"));
        return;
    }
    
    // 设置为根 Widget
    WidgetTree->RootWidget = RootCanvas;

    // 创建通知边框 (圆角背景)
    NotificationBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotificationBorder"));
    if (!NotificationBorder)
    {
        return;
    }
    
    // 应用圆角效果 - 使用 Panel 类型的圆角半径
    FLinearColor BgColor = FLinearColor(0.15f, 0.15f, 0.18f, 0.95f);
    MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
        NotificationBorder,
        Theme,
        EMARoundedElementType::Panel
    );
    
    // 如果主题为空，使用默认颜色
    if (!Theme)
    {
        NotificationBorder->SetBrushColor(BgColor);
    }
    
    NotificationBorder->SetPadding(FMargin(NotificationPadding));

    // 创建内容垂直布局
    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
    if (!ContentBox)
    {
        return;
    }

    // 创建通知消息文本
    NotificationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotificationText"));
    if (NotificationText)
    {
        NotificationText->SetText(FText::FromString(TEXT("")));
        // 使用主题颜色或 fallback 默认值
        FLinearColor NotificationTextColor = Theme ? Theme->TextColor : FLinearColor::White;
        NotificationText->SetColorAndOpacity(NotificationTextColor);
        
        // 设置字体
        FSlateFontInfo FontInfo = Theme ? Theme->BodyFont : NotificationText->GetFont();
        FontInfo.Size = 14;
        NotificationText->SetFont(FontInfo);
    }

    // 创建按键提示文本
    KeyHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyHintText"));
    if (KeyHintText)
    {
        KeyHintText->SetText(FText::FromString(TEXT("")));
        // 使用主题颜色或 fallback 默认值 (稍暗的文字颜色)
        FLinearColor HintColor = Theme ? Theme->TextColor : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
        HintColor.A = 0.8f;
        KeyHintText->SetColorAndOpacity(HintColor);
        
        // 设置字体 (稍小)
        FSlateFontInfo HintFontInfo = Theme ? Theme->BodyFont : KeyHintText->GetFont();
        HintFontInfo.Size = 12;
        KeyHintText->SetFont(HintFontInfo);
    }

    // 组装 UI 层级
    // ContentBox -> NotificationText, KeyHintText
    if (NotificationText)
    {
        ContentBox->AddChild(NotificationText);
    }
    if (KeyHintText)
    {
        ContentBox->AddChild(KeyHintText);
    }

    // 设置文本间距
    if (NotificationText)
    {
        if (UVerticalBoxSlot* NotificationSlot = Cast<UVerticalBoxSlot>(NotificationText->Slot))
        {
            NotificationSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        }
    }

    // NotificationBorder -> ContentBox
    NotificationBorder->SetContent(ContentBox);

    // RootCanvas -> NotificationBorder
    RootCanvas->AddChild(NotificationBorder);

    // 设置通知边框位置和大小
    if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(NotificationBorder->Slot))
    {
        BorderSlot->SetPosition(FVector2D(0.0f, 0.0f));
        BorderSlot->SetSize(FVector2D(NotificationWidth, 0.0f));
        BorderSlot->SetAutoSize(true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Notification] UI built"));
}

void UMANotificationWidget::UpdateNotificationContent()
{
    if (NotificationText)
    {
        const FMANotificationContentModel Model = Coordinator.BuildContentModel(NotificationState.Type, Theme);
        NotificationText->SetText(FText::FromString(Model.Message));
        if (NotificationBorder)
        {
            MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
                NotificationBorder,
                Theme,
                EMARoundedElementType::Panel
            );
            NotificationBorder->SetBrushColor(Model.BackgroundColor);
        }

        if (KeyHintText)
        {
            KeyHintText->SetText(FText::FromString(Model.KeyHint));
        }
    }
}

void UMANotificationWidget::UpdateAnimation(float DeltaTime)
{
    const FMANotificationAnimationFrame Frame = Coordinator.StepAnimation(
        NotificationState,
        DeltaTime,
        SlideInDuration,
        SlideOutDuration,
        AnimStartOffsetX,
        AnimEndOffsetX);

    ApplyAnimationState();

    if (Frame.bFinished)
    {
        OnAnimationFinished();
    }
}

void UMANotificationWidget::ApplyAnimationState()
{
    // 应用位置偏移 (X 方向，从左边缘滑入)
    if (NotificationBorder)
    {
        if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(NotificationBorder->Slot))
        {
            BorderSlot->SetPosition(FVector2D(NotificationState.CurrentOffsetX, 0.0f));
        }
    }

    // 应用透明度
    SetRenderOpacity(NotificationState.CurrentOpacity);
}

void UMANotificationWidget::OnAnimationFinished()
{
    Coordinator.FinishAnimation(NotificationState);

    if (!NotificationState.bVisible)
    {
        // 隐藏动画完成，隐藏 Widget
        SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Log, TEXT("[Notification] Hide animation finished"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Notification] Show animation finished"));
    }
}

FLinearColor UMANotificationWidget::GetBackgroundColorForType(EMANotificationType Type) const
{
    return FMANotificationCoordinator().GetBackgroundColorForType(Type, Theme);
}
