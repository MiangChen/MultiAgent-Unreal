// MATaskPlannerWidget.cpp
// Task Planner Workbench main container Widget

#include "MATaskPlannerWidget.h"
#include "MADAGCanvasWidget.h"
#include "MANodePaletteWidget.h"
#include "../MATaskGraphModel.h"
#include "../Application/MATaskPlannerCoordinator.h"
#include "../Infrastructure/MATaskPlannerRuntimeAdapter.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "../../Core/MAFrostedGlassUtils.h"
#include "../../Core/MAUITheme.h"
#include "../../Components/Presentation/MAStyledButton.h"
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
#include "Components/BackgroundBlur.h"
#include "Blueprint/WidgetTree.h"
#include "Misc/DateTime.h"
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
    
    // Bind submit task graph button event (deprecated)
    if (SubmitTaskGraphButton && !SubmitTaskGraphButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnSubmitTaskGraphButtonClicked))
    {
        SubmitTaskGraphButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnSubmitTaskGraphButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("SubmitTaskGraphButton event bound (deprecated)"));
    }

    // Bind save button event
    if (SaveButton && !SaveButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnSaveButtonClicked))
    {
        SaveButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnSaveButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("SaveButton event bound"));
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
    AppendStatusLog(TEXT("Task Decomposition Workbench started"));
    AppendStatusLog(TEXT("Tip: Drag nodes from the left toolbar to the canvas to create tasks"));
    
    // Try to load task graph from runtime storage.
    LoadRuntimeTaskGraph();
    
    // Bind runtime task graph change events.
    BindRuntimeEvents();
    
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

    // 在构建 UI 之前先从主题获取颜色
    if (!Theme)
    {
        // 创建默认主题作为 fallback
        Theme = NewObject<UMAUITheme>();
    }
    BackgroundColor = Theme->BackgroundColor;
    PanelBackgroundColor = Theme->CanvasBackgroundColor;

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
    BackgroundOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundOverlay"));
    FLinearColor OverlayBgColor = Theme ? Theme->OverlayColor : FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    BackgroundOverlay->SetBrushColor(OverlayBgColor);
    
    UCanvasPanelSlot* OverlaySlot = RootCanvas->AddChildToCanvas(BackgroundOverlay);
    OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    OverlaySlot->SetOffsets(FMargin(0.0f));

    // === 使用 MAFrostedGlassUtils 创建毛玻璃效果容器 ===
    FMAFrostedGlassResult FrostedGlass = MAFrostedGlassUtils::CreateFromTheme(
        WidgetTree,
        RootCanvas,
        Theme,
        FAnchors(0.1f, 0.075f, 0.9f, 0.925f),  // 窗口位置：居中 80% 宽度，85% 高度
        FMargin(0.0f),
        TEXT("TaskPlanner")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("BuildUI: Failed to create frosted glass container!"));
        return;
    }

    // Create main vertical layout (title bar + content)
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    FrostedGlass.ContentContainer->AddChild(MainVBox);

    // Create title bar with close button
    UHorizontalBox* TitleBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TitleBar"));
    UVerticalBoxSlot* TitleBarSlot = MainVBox->AddChildToVerticalBox(TitleBar);
    TitleBarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    TitleBarSlot->SetPadding(FMargin(0, 0, 0, 5));

    // Title text
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Task Decomposition Workbench")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    // 标题在毛玻璃背景上，使用深色文字
    FLinearColor TitleTextColor = Theme ? Theme->TextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
    TitleText->SetColorAndOpacity(FSlateColor(TitleTextColor));
    
    UHorizontalBoxSlot* TitleTextSlot = TitleBar->AddChildToHorizontalBox(TitleText);
    TitleTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TitleTextSlot->SetVerticalAlignment(VAlign_Center);

    // Hint text
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(TEXT("Press Z to close the window")));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 10;
    HintText->SetFont(HintFont);
    FLinearColor HintTextColor = Theme ? Theme->HintTextColor : FLinearColor(0.5f, 0.5f, 0.6f);
    HintText->SetColorAndOpacity(FSlateColor(HintTextColor));
    
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
    
    // Hover state - semi-transparent red (use Theme->DangerColor)
    FLinearColor DangerCol = Theme ? Theme->DangerColor : FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);
    FSlateBrush HoverBrush;
    HoverBrush.TintColor = FSlateColor(FLinearColor(DangerCol.R, DangerCol.G, DangerCol.B, 0.3f));
    CloseButtonStyle.SetHovered(HoverBrush);
    
    // Pressed state - darker red
    FSlateBrush PressedBrush;
    PressedBrush.TintColor = FSlateColor(FLinearColor(DangerCol.R * 0.75f, DangerCol.G * 0.5f, DangerCol.B * 0.5f, 0.5f));
    CloseButtonStyle.SetPressed(PressedBrush);
    
    CloseButton->SetStyle(CloseButtonStyle);
    
    // Create X text
    CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
    CloseText->SetText(FText::FromString(TEXT("✕")));
    FLinearColor CloseTextColor = Theme ? Theme->SecondaryTextColor : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
    CloseText->SetColorAndOpacity(FSlateColor(CloseTextColor));
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
    LeftPanelBorder->SetPadding(FMargin(10.0f));
    
    // Apply rounded corners using MARoundedBorderUtils
    MARoundedBorderUtils::ApplyRoundedCorners(LeftPanelBorder, PanelBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);

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

    // Label - use Theme->LabelTextColor with fallback
    StatusLogLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusLogLabel"));
    StatusLogLabel->SetText(FText::FromString(TEXT("Status Log:")));
    FLinearColor LabelTextColor = Theme ? Theme->LabelTextColor : FLinearColor::White;
    StatusLogLabel->SetColorAndOpacity(FSlateColor(LabelTextColor));
    FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    StatusLogLabel->SetFont(LabelFont);
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(StatusLogLabel);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 3));

    // Status log text box (read-only) - wrapped in rounded border
    StatusLogBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("StatusLogBox"));
    StatusLogBox->SetIsReadOnly(true);
    StatusLogBox->SetText(FText::GetEmpty());
    
    // 设置文本样式：使用主题输入文字颜色，字号 12，透明背景
    FEditableTextBoxStyle StatusLogStyle;
    FSlateColor StatusLogSlateColor = FSlateColor(Theme ? Theme->InputTextColor : FLinearColor::White);
    StatusLogStyle.SetForegroundColor(StatusLogSlateColor);
    StatusLogStyle.SetFocusedForegroundColor(StatusLogSlateColor);
    StatusLogStyle.SetReadOnlyForegroundColor(StatusLogSlateColor);
    StatusLogStyle.TextStyle.ColorAndOpacity = StatusLogSlateColor;
    FSlateFontInfo StatusLogFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    StatusLogStyle.SetFont(StatusLogFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush StatusLogTransparentBrush;
    StatusLogTransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);
    StatusLogStyle.SetBackgroundImageNormal(StatusLogTransparentBrush);
    StatusLogStyle.SetBackgroundImageHovered(StatusLogTransparentBrush);
    StatusLogStyle.SetBackgroundImageFocused(StatusLogTransparentBrush);
    StatusLogStyle.SetBackgroundImageReadOnly(StatusLogTransparentBrush);
    
    StatusLogBox->WidgetStyle = StatusLogStyle;
    
    // 创建圆角 Border 包装文本框
    StatusLogBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StatusLogBorder"));
    StatusLogBorder->SetPadding(FMargin(8.0f, 4.0f));
    
    // 应用圆角效果 - 只读文本框使用主题输入框背景色
    FLinearColor StatusLogBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(0.22f, 0.22f, 0.22f, 0.7f);
    MARoundedBorderUtils::ApplyRoundedCorners(StatusLogBorder, StatusLogBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    StatusLogBorder->AddChild(StatusLogBox);
    
    // Add border to section - let it fill available space
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(StatusLogBorder);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    return Section;
}

