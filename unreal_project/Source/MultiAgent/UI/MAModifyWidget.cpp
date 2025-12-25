// MAModifyWidget.cpp
// Modify 模式修改面板 Widget - 纯 C++ 实现
// Requirements: 3.2, 3.3, 4.1, 4.2, 4.3, 5.1, 5.2, 5.3, 5.4

#include "MAModifyWidget.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAModifyWidget, Log, All);

// 提示文字常量
static const FString DefaultHintText = TEXT("点击场景中的 Actor 进行选择");

UMAModifyWidget::UMAModifyWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAModifyWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAModifyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定确认按钮事件
    if (ConfirmButton && !ConfirmButton->OnClicked.IsAlreadyBound(this, &UMAModifyWidget::OnConfirmButtonClicked))
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UMAModifyWidget::OnConfirmButtonClicked);
        UE_LOG(LogMAModifyWidget, Log, TEXT("ConfirmButton event bound"));
    }
    
    // 初始化为未选中状态
    ClearSelection();
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("MAModifyWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMAModifyWidget::RebuildWidget()
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

void UMAModifyWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建背景 Border - 位于屏幕右侧
    // Requirements: 3.5 - 修改面板宽度约为屏幕宽度的 20%
    UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.95f));
    BackgroundBorder->SetPadding(FMargin(15.0f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    // 设置为右侧锚点，宽度约 20% 屏幕宽度 (约 380 像素)
    BorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
    BorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    BorderSlot->SetPosition(FVector2D(-20, 80));  // 右边距 20，顶部距离 80 (避开模式指示器)
    BorderSlot->SetSize(FVector2D(350, 300));
    BorderSlot->SetAutoSize(false);

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    BackgroundBorder->AddChild(MainVBox);

    // 标题 - Requirements: 3.1
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("修改面板")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.0f)));  // 橙色，与 Modify 模式颜色一致
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 提示文本 - Requirements: 4.3
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(DefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 多行文本框 - Requirements: 3.2, 4.4
    LabelTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("LabelTextBox"));
    LabelTextBox->SetHintText(FText::FromString(TEXT("选中 Actor 后显示标签信息...")));
    
    // 使用 SizeBox 设置最小高度
    USizeBox* TextBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TextBoxSizeBox"));
    TextBoxSizeBox->SetMinDesiredHeight(150.0f);
    TextBoxSizeBox->AddChild(LabelTextBox);
    
    UVerticalBoxSlot* TextBoxSlot = MainVBox->AddChildToVerticalBox(TextBoxSizeBox);
    TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TextBoxSlot->SetPadding(FMargin(0, 0, 0, 15));

    // 确认按钮 - Requirements: 3.3
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
    ButtonText->SetText(FText::FromString(TEXT("  确认  ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo ButtonFont = ButtonText->GetFont();
    ButtonFont.Size = 14;
    ButtonText->SetFont(ButtonFont);
    ConfirmButton->AddChild(ButtonText);
    
    UVerticalBoxSlot* ButtonSlot = MainVBox->AddChildToVerticalBox(ConfirmButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Center);

    UE_LOG(LogMAModifyWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}


//=========================================================================
// GenerateActorLabel - 占位符实现
// Requirements: 4.2
//=========================================================================

FString UMAModifyWidget::GenerateActorLabel(AActor* Actor) const
{
    if (!Actor)
    {
        return FString();
    }
    
    // 获取 Actor 名称
    FString ActorName = Actor->GetName();
    
    // 占位符标签 - 未来从 IMALabelProvider 获取
    FString Label = TEXT("[placeholder]");
    
    // 格式: "Actor: [ActorName]\nLabel: [placeholder]"
    // Requirements: 4.2
    return FString::Printf(TEXT("Actor: %s\nLabel: %s"), *ActorName, *Label);
}


//=========================================================================
// SetSelectedActor - 设置选中的 Actor
// Requirements: 4.1
//=========================================================================

void UMAModifyWidget::SetSelectedActor(AActor* Actor)
{
    if (!Actor)
    {
        // 如果传入 nullptr，清除选择
        ClearSelection();
        return;
    }
    
    SelectedActor = Actor;
    
    // 生成并显示 Actor 标签
    FString LabelContent = GenerateActorLabel(Actor);
    SetLabelText(LabelContent);
    
    // 更新提示文本
    if (HintText)
    {
        HintText->SetText(FText::FromString(FString::Printf(TEXT("已选中: %s"), *Actor->GetName())));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));  // 绿色表示已选中
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("SetSelectedActor: %s"), *Actor->GetName());
}

//=========================================================================
// ClearSelection - 清除选中状态
// Requirements: 4.3
//=========================================================================

void UMAModifyWidget::ClearSelection()
{
    SelectedActor = nullptr;
    
    // 清空文本框
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::GetEmpty());
    }
    
    // 显示提示文字
    if (HintText)
    {
        HintText->SetText(FText::FromString(DefaultHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));  // 灰色
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("ClearSelection: Selection cleared"));
}

//=========================================================================
// GetLabelText / SetLabelText - 文本框内容访问
//=========================================================================

FString UMAModifyWidget::GetLabelText() const
{
    if (LabelTextBox)
    {
        return LabelTextBox->GetText().ToString();
    }
    return FString();
}

void UMAModifyWidget::SetLabelText(const FString& Text)
{
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::FromString(Text));
    }
}


//=========================================================================
// OnConfirmButtonClicked - 确认按钮点击处理
// Requirements: 5.1, 5.2, 5.3, 5.4
//=========================================================================

void UMAModifyWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMAModifyWidget, Log, TEXT("ConfirmButton clicked"));
    
    // 检查是否有选中的 Actor
    if (!SelectedActor)
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: No Actor selected, ignoring"));
        return;
    }
    
    // 获取文本框内容
    FString LabelContent = GetLabelText();
    
    // Requirements: 5.3 - 输出调试日志
    UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Actor=%s, LabelText=%s"), 
        *SelectedActor->GetName(), *LabelContent);
    
    // Requirements: 5.1, 5.2 - 广播 OnModifyConfirmed 委托
    OnModifyConfirmed.Broadcast(SelectedActor, LabelContent);
    
    // Requirements: 5.4 - 确认后清空文本框，取消 Actor 高亮
    // 注意：高亮的清除由 MAPlayerController 处理，这里只清空 Widget 状态
    ClearSelection();
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("OnConfirmButtonClicked: Modification confirmed and selection cleared"));
}
