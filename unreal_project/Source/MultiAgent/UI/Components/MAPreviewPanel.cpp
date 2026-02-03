// MAPreviewPanel.cpp
// 预览面板 Widget 实现

#include "MAPreviewPanel.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAFrostedGlassUtils.h"
#include "MATaskGraphPreview.h"
#include "MASkillListPreview.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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
DEFINE_LOG_CATEGORY(LogMAPreviewPanel);

//=============================================================================
// 主题颜色辅助函数
//=============================================================================

namespace MAPreviewPanelPrivate
{
    /** 获取主题或创建默认主题 */
    UMAUITheme* GetOrCreateTheme(UMAPreviewPanel* Widget)
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

UMAPreviewPanel::UMAPreviewPanel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAPreviewPanel::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMAPreviewPanel::NativeConstruct()
{
    Super::NativeConstruct();
}

TSharedRef<SWidget> UMAPreviewPanel::RebuildWidget()
{
    BuildUI();
    
    // 使用 Super::RebuildWidget() 让 UMG 正确处理 Widget 树
    return Super::RebuildWidget();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMAPreviewPanel::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAPreviewPanel, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }
    
    // 获取主题 (确保有可用的主题)
    UMAUITheme* CurrentTheme = MAPreviewPanelPrivate::GetOrCreateTheme(this);
    
    // 创建根 CanvasPanel - 用于定位面板
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAPreviewPanel, Error, TEXT("BuildUI: Failed to create RootCanvas"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;
    
    // 使用 MAFrostedGlassUtils 创建毛玻璃效果面板
    // 位置：右上角，距离右边 10px，距离顶部 170px (在 SystemLogPanel 下方)
    // 尺寸：宽度 480px，高度自适应 (使用较大的高度以容纳内容)
    FMAFrostedGlassResult FrostedGlass = MAFrostedGlassUtils::CreateFixedSizePanelFromTheme(
        WidgetTree,
        RootCanvas,
        CurrentTheme,
        FVector2D(-10, 170),          // 位置：右边距 10，顶部距离 170
        FVector2D(PanelWidth, 440),   // 尺寸：宽度 480，高度 440
        FVector2D(1.0f, 0.0f),        // 对齐：右上角对齐
        FAnchors(1.0f, 0.0f, 1.0f, 0.0f),  // 锚点：右上角
        TEXT("PreviewPanel")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMAPreviewPanel, Error, TEXT("BuildUI: Failed to create frosted glass panel"));
        return;
    }
    
    // 保存内容容器引用
    PanelBackground = FrostedGlass.ContentContainer;
    
    // 创建面板容器
    PanelContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelContainer"));
    if (!PanelContainer)
    {
        UE_LOG(LogMAPreviewPanel, Error, TEXT("BuildUI: Failed to create PanelContainer"));
        return;
    }
    
    // 添加任务图预览区
    UWidget* TaskGraphSection = CreateTaskGraphSection();
    if (TaskGraphSection)
    {
        UVerticalBoxSlot* TaskSlot = PanelContainer->AddChildToVerticalBox(TaskGraphSection);
        TaskSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
        TaskSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    // 添加分隔线
    UWidget* Separator = CreateSeparator();
    if (Separator)
    {
        UVerticalBoxSlot* SepSlot = PanelContainer->AddChildToVerticalBox(Separator);
        SepSlot->SetPadding(FMargin(0.0f, 4.0f));
    }
    
    // 添加技能列表预览区
    UWidget* SkillListSection = CreateSkillListSection();
    if (SkillListSection)
    {
        UVerticalBoxSlot* SkillSlot = PanelContainer->AddChildToVerticalBox(SkillListSection);
        SkillSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
        SkillSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    
    // 组装层级
    if (PanelBackground)
    {
        PanelBackground->AddChild(PanelContainer);
    }
}

UWidget* UMAPreviewPanel::CreateTaskGraphSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = MAPreviewPanelPrivate::GetOrCreateTheme(this);
    FLinearColor SectionBgColor = MAPreviewPanelPrivate::GetSectionBackgroundColor(CurrentTheme);
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

UWidget* UMAPreviewPanel::CreateSkillListSection()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = MAPreviewPanelPrivate::GetOrCreateTheme(this);
    FLinearColor SectionBgColor = MAPreviewPanelPrivate::GetSectionBackgroundColor(CurrentTheme);
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

UWidget* UMAPreviewPanel::CreateSeparator()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 获取主题
    UMAUITheme* CurrentTheme = MAPreviewPanelPrivate::GetOrCreateTheme(this);
    
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

void UMAPreviewPanel::UpdateTaskGraphPreview(const FMATaskGraphData& Data)
{
    if (TaskGraphPreview)
    {
        TaskGraphPreview->UpdatePreview(Data);
    }
}

void UMAPreviewPanel::UpdateSkillListPreview(const FMASkillAllocationData& Data)
{
    if (SkillListPreview)
    {
        SkillListPreview->UpdatePreview(Data);
    }
}

void UMAPreviewPanel::UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    if (SkillListPreview)
    {
        UE_LOG(LogMAPreviewPanel, Log, TEXT("UpdateSkillStatus: Forwarding to SkillListPreview - TimeStep=%d, RobotId=%s, Status=%d"),
            TimeStep, *RobotId, static_cast<int32>(NewStatus));
        SkillListPreview->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
    else
    {
        UE_LOG(LogMAPreviewPanel, Warning, TEXT("UpdateSkillStatus: SkillListPreview is null"));
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMAPreviewPanel::ApplyTheme(UMAUITheme* InTheme)
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
    
    if (TaskGraphSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(TaskGraphSectionBorder, SectionBgColor, CornerRadius);
    }
    if (SkillListSectionBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(SkillListSectionBorder, SectionBgColor, CornerRadius);
    }
    
    // 应用标题颜色 (保持字号不变，只更新颜色)
    FSlateFontInfo SectionTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
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
    
    // 应用主题到子组件
    if (TaskGraphPreview)
    {
        TaskGraphPreview->ApplyTheme(Theme);
    }
    if (SkillListPreview)
    {
        SkillListPreview->ApplyTheme(Theme);
    }
}
