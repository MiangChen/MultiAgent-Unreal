// MATaskPlannerWidget.cpp
// Task Planner Workbench main container Widget - Pure C++ implementation

#include "MATaskPlannerWidget.h"
#include "MADAGCanvasWidget.h"
#include "MANodePaletteWidget.h"
#include "MATaskGraphModel.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../../Core/Comm/MACommSubsystem.h"
#include "../../Core/Manager/MATempDataManager.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogMATaskPlanner);

//=============================================================================
// Constructor
//=============================================================================

UMATaskPlannerWidget::UMATaskPlannerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Ensure WidgetTree exists
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// UUserWidget Overrides
//=============================================================================

void UMATaskPlannerWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // Build UI here, ensure WidgetTree is initialized
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMATaskPlannerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind button events
    if (UpdateGraphButton && !UpdateGraphButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnUpdateGraphButtonClicked))
    {
        UpdateGraphButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnUpdateGraphButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("UpdateGraphButton event bound"));
    }
    
    // Bind send command button event
    if (SendCommandButton && !SendCommandButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnSendCommandButtonClicked))
    {
        SendCommandButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnSendCommandButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("SendCommandButton event bound"));
    }
    
    // Bind submit task graph button event
    if (SubmitTaskGraphButton && !SubmitTaskGraphButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnSubmitTaskGraphButtonClicked))
    {
        SubmitTaskGraphButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnSubmitTaskGraphButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("SubmitTaskGraphButton event bound"));
    }

    // Bind close button event
    if (CloseButton && !CloseButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnCloseButtonClicked))
    {
        CloseButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnCloseButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("CloseButton event bound"));
    }

    // Bind data model event
    if (GraphModel && !GraphModel->OnDataChanged.IsAlreadyBound(this, &UMATaskPlannerWidget::OnModelDataChanged))
    {
        GraphModel->OnDataChanged.AddDynamic(this, &UMATaskPlannerWidget::OnModelDataChanged);
        UE_LOG(LogMATaskPlanner, Log, TEXT("GraphModel OnDataChanged event bound"));
    }
    
    // Bind node palette event
    if (NodePalette && !NodePalette->OnNodeTemplateSelected.IsAlreadyBound(this, &UMATaskPlannerWidget::OnNodeTemplateSelected))
    {
        NodePalette->OnNodeTemplateSelected.AddDynamic(this, &UMATaskPlannerWidget::OnNodeTemplateSelected);
        UE_LOG(LogMATaskPlanner, Log, TEXT("NodePalette OnNodeTemplateSelected event bound"));
    }
    
    // Bind canvas events
    if (DAGCanvas)
    {
        if (!DAGCanvas->OnNodeDeleteRequested.IsAlreadyBound(this, &UMATaskPlannerWidget::OnNodeDeleteRequested))
        {
            DAGCanvas->OnNodeDeleteRequested.AddDynamic(this, &UMATaskPlannerWidget::OnNodeDeleteRequested);
        }
        if (!DAGCanvas->OnNodeEditRequested.IsAlreadyBound(this, &UMATaskPlannerWidget::OnNodeEditRequested))
        {
            DAGCanvas->OnNodeEditRequested.AddDynamic(this, &UMATaskPlannerWidget::OnNodeEditRequested);
        }
        if (!DAGCanvas->OnEdgeCreated.IsAlreadyBound(this, &UMATaskPlannerWidget::OnEdgeCreated))
        {
            DAGCanvas->OnEdgeCreated.AddDynamic(this, &UMATaskPlannerWidget::OnEdgeCreated);
        }
        if (!DAGCanvas->OnEdgeDeleteRequested.IsAlreadyBound(this, &UMATaskPlannerWidget::OnEdgeDeleteRequested))
        {
            DAGCanvas->OnEdgeDeleteRequested.AddDynamic(this, &UMATaskPlannerWidget::OnEdgeDeleteRequested);
        }
        UE_LOG(LogMATaskPlanner, Log, TEXT("DAGCanvas events bound"));
    }
    
    // Initialize status log
    AppendStatusLog(TEXT("Task Planner Workbench started"));
    AppendStatusLog(TEXT("Tip: Drag nodes from the left toolbar to the canvas to create tasks"));
    
    // Try to load task graph from temp file (Requirement 4.1)
    LoadFromTempFile();
    
    // Bind TempDataManager data change event (Requirement 4.4)
    BindTempDataManagerEvents();
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("MATaskPlannerWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMATaskPlannerWidget::RebuildWidget()
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

FReply UMATaskPlannerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Intercept all mouse button events to prevent click-through to game scene
    // Let child widgets handle specific interaction logic
    FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    
    // Return Handled regardless of whether child widget processed it, to prevent event pass-through
    return FReply::Handled();
}

