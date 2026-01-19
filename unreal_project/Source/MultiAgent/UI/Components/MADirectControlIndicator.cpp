// MADirectControlIndicator.cpp
// Direct Control 指示器 Widget 实现

#include "MADirectControlIndicator.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogMADirectControlIndicator, Log, All);

UMADirectControlIndicator::UMADirectControlIndicator(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMADirectControlIndicator::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

TSharedRef<SWidget> UMADirectControlIndicator::RebuildWidget()
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

void UMADirectControlIndicator::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMADirectControlIndicator, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMADirectControlIndicator, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMADirectControlIndicator, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建背景 Border - 半透明深色背景，带圆角
    BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    
    // 应用圆角效果 - 使用 Panel 类型的圆角半径
    FLinearColor BgColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);
    MARoundedBorderUtils::ApplyRoundedCorners(
        BackgroundBorder,
        BgColor,
        MARoundedBorderUtils::DefaultPanelCornerRadius
    );
    
    BackgroundBorder->SetPadding(FMargin(15.0f, 8.0f, 15.0f, 8.0f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    // 设置为顶部居中
    BorderSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
    BorderSlot->SetAlignment(FVector2D(0.5f, 0.0f));
    BorderSlot->SetPosition(FVector2D(0, 50));  // 距离顶部 50 像素
    BorderSlot->SetAutoSize(true);

    // 创建文本
    IndicatorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IndicatorText"));
    IndicatorText->SetText(FText::FromString(TEXT("Direct Control: ---")));
    
    // 设置字体样式
    FSlateFontInfo FontInfo = IndicatorText->GetFont();
    FontInfo.Size = 16;
    IndicatorText->SetFont(FontInfo);
    
    // 设置绿色文字
    IndicatorText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.0f, 0.2f)));
    
    BackgroundBorder->AddChild(IndicatorText);

    UE_LOG(LogMADirectControlIndicator, Log, TEXT("BuildUI: UI construction completed successfully"));
}

void UMADirectControlIndicator::SetAgentName(const FString& AgentName)
{
    CurrentAgentName = AgentName;
    
    if (IndicatorText)
    {
        FString DisplayText = FString::Printf(TEXT("Direct Control: %s"), *AgentName);
        IndicatorText->SetText(FText::FromString(DisplayText));
        UE_LOG(LogMADirectControlIndicator, Log, TEXT("SetAgentName: %s"), *AgentName);
    }
}

FString UMADirectControlIndicator::GetAgentName() const
{
    return CurrentAgentName;
}