UVerticalBox* UMATaskPlannerWidget::CreateJsonEditorSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JsonEditorSection"));

    // Label - use Theme->LabelTextColor with fallback
    JsonEditorLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonEditorLabel"));
    JsonEditorLabel->SetText(FText::FromString(TEXT("JSON Editor:")));
    FLinearColor LabelTextColor = Theme ? Theme->LabelTextColor : FLinearColor::White;
    JsonEditorLabel->SetColorAndOpacity(FSlateColor(LabelTextColor));
    FSlateFontInfo LabelFont2 = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    JsonEditorLabel->SetFont(LabelFont2);
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(JsonEditorLabel);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // JSON editor text box (editable) - wrapped in rounded border
    JsonEditorBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditorBox"));
    JsonEditorBox->SetIsReadOnly(false);
    JsonEditorBox->SetText(FText::FromString(TEXT("{\n  \"description\": \"\",\n  \"nodes\": [],\n  \"edges\": []\n}")));
    
    // 设置文本样式：使用主题输入文字颜色，字号 12，透明背景
    FEditableTextBoxStyle JsonEditorStyle;
    FSlateColor JsonEditorSlateColor = FSlateColor(Theme ? Theme->InputTextColor : FLinearColor::White);
    JsonEditorStyle.SetForegroundColor(JsonEditorSlateColor);
    JsonEditorStyle.SetFocusedForegroundColor(JsonEditorSlateColor);
    FSlateFontInfo JsonEditorFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    JsonEditorStyle.SetFont(JsonEditorFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush JsonEditorTransparentBrush;
    JsonEditorTransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);
    JsonEditorStyle.SetBackgroundImageNormal(JsonEditorTransparentBrush);
    JsonEditorStyle.SetBackgroundImageHovered(JsonEditorTransparentBrush);
    JsonEditorStyle.SetBackgroundImageFocused(JsonEditorTransparentBrush);
    JsonEditorStyle.SetBackgroundImageReadOnly(JsonEditorTransparentBrush);
    
    JsonEditorBox->WidgetStyle = JsonEditorStyle;
    
    // 创建圆角 Border 包装文本框
    JsonEditorBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonEditorBorder"));
    JsonEditorBorder->SetPadding(FMargin(8.0f, 4.0f));
    
    // 应用圆角效果 - 可编辑文本框使用主题输入框背景色
    FLinearColor JsonEditorBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(JsonEditorBorder, JsonEditorBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    JsonEditorBorder->AddChild(JsonEditorBox);
    
    // Use SizeBox to set minimum height
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditorSizeBox"));
    SizeBox->SetMinDesiredHeight(200.0f);
    SizeBox->AddChild(JsonEditorBorder);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 8));

    // Horizontal box for buttons - both buttons in same row
    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
    
    // "Update" button - 使用 MAStyledButton，与 Save 按钮样式一致
    UpdateGraphButton = WidgetTree->ConstructWidget<UMAStyledButton>(UMAStyledButton::StaticClass(), TEXT("UpdateGraphButton"));
    UpdateGraphButton->SetButtonText(FText::FromString(TEXT("Update")));
    UpdateGraphButton->SetButtonStyle(EMAButtonStyle::Secondary);
    
    UHorizontalBoxSlot* UpdateBtnSlot = ButtonRow->AddChildToHorizontalBox(UpdateGraphButton);
    UpdateBtnSlot->SetPadding(FMargin(0, 0, 10, 0));
    UpdateBtnSlot->SetVerticalAlignment(VAlign_Center);

    // "Save" button - 黄色保存按钮，使用 MAStyledButton
    SaveButton = WidgetTree->ConstructWidget<UMAStyledButton>(UMAStyledButton::StaticClass(), TEXT("SaveButton"));
    SaveButton->SetButtonText(FText::FromString(TEXT("Save")));
    SaveButton->SetButtonStyle(EMAButtonStyle::Success);

    UHorizontalBoxSlot* SaveBtnSlot = ButtonRow->AddChildToHorizontalBox(SaveButton);
    SaveBtnSlot->SetVerticalAlignment(VAlign_Center);

    UVerticalBoxSlot* ButtonRowSlot = Section->AddChildToVerticalBox(ButtonRow);
    ButtonRowSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UVerticalBox* UMATaskPlannerWidget::CreateUserInputSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UserInputSection"));

    // Label - use Theme->LabelTextColor with fallback
    UserInputLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UserInputLabel"));
    UserInputLabel->SetText(FText::FromString(TEXT("Instruction:")));
    FLinearColor LabelTextColor = Theme ? Theme->LabelTextColor : FLinearColor::White;
    UserInputLabel->SetColorAndOpacity(FSlateColor(LabelTextColor));
    FSlateFontInfo LabelFont3 = FCoreStyle::GetDefaultFontStyle("Bold", 14);
    UserInputLabel->SetFont(LabelFont3);
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(UserInputLabel);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // User input text box (editable) - wrapped in rounded border
    UserInputBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("UserInputBox"));
    UserInputBox->SetIsReadOnly(false);
    UserInputBox->SetHintText(FText::FromString(TEXT("Enter natural language command, e.g.: Have the robot patrol...")));
    
    // 设置文本样式：使用主题输入文字颜色，字号 12，透明背景
    FEditableTextBoxStyle UserInputStyle;
    FSlateColor UserInputSlateColor = FSlateColor(Theme ? Theme->InputTextColor : FLinearColor::White);
    UserInputStyle.SetForegroundColor(UserInputSlateColor);
    UserInputStyle.SetFocusedForegroundColor(UserInputSlateColor);
    FSlateFontInfo UserInputFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    UserInputStyle.SetFont(UserInputFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush TransparentBrush;
    TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);
    UserInputStyle.SetBackgroundImageNormal(TransparentBrush);
    UserInputStyle.SetBackgroundImageHovered(TransparentBrush);
    UserInputStyle.SetBackgroundImageFocused(TransparentBrush);
    UserInputStyle.SetBackgroundImageReadOnly(TransparentBrush);
    
    UserInputBox->WidgetStyle = UserInputStyle;
    
    // 创建圆角 Border 包装文本框
    UserInputBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UserInputBorder"));
    UserInputBorder->SetPadding(FMargin(8.0f, 4.0f));  // 内边距
    
    // 应用圆角效果 - 使用主题输入框背景色
    FLinearColor TextBoxBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(UserInputBorder, TextBoxBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    UserInputBorder->AddChild(UserInputBox);
    
    // Use SizeBox to set minimum height
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("UserInputSizeBox"));
    SizeBox->SetMinDesiredHeight(60.0f);
    SizeBox->AddChild(UserInputBorder);  // 改为添加 Border 而不是直接添加 TextBox
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 8));

    // "Send" button - using MAStyledButton for rounded corners
    SendCommandButton = WidgetTree->ConstructWidget<UMAStyledButton>(UMAStyledButton::StaticClass(), TEXT("SendCommandButton"));
    SendCommandButton->SetButtonText(FText::FromString(TEXT("Send")));
    SendCommandButton->SetButtonStyle(EMAButtonStyle::Primary);
    
    UVerticalBoxSlot* ButtonSlot = Section->AddChildToVerticalBox(SendCommandButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UBorder* UMATaskPlannerWidget::CreateRightPanel()
{
    // Right panel background
    UBorder* RightPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBorder"));
    RightPanelBorder->SetPadding(FMargin(10.0f));
    
    // Apply rounded corners using MARoundedBorderUtils
    MARoundedBorderUtils::ApplyRoundedCorners(RightPanelBorder, PanelBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);

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
    const FMATaskPlannerActionResult Result = FMATaskPlannerCoordinator::LoadGraphJson(
        GraphModel,
        JsonString,
        TEXT("[Warning] JSON editor is empty"),
        TEXT("Task graph loaded: "));
    ApplyActionResult(Result);
    return Result.bSuccess;
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

    const FMATaskPlannerActionResult Result = FMATaskPlannerCoordinator::LoadGraphJson(
        GraphModel,
        GetJsonText(),
        TEXT("[Warning] JSON editor is empty"),
        TEXT("[Success] Task graph updated: "));
    ApplyActionResult(Result);
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

    FString RuntimeError;
    if (!FMATaskPlannerRuntimeAdapter::TrySubmitTaskGraph(this, TaskGraphJson, RuntimeError))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] %s"), *RuntimeError));
        return;
    }

    // Log
    FMATaskGraphData Data = GraphModel->GetWorkingData();
    AppendStatusLog(FString::Printf(TEXT("[Submitted] Task graph sent: %d nodes, %d edges"),
        Data.Nodes.Num(), Data.Edges.Num()));

    UE_LOG(LogMATaskPlanner, Log, TEXT("Task graph submitted: %s"), *TaskGraphJson);
}