FReply UMATaskPlannerWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Intercept all mouse button release events
    FReply Reply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    return FReply::Handled();
}

FReply UMATaskPlannerWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Intercept mouse wheel events
    FReply Reply = Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    return FReply::Handled();
}

//=============================================================================
// UI Construction
//=============================================================================

void UMATaskPlannerWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("BuildUI: Starting UI construction..."));

    // Create data model
    GraphModel = NewObject<UMATaskGraphModel>(this, TEXT("GraphModel"));

    // Create root CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // Create semi-transparent background overlay (click blocker)
    UBorder* BackgroundOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundOverlay"));
    BackgroundOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));  // Semi-transparent black
    
    UCanvasPanelSlot* OverlaySlot = RootCanvas->AddChildToCanvas(BackgroundOverlay);
    OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    OverlaySlot->SetOffsets(FMargin(0.0f));

    // Create main background Border - windowed mode (80% width, 85% height, centered)
    UBorder* MainBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBackground"));
    MainBackground->SetBrushColor(BackgroundColor);
    MainBackground->SetPadding(FMargin(10.0f));
    
    // Apply rounded corners using brush
    FSlateBrush RoundedBrush;
    RoundedBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
    RoundedBrush.TintColor = FSlateColor(BackgroundColor);
    RoundedBrush.OutlineSettings.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
    RoundedBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    MainBackground->SetBrush(RoundedBrush);
    
    UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainBackground);
    // Windowed mode: centered with 80% width and 85% height (Requirements 6.7, 6.8)
    MainSlot->SetAnchors(FAnchors(0.1f, 0.075f, 0.9f, 0.925f));  // 10% margin on sides, 7.5% on top/bottom
    MainSlot->SetOffsets(FMargin(0.0f));

    // Create main vertical layout (title bar + content)
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    MainBackground->AddChild(MainVBox);

    // Create title bar with close button
    UHorizontalBox* TitleBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TitleBar"));
    UVerticalBoxSlot* TitleBarSlot = MainVBox->AddChildToVerticalBox(TitleBar);
    TitleBarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    TitleBarSlot->SetPadding(FMargin(0, 0, 0, 5));

    // Title text
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Task Planner Workbench")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    
    UHorizontalBoxSlot* TitleTextSlot = TitleBar->AddChildToHorizontalBox(TitleText);
    TitleTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TitleTextSlot->SetVerticalAlignment(VAlign_Center);

    // Hint text
    UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(TEXT("Press Z to close the window")));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 10;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.6f)));
    
    UHorizontalBoxSlot* HintSlot = TitleBar->AddChildToHorizontalBox(HintText);
    HintSlot->SetVerticalAlignment(VAlign_Center);
    HintSlot->SetPadding(FMargin(0, 0, 20, 0));

    // Close button (X) in top-right corner
    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
    
    // Set button style - transparent normal, semi-transparent red hover, darker red pressed
    FButtonStyle CloseButtonStyle;
    
    // Normal state - transparent
    FSlateBrush NormalBrush;
    NormalBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
    CloseButtonStyle.SetNormal(NormalBrush);
    
    // Hover state - semi-transparent red
    FSlateBrush HoverBrush;
    HoverBrush.TintColor = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f, 0.3f));
    CloseButtonStyle.SetHovered(HoverBrush);
    
    // Pressed state - darker red
    FSlateBrush PressedBrush;
    PressedBrush.TintColor = FSlateColor(FLinearColor(0.6f, 0.1f, 0.1f, 0.5f));
    CloseButtonStyle.SetPressed(PressedBrush);
    
    CloseButton->SetStyle(CloseButtonStyle);
    
    // Create X text
    UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
    CloseText->SetText(FText::FromString(TEXT("✕")));
    CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));
    FSlateFontInfo CloseFont = FCoreStyle::GetDefaultFontStyle("Regular", 18);
    CloseText->SetFont(CloseFont);
    CloseButton->AddChild(CloseText);
    
    UHorizontalBoxSlot* CloseSlot = TitleBar->AddChildToHorizontalBox(CloseButton);
    CloseSlot->SetVerticalAlignment(VAlign_Top);
    CloseSlot->SetHorizontalAlignment(HAlign_Right);

    // Create main horizontal layout
    UHorizontalBox* MainHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainHBox"));
    UVerticalBoxSlot* MainHBoxSlot = MainVBox->AddChildToVerticalBox(MainHBox);
    MainHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    // Create left panel
    UBorder* LeftPanel = CreateLeftPanel();
    UHorizontalBoxSlot* LeftSlot = MainHBox->AddChildToHorizontalBox(LeftPanel);
    LeftSlot->SetPadding(FMargin(0, 0, 5, 0));
    // Set left panel ratio (using FillSize)
    FSlateChildSize LeftSize(ESlateSizeRule::Fill);
    LeftSize.Value = 0.35f;
    LeftSlot->SetSize(LeftSize);

    // Create right panel
    UBorder* RightPanel = CreateRightPanel();
    UHorizontalBoxSlot* RightSlot = MainHBox->AddChildToHorizontalBox(RightPanel);
    // Set right panel ratio (using FillSize)
    FSlateChildSize RightSize(ESlateSizeRule::Fill);
    RightSize.Value = 0.65f;
    RightSlot->SetSize(RightSize);

    UE_LOG(LogMATaskPlanner, Log, TEXT("BuildUI: UI construction completed successfully (windowed mode)"));
}

