// MASimpleMainWidget.cpp
// 简单的主界面 Widget - 纯 C++ 实现

#include "MASimpleMainWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASimpleWidget, Log, All);

UMASimpleMainWidget::UMASimpleMainWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMASimpleMainWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMASimpleMainWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定事件
    if (SendButton && !SendButton->OnClicked.IsAlreadyBound(this, &UMASimpleMainWidget::OnSendButtonClicked))
    {
        SendButton->OnClicked.AddDynamic(this, &UMASimpleMainWidget::OnSendButtonClicked);
        UE_LOG(LogMASimpleWidget, Log, TEXT("SendButton event bound"));
    }

    if (InputTextBox && !InputTextBox->OnTextCommitted.IsAlreadyBound(this, &UMASimpleMainWidget::OnInputTextCommitted))
    {
        InputTextBox->OnTextCommitted.AddDynamic(this, &UMASimpleMainWidget::OnInputTextCommitted);
        UE_LOG(LogMASimpleWidget, Log, TEXT("InputTextBox event bound"));
    }
    
    UE_LOG(LogMASimpleWidget, Log, TEXT("MASimpleMainWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMASimpleMainWidget::RebuildWidget()
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

void UMASimpleMainWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMASimpleWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMASimpleWidget, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMASimpleWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建背景 Border
    UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.95f));
    BackgroundBorder->SetPadding(FMargin(15.0f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    // 设置为左上角，固定大小
    BorderSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    BorderSlot->SetPosition(FVector2D(20, 20));
    BorderSlot->SetSize(FVector2D(450, 350));
    BorderSlot->SetAutoSize(false);

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    BackgroundBorder->AddChild(MainVBox);

    // 标题
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("MultiAgent 指令输入")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 18;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 1.0f)));
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 15));

    // 提示文本
    UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(TEXT("按 Z 键切换显示/隐藏")));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 12;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 输入区域 (水平布局)
    UHorizontalBox* InputHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InputHBox"));
    
    // 输入框
    InputTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputTextBox"));
    InputTextBox->SetHintText(FText::FromString(TEXT("输入指令后按回车...")));
    
    UHorizontalBoxSlot* InputSlot = InputHBox->AddChildToHorizontalBox(InputTextBox);
    InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    InputSlot->SetPadding(FMargin(0, 0, 10, 0));

    // 发送按钮
    SendButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
    ButtonText->SetText(FText::FromString(TEXT(" 发送 ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    SendButton->AddChild(ButtonText);
    
    UHorizontalBoxSlot* ButtonSlot = InputHBox->AddChildToHorizontalBox(SendButton);
    ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    UVerticalBoxSlot* InputRowSlot = MainVBox->AddChildToVerticalBox(InputHBox);
    InputRowSlot->SetPadding(FMargin(0, 0, 0, 15));

    // 结果标签
    UTextBlock* ResultLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultLabel"));
    ResultLabel->SetText(FText::FromString(TEXT("响应结果:")));
    ResultLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
    
    UVerticalBoxSlot* LabelSlot = MainVBox->AddChildToVerticalBox(ResultLabel);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // 结果显示框
    ResultTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("ResultTextBox"));
    ResultTextBox->SetIsReadOnly(true);
    ResultTextBox->SetText(FText::FromString(TEXT("等待输入指令...\n\n提示:\n- 输入任意文本后按回车发送\n- 系统将返回模拟响应")));
    
    // 使用 SizeBox 设置最小高度
    USizeBox* ResultSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResultSizeBox"));
    ResultSizeBox->SetMinDesiredHeight(150.0f);
    ResultSizeBox->AddChild(ResultTextBox);
    
    UVerticalBoxSlot* ResultSlot = MainVBox->AddChildToVerticalBox(ResultSizeBox);
    ResultSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UE_LOG(LogMASimpleWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}

void UMASimpleMainWidget::SetResultText(const FString& Text)
{
    if (ResultTextBox)
    {
        ResultTextBox->SetText(FText::FromString(Text));
        UE_LOG(LogMASimpleWidget, Verbose, TEXT("SetResultText: %s"), *Text);
    }
}

void UMASimpleMainWidget::FocusInputBox()
{
    if (InputTextBox)
    {
        InputTextBox->SetKeyboardFocus();
        UE_LOG(LogMASimpleWidget, Verbose, TEXT("FocusInputBox called"));
    }
}

void UMASimpleMainWidget::ClearInputBox()
{
    if (InputTextBox)
    {
        InputTextBox->SetText(FText::GetEmpty());
    }
}

FString UMASimpleMainWidget::GetInputText() const
{
    if (InputTextBox)
    {
        return InputTextBox->GetText().ToString();
    }
    return FString();
}

void UMASimpleMainWidget::OnSendButtonClicked()
{
    UE_LOG(LogMASimpleWidget, Log, TEXT("SendButton clicked"));
    SubmitCommand();
}

void UMASimpleMainWidget::OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        UE_LOG(LogMASimpleWidget, Log, TEXT("Enter pressed, submitting command"));
        SubmitCommand();
    }
}

void UMASimpleMainWidget::SubmitCommand()
{
    FString Command = GetInputText().TrimStartAndEnd();
    
    if (Command.IsEmpty())
    {
        UE_LOG(LogMASimpleWidget, Verbose, TEXT("Empty command, ignoring"));
        return;
    }
    
    UE_LOG(LogMASimpleWidget, Log, TEXT("Submitting command: %s"), *Command);
    
    // 广播委托
    OnCommandSubmitted.Broadcast(Command);
    
    // 清空输入框
    ClearInputBox();
    
    // 保持焦点
    FocusInputBox();
}
