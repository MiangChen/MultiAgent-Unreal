// MAInstructionPanel.cpp
// 指令输入面板 Widget 实现

#include "MAInstructionPanel.h"
#include "../Core/MAUITheme.h"
#include "../Core/MAUIManager.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAFrostedGlassUtils.h"
#include "MAStyledButton.h"
#include "../../UI/HUD/MAHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/BackgroundBlur.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateTypes.h"

//=============================================================================
// 日志类别定义
//=============================================================================
DEFINE_LOG_CATEGORY(LogMAInstructionPanel);

//=============================================================================
// 主题颜色辅助函数
//=============================================================================

namespace MAInstructionPanelPrivate
{
    /** 获取主题或创建默认主题 */
    UMAUITheme* GetOrCreateTheme(UMAInstructionPanel* Widget)
    {
        UMAUITheme* CurrentTheme = Widget->GetTheme();
        if (CurrentTheme)
        {
            return CurrentTheme;
        }
        
        // 创建默认主题作为 fallback
        static UMAUITheme* DefaultTheme = nullptr;
        if (!DefaultTheme)
        {
            DefaultTheme = NewObject<UMAUITheme>();
            DefaultTheme->AddToRoot(); // 防止被 GC
        }
        return DefaultTheme;
    }
    
    /** 计算区域背景颜色 (比主背景稍亮) */
    FLinearColor GetSectionBackgroundColor(UMAUITheme* Theme)
    {
        FLinearColor SectionBgColor = Theme->BackgroundColor;
        SectionBgColor.R += 0.05f;
        SectionBgColor.G += 0.05f;
        SectionBgColor.B += 0.05f;
        return SectionBgColor;
    }
}

//=============================================================================
// 构造函数
//=============================================================================

UMAInstructionPanel::UMAInstructionPanel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAInstructionPanel::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMAInstructionPanel::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定提交按钮点击事件
    if (SubmitButton)
    {
        SubmitButton->OnClicked.AddDynamic(this, &UMAInstructionPanel::OnSubmitClicked);
    }
}