UBorder* UMATaskPlannerWidget::CreateLeftPanel()
{
    // Left panel background
    UBorder* LeftPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanelBorder"));
    LeftPanelBorder->SetBrushColor(PanelBackgroundColor);
    LeftPanelBorder->SetPadding(FMargin(10.0f));

    // Vertical layout
    UVerticalBox* LeftVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVBox"));
    LeftPanelBorder->AddChild(LeftVBox);

    // Status log section - auto size based on content
    UVerticalBox* StatusLogSection = CreateStatusLogSection();
    UVerticalBoxSlot* StatusSlot = LeftVBox->AddChildToVerticalBox(StatusLogSection);
    FSlateChildSize StatusSize(ESlateSizeRule::Fill);
    StatusSize.Value = 0.22f;
    StatusSlot->SetSize(StatusSize);
    StatusSlot->SetPadding(FMargin(0, 0, 0, 3));

    // JSON editor section - takes most space
    UVerticalBox* JsonEditorSection = CreateJsonEditorSection();
    UVerticalBoxSlot* JsonSlot = LeftVBox->AddChildToVerticalBox(JsonEditorSection);
    FSlateChildSize JsonSize(ESlateSizeRule::Fill);
    JsonSize.Value = 0.58f;
    JsonSlot->SetSize(JsonSize);
    JsonSlot->SetPadding(FMargin(0, 0, 0, 3));

    // User command input section
    UVerticalBox* UserInputSection = CreateUserInputSection();
    UVerticalBoxSlot* UserInputSlot = LeftVBox->AddChildToVerticalBox(UserInputSection);
    FSlateChildSize UserInputSize(ESlateSizeRule::Fill);
    UserInputSize.Value = 0.2f;
    UserInputSlot->SetSize(UserInputSize);

    return LeftPanelBorder;
}

UVerticalBox* UMATaskPlannerWidget::CreateStatusLogSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusLogSection"));

    // Label - white color, font size 14
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusLogLabel"));
    Label->SetText(FText::FromString(TEXT("Status Log:")));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    Label->SetFont(LabelFont);
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 3));

    // Status log text box (read-only) - directly add without ScrollBox
    StatusLogBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("StatusLogBox"));
    StatusLogBox->SetIsReadOnly(true);
    StatusLogBox->SetText(FText::GetEmpty());
    
    // 设置文本样式：纯黑色，字号 12
    FEditableTextBoxStyle StatusLogStyle;
    FSlateColor BlackColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
    StatusLogStyle.SetForegroundColor(BlackColor);
    StatusLogStyle.SetFocusedForegroundColor(BlackColor);
    StatusLogStyle.SetReadOnlyForegroundColor(BlackColor);
    StatusLogStyle.TextStyle.ColorAndOpacity = BlackColor;
    FSlateFontInfo StatusLogFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    StatusLogStyle.SetFont(StatusLogFont);
    StatusLogBox->WidgetStyle = StatusLogStyle;
    
    // Add directly to section - let it fill available space
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(StatusLogBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    return Section;
}

