// MAEmergencyWidget.cpp
// 突发事件详情界面 Widget 实现
// Requirements: 2.2, 2.3, 3.1, 3.2, 3.3, 3.4

#include "MAEmergencyWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SceneCaptureComponent2D.h"
#include "MACameraSensorComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAEmergencyWidget, Log, All);

UMAEmergencyWidget::UMAEmergencyWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 初始化指针
    RootCanvas = nullptr;
    BackgroundBorder = nullptr;
    CameraFeedImage = nullptr;
    InfoTextBox = nullptr;
    ActionButton1 = nullptr;
    ActionButton2 = nullptr;
    ActionButton3 = nullptr;
    InputTextBox = nullptr;
    SendButton = nullptr;
    RenderTarget = nullptr;
    CameraMaterial = nullptr;
    
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAEmergencyWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAEmergencyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("NativeConstruct called"));
    
    // 创建相机渲染资源
    CreateCameraRenderResources();
    
    // 绑定按钮事件
    BindButtonEvents();
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("EmergencyWidget constructed successfully"));
}

TSharedRef<SWidget> UMAEmergencyWidget::RebuildWidget()
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

void UMAEmergencyWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAEmergencyWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Building UI layout..."));
    
    // 创建根 Canvas
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAEmergencyWidget, Error, TEXT("Failed to create RootCanvas"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;
    
    //=========================================================================
    // 创建背景边框 - 半透明深色背景
    // 位置: (20, 100), 大小: (960, 500)
    //=========================================================================
    BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    if (BackgroundBorder)
    {
        BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.95f));
        BackgroundBorder->SetPadding(FMargin(20.0f));
        
        UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
        if (BorderSlot)
        {
            BorderSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            BorderSlot->SetPosition(FVector2D(20, 100));
            BorderSlot->SetSize(FVector2D(960, 500));
            BorderSlot->SetAutoSize(false);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created BackgroundBorder"));
    }
    
    // 创建内部 Canvas 用于布局
    UCanvasPanel* InnerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InnerCanvas"));
    if (InnerCanvas && BackgroundBorder)
    {
        BackgroundBorder->AddChild(InnerCanvas);
    }
    
    if (!InnerCanvas)
    {
        UE_LOG(LogMAEmergencyWidget, Error, TEXT("Failed to create InnerCanvas"));
        return;
    }
    
    //=========================================================================
    // 标题
    //=========================================================================
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("突发事件详情")));
        FSlateFontInfo TitleFont = TitleText->GetFont();
        TitleFont.Size = 20;
        TitleText->SetFont(TitleFont);
        TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f)));
        
        UCanvasPanelSlot* TitleSlot = InnerCanvas->AddChildToCanvas(TitleText);
        if (TitleSlot)
        {
            TitleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            TitleSlot->SetPosition(FVector2D(0, 0));
            TitleSlot->SetAutoSize(true);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created TitleText"));
    }
    
    //=========================================================================
    // 相机画面区域 (左中)
    // 位置: (0, 40), 大小: (400, 300)
    //=========================================================================
    CameraFeedImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CameraFeedImage"));
    if (CameraFeedImage)
    {
        UCanvasPanelSlot* CameraSlot = InnerCanvas->AddChildToCanvas(CameraFeedImage);
        if (CameraSlot)
        {
            CameraSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            CameraSlot->SetPosition(FVector2D(0, 40));
            CameraSlot->SetSize(FVector2D(400, 300));
            CameraSlot->SetAutoSize(false);
        }
        
        // 设置默认黑色背景
        CameraFeedImage->SetColorAndOpacity(FLinearColor::Black);
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created CameraFeedImage"));
    }
    
    //=========================================================================
    // 信息显示区域 (右中)
    // 位置: (420, 40), 大小: (300, 200)
    //=========================================================================
    InfoTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("InfoTextBox"));
    if (InfoTextBox)
    {
        UCanvasPanelSlot* InfoSlot = InnerCanvas->AddChildToCanvas(InfoTextBox);
        if (InfoSlot)
        {
            InfoSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            InfoSlot->SetPosition(FVector2D(420, 40));
            InfoSlot->SetSize(FVector2D(300, 200));
            InfoSlot->SetAutoSize(false);
        }
        
        // 设置为只读
        InfoTextBox->SetIsReadOnly(true);
        InfoTextBox->SetText(FText::FromString(TEXT("突发事件信息显示区域\n等待事件数据...\n\n按 X 键关闭此界面\n按 - 键结束事件")));
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created InfoTextBox"));
    }
    
    //=========================================================================
    // 操作按钮 (右侧)
    // 按钮 1: (740, 50), 按钮 2: (740, 100), 按钮 3: (740, 150)
    // 大小: (160, 40)
    //=========================================================================
    
    // 按钮 1: 扩大搜索范围
    ActionButton1 = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ActionButton1"));
    if (ActionButton1)
    {
        UCanvasPanelSlot* Button1Slot = InnerCanvas->AddChildToCanvas(ActionButton1);
        if (Button1Slot)
        {
            Button1Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Button1Slot->SetPosition(FVector2D(740, 50));
            Button1Slot->SetSize(FVector2D(160, 40));
            Button1Slot->SetAutoSize(false);
        }
        
        // 添加按钮文本
        UTextBlock* Button1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button1Text"));
        if (Button1Text)
        {
            Button1Text->SetText(FText::FromString(TEXT("扩大搜索范围")));
            Button1Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            ActionButton1->AddChild(Button1Text);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created ActionButton1"));
    }
    
    // 按钮 2: 忽略并返回
    ActionButton2 = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ActionButton2"));
    if (ActionButton2)
    {
        UCanvasPanelSlot* Button2Slot = InnerCanvas->AddChildToCanvas(ActionButton2);
        if (Button2Slot)
        {
            Button2Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Button2Slot->SetPosition(FVector2D(740, 100));
            Button2Slot->SetSize(FVector2D(160, 40));
            Button2Slot->SetAutoSize(false);
        }
        
        // 添加按钮文本
        UTextBlock* Button2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button2Text"));
        if (Button2Text)
        {
            Button2Text->SetText(FText::FromString(TEXT("忽略并返回")));
            Button2Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            ActionButton2->AddChild(Button2Text);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created ActionButton2"));
    }
    
    // 按钮 3: 切换灭火任务
    ActionButton3 = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ActionButton3"));
    if (ActionButton3)
    {
        UCanvasPanelSlot* Button3Slot = InnerCanvas->AddChildToCanvas(ActionButton3);
        if (Button3Slot)
        {
            Button3Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            Button3Slot->SetPosition(FVector2D(740, 150));
            Button3Slot->SetSize(FVector2D(160, 40));
            Button3Slot->SetAutoSize(false);
        }
        
        // 添加按钮文本
        UTextBlock* Button3Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button3Text"));
        if (Button3Text)
        {
            Button3Text->SetText(FText::FromString(TEXT("切换灭火任务")));
            Button3Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            ActionButton3->AddChild(Button3Text);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created ActionButton3"));
    }
    
    //=========================================================================
    // 输入区域 (底部)
    // 输入框: (0, 360), 大小: (700, 80)
    // 发送按钮: (720, 380), 大小: (80, 40)
    //=========================================================================
    
    // 输入文本框
    InputTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("InputTextBox"));
    if (InputTextBox)
    {
        UCanvasPanelSlot* InputSlot = InnerCanvas->AddChildToCanvas(InputTextBox);
        if (InputSlot)
        {
            InputSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            InputSlot->SetPosition(FVector2D(0, 360));
            InputSlot->SetSize(FVector2D(700, 80));
            InputSlot->SetAutoSize(false);
        }
        
        // 设置提示文本
        InputTextBox->SetHintText(FText::FromString(TEXT("输入指令或消息...")));
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created InputTextBox"));
    }
    
    // 发送按钮
    SendButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendButton"));
    if (SendButton)
    {
        UCanvasPanelSlot* SendSlot = InnerCanvas->AddChildToCanvas(SendButton);
        if (SendSlot)
        {
            SendSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            SendSlot->SetPosition(FVector2D(720, 380));
            SendSlot->SetSize(FVector2D(80, 40));
            SendSlot->SetAutoSize(false);
        }
        
        // 添加按钮文本
        UTextBlock* SendText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SendText"));
        if (SendText)
        {
            SendText->SetText(FText::FromString(TEXT("发送")));
            SendText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            SendButton->AddChild(SendText);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created SendButton"));
    }
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("UI layout built successfully"));
}

void UMAEmergencyWidget::CreateCameraRenderResources()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Creating camera render resources..."));
    
    // 我们不需要创建自己的渲染目标，而是直接使用相机传感器的渲染目标
    // 这样可以避免额外的渲染开销，并确保与相机传感器的一致性
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Camera render resources initialized (will use camera's render target)"));
}

void UMAEmergencyWidget::BindButtonEvents()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Binding button events..."));
    
    // 绑定操作按钮事件
    if (ActionButton1)
    {
        ActionButton1->OnClicked.AddDynamic(this, &UMAEmergencyWidget::OnActionButton1Clicked);
    }
    
    if (ActionButton2)
    {
        ActionButton2->OnClicked.AddDynamic(this, &UMAEmergencyWidget::OnActionButton2Clicked);
    }
    
    if (ActionButton3)
    {
        ActionButton3->OnClicked.AddDynamic(this, &UMAEmergencyWidget::OnActionButton3Clicked);
    }
    
    // 绑定发送按钮事件
    if (SendButton)
    {
        SendButton->OnClicked.AddDynamic(this, &UMAEmergencyWidget::OnSendButtonClicked);
    }
    
    // 绑定输入框回车事件
    if (InputTextBox)
    {
        InputTextBox->OnTextCommitted.AddDynamic(this, &UMAEmergencyWidget::OnInputTextCommitted);
    }
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Button events bound successfully"));
}

