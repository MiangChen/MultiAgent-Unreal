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
// 通知控制 (Requirements: 4.1, 4.2, 4.3)
//=============================================================================

void UMANotificationWidget::ShowNotification(EMANotificationType Type)
{
    if (Type == EMANotificationType::None)
    {
        HideNotification();
        return;
    }

    // 如果已经在显示相同类型的通知且不在动画中，忽略
    if (bIsVisible && CurrentType == Type && !bIsAnimating && bIsShowing)
    {
        UE_LOG(LogTemp, Log, TEXT("[Notification] Ignoring duplicate notification type: %d (already showing)"), static_cast<int32>(Type));
        return;
    }

    // 如果正在隐藏动画中，立即停止并重新开始显示动画
    if (bIsAnimating && !bIsShowing)
    {
        UE_LOG(LogTemp, Log, TEXT("[Notification] Interrupting hide animation to show new notification"));
        // 不需要重置位置，从当前位置开始显示动画
    }
    else
    {
        // 设置初始动画状态 (从左边缘开始)
        CurrentOffsetX = AnimStartOffsetX;
        CurrentOpacity = 0.0f;
    }

    CurrentType = Type;
    bIsVisible = true;
    bIsShowing = true;
    bIsAnimating = true;
    CurrentAnimTime = 0.0f;

    // 更新内容
    UpdateNotificationContent();

    // 显示 Widget
    SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Log, TEXT("[Notification] Showing notification type: %d"), static_cast<int32>(Type));
}

void UMANotificationWidget::HideNotification()
{
    if (!bIsVisible)
    {
        return;
    }

    bIsShowing = false;
    bIsAnimating = true;
    CurrentAnimTime = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("[Notification] Hiding notification"));
}

//=============================================================================
// 消息获取 (Requirements: 4.1, 4.2, 4.3)
//=============================================================================

FString UMANotificationWidget::GetNotificationMessage(EMANotificationType Type)
{
    switch (Type)
    {
        case EMANotificationType::TaskGraphUpdate:
            return TEXT("Task Graph Updated");
        case EMANotificationType::SkillListUpdate:
            return TEXT("Skill List Updated");
        case EMANotificationType::SkillListExecuting:
            return TEXT("Skills Executing");
        case EMANotificationType::EmergencyEvent:
            return TEXT("Unexpected Situation Detected");
        case EMANotificationType::RequestUserCommand:
            return TEXT("Command Input Required");
        default:
            return TEXT("");
    }
}

FString UMANotificationWidget::GetKeyHint(EMANotificationType Type)
{
    switch (Type)
    {
        case EMANotificationType::TaskGraphUpdate:
            return TEXT("Press 'Z' to review Task Graph");
        case EMANotificationType::SkillListUpdate:
            return TEXT("Press 'N' to review Skill List");
        case EMANotificationType::SkillListExecuting:
            return TEXT("Skills are currently being executed");
        case EMANotificationType::EmergencyEvent:
            return TEXT("Press 'X' to check Unexpected Situation");
        case EMANotificationType::RequestUserCommand:
            return TEXT("Please enter command in the right sidebar");
        default:
            return TEXT("");
    }
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
        FLinearColor BgColor = GetBackgroundColorForType(CurrentType);
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

    if (bIsAnimating)
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
        FString Message = GetNotificationMessage(CurrentType);
        NotificationText->SetText(FText::FromString(Message));
    }

    if (KeyHintText)
    {
        FString Hint = GetKeyHint(CurrentType);
        KeyHintText->SetText(FText::FromString(Hint));
    }

    // 更新背景颜色和圆角
    if (NotificationBorder)
    {
        FLinearColor BgColor = GetBackgroundColorForType(CurrentType);
        MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
            NotificationBorder,
            Theme,
            EMARoundedElementType::Panel
        );
        NotificationBorder->SetBrushColor(BgColor);
    }
}

void UMANotificationWidget::UpdateAnimation(float DeltaTime)
{
    CurrentAnimTime += DeltaTime;

    float Duration = bIsShowing ? SlideInDuration : SlideOutDuration;
    float Progress = FMath::Clamp(CurrentAnimTime / Duration, 0.0f, 1.0f);

    // 使用 ease-out 缓动函数 (更丝滑的动画)
    float EasedProgress = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);

    if (bIsShowing)
    {
        // 滑入动画：从左边缘滑入
        CurrentOffsetX = FMath::Lerp(AnimStartOffsetX, AnimEndOffsetX, EasedProgress);
        CurrentOpacity = EasedProgress;
    }
    else
    {
        // 滑出动画：向左边缘滑出
        CurrentOffsetX = FMath::Lerp(AnimEndOffsetX, AnimStartOffsetX, EasedProgress);
        CurrentOpacity = 1.0f - EasedProgress;
    }

    ApplyAnimationState();

    // 检查动画是否完成
    if (Progress >= 1.0f)
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
            BorderSlot->SetPosition(FVector2D(CurrentOffsetX, 0.0f));
        }
    }

    // 应用透明度
    SetRenderOpacity(CurrentOpacity);
}

void UMANotificationWidget::OnAnimationFinished()
{
    bIsAnimating = false;

    if (!bIsShowing)
    {
        // 隐藏动画完成，隐藏 Widget
        bIsVisible = false;
        CurrentType = EMANotificationType::None;
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
    // 根据通知类型返回不同的背景颜色
    switch (Type)
    {
        case EMANotificationType::TaskGraphUpdate:
            // 蓝色调 - 任务图更新
            if (Theme)
            {
                FLinearColor Color = Theme->PrimaryColor;
                Color.A = 0.95f;
                return Color * 0.3f + FLinearColor(0.1f, 0.1f, 0.12f, 0.95f) * 0.7f;
            }
            return FLinearColor(0.12f, 0.15f, 0.22f, 0.95f);

        case EMANotificationType::SkillListUpdate:
            // 绿色调 - 技能列表更新
            if (Theme)
            {
                FLinearColor Color = Theme->SuccessColor;
                Color.A = 0.95f;
                return Color * 0.3f + FLinearColor(0.1f, 0.1f, 0.12f, 0.95f) * 0.7f;
            }
            return FLinearColor(0.12f, 0.18f, 0.14f, 0.95f);

        case EMANotificationType::SkillListExecuting:
            // 黄色调 - 技能列表执行中
            if (Theme)
            {
                FLinearColor Color = Theme->WarningColor;
                Color.A = 0.95f;
                return Color * 0.3f + FLinearColor(0.1f, 0.1f, 0.12f, 0.95f) * 0.7f;
            }
            return FLinearColor(0.22f, 0.20f, 0.12f, 0.95f);

        case EMANotificationType::EmergencyEvent:
            // 红色调 - 突发事件
            if (Theme)
            {
                FLinearColor Color = Theme->DangerColor;
                Color.A = 0.95f;
                return Color * 0.3f + FLinearColor(0.1f, 0.1f, 0.12f, 0.95f) * 0.7f;
            }
            return FLinearColor(0.22f, 0.12f, 0.12f, 0.95f);

        case EMANotificationType::RequestUserCommand:
            // 蓝色调 - 索要用户指令
            return FLinearColor(0.1f, 0.3f, 0.6f, 0.9f);

        default:
            // 默认深色背景
            if (Theme)
            {
                return Theme->BackgroundColor;
            }
            return FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);
    }
}
