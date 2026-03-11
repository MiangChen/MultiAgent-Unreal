// MABaseModalWidget.cpp
// 基础模态窗口抽象类实现

#include "MABaseModalWidget.h"
#include "../../Components/Presentation/MAStyledButton.h"
#include "../MAUITheme.h"
#include "../MARoundedBorderUtils.h"
#include "../MAFrostedGlassUtils.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/BackgroundBlur.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMABaseModal, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMABaseModalWidget::UMABaseModalWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , RootPanel(nullptr)
    , BackgroundOverlay(nullptr)
    , ModalSizeBox(nullptr)
    , BackgroundBorder(nullptr)
    , ModalVerticalBox(nullptr)
    , TitleText(nullptr)
    , ContentContainer(nullptr)
    , ButtonContainer(nullptr)
    , ConfirmButton(nullptr)
    , RejectButton(nullptr)
    , EditButton(nullptr)
    , CloseButton(nullptr)
    , Theme(nullptr)
    , ModalTitle(FText::FromString(TEXT("Modal")))
    , ModalWidth(800.0f)
    , ModalHeight(600.0f)
    , bShowEditButton(true)
    , ConfirmButtonText(FText::FromString(TEXT("Confirm")))
    , RejectButtonText(FText::FromString(TEXT("Reject")))
    , EditButtonText(FText::FromString(TEXT("Edit")))
    , bIsEditMode(false)
    , ModalType(EMAModalType::None)
    , bIsAnimating(false)
    , bIsShowing(false)
    , CurrentAnimTime(0.0f)
    , TargetOpacity(1.0f)
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

void UMABaseModalWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMABaseModalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 绑定按钮事件
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UMABaseModalWidget::OnConfirmButtonClicked);
    }
    if (RejectButton)
    {
        RejectButton->OnClicked.AddDynamic(this, &UMABaseModalWidget::OnRejectButtonClicked);
    }
    if (EditButton)
    {
        EditButton->OnClicked.AddDynamic(this, &UMABaseModalWidget::OnEditButtonClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UMABaseModalWidget::OnCloseButtonClicked);
    }

    // 更新按钮可见性
    UpdateButtonVisibility();

    // 设置初始透明度为 0（准备淡入动画）
    SetRenderOpacity(0.0f);

    UE_LOG(LogMABaseModal, Log, TEXT("MABaseModalWidget constructed"));
}

void UMABaseModalWidget::NativeDestruct()
{
    // 解绑按钮事件
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.RemoveDynamic(this, &UMABaseModalWidget::OnConfirmButtonClicked);
    }
    if (RejectButton)
    {
        RejectButton->OnClicked.RemoveDynamic(this, &UMABaseModalWidget::OnRejectButtonClicked);
    }
    if (EditButton)
    {
        EditButton->OnClicked.RemoveDynamic(this, &UMABaseModalWidget::OnEditButtonClicked);
    }
    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UMABaseModalWidget::OnCloseButtonClicked);
    }

    Super::NativeDestruct();
}

void UMABaseModalWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 更新动画
    if (bIsAnimating)
    {
        UpdateAnimation(InDeltaTime);
    }
}

TSharedRef<SWidget> UMABaseModalWidget::RebuildWidget()
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

void UMABaseModalWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMABaseModal, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMABaseModal, Log, TEXT("BuildUI: Starting modal construction..."));

    // 创建根 CanvasPanel
    RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel"));
    if (!RootPanel)
    {
        UE_LOG(LogMABaseModal, Error, TEXT("BuildUI: Failed to create RootPanel!"));
        return;
    }
    WidgetTree->RootWidget = RootPanel;

    // 创建背景遮罩 (半透明黑色)
    BackgroundOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundOverlay"));
    if (BackgroundOverlay)
    {
        RootPanel->AddChild(BackgroundOverlay);
        
        // 设置为全屏
        if (UCanvasPanelSlot* OverlaySlot = Cast<UCanvasPanelSlot>(BackgroundOverlay->Slot))
        {
            OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
            OverlaySlot->SetOffsets(FMargin(0.0f));
        }
        
        // 设置遮罩背景颜色 - 使用 Theme 或 fallback 默认值
        FLinearColor OverlayBgColor = Theme ? Theme->OverlayColor : FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
        BackgroundOverlay->SetBrushColor(OverlayBgColor);
        
        // 设置为 Visible 以阻止点击穿透到场景
        BackgroundOverlay->SetVisibility(ESlateVisibility::Visible);
    }

    // === 使用毛玻璃效果创建模态背景 ===
    // 获取毛玻璃配置
    FMAFrostedGlassConfig GlassConfig = MAFrostedGlassUtils::GetConfigFromTheme(Theme);
    
    // 1. 创建阴影层 (必须在 ModalSizeBox 之前添加，这样阴影才会在弹窗下面)
    UBorder* ShadowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Modal_Shadow"));
    if (ShadowBorder)
    {
        FLinearColor ShadowColor(0.0f, 0.0f, 0.0f, GlassConfig.ShadowOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(ShadowBorder, ShadowColor, GlassConfig.CornerRadius);
        ShadowBorder->SetPadding(FMargin(0.0f));
        
        // 阴影作为独立元素添加到 RootPanel，位置与 ModalSizeBox 相同但有偏移
        RootPanel->AddChild(ShadowBorder);
        if (UCanvasPanelSlot* ShadowSlot = Cast<UCanvasPanelSlot>(ShadowBorder->Slot))
        {
            // 使用与 ModalSizeBox 相同的锚点和对齐方式
            ShadowSlot->SetAnchors(FAnchors(0.55f, 0.5f, 0.55f, 0.5f));
            ShadowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            // 阴影偏移：向右下偏移
            ShadowSlot->SetPosition(FVector2D(GlassConfig.ShadowOffset, GlassConfig.ShadowOffset));
            ShadowSlot->SetSize(FVector2D(ModalWidth, ModalHeight));
            ShadowSlot->SetAutoSize(false);
        }
    }

    // 创建模态 SizeBox (用于控制模态大小和居中偏右)
    ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ModalSizeBox"));
    if (ModalSizeBox)
    {
        RootPanel->AddChild(ModalSizeBox);
        
        // 设置居中偏右 (锚点在中心偏右，对齐点在中心)
        // 锚点 (0.55, 0.5) 表示在水平方向上偏右 5%
        if (UCanvasPanelSlot* ModalSlot = Cast<UCanvasPanelSlot>(ModalSizeBox->Slot))
        {
            ModalSlot->SetAnchors(FAnchors(0.55f, 0.5f, 0.55f, 0.5f));
            ModalSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            ModalSlot->SetAutoSize(true);
        }
        
        // 设置模态大小 (覆盖约 80% 画面)
        // 假设屏幕宽度 1920，高度 1080，80% 约为 1536x864
        // 但我们使用相对大小，在子类中可以覆盖
        ModalSizeBox->SetWidthOverride(ModalWidth);
        ModalSizeBox->SetHeightOverride(ModalHeight);
    }

    // 2. 创建玻璃边框层
    UBorder* GlassBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Modal_GlassBorder"));
    if (GlassBorder && ModalSizeBox)
    {
        FLinearColor GlassBorderColor(1.0f, 1.0f, 1.0f, GlassConfig.GlassBorderOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(GlassBorder, GlassBorderColor, GlassConfig.CornerRadius);
        // 增加 padding 以确保 BackgroundBlur 的矩形边缘被圆角边框覆盖
        float EffectivePadding = FMath::Max(GlassConfig.GlassBorderThickness, GlassConfig.CornerRadius * 0.3f);
        GlassBorder->SetPadding(FMargin(EffectivePadding));
        // 启用裁剪，确保内容不会超出圆角边框
        GlassBorder->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        ModalSizeBox->AddChild(GlassBorder);
    }
    
    // 3. 创建模糊层
    UBackgroundBlur* BlurBackground = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("Modal_Blur"));
    if (BlurBackground && GlassBorder)
    {
        BlurBackground->SetBlurStrength(GlassConfig.BlurStrength);
        BlurBackground->SetApplyAlphaToBlur(true);
        // 设置低质量回退画刷为透明，避免黑色背景
        FSlateBrush TransparentFallbackBrush;
        TransparentFallbackBrush.TintColor = FSlateColor(FLinearColor::Transparent);
        BlurBackground->SetLowQualityFallbackBrush(TransparentFallbackBrush);
        // 增加模糊层的 padding，使其边缘更靠内，避免超出圆角边框
        float BlurPadding = GlassConfig.ContentPadding + GlassConfig.CornerRadius * 0.15f;
        BlurBackground->SetPadding(FMargin(BlurPadding));
        GlassBorder->AddChild(BlurBackground);
    }
    
    // 4. 创建光泽层
    UBorder* GlossOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Modal_Gloss"));
    if (GlossOverlay && BlurBackground)
    {
        FLinearColor GlossColor(1.0f, 1.0f, 1.0f, GlassConfig.GlossOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(GlossOverlay, GlossColor, GlassConfig.CornerRadius - 2.0f);
        GlossOverlay->SetPadding(FMargin(0.0f));
        BlurBackground->AddChild(GlossOverlay);
    }
    
    // BackgroundBorder 现在指向光泽层作为内容容器
    BackgroundBorder = GlossOverlay;

    // 创建模态内容垂直布局
    ModalVerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModalVerticalBox"));
    if (ModalVerticalBox && BackgroundBorder)
    {
        BackgroundBorder->AddChild(ModalVerticalBox);
    }

    // 创建标题栏水平布局 (标题 + 关闭按钮)
    UHorizontalBox* TitleBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TitleBar"));
    if (TitleBar && ModalVerticalBox)
    {
        ModalVerticalBox->AddChild(TitleBar);
        
        // 设置底部间距
        if (UVerticalBoxSlot* TitleBarSlot = Cast<UVerticalBoxSlot>(TitleBar->Slot))
        {
            float Spacing = Theme ? Theme->ElementSpacing : 12.0f;
            TitleBarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Spacing));
        }
        
        // 创建标题文本
        TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
        if (TitleText)
        {
            TitleBar->AddChild(TitleText);
            
            // 设置标题样式
            TitleText->SetText(GetModalTitleText());
            
            // 设置字体
            FSlateFontInfo TitleFont = Theme ? Theme->TitleFont : FSlateFontInfo();
            if (TitleFont.FontObject == nullptr)
            {
                TitleFont = TitleText->GetFont();
                TitleFont.Size = 24;
            }
            TitleText->SetFont(TitleFont);
            
            // 设置文字颜色
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);
            TitleText->SetColorAndOpacity(FSlateColor(TextColor));
            
            // 标题占据剩余空间
            if (UHorizontalBoxSlot* TitleSlot = Cast<UHorizontalBoxSlot>(TitleText->Slot))
            {
                TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                TitleSlot->SetVerticalAlignment(VAlign_Center);
            }
        }
        
        // 创建关闭按钮 (右上角 X)
        CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
        if (CloseButton)
        {
            TitleBar->AddChild(CloseButton);
            
            // 设置按钮样式 - 透明背景，悬浮和按下时有效果
            FButtonStyle CloseButtonStyle;
            
            // 正常状态 - 透明
            FSlateBrush NormalBrush;
            NormalBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
            CloseButtonStyle.SetNormal(NormalBrush);
            
            // 获取 DangerColor - 使用 Theme 或 fallback 默认值
            FLinearColor DangerCol = Theme ? Theme->DangerColor : FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);
            
            // 悬浮状态 - 使用 DangerColor 半透明
            FSlateBrush HoverBrush;
            FLinearColor HoverColor = DangerCol;
            HoverColor.A = 0.3f;
            HoverBrush.TintColor = FSlateColor(HoverColor);
            CloseButtonStyle.SetHovered(HoverBrush);
            
            // 按下状态 - 使用 DangerColor 更深
            FSlateBrush PressedBrush;
            FLinearColor PressedColor = DangerCol * 0.7f;
            PressedColor.A = 0.5f;
            PressedBrush.TintColor = FSlateColor(PressedColor);
            CloseButtonStyle.SetPressed(PressedBrush);
            
            CloseButton->SetStyle(CloseButtonStyle);
            
            // 创建 X 文本
            UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
            if (CloseText)
            {
                CloseButton->AddChild(CloseText);
                CloseText->SetText(FText::FromString(TEXT("✕")));
                
                // 使用 Theme 或 fallback 默认值
                FLinearColor CloseTextColor = Theme ? Theme->SecondaryTextColor : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
                CloseText->SetColorAndOpacity(FSlateColor(CloseTextColor));
                
                FSlateFontInfo CloseFont = FCoreStyle::GetDefaultFontStyle("Regular", 18);
                CloseText->SetFont(CloseFont);
            }
            
            // 设置按钮对齐
            if (UHorizontalBoxSlot* CloseSlot = Cast<UHorizontalBoxSlot>(CloseButton->Slot))
            {
                CloseSlot->SetVerticalAlignment(VAlign_Top);
                CloseSlot->SetHorizontalAlignment(HAlign_Right);
            }
        }
    }

    // 创建内容容器 (子类填充)
    ContentContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentContainer"));
    if (ContentContainer && ModalVerticalBox)
    {
        ModalVerticalBox->AddChild(ContentContainer);
        
        // 设置填充剩余空间
        if (UVerticalBoxSlot* ContentSlot = Cast<UVerticalBoxSlot>(ContentContainer->Slot))
        {
            ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        
        // 调用子类方法构建内容区域
        BuildContentArea(ContentContainer);
    }

    // 创建按钮容器
    ButtonContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonContainer"));
    if (ButtonContainer && ModalVerticalBox)
    {
        ModalVerticalBox->AddChild(ButtonContainer);
        
        // 设置顶部间距
        if (UVerticalBoxSlot* ButtonContainerSlot = Cast<UVerticalBoxSlot>(ButtonContainer->Slot))
        {
            float Spacing = Theme ? Theme->ElementSpacing : 12.0f;
            ButtonContainerSlot->SetPadding(FMargin(0.0f, Spacing, 0.0f, 0.0f));
            ButtonContainerSlot->SetHorizontalAlignment(HAlign_Right);
        }
    }

    // 创建按钮间的弹性空间
    USpacer* ButtonSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ButtonSpacer"));
    if (ButtonSpacer && ButtonContainer)
    {
        ButtonContainer->AddChild(ButtonSpacer);
        
        if (UHorizontalBoxSlot* SpacerSlot = Cast<UHorizontalBoxSlot>(ButtonSpacer->Slot))
        {
            SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    // 创建编辑按钮 (可选)
    if (bShowEditButton)
    {
        EditButton = CreateWidget<UMAStyledButton>(this, UMAStyledButton::StaticClass());
        if (EditButton && ButtonContainer)
        {
            ButtonContainer->AddChild(EditButton);
            EditButton->SetButtonText(EditButtonText);
            EditButton->SetButtonStyle(EMAButtonStyle::Secondary);
            
            if (Theme)
            {
                EditButton->ApplyTheme(Theme);
            }
            
            // 设置按钮间距
            if (UHorizontalBoxSlot* EditSlot = Cast<UHorizontalBoxSlot>(EditButton->Slot))
            {
                EditSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
            }
        }
    }

    // 创建拒绝按钮
    RejectButton = CreateWidget<UMAStyledButton>(this, UMAStyledButton::StaticClass());
    if (RejectButton && ButtonContainer)
    {
        ButtonContainer->AddChild(RejectButton);
        RejectButton->SetButtonText(RejectButtonText);
        RejectButton->SetButtonStyle(EMAButtonStyle::Danger);
        
        if (Theme)
        {
            RejectButton->ApplyTheme(Theme);
        }
        
        // 设置按钮间距
        if (UHorizontalBoxSlot* RejectSlot = Cast<UHorizontalBoxSlot>(RejectButton->Slot))
        {
            RejectSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        }
    }

    // 创建确认按钮
    ConfirmButton = CreateWidget<UMAStyledButton>(this, UMAStyledButton::StaticClass());
    if (ConfirmButton && ButtonContainer)
    {
        ButtonContainer->AddChild(ConfirmButton);
        ConfirmButton->SetButtonText(ConfirmButtonText);
        ConfirmButton->SetButtonStyle(EMAButtonStyle::Success);
        
        if (Theme)
        {
            ConfirmButton->ApplyTheme(Theme);
        }
    }

    UE_LOG(LogMABaseModal, Log, TEXT("BuildUI: Modal construction completed"));
}


//=============================================================================
// 模式控制
//=============================================================================

void UMABaseModalWidget::SetEditMode(bool bEditable)
{
    if (bIsEditMode != bEditable)
    {
        bIsEditMode = bEditable;
        
        // 更新按钮可见性
        UpdateButtonVisibility();
        
        // 通知子类
        OnEditModeChanged(bEditable);
        
        UE_LOG(LogMABaseModal, Log, TEXT("Edit mode changed to: %s"), bEditable ? TEXT("true") : TEXT("false"));
    }
}

void UMABaseModalWidget::SetModalType(EMAModalType InModalType)
{
    ModalType = InModalType;
    
    // 更新标题
    if (TitleText)
    {
        TitleText->SetText(GetModalTitleText());
    }
    
    UE_LOG(LogMABaseModal, Log, TEXT("Modal type set to: %d"), static_cast<int32>(InModalType));
}

//=============================================================================
// 动画控制
//=============================================================================

void UMABaseModalWidget::PlayShowAnimation()
{
    UE_LOG(LogMABaseModal, Log, TEXT("Playing show animation (fade in, 200ms)"));
    
    bIsAnimating = true;
    bIsShowing = true;
    CurrentAnimTime = 0.0f;
    TargetOpacity = 1.0f;
    
    // 确保可见
    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(0.0f);
}

void UMABaseModalWidget::PlayHideAnimation()
{
    UE_LOG(LogMABaseModal, Log, TEXT("Playing hide animation (fade out, 200ms)"));
    
    bIsAnimating = true;
    bIsShowing = false;
    CurrentAnimTime = 0.0f;
    TargetOpacity = 0.0f;
}

void UMABaseModalWidget::UpdateAnimation(float DeltaTime)
{
    const FMAModalAnimationStep Step = Coordinator.StepAnimation(CurrentAnimTime, DeltaTime, bIsShowing, AnimationDuration);
    CurrentAnimTime = Step.NextTime;
    SetRenderOpacity(Step.Opacity);

    if (Step.bFinished)
    {
        OnAnimationFinished();
    }
}

void UMABaseModalWidget::OnAnimationFinished()
{
    bIsAnimating = false;
    
    if (bIsShowing)
    {
        // 显示动画完成
        SetRenderOpacity(1.0f);
        OnShowAnimationComplete.Broadcast();
        UE_LOG(LogMABaseModal, Log, TEXT("Show animation completed"));
    }
    else
    {
        // 隐藏动画完成
        SetRenderOpacity(0.0f);
        SetVisibility(ESlateVisibility::Collapsed);
        OnHideAnimationComplete.Broadcast();
        UE_LOG(LogMABaseModal, Log, TEXT("Hide animation completed"));
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMABaseModalWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        UE_LOG(LogMABaseModal, Warning, TEXT("ApplyTheme: Theme is null, using default styles"));
        return;
    }
    
    // 应用遮罩背景颜色
    if (BackgroundOverlay)
    {
        BackgroundOverlay->SetBrushColor(Theme->OverlayColor);
    }
    
    // 应用圆角效果到背景边框
    if (BackgroundBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
            BackgroundBorder, 
            Theme, 
            EMARoundedElementType::Panel
        );
        BackgroundBorder->SetPadding(Theme->ContentPadding);
    }
    
    // 应用标题样式
    if (TitleText)
    {
        TitleText->SetFont(Theme->TitleFont);
        TitleText->SetColorAndOpacity(FSlateColor(Theme->TextColor));
    }
    
    // 应用关闭按钮样式
    if (CloseButton)
    {
        FButtonStyle CloseButtonStyle;
        
        // 正常状态 - 透明
        FSlateBrush NormalBrush;
        NormalBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
        CloseButtonStyle.SetNormal(NormalBrush);
        
        // 悬浮状态 - 使用 DangerColor 半透明
        FSlateBrush HoverBrush;
        FLinearColor HoverColor = Theme->DangerColor;
        HoverColor.A = 0.3f;
        HoverBrush.TintColor = FSlateColor(HoverColor);
        CloseButtonStyle.SetHovered(HoverBrush);
        
        // 按下状态 - 使用 DangerColor 更深
        FSlateBrush PressedBrush;
        FLinearColor PressedColor = Theme->DangerColor * 0.7f;
        PressedColor.A = 0.5f;
        PressedBrush.TintColor = FSlateColor(PressedColor);
        CloseButtonStyle.SetPressed(PressedBrush);
        
        CloseButton->SetStyle(CloseButtonStyle);
        
        // 更新关闭按钮文字颜色
        if (CloseButton->GetChildrenCount() > 0)
        {
            UTextBlock* CloseText = Cast<UTextBlock>(CloseButton->GetChildAt(0));
            if (CloseText)
            {
                CloseText->SetColorAndOpacity(FSlateColor(Theme->SecondaryTextColor));
            }
        }
    }
    
    // 应用按钮主题
    if (ConfirmButton)
    {
        ConfirmButton->ApplyTheme(Theme);
    }
    if (RejectButton)
    {
        RejectButton->ApplyTheme(Theme);
    }
    if (EditButton)
    {
        EditButton->ApplyTheme(Theme);
    }
    
    // 通知子类
    OnThemeApplied();
    
    UE_LOG(LogMABaseModal, Log, TEXT("Theme applied to modal"));
}

//=============================================================================
// 标题设置
//=============================================================================

void UMABaseModalWidget::SetTitle(const FText& InTitle)
{
    ModalTitle = InTitle;
    
    if (TitleText)
    {
        TitleText->SetText(InTitle);
    }
}

FText UMABaseModalWidget::GetModalTitleText() const
{
    // 如果有自定义标题，使用自定义标题
    if (!ModalTitle.IsEmpty())
    {
        return ModalTitle;
    }
    
    // 根据模态类型返回默认标题
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        return FText::FromString(TEXT("Task Graph"));
    case EMAModalType::SkillAllocation:
        return FText::FromString(TEXT("Skill List"));
    default:
        return FText::FromString(TEXT("Modal"));
    }
}

