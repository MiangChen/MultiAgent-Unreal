// MASystemLogPanel.cpp
// 系统日志面板 Widget 实现

#include "MASystemLogPanel.h"
#include "../../Core/MAUITheme.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "../../Core/MAFrostedGlassUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/BackgroundBlur.h"
#include "Styling/SlateTypes.h"

//=============================================================================
// 日志类别定义
//=============================================================================
DEFINE_LOG_CATEGORY(LogMASystemLogPanel);

//=============================================================================
// 主题颜色辅助函数
//=============================================================================

namespace
{
    /** 获取主题或创建默认主题 */
    UMAUITheme* GetOrCreateTheme(UMASystemLogPanel* Widget)
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

UMASystemLogPanel::UMASystemLogPanel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMASystemLogPanel::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMASystemLogPanel::NativeConstruct()
{
    Super::NativeConstruct();
}

TSharedRef<SWidget> UMASystemLogPanel::RebuildWidget()
{
    BuildUI();
    
    // 使用 Super::RebuildWidget() 让 UMG 正确处理 Widget 树
    return Super::RebuildWidget();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMASystemLogPanel::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMASystemLogPanel, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }
    
    // 获取主题 (确保有可用的主题)
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    
    // 创建根 CanvasPanel - 用于定位面板
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMASystemLogPanel, Error, TEXT("BuildUI: Failed to create RootCanvas"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;
    
    // 使用 MAFrostedGlassUtils 创建毛玻璃效果面板
    // 位置：右上角，距离右边 10px，距离顶部 10px
    // 尺寸：宽度 480px，高度 150px
    FMAFrostedGlassResult FrostedGlass = MAFrostedGlassUtils::CreateFixedSizePanelFromTheme(
        WidgetTree,
        RootCanvas,
        CurrentTheme,
        FVector2D(-10, 10),           // 位置：右边距 10，顶部距离 10
        FVector2D(PanelWidth, PanelHeight),  // 尺寸：宽度 480，高度 150
        FVector2D(1.0f, 0.0f),        // 对齐：右上角对齐
        FAnchors(1.0f, 0.0f, 1.0f, 0.0f),  // 锚点：右上角
        TEXT("SystemLogPanel")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMASystemLogPanel, Error, TEXT("BuildUI: Failed to create frosted glass panel"));
        return;
    }
    
    // 保存内容容器引用
    PanelBackground = FrostedGlass.ContentContainer;
    
    // 创建日志区
    UWidget* LogSection = CreateLogSection();
    if (LogSection && PanelBackground)
    {
        PanelBackground->AddChild(LogSection);
    }
}

UWidget* UMASystemLogPanel::CreateLogSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    FLinearColor SectionBgColor = GetSectionBackgroundColor(CurrentTheme);
    float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(CurrentTheme, EMARoundedElementType::Panel);
    
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
    return LogSectionBorder;
}

//=============================================================================
// 日志方法
//=============================================================================

void UMASystemLogPanel::AppendLog(const FString& Message, bool bIsError)
{
    Coordinator.AppendLog(LogEntries, Message, bIsError, MaxLogEntries);
    
    // 刷新显示
    RefreshLogDisplay();
    
    // 滚动到底部
    ScrollToLogBottom();
    
    UE_LOG(LogMASystemLogPanel, Log, TEXT("%s: %s"), bIsError ? TEXT("ERROR") : TEXT("INFO"), *Message);
}

void UMASystemLogPanel::ClearLogs()
{
    Coordinator.ClearLogs(LogEntries);
    RefreshLogDisplay();
}

void UMASystemLogPanel::RefreshLogDisplay()
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

UWidget* UMASystemLogPanel::CreateLogEntryWidget(const FMALogEntry& Entry)
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!LogText)
    {
        return nullptr;
    }

    const UMAUITheme* CurrentTheme = GetOrCreateTheme(this);
    const FMALogEntryViewModel Model = Coordinator.BuildEntryViewModel(Entry, CurrentTheme);
    LogText->SetText(FText::FromString(Model.FormattedMessage));
    LogText->SetColorAndOpacity(FSlateColor(Model.TextColor));
    
    // 设置字体
    FSlateFontInfo LogFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    LogText->SetFont(LogFont);
    
    // 启用自动换行
    LogText->SetAutoWrapText(true);
    
    return LogText;
}

void UMASystemLogPanel::ScrollToLogBottom()
{
    if (LogScrollBox)
    {
        LogScrollBox->ScrollToEnd();
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMASystemLogPanel::ApplyTheme(UMAUITheme* InTheme)
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
    
    const FLinearColor SectionBgColor = Coordinator.BuildSectionBackgroundColor(Theme);
    const float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Panel);
    
    if (LogSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(LogSectionBorder, SectionBgColor, CornerRadius);
    }
    
    // 应用标题颜色 (保持字号不变，只更新颜色)
    FSlateFontInfo SectionTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    if (LogTitle)
    {
        LogTitle->SetColorAndOpacity(FSlateColor(Theme->TextColor));
        LogTitle->SetFont(SectionTitleFont);
    }
    
    // 刷新日志显示以应用新颜色
    RefreshLogDisplay();
}
