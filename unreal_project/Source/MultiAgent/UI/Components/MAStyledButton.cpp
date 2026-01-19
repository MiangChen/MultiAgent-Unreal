// MAStyledButton.cpp
// 样式化按钮组件实现

#include "MAStyledButton.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAStyledButton, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMAStyledButton::UMAStyledButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ButtonStyle(EMAButtonStyle::Primary)
    , ButtonText(FText::FromString(TEXT("Button")))
    , Theme(nullptr)
    , RootSizeBox(nullptr)
    , ButtonBorder(nullptr)
    , InternalButton(nullptr)
    , ButtonLabel(nullptr)
    , bIsEnabled(true)
    , bIsHovered(false)
    , bIsPressed(false)
    , OriginalPositionOffset(FVector2D::ZeroVector)
    , OriginalScale(FVector2D(1.0f, 1.0f))
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    // 初始化动画状态
    CurrentAnimState.PositionOffset = FVector2D::ZeroVector;
    CurrentAnimState.Scale = FVector2D(1.0f, 1.0f);
    CurrentAnimState.TintColor = FLinearColor::White;
    CurrentAnimState.ShadowOpacity = NormalShadowOpacity;

    TargetAnimState = CurrentAnimState;
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAStyledButton::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAStyledButton::NativeConstruct()
{
    Super::NativeConstruct();

    // 绑定按钮事件
    if (InternalButton)
    {
        InternalButton->OnClicked.AddDynamic(this, &UMAStyledButton::OnButtonClicked);
        InternalButton->OnHovered.AddDynamic(this, &UMAStyledButton::OnButtonHovered);
        InternalButton->OnUnhovered.AddDynamic(this, &UMAStyledButton::OnButtonUnhovered);
        InternalButton->OnPressed.AddDynamic(this, &UMAStyledButton::OnButtonPressed);
        InternalButton->OnReleased.AddDynamic(this, &UMAStyledButton::OnButtonReleased);
    }

    // 初始化颜色
    UpdateButtonColors();
}

void UMAStyledButton::NativeDestruct()
{
    // 解绑按钮事件
    if (InternalButton)
    {
        InternalButton->OnClicked.RemoveDynamic(this, &UMAStyledButton::OnButtonClicked);
        InternalButton->OnHovered.RemoveDynamic(this, &UMAStyledButton::OnButtonHovered);
        InternalButton->OnUnhovered.RemoveDynamic(this, &UMAStyledButton::OnButtonUnhovered);
        InternalButton->OnPressed.RemoveDynamic(this, &UMAStyledButton::OnButtonPressed);
        InternalButton->OnReleased.RemoveDynamic(this, &UMAStyledButton::OnButtonReleased);
    }

    Super::NativeDestruct();
}

void UMAStyledButton::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 更新动画状态
    UpdateAnimationState(InDeltaTime);
}

TSharedRef<SWidget> UMAStyledButton::RebuildWidget()
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