void UMATaskPlannerWidget::OnSaveButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("SaveButton clicked"));
    SaveAndNavigateToModal();
}

void UMATaskPlannerWidget::SaveAndNavigateToModal()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("SaveAndNavigateToModal: Saving data and navigating to modal"));

    if (!GraphModel)
    {
        AppendStatusLog(TEXT("[Error] Cannot save data: TaskGraphModel unavailable"));
        return;
    }

    FString RuntimeError;
    if (!PersistTaskGraph(&RuntimeError))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Cannot save data: %s"), *RuntimeError));
        return;
    }

    AppendStatusLog(TEXT("[Save] Task graph saved"));

    if (!FMATaskPlannerRuntimeAdapter::TryNavigateToTaskGraphModal(this, RuntimeError))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Cannot navigate: %s"), *RuntimeError));
        return;
    }

    UE_LOG(LogMATaskPlanner, Log, TEXT("SaveAndNavigateToModal: Navigation to TaskGraphModal initiated"));
}

void UMATaskPlannerWidget::OnModelDataChanged()
{
    // Sync to JSON editor in real-time during canvas operations
    SyncJsonEditorFromModel();
}

void UMATaskPlannerWidget::OnNodeTemplateSelected(const FMANodeTemplate& Template)
{
    ApplyActionResult(FMATaskPlannerCoordinator::AddNodeFromTemplate(GraphModel, Template));
}