UVerticalBox* UMATaskPlannerWidget::CreateJsonEditorSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JsonEditorSection"));

    // Label - white color, font size 14
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonEditorLabel"));
    Label->SetText(FText::FromString(TEXT("JSON Editor:")));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo LabelFont2 = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    Label->SetFont(LabelFont2);
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // JSON editor text box (editable)
    JsonEditorBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditorBox"));
    JsonEditorBox->SetIsReadOnly(false);
    JsonEditorBox->SetText(FText::FromString(TEXT("{\n  \"description\": \"\",\n  \"nodes\": [],\n  \"edges\": []\n}")));
    
    // 设置文本样式：纯黑色，字号 12
    FEditableTextBoxStyle JsonEditorStyle;
    FSlateColor BlackColor2 = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
    JsonEditorStyle.SetForegroundColor(BlackColor2);
    JsonEditorStyle.SetFocusedForegroundColor(BlackColor2);
    FSlateFontInfo JsonEditorFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    JsonEditorStyle.SetFont(JsonEditorFont);
    JsonEditorBox->WidgetStyle = JsonEditorStyle;
    
    // Use SizeBox to set minimum height
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditorSizeBox"));
    SizeBox->SetMinDesiredHeight(200.0f);
    SizeBox->AddChild(JsonEditorBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 8));

    // Horizontal box for buttons - both buttons in same row
    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
    
    // "Update Graph" button - smaller with font size 14
    UpdateGraphButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpdateGraphButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpdateButtonText"));
    ButtonText->SetText(FText::FromString(TEXT("Update Graph")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo UpdateBtnFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    ButtonText->SetFont(UpdateBtnFont);
    UpdateGraphButton->AddChild(ButtonText);
    
    UHorizontalBoxSlot* UpdateBtnSlot = ButtonRow->AddChildToHorizontalBox(UpdateGraphButton);
    UpdateBtnSlot->SetPadding(FMargin(0, 0, 10, 0));
    UpdateBtnSlot->SetVerticalAlignment(VAlign_Center);

    // "Submit Graph" button - smaller with font size 14
    SubmitTaskGraphButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SubmitTaskGraphButton"));

    UTextBlock* SubmitButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubmitButtonText"));
    SubmitButtonText->SetText(FText::FromString(TEXT("Submit Graph")));
    SubmitButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo SubmitBtnFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    SubmitButtonText->SetFont(SubmitBtnFont);
    SubmitTaskGraphButton->AddChild(SubmitButtonText);

    UHorizontalBoxSlot* SubmitBtnSlot = ButtonRow->AddChildToHorizontalBox(SubmitTaskGraphButton);
    SubmitBtnSlot->SetVerticalAlignment(VAlign_Center);

    UVerticalBoxSlot* ButtonRowSlot = Section->AddChildToVerticalBox(ButtonRow);
    ButtonRowSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UVerticalBox* UMATaskPlannerWidget::CreateUserInputSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UserInputSection"));

    // Label - white color, font size 14
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UserInputLabel"));
    Label->SetText(FText::FromString(TEXT("Command Input:")));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo LabelFont3 = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    Label->SetFont(LabelFont3);
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // User input text box (editable)
    UserInputBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("UserInputBox"));
    UserInputBox->SetIsReadOnly(false);
    UserInputBox->SetHintText(FText::FromString(TEXT("Enter natural language command, e.g.: Have the robot patrol...")));
    
    // 设置文本样式：纯黑色，字号 12
    FEditableTextBoxStyle UserInputStyle;
    FSlateColor BlackColor3 = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
    UserInputStyle.SetForegroundColor(BlackColor3);
    UserInputStyle.SetFocusedForegroundColor(BlackColor3);
    FSlateFontInfo UserInputFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    UserInputStyle.SetFont(UserInputFont);
    UserInputBox->WidgetStyle = UserInputStyle;
    
    // Use SizeBox to set minimum height
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("UserInputSizeBox"));
    SizeBox->SetMinDesiredHeight(60.0f);
    SizeBox->AddChild(UserInputBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 8));

    // "Send" button - blue background with white text (like the main UI Send button)
    SendCommandButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendCommandButton"));
    
    // Set blue background color
    FButtonStyle SendButtonStyle;
    FSlateBrush BlueBrush;
    BlueBrush.TintColor = FSlateColor(FLinearColor(0.2f, 0.4f, 0.8f, 1.0f));  // Blue color
    SendButtonStyle.SetNormal(BlueBrush);
    
    FSlateBrush BlueHoverBrush;
    BlueHoverBrush.TintColor = FSlateColor(FLinearColor(0.3f, 0.5f, 0.9f, 1.0f));  // Lighter blue on hover
    SendButtonStyle.SetHovered(BlueHoverBrush);
    
    FSlateBrush BluePressedBrush;
    BluePressedBrush.TintColor = FSlateColor(FLinearColor(0.15f, 0.35f, 0.7f, 1.0f));  // Darker blue when pressed
    SendButtonStyle.SetPressed(BluePressedBrush);
    
    SendCommandButton->SetStyle(SendButtonStyle);
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SendButtonText"));
    ButtonText->SetText(FText::FromString(TEXT("  Send  ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo SendBtnFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    ButtonText->SetFont(SendBtnFont);
    SendCommandButton->AddChild(ButtonText);
    
    UVerticalBoxSlot* ButtonSlot = Section->AddChildToVerticalBox(SendCommandButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UBorder* UMATaskPlannerWidget::CreateRightPanel()
{
    // Right panel background
    UBorder* RightPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBorder"));
    RightPanelBorder->SetBrushColor(PanelBackgroundColor);
    RightPanelBorder->SetPadding(FMargin(10.0f));

    // Horizontal layout (canvas + toolbar)
    UHorizontalBox* RightHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RightHBox"));
    RightPanelBorder->AddChild(RightHBox);

    // DAG canvas
    DAGCanvas = WidgetTree->ConstructWidget<UMADAGCanvasWidget>(UMADAGCanvasWidget::StaticClass(), TEXT("DAGCanvas"));
    DAGCanvas->BindToModel(GraphModel);
    
    UHorizontalBoxSlot* CanvasSlot = RightHBox->AddChildToHorizontalBox(DAGCanvas);
    CanvasSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CanvasSlot->SetPadding(FMargin(0, 0, 10, 0));

    // Node toolbar
    NodePalette = WidgetTree->ConstructWidget<UMANodePaletteWidget>(UMANodePaletteWidget::StaticClass(), TEXT("NodePalette"));
    NodePalette->InitializeTemplates();
    
    UHorizontalBoxSlot* PaletteSlot = RightHBox->AddChildToHorizontalBox(NodePalette);
    PaletteSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    return RightPanelBorder;
}

//=============================================================================
// Public Interface
//=============================================================================

void UMATaskPlannerWidget::LoadTaskGraph(const FMATaskGraphData& Data)
{
    if (!GraphModel)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("LoadTaskGraph: GraphModel is null!"));
        return;
    }
    
    GraphModel->LoadFromData(Data);
    SyncJsonEditorFromModel();
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(FString::Printf(TEXT("Task graph loaded: %d nodes, %d edges"),
        Data.Nodes.Num(), Data.Edges.Num()));
    
    OnTaskGraphChanged.Broadcast(Data);
}

bool UMATaskPlannerWidget::LoadTaskGraphFromJson(const FString& JsonString)
{
    if (!GraphModel)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("LoadTaskGraphFromJson: GraphModel is null!"));
        return false;
    }
    
    FString ErrorMessage;
    if (!GraphModel->LoadFromJsonWithError(JsonString, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] JSON parse failed: %s"), *ErrorMessage));
        return false;
    }
    
    SyncJsonEditorFromModel();
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    FMATaskGraphData Data = GraphModel->GetWorkingData();
    AppendStatusLog(FString::Printf(TEXT("Task graph loaded: %d nodes, %d edges"),
        Data.Nodes.Num(), Data.Edges.Num()));
    
    OnTaskGraphChanged.Broadcast(Data);
    return true;
}