void UMAStyledButton::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAStyledButton, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMAStyledButton, Log, TEXT("BuildUI: Starting button construction..."));

    // 创建根 SizeBox
    RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
    if (!RootSizeBox)
    {
        UE_LOG(LogMAStyledButton, Error, TEXT("BuildUI: Failed to create RootSizeBox!"));
        return;
    }
    WidgetTree->RootWidget = RootSizeBox;

    // 设置最小尺寸
    RootSizeBox->SetMinDesiredWidth(100.0f);
    RootSizeBox->SetMinDesiredHeight(36.0f);

    // 创建边框 (用于圆角背景)
    ButtonBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ButtonBorder"));
    if (!ButtonBorder)
    {
        UE_LOG(LogMAStyledButton, Error, TEXT("BuildUI: Failed to create ButtonBorder!"));
        return;
    }
    RootSizeBox->AddChild(ButtonBorder);

    // 设置边框样式
    ButtonBorder->SetPadding(FMargin(16.0f, 8.0f));
    ButtonBorder->SetHorizontalAlignment(HAlign_Center);
    ButtonBorder->SetVerticalAlignment(VAlign_Center);

    // 创建内部按钮
    InternalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("InternalButton"));
    if (!InternalButton)
    {
        UE_LOG(LogMAStyledButton, Error, TEXT("BuildUI: Failed to create InternalButton!"));
        return;
    }
    ButtonBorder->AddChild(InternalButton);

    // 设置按钮样式为透明 (使用边框作为背景)
    FButtonStyle TransparentStyle;
    TransparentStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
    TransparentStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
    TransparentStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
    TransparentStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
    InternalButton->SetStyle(TransparentStyle);

    // 创建按钮文本
    ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonLabel"));
    if (!ButtonLabel)
    {
        UE_LOG(LogMAStyledButton, Error, TEXT("BuildUI: Failed to create ButtonLabel!"));
        return;
    }
    InternalButton->AddChild(ButtonLabel);

    // 设置文本
    ButtonLabel->SetText(ButtonText);
    ButtonLabel->SetJustification(ETextJustify::Center);

    // 设置字体 - 使用加粗字体
    FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    ButtonLabel->SetFont(ButtonFont);

    // 初始化颜色和圆角
    UpdateButtonColors();
    
    // 应用圆角效果 (Requirements: 5.1)
    ApplyRoundedCornersToButton();

    UE_LOG(LogMAStyledButton, Log, TEXT("BuildUI: Button construction completed"));
}

//=============================================================================
// 属性设置
//=============================================================================

void UMAStyledButton::SetButtonStyle(EMAButtonStyle InStyle)
{
    if (ButtonStyle != InStyle)
    {
        ButtonStyle = InStyle;
        UpdateButtonColors();
        UE_LOG(LogMAStyledButton, Verbose, TEXT("Button style changed to: %d"), static_cast<int32>(InStyle));
    }
}

void UMAStyledButton::SetButtonText(const FText& InText)
{
    ButtonText = InText;
    if (ButtonLabel)
    {
        ButtonLabel->SetText(ButtonText);
    }
}

void UMAStyledButton::SetButtonEnabled(bool bEnabled)
{
    bIsEnabled = bEnabled;
    if (InternalButton)
    {
        InternalButton->SetIsEnabled(bEnabled);
    }

    // 更新视觉状态
    if (ButtonBorder)
    {
        float Opacity = bEnabled ? 1.0f : 0.5f;
        ButtonBorder->SetRenderOpacity(Opacity);
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMAStyledButton::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;

    if (!Theme)
    {
        UE_LOG(LogMAStyledButton, Warning, TEXT("ApplyTheme: Theme is null, using default colors"));
        CurrentCornerRadius = MARoundedBorderUtils::DefaultButtonCornerRadius;
        UpdateButtonColors();
        ApplyRoundedCornersToButton();
        return;
    }

    // 更新圆角半径 (Requirements: 5.2)
    CurrentCornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Button);

    // 应用字体
    if (ButtonLabel)
    {
        ButtonLabel->SetFont(Theme->ButtonFont);
        ButtonLabel->SetColorAndOpacity(FSlateColor(Theme->TextColor));
    }

    // 更新颜色
    UpdateButtonColors();
    
    // 应用圆角效果 (Requirements: 5.2)
    ApplyRoundedCornersToButton();

    UE_LOG(LogMAStyledButton, Log, TEXT("Theme applied to button with corner radius: %f"), CurrentCornerRadius);
}

//=============================================================================
// 颜色更新
//=============================================================================

void UMAStyledButton::UpdateButtonColors()
{
    BaseColor = GetBaseColorForStyle(ButtonStyle);
    HoverColor = GetHoverColorForStyle(ButtonStyle);

    // 应用基础颜色到边框 - 使用圆角画刷 (Requirements: 5.1)
    if (ButtonBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(ButtonBorder, BaseColor, CurrentCornerRadius);
    }

    // 应用文本颜色
    if (ButtonLabel)
    {
        FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);
        ButtonLabel->SetColorAndOpacity(FSlateColor(TextColor));
    }
}