void UMATaskPlannerWidget::OnNodeDeleteRequested(const FString& NodeId)
{
    ApplyActionResult(FMATaskPlannerCoordinator::DeleteNode(GraphModel, NodeId));
}

void UMATaskPlannerWidget::OnNodeEditRequested(const FString& NodeId)
{
    ApplyActionResult(FMATaskPlannerCoordinator::DescribeNode(GraphModel, NodeId));
    
    // Select this node
    if (DAGCanvas)
    {
        DAGCanvas->SelectNode(NodeId);
    }
}

void UMATaskPlannerWidget::OnEdgeCreated(const FString& FromNodeId, const FString& ToNodeId)
{
    ApplyActionResult(FMATaskPlannerCoordinator::AddEdge(GraphModel, FromNodeId, ToNodeId));
}

void UMATaskPlannerWidget::OnEdgeDeleteRequested(const FString& FromNodeId, const FString& ToNodeId)
{
    ApplyActionResult(FMATaskPlannerCoordinator::DeleteEdge(GraphModel, FromNodeId, ToNodeId));
}

void UMATaskPlannerWidget::OnCloseButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("Close button clicked"));

    FString RuntimeError;
    if (FMATaskPlannerRuntimeAdapter::TryHideTaskPlanner(this, RuntimeError))
    {
        UE_LOG(LogMATaskPlanner, Log, TEXT("TaskPlannerWidget hidden via UIManager"));
        return;
    }

    UE_LOG(LogMATaskPlanner, Warning, TEXT("OnCloseButtonClicked: %s"), *RuntimeError);
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
    const FMATaskPlannerActionResult Result = FMATaskPlannerCoordinator::LoadMockData(GraphModel, FPaths::ProjectDir());
    ApplyActionResult(Result);
    return Result.bSuccess;
}