void UMATaskPlannerWidget::AppendStatusLog(const FString& Message)
{
    if (!StatusLogBox)
    {
        return;
    }
    
    FString Timestamp = GetTimestamp();
    FString CurrentText = StatusLogBox->GetText().ToString();
    FString NewLine = FString::Printf(TEXT("[%s] %s"), *Timestamp, *Message);
    
    if (CurrentText.IsEmpty())
    {
        StatusLogBox->SetText(FText::FromString(NewLine));
    }
    else
    {
        StatusLogBox->SetText(FText::FromString(CurrentText + TEXT("\n") + NewLine));
    }
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("StatusLog: %s"), *Message);
}

void UMATaskPlannerWidget::ClearStatusLog()
{
    if (StatusLogBox)
    {
        StatusLogBox->SetText(FText::GetEmpty());
    }
}

FString UMATaskPlannerWidget::GetJsonText() const
{
    if (JsonEditorBox)
    {
        return JsonEditorBox->GetText().ToString();
    }
    return FString();
}

void UMATaskPlannerWidget::SetJsonText(const FString& JsonText)
{
    if (JsonEditorBox)
    {
        JsonEditorBox->SetText(FText::FromString(JsonText));
    }
}

void UMATaskPlannerWidget::FocusJsonEditor()
{
    if (JsonEditorBox)
    {
        JsonEditorBox->SetKeyboardFocus();
    }
}