TSharedRef<SWidget> UMAInstructionPanel::RebuildWidget()
{
    BuildUI();
    
    // 使用 Super::RebuildWidget() 让 UMG 正确处理 Widget 树
    return Super::RebuildWidget();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMAInstructionPanel::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAInstructionPanel, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }
    
    // 获取主题 (确保有可用的主题)
    UMAUITheme* CurrentTheme = MAInstructionPanelPrivate::GetOrCreateTheme(this);
    
    FLinearColor SectionBgColor = MAInstructionPanelPrivate::GetSectionBackgroundColor(CurrentTheme);
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Panel);
    float ButtonCornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Button);
    
    // 创建根 CanvasPanel - 用于定位面板
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAInstructionPanel, Error, TEXT("BuildUI: Failed to create RootCanvas"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;
    
    // 使用 MAFrostedGlassUtils 创建毛玻璃效果面板
    // 位置：右下角，距离右边 10px，距离底部 10px
    // 尺寸：宽度 480px，高度 200px
    FMAFrostedGlassResult FrostedGlass = MAFrostedGlassUtils::CreateFixedSizePanelFromTheme(
        WidgetTree,
        RootCanvas,
        CurrentTheme,
        FVector2D(-10, -10),           // 位置：右边距 10，底部距离 10
        FVector2D(PanelWidth, PanelHeight),  // 尺寸：宽度 480，高度 200
        FVector2D(1.0f, 1.0f),         // 对齐：右下角对齐
        FAnchors(1.0f, 1.0f, 1.0f, 1.0f),  // 锚点：右下角
        TEXT("InstructionPanel")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMAInstructionPanel, Error, TEXT("BuildUI: Failed to create frosted glass panel"));
        return;
    }
    
    // 保存内容容器引用
    PanelBackground = FrostedGlass.ContentContainer;
    
    // 创建命令区边框
    CommandSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CommandSectionBorder"));
    if (!CommandSectionBorder)
    {
        return;
    }
    CommandSectionBorder->SetPadding(FMargin(8.0f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(CommandSectionBorder, SectionBgColor, CornerRadius);
    
    // 创建垂直布局
    PanelContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelContainer"));
    if (!PanelContainer)
    {
        return;
    }
    
    // 创建标题
    CommandTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CommandTitle"));
    if (CommandTitle)
    {
        CommandTitle->SetText(FText::FromString(TEXT("Instruction")));
        CommandTitle->SetColorAndOpacity(FSlateColor(CurrentTheme->TextColor));
        
        FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
        CommandTitle->SetFont(TitleFont);
        
        UVerticalBoxSlot* TitleSlot = PanelContainer->AddChildToVerticalBox(CommandTitle);
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
    
    // 创建多行命令输入框
    CommandInput = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("CommandInput"));
    if (CommandInput)
    {
        CommandInput->SetHintText(FText::FromString(TEXT("Enter command...")));
        
        // 设置文本样式：使用主题颜色
        FEditableTextBoxStyle TextBoxStyle;
        FSlateColor InputColor = FSlateColor(CurrentTheme->InputTextColor);
        TextBoxStyle.SetForegroundColor(InputColor);
        TextBoxStyle.SetFocusedForegroundColor(InputColor);
        FSlateFontInfo InputFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
        TextBoxStyle.SetFont(InputFont);
        
        // 设置透明背景，让外层圆角 Border 的背景显示出来
        FSlateBrush TransparentBrush;
        TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);
        TextBoxStyle.SetBackgroundImageNormal(TransparentBrush);
        TextBoxStyle.SetBackgroundImageHovered(TransparentBrush);
        TextBoxStyle.SetBackgroundImageFocused(TransparentBrush);
        TextBoxStyle.SetBackgroundImageReadOnly(TransparentBrush);
        
        CommandInput->WidgetStyle = TextBoxStyle;
        
        // 创建圆角 Border 包装文本框
        CommandInputBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CommandInputBorder"));
        CommandInputBorder->SetPadding(FMargin(8.0f, 4.0f));
        
        // 应用圆角效果 - 使用主题输入框背景颜色
        MARoundedBorderUtils::ApplyRoundedCorners(CommandInputBorder, CurrentTheme->InputBackgroundColor, ButtonCornerRadius);
        
        CommandInputBorder->AddChild(CommandInput);
        
        // 添加到垂直布局，填充剩余空间
        UVerticalBoxSlot* InputSlot = PanelContainer->AddChildToVerticalBox(CommandInputBorder);
        InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        InputSlot->SetHorizontalAlignment(HAlign_Fill);
        InputSlot->SetVerticalAlignment(VAlign_Fill);
        InputSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
    
    // 创建提交按钮 (底部)
    SubmitButton = WidgetTree->ConstructWidget<UMAStyledButton>(UMAStyledButton::StaticClass(), TEXT("SubmitButton"));
    if (SubmitButton)
    {
        SubmitButton->SetButtonText(FText::FromString(TEXT("Send")));
        SubmitButton->SetButtonStyle(EMAButtonStyle::Primary);
        
        UVerticalBoxSlot* ButtonSlot = PanelContainer->AddChildToVerticalBox(SubmitButton);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetHorizontalAlignment(HAlign_Right);
    }
    
    CommandSectionBorder->SetContent(PanelContainer);
    
    // 组装层级
    if (PanelBackground)
    {
        PanelBackground->AddChild(CommandSectionBorder);
    }
}

//=============================================================================
// 命令输入方法
//=============================================================================

FString UMAInstructionPanel::GetCommandText() const
{
    if (CommandInput)
    {
        return CommandInput->GetText().ToString();
    }
    return FString();
}

void UMAInstructionPanel::SetCommandText(const FString& Text)
{
    if (CommandInput)
    {
        CommandInput->SetText(FText::FromString(Text));
    }
}

void UMAInstructionPanel::ClearCommandInput()
{
    if (CommandInput)
    {
        CommandInput->SetText(FText::GetEmpty());
    }
}

void UMAInstructionPanel::FocusCommandInput()
{
    if (CommandInput)
    {
        CommandInput->SetKeyboardFocus();
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMAInstructionPanel::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        return;
    }
    
    // 应用背景颜色
    if (PanelBackground)
    {
        PanelBackground->SetBrushColor(Theme->BackgroundColor);
    }
    
    // 应用区域背景颜色 (稍微亮一点) 并应用圆角
    FLinearColor SectionBgColor = Theme->BackgroundColor;
    SectionBgColor.R += 0.05f;
    SectionBgColor.G += 0.05f;
    SectionBgColor.B += 0.05f;
    
    // 获取圆角半径
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Panel);
    float ButtonCornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Button);
    
    if (CommandSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(CommandSectionBorder, SectionBgColor, CornerRadius);
    }
    
    // 应用标题颜色 (保持字号不变，只更新颜色)
    FSlateFontInfo SectionTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    if (CommandTitle)
    {
        CommandTitle->SetColorAndOpacity(FSlateColor(Theme->TextColor));
        CommandTitle->SetFont(SectionTitleFont);
    }
    
    // 应用命令输入框颜色
    if (CommandInput)
    {
        // 更新输入框文字颜色
        FEditableTextBoxStyle TextBoxStyle = CommandInput->WidgetStyle;
        FSlateColor InputColor = FSlateColor(Theme->InputTextColor);
        TextBoxStyle.SetForegroundColor(InputColor);
        TextBoxStyle.SetFocusedForegroundColor(InputColor);
        CommandInput->WidgetStyle = TextBoxStyle;
    }
    if (CommandInputBorder)
    {
        // 更新输入框背景颜色
        MARoundedBorderUtils::ApplyRoundedCorners(CommandInputBorder, Theme->InputBackgroundColor, ButtonCornerRadius);
    }
    
    // 应用主题到子组件
    if (SubmitButton)
    {
        SubmitButton->ApplyTheme(Theme);
    }
}

//=============================================================================
// 事件回调
//=============================================================================

void UMAInstructionPanel::OnSubmitClicked()
{
    FString Command = GetCommandText();
    if (!Command.IsEmpty())
    {
        // 广播命令提交事件
        OnCommandSubmitted.Broadcast(Command);
        
        // 清空输入框
        ClearCommandInput();
        
        UE_LOG(LogMAInstructionPanel, Log, TEXT("Command submitted: %s"), *Command);
        
        // 通知 UIManager 关闭索要指令通知
        APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
        if (PC)
        {
            if (AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD()))
            {
                if (UMAUIManager* UIManager = HUD->GetUIManager())
                {
                    UIManager->DismissRequestUserCommandNotification();
                }
            }
        }
    }
}