//=============================================================================
// 按钮可见性更新
//=============================================================================

void UMABaseModalWidget::UpdateButtonVisibility()
{
    if (EditButton)
    {
        const FMAModalButtonModel Model = Coordinator.BuildButtonModel(bShowEditButton, bIsEditMode);
        EditButton->SetVisibility(Model.EditVisibility);
    }
    
    // 在编辑模式下，确认按钮文本可能需要改变
    if (ConfirmButton)
    {
        ConfirmButton->SetButtonText(ConfirmButtonText);
    }
}

//=============================================================================
// 按钮回调
//=============================================================================

void UMABaseModalWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMABaseModal, Log, TEXT("Confirm button clicked"));
    
    // 广播确认事件
    OnConfirmClicked.Broadcast();
}

void UMABaseModalWidget::OnRejectButtonClicked()
{
    UE_LOG(LogMABaseModal, Log, TEXT("Reject button clicked"));
    
    // 广播拒绝事件
    OnRejectClicked.Broadcast();
}

void UMABaseModalWidget::OnEditButtonClicked()
{
    UE_LOG(LogMABaseModal, Log, TEXT("Edit button clicked"));
    
    // 广播编辑事件
    OnEditClicked.Broadcast();
}

void UMABaseModalWidget::OnCloseButtonClicked()
{
    UE_LOG(LogMABaseModal, Log, TEXT("Close button clicked"));
    
    // 播放隐藏动画关闭模态窗口
    PlayHideAnimation();
}