//=============================================================================
// Event Handling
//=============================================================================

void UMATaskPlannerWidget::OnUpdateGraphButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("UpdateGraphButton clicked"));
    
    FString JsonText = GetJsonText();
    
    if (JsonText.IsEmpty())
    {
        AppendStatusLog(TEXT("[Warning] JSON editor is empty"));
        return;
    }
    
    FString ErrorMessage;
    if (!GraphModel->LoadFromJsonWithError(JsonText, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] JSON parse failed: %s"), *ErrorMessage));
        return;
    }
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(TEXT("[Success] Task graph updated"));
    
    // Save to temp file (Requirement 4.3)
    SaveToTempFile();
    
    OnTaskGraphChanged.Broadcast(GraphModel->GetWorkingData());
}

void UMATaskPlannerWidget::OnSendCommandButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("SendCommandButton clicked"));
    
    if (!UserInputBox)
    {
        return;
    }
    
    FString Command = UserInputBox->GetText().ToString().TrimStartAndEnd();
    
    if (Command.IsEmpty())
    {
        AppendStatusLog(TEXT("[Warning] Please enter a command"));
        return;
    }
    
    // Log the sent command
    AppendStatusLog(FString::Printf(TEXT("[Sent] %s"), *Command));
    
    // Clear input box
    UserInputBox->SetText(FText::GetEmpty());
    
    // Broadcast command submitted event (forwarded to backend by HUD or other components)
    OnCommandSubmitted.Broadcast(Command);
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("Command submitted: %s"), *Command);
}

void UMATaskPlannerWidget::OnSubmitTaskGraphButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("SubmitTaskGraphButton clicked"));

    if (!GraphModel)
    {
        AppendStatusLog(TEXT("[Error] Task graph model not initialized"));
        return;
    }

    // Get task graph JSON
    FString TaskGraphJson = GraphModel->ToJson();

    if (TaskGraphJson.IsEmpty())
    {
        AppendStatusLog(TEXT("[Warning] Task graph is empty"));
        return;
    }

    // Get CommSubsystem and send task graph
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        AppendStatusLog(TEXT("[Error] Unable to get GameInstance"));
        return;
    }

    UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
    if (!CommSubsystem)
    {
        AppendStatusLog(TEXT("[Error] Unable to get communication subsystem"));
        return;
    }

    // Send task graph to backend
    CommSubsystem->SendTaskGraphSubmitMessage(TaskGraphJson);

    // Log
    FMATaskGraphData Data = GraphModel->GetWorkingData();
    AppendStatusLog(FString::Printf(TEXT("[Submitted] Task graph sent: %d nodes, %d edges"),
        Data.Nodes.Num(), Data.Edges.Num()));

    UE_LOG(LogMATaskPlanner, Log, TEXT("Task graph submitted: %s"), *TaskGraphJson);
}

