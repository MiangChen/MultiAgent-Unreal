// MARightSidebarWidget.cpp
// 右侧边栏 Widget 实现

#include "MARightSidebarWidget.h"
#include "../Core/MAUITheme.h"
#include "../Core/MAUIManager.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAFrostedGlassUtils.h"
#include "MAStyledButton.h"
#include "MATaskGraphPreview.h"
#include "MASkillListPreview.h"
#include "../../UI/HUD/MAHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/BackgroundBlur.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateTypes.h"

//=============================================================================
// 主题颜色辅助函数
//=============================================================================

namespace
{
    /** 获取主题或创建默认主题 */
    UMAUITheme* GetOrCreateTheme(UMARightSidebarWidget* Widget)
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
// 日志类别
//=============================================================================
DEFINE_LOG_CATEGORY_STATIC(LogMARightSidebar, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMARightSidebarWidget::UMARightSidebarWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMARightSidebarWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMARightSidebarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定提交按钮点击事件
    if (SubmitButton)
    {
        SubmitButton->OnClicked.AddDynamic(this, &UMARightSidebarWidget::OnSubmitClicked);
    }
}

TSharedRef<SWidget> UMARightSidebarWidget::RebuildWidget()
{
    BuildUI();
    
    // 使用 Super::RebuildWidget() 让 UMG 正确处理 Widget 树
    return Super::RebuildWidget();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMARightSidebarWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMARightSidebar, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }
    
    // 获取主题 (确保有可用的主题)
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    
    // 获取毛玻璃配置
    FMAFrostedGlassConfig GlassConfig = MAFrostedGlassUtils::GetConfigFromTheme(CurrentTheme);
    
    // 创建根 SizeBox - 控制侧边栏宽度
    RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
    if (!RootSizeBox)
    {
        UE_LOG(LogMARightSidebar, Error, TEXT("BuildUI: Failed to create RootSizeBox"));
        return;
    }
    RootSizeBox->SetWidthOverride(SidebarWidth);
    WidgetTree->RootWidget = RootSizeBox;
    
    // === 创建毛玻璃效果层级 ===
    
    // 1. 玻璃边框层
    UBorder* GlassBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightSidebar_GlassBorder"));
    if (GlassBorder)
    {
        FLinearColor GlassBorderColor(1.0f, 1.0f, 1.0f, GlassConfig.GlassBorderOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(GlassBorder, GlassBorderColor, GlassConfig.CornerRadius);
        // 增加 padding 以确保 BackgroundBlur 的矩形边缘被圆角边框覆盖
        float EffectivePadding = FMath::Max(GlassConfig.GlassBorderThickness, GlassConfig.CornerRadius * 0.3f);
        GlassBorder->SetPadding(FMargin(EffectivePadding));
        // 启用裁剪，确保内容不会超出圆角边框
        GlassBorder->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        RootSizeBox->AddChild(GlassBorder);
    }
    
    // 2. 模糊层
    UBackgroundBlur* BlurBackground = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("RightSidebar_Blur"));
    if (BlurBackground && GlassBorder)
    {
        BlurBackground->SetBlurStrength(GlassConfig.BlurStrength);
        BlurBackground->SetApplyAlphaToBlur(true);
        // 设置低质量回退画刷为透明，避免黑色背景
        FSlateBrush TransparentFallbackBrush;
        TransparentFallbackBrush.TintColor = FSlateColor(FLinearColor::Transparent);
        BlurBackground->SetLowQualityFallbackBrush(TransparentFallbackBrush);
        // 增加模糊层的 padding，使其边缘更靠内，避免超出圆角边框
        float BlurPadding = 8.0f + GlassConfig.CornerRadius * 0.15f;
        BlurBackground->SetPadding(FMargin(BlurPadding));
        GlassBorder->AddChild(BlurBackground);
    }
    