//=============================================================================
// Runtime Integration
//=============================================================================

void UMATaskPlannerWidget::LoadRuntimeTaskGraph()
{
    FMATaskGraphData Data;
    FString RuntimeError;
    if (!FMATaskPlannerRuntimeAdapter::TryLoadTaskGraph(this, Data, RuntimeError))
    {
        UE_LOG(LogMATaskPlanner, Verbose, TEXT("LoadRuntimeTaskGraph skipped: %s"), *RuntimeError);
        return;
    }

    ApplyActionResult(FMATaskPlannerCoordinator::LoadGraphData(
        GraphModel,
        Data,
        FString::Printf(TEXT("[Loaded] Task graph from runtime storage: %d nodes, %d edges"), Data.Nodes.Num(), Data.Edges.Num())));
}

void UMATaskPlannerWidget::BindRuntimeEvents()
{
    FString RuntimeError;
    if (!FMATaskPlannerRuntimeAdapter::BindTaskGraphChanged(this, this, RuntimeError))
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("BindRuntimeEvents: %s"), *RuntimeError);
        return;
    }

    UE_LOG(LogMATaskPlanner, Log, TEXT("BindRuntimeEvents: Bound OnTaskGraphChanged event"));
}

void UMATaskPlannerWidget::OnRuntimeTaskGraphChanged(const FMATaskGraphData& NewData)
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("OnRuntimeTaskGraphChanged: Received data update (Nodes: %d, Edges: %d)"),
        NewData.Nodes.Num(), NewData.Edges.Num());
    ApplyActionResult(FMATaskPlannerCoordinator::LoadGraphData(
        GraphModel,
        NewData,
        FString::Printf(TEXT("[Updated] Task graph refreshed: %d nodes, %d edges"), NewData.Nodes.Num(), NewData.Edges.Num())));
}