void UMATaskPlannerWidget::OnModelDataChanged()
{
    // Sync to JSON editor in real-time during canvas operations
    SyncJsonEditorFromModel();
}

void UMATaskPlannerWidget::OnNodeTemplateSelected(const FMANodeTemplate& Template)
{
    if (!GraphModel)
    {
        return;
    }
    
    // Create new node
    FString NewNodeId = GraphModel->CreateNode(Template.DefaultDescription, Template.DefaultLocation);
    
    AppendStatusLog(FString::Printf(TEXT("Node created: %s (%s)"), *NewNodeId, *Template.TemplateName));
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    // Save to temp file (Requirement 4.3)
    SaveToTempFile();
}

void UMATaskPlannerWidget::OnNodeDeleteRequested(const FString& NodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    if (GraphModel->RemoveNode(NodeId))
    {
        AppendStatusLog(FString::Printf(TEXT("Node deleted: %s"), *NodeId));
        
        if (DAGCanvas)
        {
            DAGCanvas->RefreshFromModel();
        }
        
        // Save to temp file (Requirement 4.3)
        SaveToTempFile();
    }
}

void UMATaskPlannerWidget::OnNodeEditRequested(const FString& NodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    // Get node data
    FMATaskNodeData NodeData;
    if (!GraphModel->FindNode(NodeId, NodeData))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Node not found: %s"), *NodeId));
        return;
    }
    
    // TODO: Show node edit dialog
    // Currently just display node info in status log
    AppendStatusLog(FString::Printf(TEXT("Edit node: %s"), *NodeId));
    AppendStatusLog(FString::Printf(TEXT("  Description: %s"), *NodeData.Description));
    AppendStatusLog(FString::Printf(TEXT("  Location: %s"), *NodeData.Location));
    
    // Select this node
    if (DAGCanvas)
    {
        DAGCanvas->SelectNode(NodeId);
    }
}

void UMATaskPlannerWidget::OnEdgeCreated(const FString& FromNodeId, const FString& ToNodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    if (GraphModel->AddEdge(FromNodeId, ToNodeId))
    {
        AppendStatusLog(FString::Printf(TEXT("Edge created: %s -> %s"), *FromNodeId, *ToNodeId));
        
        // Save to temp file (Requirement 4.3)
        SaveToTempFile();
    }
}

void UMATaskPlannerWidget::OnEdgeDeleteRequested(const FString& FromNodeId, const FString& ToNodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    if (GraphModel->RemoveEdge(FromNodeId, ToNodeId))
    {
        AppendStatusLog(FString::Printf(TEXT("Edge deleted: %s -> %s"), *FromNodeId, *ToNodeId));
        
        if (DAGCanvas)
        {
            DAGCanvas->RefreshFromModel();
        }
        
        // Save to temp file (Requirement 4.3)
        SaveToTempFile();
    }
}

void UMATaskPlannerWidget::OnCloseButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("Close button clicked"));
    
    // Hide the widget (same as pressing Z key)
    SetVisibility(ESlateVisibility::Collapsed);
}

//=============================================================================
// Helper Methods
//=============================================================================

void UMATaskPlannerWidget::SyncJsonEditorFromModel()
{
    if (!GraphModel || !JsonEditorBox)
    {
        return;
    }
    
    FString JsonText = GraphModel->ToJson();
    JsonEditorBox->SetText(FText::FromString(JsonText));
}

FString UMATaskPlannerWidget::GetTimestamp() const
{
    FDateTime Now = FDateTime::Now();
    return FString::Printf(TEXT("%02d:%02d:%02d"), Now.GetHour(), Now.GetMinute(), Now.GetSecond());
}

