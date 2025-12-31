// MAEmergencyWidget.cpp
// Emergency Event Details Widget Implementation
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
#include "MACommSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAEmergencyWidget, Log, All);

UMAEmergencyWidget::UMAEmergencyWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Initialize pointers
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
    
    // Ensure WidgetTree exists
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAEmergencyWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // Build UI here, ensure WidgetTree is initialized
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAEmergencyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("NativeConstruct called"));
    
    // Create camera render resources
    CreateCameraRenderResources();
    
    // Bind button events
    BindButtonEvents();
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("EmergencyWidget constructed successfully"));
}

TSharedRef<SWidget> UMAEmergencyWidget::RebuildWidget()
{
    // Ensure WidgetTree exists
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    
    // Build UI
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
    
    // Create root Canvas
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAEmergencyWidget, Error, TEXT("Failed to create RootCanvas"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;
    
    //=========================================================================
    // Create background border - semi-transparent dark background
    // Position: (20, 100), Size: (960, 500)
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
    
    // Create inner Canvas for layout
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
    // Title
    //=========================================================================
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("Emergency Event Details")));
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
    // Camera feed area (center-left)
    // Position: (0, 40), Size: (400, 300)
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
        
        // Set default black background
        CameraFeedImage->SetColorAndOpacity(FLinearColor::Black);
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created CameraFeedImage"));
    }
    
    //=========================================================================
    // Info display area (center-right)
    // Position: (420, 40), Size: (300, 200)
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
        
        // Set as read-only
        InfoTextBox->SetIsReadOnly(true);
        InfoTextBox->SetText(FText::FromString(TEXT("Emergency event information area\nWaiting for event data...\n\nPress X to close this panel\nPress - to end event")));
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created InfoTextBox"));
    }
    
    //=========================================================================
    // Action buttons (right side)
    // Button 1: (740, 50), Button 2: (740, 100), Button 3: (740, 150)
    // Size: (160, 40)
    //=========================================================================
    
    // Button 1: Expand Search Area
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
        
        // Add button text
        UTextBlock* Button1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button1Text"));
        if (Button1Text)
        {
            Button1Text->SetText(FText::FromString(TEXT("Expand Search Area")));
            Button1Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            ActionButton1->AddChild(Button1Text);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created ActionButton1"));
    }
    
    // Button 2: Ignore and Return
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
        
        // Add button text
        UTextBlock* Button2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button2Text"));
        if (Button2Text)
        {
            Button2Text->SetText(FText::FromString(TEXT("Ignore and Return")));
            Button2Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            ActionButton2->AddChild(Button2Text);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created ActionButton2"));
    }
    
    // Button 3: Switch to Firefighting
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
        
        // Add button text
        UTextBlock* Button3Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button3Text"));
        if (Button3Text)
        {
            Button3Text->SetText(FText::FromString(TEXT("Switch to Firefighting")));
            Button3Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            ActionButton3->AddChild(Button3Text);
        }
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created ActionButton3"));
    }
    
    //=========================================================================
    // Input area (bottom)
    // Input box: (0, 360), Size: (700, 80)
    // Send button: (720, 380), Size: (80, 40)
    //=========================================================================
    
    // Input text box
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
        
        // Set hint text
        InputTextBox->SetHintText(FText::FromString(TEXT("Enter command or message...")));
        
        UE_LOG(LogMAEmergencyWidget, Log, TEXT("Created InputTextBox"));
    }
    
    // Send button
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
        
        // Add button text
        UTextBlock* SendText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SendText"));
        if (SendText)
        {
            SendText->SetText(FText::FromString(TEXT("Send")));
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
    
    // We don't need to create our own render target, instead use the camera sensor's render target directly
    // This avoids extra rendering overhead and ensures consistency with the camera sensor
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Camera render resources initialized (will use camera's render target)"));
}

void UMAEmergencyWidget::BindButtonEvents()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Binding button events..."));
    
    // Bind action button events
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
    
    // Bind send button event
    if (SendButton)
    {
        SendButton->OnClicked.AddDynamic(this, &UMAEmergencyWidget::OnSendButtonClicked);
    }
    
    // Bind input box enter key event
    if (InputTextBox)
    {
        InputTextBox->OnTextCommitted.AddDynamic(this, &UMAEmergencyWidget::OnInputTextCommitted);
    }
    
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Button events bound successfully"));
}