bool UMATaskPlannerWidget::PersistTaskGraph(FString* OutError)
{
    if (!GraphModel)
    {
        UE_LOG(LogMATaskPlanner, Warning, TEXT("PersistTaskGraph: GraphModel is null"));
        if (OutError)
        {
            *OutError = TEXT("TaskGraphModel is null");
        }
        return false;
    }

    FString RuntimeError;
    if (!FMATaskPlannerRuntimeAdapter::TrySaveTaskGraph(this, GraphModel->GetWorkingData(), RuntimeError))
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("PersistTaskGraph: %s"), *RuntimeError);
        if (OutError)
        {
            *OutError = RuntimeError;
        }
        return false;
    }

    UE_LOG(LogMATaskPlanner, Log, TEXT("PersistTaskGraph: Saved task graph to runtime storage"));
    return true;
}

void UMATaskPlannerWidget::ApplyActionResult(const FMATaskPlannerActionResult& Result)
{
    if (!Result.Message.IsEmpty())
    {
        AppendStatusLog(Result.Message);
    }

    for (const FString& Line : Result.DetailLines)
    {
        AppendStatusLog(Line);
    }

    if (Result.bShouldPersist && GraphModel)
    {
        FString RuntimeError;
        if (!PersistTaskGraph(&RuntimeError))
        {
            AppendStatusLog(FString::Printf(TEXT("[Error] Failed to save task graph: %s"), *RuntimeError));
        }
    }

    if (Result.bGraphChanged)
    {
        const FMATaskGraphData DataToBroadcast = Result.bHasData ? Result.Data : (GraphModel ? GraphModel->GetWorkingData() : FMATaskGraphData());
        OnTaskGraphChanged.Broadcast(DataToBroadcast);
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMATaskPlannerWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        return;
    }
    
    // 从 Theme 更新颜色配置
    BackgroundColor = Theme->BackgroundColor;
    PanelBackgroundColor = Theme->CanvasBackgroundColor;
    TitleColor = Theme->TextColor;
    LabelColor = Theme->InputTextColor;
    ButtonColor = Theme->PrimaryColor;
    
    // 更新 UI 元素颜色
    // 标题在深色背景上，使用 LabelTextColor（浅色）而不是 TextColor（黑色）
    if (TitleText)
    {
        TitleText->SetColorAndOpacity(FSlateColor(Theme->LabelTextColor));
    }
    
    if (HintText)
    {
        HintText->SetColorAndOpacity(FSlateColor(Theme->HintTextColor));
    }
    
    if (CloseText)
    {
        CloseText->SetColorAndOpacity(FSlateColor(Theme->SecondaryTextColor));
    }
    
    if (StatusLogLabel)
    {
        StatusLogLabel->SetColorAndOpacity(FSlateColor(Theme->LabelTextColor));
    }
    
    if (JsonEditorLabel)
    {
        JsonEditorLabel->SetColorAndOpacity(FSlateColor(Theme->LabelTextColor));
    }
    
    if (UserInputLabel)
    {
        UserInputLabel->SetColorAndOpacity(FSlateColor(Theme->LabelTextColor));
    }
    
    if (BackgroundOverlay)
    {
        BackgroundOverlay->SetBrushColor(Theme->OverlayColor);
    }
    
    // 更新输入框 Border 背景颜色
    if (StatusLogBorder)
    {
        // 只读文本框使用稍微变暗的输入框背景色
        FLinearColor StatusLogBgColor = FLinearColor(Theme->InputBackgroundColor.R * 0.85f, Theme->InputBackgroundColor.G * 0.85f, Theme->InputBackgroundColor.B * 0.85f, 1.0f);
        MARoundedBorderUtils::ApplyRoundedCorners(StatusLogBorder, StatusLogBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    }
    
    if (JsonEditorBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(JsonEditorBorder, Theme->InputBackgroundColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    }
    
    if (UserInputBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(UserInputBorder, Theme->InputBackgroundColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    }
    
    // 将主题传递给子组件
    if (DAGCanvas)
    {
        DAGCanvas->ApplyTheme(Theme);
    }
    
    if (NodePalette)
    {
        NodePalette->ApplyTheme(Theme);
    }
    
    UE_LOG(LogMATaskPlanner, Verbose, TEXT("ApplyTheme: Theme applied to task planner widget"));
}