bool UMATaskPlannerWidget::LoadMockData()
{
    // Build mock data file path
    FString FilePath = FPaths::ProjectDir() / TEXT("datasets/response_example.json");
    
    // Check if file exists
    if (!FPaths::FileExists(FilePath))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Mock data file not found: %s"), *FilePath));
        UE_LOG(LogMATaskPlanner, Warning, TEXT("Mock data file not found: %s"), *FilePath);
        return false;
    }
    
    // Read file content
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Unable to read file: %s"), *FilePath));
        UE_LOG(LogMATaskPlanner, Error, TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }
    
    // Parse JSON (using response_example.json format)
    FMATaskGraphData Data;
    FString ErrorMessage;
    if (!FMATaskGraphData::FromResponseJson(JsonContent, Data, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Mock data format error: %s"), *ErrorMessage));
        UE_LOG(LogMATaskPlanner, Error, TEXT("Failed to parse mock data: %s"), *ErrorMessage);
        return false;
    }
    
    // Load data
    LoadTaskGraph(Data);
    AppendStatusLog(TEXT("[Success] Mock data loaded"));
    
    return true;
}

//=============================================================================
// TempDataManager Integration
//=============================================================================

void UMATaskPlannerWidget::LoadFromTempFile()
{
    // Get TempDataManager
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("LoadFromTempFile: Unable to get GameInstance"));
        return;
    }

    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("LoadFromTempFile: Unable to get TempDataManager"));
        return;
    }

    // Check if temp file exists
    if (!TempDataMgr->TaskGraphFileExists())
    {
        UE_LOG(LogMATaskPlanner, Log, TEXT("LoadFromTempFile: Temp file does not exist, skipping load"));
        return;
    }

    // Load task graph from temp file
    FMATaskGraphData Data;
    if (TempDataMgr->LoadTaskGraph(Data))
    {
        // Load data to model without broadcasting (to avoid recursive save)
        if (GraphModel)
        {
            GraphModel->LoadFromData(Data);
            SyncJsonEditorFromModel();
            
            if (DAGCanvas)
            {
                DAGCanvas->RefreshFromModel();
            }
            
            AppendStatusLog(FString::Printf(TEXT("[Loaded] Task graph from temp file: %d nodes, %d edges"),
                Data.Nodes.Num(), Data.Edges.Num()));
        }
    }
    else
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("LoadFromTempFile: Failed to load task graph from temp file"));
    }
}

void UMATaskPlannerWidget::BindTempDataManagerEvents()
{
    // Get TempDataManager
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("BindTempDataManagerEvents: Unable to get GameInstance"));
        return;
    }

    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("BindTempDataManagerEvents: Unable to get TempDataManager"));
        return;
    }

    // Bind task graph data change event
    if (!TempDataMgr->OnTaskGraphChanged.IsAlreadyBound(this, &UMATaskPlannerWidget::OnTempDataTaskGraphChanged))
    {
        TempDataMgr->OnTaskGraphChanged.AddDynamic(this, &UMATaskPlannerWidget::OnTempDataTaskGraphChanged);
        UE_LOG(LogMATaskPlanner, Log, TEXT("BindTempDataManagerEvents: Bound OnTaskGraphChanged event"));
    }
}

void UMATaskPlannerWidget::OnTempDataTaskGraphChanged(const FMATaskGraphData& NewData)
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("OnTempDataTaskGraphChanged: Received data update (Nodes: %d, Edges: %d)"),
        NewData.Nodes.Num(), NewData.Edges.Num());

    // Load data to model
    if (GraphModel)
    {
        GraphModel->LoadFromData(NewData);
        SyncJsonEditorFromModel();
        
        if (DAGCanvas)
        {
            DAGCanvas->RefreshFromModel();
        }
        
        AppendStatusLog(FString::Printf(TEXT("[Updated] Task graph refreshed: %d nodes, %d edges"),
            NewData.Nodes.Num(), NewData.Edges.Num()));
    }
}

void UMATaskPlannerWidget::SaveToTempFile()
{
    if (!GraphModel)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("SaveToTempFile: GraphModel is null"));
        return;
    }

    // Get TempDataManager
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("SaveToTempFile: Unable to get GameInstance"));
        return;
    }

    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("SaveToTempFile: Unable to get TempDataManager"));
        return;
    }

    // Get current data from model
    FMATaskGraphData Data = GraphModel->GetWorkingData();

    // Save to temp file
    if (TempDataMgr->SaveTaskGraph(Data))
    {
        UE_LOG(LogMATaskPlanner, Log, TEXT("SaveToTempFile: Successfully saved task graph to temp file"));
    }
    else
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("SaveToTempFile: Failed to save task graph to temp file"));
        AppendStatusLog(TEXT("[Error] Failed to save task graph to temp file"));
    }
}