//=========================================================================
// 相机源管理
//=========================================================================

void UMAEmergencyWidget::SetCameraSource(UMACameraSensorComponent* Camera)
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("SetCameraSource called"));
    
    if (!Camera)
    {
        UE_LOG(LogMAEmergencyWidget, Warning, TEXT("SetCameraSource: Camera is null"));
        ClearCameraSource();
        return;
    }
    
    // 先清除之前的相机源（如果有的话，恢复其设置）
    if (CurrentCameraSource.IsValid() && CurrentCameraSource.Get() != Camera)
    {
        USceneCaptureComponent2D* OldCapture = CurrentCameraSource->GetSceneCaptureComponent();
        if (OldCapture)
        {
            OldCapture->bCaptureEveryFrame = false;
        }
    }
    
    CurrentCameraSource = Camera;
    
    // 获取相机的 SceneCaptureComponent
    USceneCaptureComponent2D* SceneCapture = Camera->GetSceneCaptureComponent();
    if (!SceneCapture)
    {
        UE_LOG(LogMAEmergencyWidget, Warning, TEXT("SetCameraSource: SceneCaptureComponent is null"));
        ClearCameraSource();
        return;
    }
    
    // 启用每帧捕获以显示实时画面
    SceneCapture->bCaptureEveryFrame = true;
    
    // 获取相机的渲染目标
    UTextureRenderTarget2D* CameraRenderTarget = SceneCapture->TextureTarget;
    if (CameraRenderTarget && CameraFeedImage)
    {
        // 使用渲染目标作为图像源
        FSlateBrush Brush;
        Brush.SetResourceObject(CameraRenderTarget);
        Brush.ImageSize = FVector2D(CameraRenderTarget->SizeX, CameraRenderTarget->SizeY);
        CameraFeedImage->SetBrush(Brush);
        CameraFeedImage->SetColorAndOpacity(FLinearColor::White);
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Camera source connected to render target (size: %dx%d)"), 
            CameraRenderTarget->SizeX, CameraRenderTarget->SizeY);
    }
    else
    {
        UE_LOG(LogMAEmergencyWidget, Warning, TEXT("Failed to get camera render target (RT=%p) or image widget is null (Image=%p)"), 
            CameraRenderTarget, CameraFeedImage);
        ClearCameraSource();
    }
}