    // 3. 光泽层作为内容容器
    SidebarBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightSidebar_Gloss"));
    if (SidebarBackground && BlurBackground)
    {
        FLinearColor GlossColor(1.0f, 1.0f, 1.0f, GlassConfig.GlossOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(SidebarBackground, GlossColor, GlassConfig.CornerRadius - 2.0f);
        SidebarBackground->SetPadding(FMargin(0.0f));
        BlurBackground->AddChild(SidebarBackground);
    }
    
    // 创建边栏容器 - 使用 WidgetTree 创建
    SidebarContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SidebarContainer"));
    if (!SidebarContainer)
    {
        UE_LOG(LogMARightSidebar, Error, TEXT("BuildUI: Failed to create SidebarContainer"));
        return;
    }
    
    // 添加系统日志区 (最上面，固定高度)
    UWidget* LogSection = CreateLogSection();
    if (LogSection)
    {
        UVerticalBoxSlot* LogSlot = SidebarContainer->AddChildToVerticalBox(LogSection);
        LogSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
        LogSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    // 添加分隔线
    UWidget* Separator1 = CreateSeparator();
    if (Separator1)
    {
        UVerticalBoxSlot* SepSlot = SidebarContainer->AddChildToVerticalBox(Separator1);
        SepSlot->SetPadding(FMargin(0.0f, 4.0f));
    }
    
    // 添加任务图预览区
    UWidget* TaskGraphSection = CreateTaskGraphSection();
    if (TaskGraphSection)
    {
        UVerticalBoxSlot* TaskSlot = SidebarContainer->AddChildToVerticalBox(TaskGraphSection);
        TaskSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));
        TaskSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    // 添加分隔线
    UWidget* Separator2 = CreateSeparator();
    if (Separator2)
    {
        UVerticalBoxSlot* SepSlot = SidebarContainer->AddChildToVerticalBox(Separator2);
        SepSlot->SetPadding(FMargin(0.0f, 4.0f));
    }
    
    // 添加技能列表预览区
    UWidget* SkillListSection = CreateSkillListSection();
    if (SkillListSection)
    {
        UVerticalBoxSlot* SkillSlot = SidebarContainer->AddChildToVerticalBox(SkillListSection);
        SkillSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));
        SkillSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    // 添加分隔线
    UWidget* Separator3 = CreateSeparator();
    if (Separator3)
    {
        UVerticalBoxSlot* SepSlot = SidebarContainer->AddChildToVerticalBox(Separator3);
        SepSlot->SetPadding(FMargin(0.0f, 4.0f));
    }
    