//=========================================================================
// Camera Source Management
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
    
    // Clear previous camera source first (if any, restore its settings)
    if (CurrentCameraSource.IsValid() && CurrentCameraSource.Get() != Camera)
    {
        USceneCaptureComponent2D* OldCapture = CurrentCameraSource->GetSceneCaptureComponent();
        if (OldCapture)
        {
            OldCapture->bCaptureEveryFrame = false;
        }
    }
    
    CurrentCameraSource = Camera;
    
    // Get camera's SceneCaptureComponent
    USceneCaptureComponent2D* SceneCapture = Camera->GetSceneCaptureComponent();
    if (!SceneCapture)
    {
        UE_LOG(LogMAEmergencyWidget, Warning, TEXT("SetCameraSource: SceneCaptureComponent is null"));
        ClearCameraSource();
        return;
    }
    
    // Enable per-frame capture to display live feed
    SceneCapture->bCaptureEveryFrame = true;
    
    // Get camera's render target
    UTextureRenderTarget2D* CameraRenderTarget = SceneCapture->TextureTarget;
    if (CameraRenderTarget && CameraFeedImage)
    {
        // Use render target as image source
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
    
    // Restore previous camera's settings
    if (CurrentCameraSource.IsValid())
    {
        USceneCaptureComponent2D* SceneCapture = CurrentCameraSource->GetSceneCaptureComponent();
        if (SceneCapture)
        {
            // Restore to not capture every frame (save performance)
            SceneCapture->bCaptureEveryFrame = false;
        }
    }
    
    CurrentCameraSource.Reset();
    
    // Set to black screen
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
// Info Display
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
// Input Control
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
// Event Handling
//=========================================================================

void UMAEmergencyWidget::OnActionButton1Clicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Action Button 1 clicked: Expand Search Area"));
    
    // Send button event message to backend
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
        {
            CommSubsystem->SendButtonEventMessage(
                TEXT("EmergencyWidget"),        // widget_name
                TEXT("btn_expand_search"),      // button_id
                TEXT("Expand Search Area")      // button_text
            );
        }
    }
    
    OnActionButtonClicked.Broadcast(0);
}

void UMAEmergencyWidget::OnActionButton2Clicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Action Button 2 clicked: Ignore and Return"));
    
    // Send button event message to backend
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
        {
            CommSubsystem->SendButtonEventMessage(
                TEXT("EmergencyWidget"),        // widget_name
                TEXT("btn_ignore_return"),      // button_id
                TEXT("Ignore and Return")       // button_text
            );
        }
    }
    
    OnActionButtonClicked.Broadcast(1);
}

void UMAEmergencyWidget::OnActionButton3Clicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Action Button 3 clicked: Switch to Firefighting"));
    
    // Send button event message to backend
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
        {
            CommSubsystem->SendButtonEventMessage(
                TEXT("EmergencyWidget"),        // widget_name
                TEXT("btn_switch_firefight"),   // button_id
                TEXT("Switch to Firefighting")  // button_text
            );
        }
    }
    
    OnActionButtonClicked.Broadcast(2);
}

void UMAEmergencyWidget::OnSendButtonClicked()
{
    UE_LOG(LogMAEmergencyWidget, Log, TEXT("Send button clicked"));
    
    // Send button event message to backend
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
        {
            CommSubsystem->SendButtonEventMessage(
                TEXT("EmergencyWidget"),    // widget_name
                TEXT("btn_send"),           // button_id
                TEXT("Send")                // button_text
            );
        }
    }
    
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
        
        // Send UI input message to backend
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
            {
                CommSubsystem->SendUIInputMessage(
                    TEXT("EmergencyWidget_InputBox"),   // input_source_id
                    Message                              // input_content
                );
            }
        }
        
        // Broadcast message sent event (for backward compatibility)
        OnMessageSent.Broadcast(Message);
        
        // Clear input box
        ClearInputBox();
        
        // Refocus input box
        FocusInputBox();
    }
    else
    {
        UE_LOG(LogMAEmergencyWidget, Warning, TEXT("Cannot submit empty message"));
    }
}