void UMAEmergencyWidget::ClearCameraSource()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("ClearCameraSource called"));
    
    // 恢复之前相机的设置
    if (CurrentCameraSource.IsValid())
    {
        USceneCaptureComponent2D* SceneCapture = CurrentCameraSource->GetSceneCaptureComponent();
        if (SceneCapture)
        {
            // 恢复为不每帧捕获（节省性能）
            SceneCapture->bCaptureEveryFrame = false;
        }
    }
    
    CurrentCameraSource.Reset();
    
    // 设置为黑屏
    if (CameraFeedImage)
    {
        CameraFeedImage->SetColorAndOpacity(FLinearColor::Black);
        FSlateBrush Brush;
        Brush.SetResourceObject(nullptr);
        CameraFeedImage->SetBrush(Brush);
    }
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Camera source cleared, showing black screen"));
}

//=========================================================================
// 信息显示
//=========================================================================

void UMAEmergencyWidget::SetInfoText(const FString& Text)
{
    if (InfoTextBox)
    {
        InfoTextBox->SetText(FText::FromString(Text));
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Info text set: %s"), *Text);
    }
}

void UMAEmergencyWidget::AppendInfoText(const FString& Text)
{
    if (InfoTextBox)
    {
        FString CurrentText = InfoTextBox->GetText().ToString();
        FString NewText = CurrentText + TEXT("\n") + Text;
        InfoTextBox->SetText(FText::FromString(NewText));
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Info text appended: %s"), *Text);
    }
}