FLinearColor UMAStyledButton::GetBaseColorForStyle(EMAButtonStyle Style) const
{
    if (Theme)
    {
        switch (Style)
        {
        case EMAButtonStyle::Primary:
            return Theme->PrimaryColor;
        case EMAButtonStyle::Secondary:
            return Theme->SecondaryColor;
        case EMAButtonStyle::Danger:
            return Theme->DangerColor;
        case EMAButtonStyle::Success:
            return Theme->SuccessColor;
        case EMAButtonStyle::Warning:
            return Theme->WarningColor;
        default:
            return Theme->PrimaryColor;
        }
    }

    // 默认颜色 (无主题时)
    switch (Style)
    {
    case EMAButtonStyle::Primary:
        return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);    // 蓝色
    case EMAButtonStyle::Secondary:
        return FLinearColor(0.3f, 0.3f, 0.35f, 1.0f);   // 灰色
    case EMAButtonStyle::Danger:
        return FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);    // 红色
    case EMAButtonStyle::Success:
        return FLinearColor(0.3f, 0.8f, 0.4f, 1.0f);    // 绿色
    case EMAButtonStyle::Warning:
        return FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);    // 黄色
    default:
        return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
    }
}

FLinearColor UMAStyledButton::GetHoverColorForStyle(EMAButtonStyle Style) const
{
    // 悬停颜色 = 基础颜色变亮
    FLinearColor Base = GetBaseColorForStyle(Style);
    
    // 增加亮度
    float BrightnessMultiplier = 1.2f;
    return FLinearColor(
        FMath::Min(Base.R * BrightnessMultiplier, 1.0f),
        FMath::Min(Base.G * BrightnessMultiplier, 1.0f),
        FMath::Min(Base.B * BrightnessMultiplier, 1.0f),
        Base.A
    );
}

//=============================================================================
// 圆角应用 (Requirements: 5.1, 5.2, 5.3)
//=============================================================================

void UMAStyledButton::ApplyRoundedCornersToButton()
{
    if (!ButtonBorder)
    {
        return;
    }
    
    // 使用 MARoundedBorderUtils 应用圆角效果
    // 这确保了悬停和按下状态都保持圆角
    MARoundedBorderUtils::ApplyRoundedCorners(ButtonBorder, BaseColor, CurrentCornerRadius);
    
    UE_LOG(LogMAStyledButton, Verbose, TEXT("Applied rounded corners with radius: %f"), CurrentCornerRadius);
}


//=============================================================================
// 动画更新
//=============================================================================

void UMAStyledButton::UpdateAnimationState(float DeltaTime)
{
    // 使用插值平滑过渡到目标状态
    float InterpAlpha = FMath::Clamp(DeltaTime * AnimationInterpSpeed, 0.0f, 1.0f);

    // 插值位置偏移
    CurrentAnimState.PositionOffset = FMath::Lerp(
        CurrentAnimState.PositionOffset,
        TargetAnimState.PositionOffset,
        InterpAlpha
    );

    // 插值缩放
    CurrentAnimState.Scale = FMath::Lerp(
        CurrentAnimState.Scale,
        TargetAnimState.Scale,
        InterpAlpha
    );

    // 插值颜色
    CurrentAnimState.TintColor = FMath::Lerp(
        CurrentAnimState.TintColor,
        TargetAnimState.TintColor,
        InterpAlpha
    );

    // 插值阴影透明度
    CurrentAnimState.ShadowOpacity = FMath::Lerp(
        CurrentAnimState.ShadowOpacity,
        TargetAnimState.ShadowOpacity,
        InterpAlpha
    );

    // 应用动画状态到 UI
    ApplyAnimationState();
}