    // 添加命令输入区 (填充剩余空间)
    UWidget* CommandSection = CreateCommandSection();
    if (CommandSection)
    {
        UVerticalBoxSlot* CommandSlot = SidebarContainer->AddChildToVerticalBox(CommandSection);
        CommandSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
        CommandSlot->SetHorizontalAlignment(HAlign_Fill);
        CommandSlot->SetVerticalAlignment(VAlign_Fill);
        CommandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    
    // 组装层级
    SidebarBackground->AddChild(SidebarContainer);
}

UWidget* UMARightSidebarWidget::CreateCommandSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    FLinearColor SectionBgColor = GetSectionBackgroundColor(CurrentTheme);
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Panel);
    float ButtonCornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Button);
    
    // 创建命令区边框
    CommandSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CommandSectionBorder"));
    if (!CommandSectionBorder)
    {
        return nullptr;
    }
    CommandSectionBorder->SetPadding(FMargin(8.0f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(CommandSectionBorder, SectionBgColor, CornerRadius);
    
    // 创建垂直布局
    UVerticalBox* CommandVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CommandVBox"));
    if (!CommandVBox)
    {
        return nullptr;
    }
    
    // 创建标题
    CommandTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CommandTitle"));
    if (CommandTitle)
    {
        CommandTitle->SetText(FText::FromString(TEXT("Command Input")));
        CommandTitle->SetColorAndOpacity(FSlateColor(CurrentTheme->TextColor));
        
        FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
        CommandTitle->SetFont(TitleFont);
        
        UVerticalBoxSlot* TitleSlot = CommandVBox->AddChildToVerticalBox(CommandTitle);
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
        UVerticalBoxSlot* InputSlot = CommandVBox->AddChildToVerticalBox(CommandInputBorder);
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
        
        UVerticalBoxSlot* ButtonSlot = CommandVBox->AddChildToVerticalBox(SubmitButton);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetHorizontalAlignment(HAlign_Right);
    }
    
    CommandSectionBorder->SetContent(CommandVBox);
    return CommandSectionBorder;
}

UWidget* UMARightSidebarWidget::CreateTaskGraphSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    FLinearColor SectionBgColor = GetSectionBackgroundColor(CurrentTheme);
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Panel);
    
    // 创建任务图区边框
    TaskGraphSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TaskGraphSectionBorder"));
    if (!TaskGraphSectionBorder)
    {
        return nullptr;
    }
    TaskGraphSectionBorder->SetPadding(FMargin(8.0f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(TaskGraphSectionBorder, SectionBgColor, CornerRadius);
    
    // 创建垂直布局
    UVerticalBox* TaskVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TaskVBox"));
    if (!TaskVBox)
    {
        return nullptr;
    }
    
    // 创建标题
    TaskGraphTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TaskGraphTitle"));
    if (TaskGraphTitle)
    {
        TaskGraphTitle->SetText(FText::FromString(TEXT("Task Graph Preview")));
        TaskGraphTitle->SetColorAndOpacity(FSlateColor(CurrentTheme->TextColor));
        
        FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
        TaskGraphTitle->SetFont(TitleFont);
        
        UVerticalBoxSlot* TitleSlot = TaskVBox->AddChildToVerticalBox(TaskGraphTitle);
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
    
    // 创建任务图预览组件
    TaskGraphPreview = WidgetTree->ConstructWidget<UMATaskGraphPreview>(UMATaskGraphPreview::StaticClass(), TEXT("TaskGraphPreview"));
    if (TaskGraphPreview)
    {
        UVerticalBoxSlot* PreviewSlot = TaskVBox->AddChildToVerticalBox(TaskGraphPreview);
        PreviewSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    TaskGraphSectionBorder->SetContent(TaskVBox);
    return TaskGraphSectionBorder;
}

UWidget* UMARightSidebarWidget::CreateSkillListSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    FLinearColor SectionBgColor = GetSectionBackgroundColor(CurrentTheme);
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Panel);
    
    // 创建技能列表区边框
    SkillListSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SkillListSectionBorder"));
    if (!SkillListSectionBorder)
    {
        return nullptr;
    }
    SkillListSectionBorder->SetPadding(FMargin(8.0f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(SkillListSectionBorder, SectionBgColor, CornerRadius);
    
    // 创建垂直布局
    UVerticalBox* SkillVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SkillVBox"));
    if (!SkillVBox)
    {
        return nullptr;
    }
    
    // 创建标题
    SkillListTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillListTitle"));
    if (SkillListTitle)
    {
        SkillListTitle->SetText(FText::FromString(TEXT("Skill List Preview")));
        SkillListTitle->SetColorAndOpacity(FSlateColor(CurrentTheme->TextColor));
        
        FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
        SkillListTitle->SetFont(TitleFont);
        
        UVerticalBoxSlot* TitleSlot = SkillVBox->AddChildToVerticalBox(SkillListTitle);
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
    
    // 创建技能列表预览组件
    SkillListPreview = WidgetTree->ConstructWidget<UMASkillListPreview>(UMASkillListPreview::StaticClass(), TEXT("SkillListPreview"));
    if (SkillListPreview)
    {
        UVerticalBoxSlot* PreviewSlot = SkillVBox->AddChildToVerticalBox(SkillListPreview);
        PreviewSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    SkillListSectionBorder->SetContent(SkillVBox);
    return SkillListSectionBorder;
}


UWidget* UMARightSidebarWidget::CreateLogSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    FLinearColor SectionBgColor = GetSectionBackgroundColor(CurrentTheme);
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Panel);
    
    // 创建外层 SizeBox 限制高度
    USizeBox* LogSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LogSizeBox"));
    if (!LogSizeBox)
    {
        return nullptr;
    }
    LogSizeBox->SetHeightOverride(LogSectionHeight);
    
    // 创建日志区边框
    LogSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LogSectionBorder"));
    if (!LogSectionBorder)
    {
        return nullptr;
    }
    LogSectionBorder->SetPadding(FMargin(8.0f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(LogSectionBorder, SectionBgColor, CornerRadius);
    
    // 创建垂直布局
    UVerticalBox* LogVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LogVBox"));
    if (!LogVBox)
    {
        return nullptr;
    }
    
    // 创建标题
    LogTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LogTitle"));
    if (LogTitle)
    {
        LogTitle->SetText(FText::FromString(TEXT("System Log")));
        LogTitle->SetColorAndOpacity(FSlateColor(CurrentTheme->TextColor));
        
        FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
        LogTitle->SetFont(TitleFont);
        
        UVerticalBoxSlot* TitleSlot = LogVBox->AddChildToVerticalBox(LogTitle);
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
    
    // 创建日志滚动框
    LogScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("LogScrollBox"));
    if (LogScrollBox)
    {
        LogScrollBox->SetOrientation(Orient_Vertical);
        LogScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
        
        // 创建日志容器
        LogContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LogContainer"));
        if (LogContainer)
        {
            LogScrollBox->AddChild(LogContainer);
        }
        
        UVerticalBoxSlot* ScrollSlot = LogVBox->AddChildToVerticalBox(LogScrollBox);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
        ScrollSlot->SetVerticalAlignment(VAlign_Fill);
    }
    
    LogSectionBorder->SetContent(LogVBox);
    LogSizeBox->SetContent(LogSectionBorder);
    return LogSizeBox;
}

UWidget* UMARightSidebarWidget::CreateSeparator()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    
    UBorder* Separator = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!Separator)
    {
        return nullptr;
    }
    
    // 使用主题的次要颜色作为分隔线颜色，带半透明
    FLinearColor SeparatorColor = CurrentTheme->SecondaryColor;
    SeparatorColor.A = 0.5f;
    Separator->SetBrushColor(SeparatorColor);
    
    // 使用 SizeBox 控制高度
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    if (!SizeBox)
    {
        return nullptr;
    }
    SizeBox->SetHeightOverride(1.0f);
    SizeBox->SetContent(Separator);
    
    return SizeBox;
}

//=============================================================================
// 预览更新方法
//=============================================================================

void UMARightSidebarWidget::UpdateTaskGraphPreview(const FMATaskGraphData& Data)
{
    if (TaskGraphPreview)
    {
        TaskGraphPreview->UpdatePreview(Data);
    }
}

void UMARightSidebarWidget::UpdateSkillListPreview(const FMASkillAllocationData& Data)
{
    if (SkillListPreview)
    {
        SkillListPreview->UpdatePreview(Data);
    }
}

void UMARightSidebarWidget::UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    if (SkillListPreview)
    {
        SkillListPreview->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
}

//=============================================================================
// 日志方法
//=============================================================================

void UMARightSidebarWidget::AppendLog(const FString& Message, bool bIsError)
{
    // 创建日志条目
    FMALogEntry Entry(Message, bIsError);
    LogEntries.Add(Entry);
    
    // 限制日志条目数量
    while (LogEntries.Num() > MaxLogEntries)
    {
        LogEntries.RemoveAt(0);
    }
    
    // 刷新显示
    RefreshLogDisplay();
    
    // 滚动到底部
    ScrollToLogBottom();
    
    UE_LOG(LogMARightSidebar, Log, TEXT("%s: %s"), bIsError ? TEXT("ERROR") : TEXT("INFO"), *Message);
}

void UMARightSidebarWidget::ClearLogs()
{
    LogEntries.Empty();
    RefreshLogDisplay();
}

void UMARightSidebarWidget::RefreshLogDisplay()
{
    if (!LogContainer)
    {
        return;
    }
    
    // 清空现有内容
    LogContainer->ClearChildren();
    
    // 添加所有日志条目
    for (const FMALogEntry& Entry : LogEntries)
    {
        UWidget* EntryWidget = CreateLogEntryWidget(Entry);
        if (EntryWidget)
        {
            UVerticalBoxSlot* Slot = LogContainer->AddChildToVerticalBox(EntryWidget);
            Slot->SetPadding(FMargin(0.0f, 2.0f));
        }
    }
}

UWidget* UMARightSidebarWidget::CreateLogEntryWidget(const FMALogEntry& Entry)
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    
    UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!LogText)
    {
        return nullptr;
    }
    
    // 格式化时间戳
    FString TimeStr = Entry.Timestamp.ToString(TEXT("%H:%M:%S"));
    FString FormattedMessage = FString::Printf(TEXT("[%s] %s"), *TimeStr, *Entry.Message);
    
    LogText->SetText(FText::FromString(FormattedMessage));
    
    // 设置颜色 (错误消息用危险色，普通消息用次要文字颜色)
    FLinearColor TextColor = Entry.bIsError ? CurrentTheme->DangerColor : CurrentTheme->SecondaryTextColor;
    LogText->SetColorAndOpacity(FSlateColor(TextColor));
    
    // 设置字体
    FSlateFontInfo LogFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    LogText->SetFont(LogFont);
    
    // 启用自动换行
    LogText->SetAutoWrapText(true);
    
    return LogText;
}

void UMARightSidebarWidget::ScrollToLogBottom()
{
    if (LogScrollBox)
    {
        LogScrollBox->ScrollToEnd();
    }
}

//=============================================================================
// 命令输入方法
//=============================================================================

FString UMARightSidebarWidget::GetCommandText() const
{
    if (CommandInput)
    {
        return CommandInput->GetText().ToString();
    }
    return FString();
}

void UMARightSidebarWidget::SetCommandText(const FString& Text)
{
    if (CommandInput)
    {
        CommandInput->SetText(FText::FromString(Text));
    }
}

void UMARightSidebarWidget::ClearCommandInput()
{
    if (CommandInput)
    {
        CommandInput->SetText(FText::GetEmpty());
    }
}

void UMARightSidebarWidget::FocusCommandInput()
{
    if (CommandInput)
    {
        CommandInput->SetKeyboardFocus();
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMARightSidebarWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        return;
    }
    
    // 应用背景颜色
    if (SidebarBackground)
    {
        SidebarBackground->SetBrushColor(Theme->BackgroundColor);
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
    if (TaskGraphSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(TaskGraphSectionBorder, SectionBgColor, CornerRadius);
    }
    if (SkillListSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(SkillListSectionBorder, SectionBgColor, CornerRadius);
    }
    if (LogSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(LogSectionBorder, SectionBgColor, CornerRadius);
    }
    
    // 应用标题颜色 (保持字号不变，只更新颜色)
    FSlateFontInfo SectionTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    if (CommandTitle)
    {
        CommandTitle->SetColorAndOpacity(FSlateColor(Theme->TextColor));
        CommandTitle->SetFont(SectionTitleFont);
    }
    if (TaskGraphTitle)
    {
        TaskGraphTitle->SetColorAndOpacity(FSlateColor(Theme->TextColor));
        TaskGraphTitle->SetFont(SectionTitleFont);
    }
    if (SkillListTitle)
    {
        SkillListTitle->SetColorAndOpacity(FSlateColor(Theme->TextColor));
        SkillListTitle->SetFont(SectionTitleFont);
    }
    if (LogTitle)
    {
        LogTitle->SetColorAndOpacity(FSlateColor(Theme->TextColor));
        LogTitle->SetFont(SectionTitleFont);
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
    if (TaskGraphPreview)
    {
        TaskGraphPreview->ApplyTheme(Theme);
    }
    if (SkillListPreview)
    {
        SkillListPreview->ApplyTheme(Theme);
    }
    
    // 刷新日志显示以应用新颜色
    RefreshLogDisplay();
}

//=============================================================================
// 事件回调
//=============================================================================

void UMARightSidebarWidget::OnSubmitClicked()
{
    FString Command = GetCommandText();
    if (!Command.IsEmpty())
    {
        // 广播命令提交事件
        OnCommandSubmitted.Broadcast(Command);
        
        // 清空输入框
        ClearCommandInput();
        
        // 记录日志
        AppendLog(FString::Printf(TEXT("Command: %s"), *Command), false);
        
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

void UMARightSidebarWidget::OnCommandInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // 只在按 Enter 键时提交
    if (CommitMethod == ETextCommit::OnEnter)
    {
        OnSubmitClicked();
    }
}