void UMAEmergencyWidget::ClearInfoText()
{
    if (InfoTextBox)
    {
        InfoTextBox->SetText(FText::GetEmpty());
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Info text cleared"));
    }
}

//=========================================================================
// 输入控制
//=========================================================================

void UMAEmergencyWidget::FocusInputBox()
{
    if (InputTextBox)
    {
        InputTextBox->SetKeyboardFocus();
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Input box focused"));
    }
}

void UMAEmergencyWidget::ClearInputBox()
{
    if (InputTextBox)
    {
        InputTextBox->SetText(FText::GetEmpty());
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Input box cleared"));
    }
}

FString UMAEmergencyWidget::GetInputText() const
{
    if (InputTextBox)
    {
        return InputTextBox->GetText().ToString();
    }
    return FString();
}

//=========================================================================
// 事件处理
//=========================================================================

void UMAEmergencyWidget::OnActionButton1Clicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Action Button 1 clicked: 扩大搜索范围"));
    OnActionButtonClicked.Broadcast(0);
}

void UMAEmergencyWidget::OnActionButton2Clicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Action Button 2 clicked: 忽略并返回"));
    OnActionButtonClicked.Broadcast(1);
}

void UMAEmergencyWidget::OnActionButton3Clicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Action Button 3 clicked: 切换灭火任务"));
    OnActionButtonClicked.Broadcast(2);
}

void UMAEmergencyWidget::OnSendButtonClicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Send button clicked"));
    SubmitMessage();
}

void UMAEmergencyWidget::OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Input text committed via Enter key"));
        SubmitMessage();
    }
}

void UMAEmergencyWidget::SubmitMessage()
{
    FString Message = GetInputText();
    if (!Message.IsEmpty())
    {
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Submitting message: %s"), *Message);
        
        // 广播消息发送事件
        OnMessageSent.Broadcast(Message);
        
        // 清空输入框
        ClearInputBox();
        
        // 重新聚焦输入框
        FocusInputBox();
    }
    else
    {
        UE_LOG(LogMAEmergencyWidget, Warning, TEXT("Cannot submit empty message"));
    }
}