void UMAStyledButton::ApplyAnimationState()
{
    if (!RootSizeBox)
    {
        return;
    }

    // 应用缩放
    RootSizeBox->SetRenderScale(CurrentAnimState.Scale);

    // 应用位置偏移 (通过 RenderTransform)
    FWidgetTransform Transform;
    Transform.Translation = CurrentAnimState.PositionOffset;
    Transform.Scale = FVector2D(1.0f, 1.0f);  // 缩放已通过 SetRenderScale 应用
    RootSizeBox->SetRenderTransform(Transform);

    // 应用颜色色调到边框 - 使用圆角画刷保持圆角效果 (Requirements: 5.3)
    if (ButtonBorder)
    {
        // 混合基础颜色和色调
        FLinearColor FinalColor;
        if (bIsHovered && !bIsPressed)
        {
            FinalColor = HoverColor;
        }
        else
        {
            FinalColor = BaseColor;
        }
        
        // 应用色调
        FinalColor = FinalColor * CurrentAnimState.TintColor;
        
        // 使用 MARoundedBorderUtils 应用圆角，确保悬停和按下状态保持圆角 (Requirements: 5.3)
        MARoundedBorderUtils::ApplyRoundedCorners(ButtonBorder, FinalColor, CurrentCornerRadius);
    }

    // 应用阴影效果 (通过边框的描边或阴影)
    // 注意: UMG 的 Border 不直接支持阴影，这里通过调整透明度模拟
    if (ButtonBorder)
    {
        // 可以通过添加额外的阴影层来实现，这里简化处理
        // 阴影效果主要通过颜色变化来体现
    }
}

//=============================================================================
// 事件回调 - 微交互动画 (Requirements: 9.1, 9.2, 9.3, 9.4, 9.5)
//=============================================================================

void UMAStyledButton::OnButtonClicked()
{
    if (!bIsEnabled)
    {
        return;
    }

    UE_LOG(LogMAStyledButton, Verbose, TEXT("Button clicked"));

    // 广播点击事件
    OnClicked.Broadcast();
}

void UMAStyledButton::OnButtonHovered()
{
    if (!bIsEnabled)
    {
        return;
    }

    bIsHovered = true;

    UE_LOG(LogMAStyledButton, Verbose, TEXT("Button hovered"));

    // 设置目标动画状态：
    // - 颜色变化 (通过 HoverColor)
    // - 向上平移 2 像素
    // - 阴影加深
    TargetAnimState.PositionOffset = FVector2D(0.0f, HoverTranslateY);
    TargetAnimState.TintColor = FLinearColor::White;  // 使用 HoverColor
    TargetAnimState.ShadowOpacity = HoverShadowOpacity;

    // 如果没有按下，保持正常缩放
    if (!bIsPressed)
    {
        TargetAnimState.Scale = FVector2D(1.0f, 1.0f);
    }
}

void UMAStyledButton::OnButtonUnhovered()
{
    bIsHovered = false;

    UE_LOG(LogMAStyledButton, Verbose, TEXT("Button unhovered"));

    // 恢复原始状态
    TargetAnimState.PositionOffset = FVector2D::ZeroVector;
    TargetAnimState.TintColor = FLinearColor::White;
    TargetAnimState.ShadowOpacity = NormalShadowOpacity;

    // 如果没有按下，恢复正常缩放
    if (!bIsPressed)
    {
        TargetAnimState.Scale = FVector2D(1.0f, 1.0f);
    }
}

void UMAStyledButton::OnButtonPressed()
{
    if (!bIsEnabled)
    {
        return;
    }

    bIsPressed = true;

    UE_LOG(LogMAStyledButton, Verbose, TEXT("Button pressed"));

    // 设置目标动画状态：缩放到 0.98
    TargetAnimState.Scale = FVector2D(PressedScale, PressedScale);
}

void UMAStyledButton::OnButtonReleased()
{
    bIsPressed = false;

    UE_LOG(LogMAStyledButton, Verbose, TEXT("Button released"));

    // 恢复原始缩放
    TargetAnimState.Scale = FVector2D(1.0f, 1.0f);

    // 如果仍然悬停，保持悬停状态
    if (bIsHovered)
    {
        TargetAnimState.PositionOffset = FVector2D(0.0f, HoverTranslateY);
        TargetAnimState.ShadowOpacity = HoverShadowOpacity;
    }
    else
    {
        TargetAnimState.PositionOffset = FVector2D::ZeroVector;
        TargetAnimState.ShadowOpacity = NormalShadowOpacity;
    }
}
