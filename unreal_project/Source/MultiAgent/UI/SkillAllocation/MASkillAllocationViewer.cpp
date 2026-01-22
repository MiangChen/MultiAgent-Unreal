// MASkillAllocationViewer.cpp
// Skill Allocation Viewer main container Widget - Pure C++ implementation

#include "MASkillAllocationViewer.h"
#include "MAGanttCanvas.h"
#include "MASkillAllocationModel.h"
#include "../../Core/Comm/MACommSubsystem.h"
#include "../../Core/Manager/MATempDataManager.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAUITheme.h"
#include "../Components/MAStyledButton.h"
#include "../Core/MAUIManager.h"
#include "../HUD/MAHUD.h"
#include "Kismet/GameplayStatics.h"
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
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogMASkillAllocationViewer);

//=============================================================================
// Constructor
//=============================================================================

UMASkillAllocationViewer::UMASkillAllocationViewer(const FObjectInitializer& ObjectInitializer)
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

void UMASkillAllocationViewer::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // Build UI here, ensure WidgetTree is initialized
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMASkillAllocationViewer::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Bind button events
    if (UpdateButton && !UpdateButton->OnClicked.IsAlreadyBound(this, &UMASkillAllocationViewer::OnUpdateButtonClicked))
    {
        UpdateButton->OnClicked.AddDynamic(this, &UMASkillAllocationViewer::OnUpdateButtonClicked);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("UpdateButton event bound"));
    }
    
    if (SaveButton && !SaveButton->OnClicked.IsAlreadyBound(this, &UMASkillAllocationViewer::OnSaveButtonClicked))
    {
        SaveButton->OnClicked.AddDynamic(this, &UMASkillAllocationViewer::OnSaveButtonClicked);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SaveButton event bound"));
    }

    // ResetButton removed - no functionality implemented

    // Bind close button event
    if (CloseButton && !CloseButton->OnClicked.IsAlreadyBound(this, &UMASkillAllocationViewer::OnCloseButtonClicked))
    {
        CloseButton->OnClicked.AddDynamic(this, &UMASkillAllocationViewer::OnCloseButtonClicked);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("CloseButton event bound"));
    }

    // Bind data model event
    if (AllocationModel && !AllocationModel->OnDataChanged.IsAlreadyBound(this, &UMASkillAllocationViewer::OnModelDataChanged))
    {
        AllocationModel->OnDataChanged.AddDynamic(this, &UMASkillAllocationViewer::OnModelDataChanged);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("AllocationModel OnDataChanged event bound"));
    }
    
    if (AllocationModel && !AllocationModel->OnSkillStatusChanged.IsAlreadyBound(this, &UMASkillAllocationViewer::OnSkillStatusUpdated))
    {
        AllocationModel->OnSkillStatusChanged.AddDynamic(this, &UMASkillAllocationViewer::OnSkillStatusUpdated);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("AllocationModel OnSkillStatusChanged event bound"));
    }
    
    // Bind TempDataManager skill allocation change event
    if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld()))
    {
        if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
        {
            if (!TempDataMgr->OnSkillAllocationChanged.IsAlreadyBound(this, &UMASkillAllocationViewer::OnTempSkillAllocationChanged))
            {
                TempDataMgr->OnSkillAllocationChanged.AddDynamic(this, &UMASkillAllocationViewer::OnTempSkillAllocationChanged);
                UE_LOG(LogMASkillAllocationViewer, Log, TEXT("TempDataManager OnSkillAllocationChanged event bound"));
            }
            
            // Bind skill status update event for real-time status display
            if (!TempDataMgr->OnSkillStatusUpdated.IsAlreadyBound(this, &UMASkillAllocationViewer::OnTempSkillStatusUpdated))
            {
                TempDataMgr->OnSkillStatusUpdated.AddDynamic(this, &UMASkillAllocationViewer::OnTempSkillStatusUpdated);
                UE_LOG(LogMASkillAllocationViewer, Log, TEXT("TempDataManager OnSkillStatusUpdated event bound"));
            }
        }
    }
    
    // Bind GanttCanvas drag events (Requirements 5.1)
    if (GanttCanvas)
    {
        // Bind drag started event (Requirements 7.1)
        if (!GanttCanvas->OnDragStarted.IsAlreadyBound(this, &UMASkillAllocationViewer::OnGanttDragStarted))
        {
            GanttCanvas->OnDragStarted.AddDynamic(this, &UMASkillAllocationViewer::OnGanttDragStarted);
            UE_LOG(LogMASkillAllocationViewer, Log, TEXT("GanttCanvas OnDragStarted event bound"));
        }
        
        if (!GanttCanvas->OnDragCompleted.IsAlreadyBound(this, &UMASkillAllocationViewer::OnGanttDragCompleted))
        {
            GanttCanvas->OnDragCompleted.AddDynamic(this, &UMASkillAllocationViewer::OnGanttDragCompleted);
            UE_LOG(LogMASkillAllocationViewer, Log, TEXT("GanttCanvas OnDragCompleted event bound"));
        }
        
        if (!GanttCanvas->OnSkillDragCancelled.IsAlreadyBound(this, &UMASkillAllocationViewer::OnGanttDragCancelled))
        {
            GanttCanvas->OnSkillDragCancelled.AddDynamic(this, &UMASkillAllocationViewer::OnGanttDragCancelled);
            UE_LOG(LogMASkillAllocationViewer, Log, TEXT("GanttCanvas OnSkillDragCancelled event bound"));
        }
        
        // Bind drag blocked event (Requirements 8.2)
        if (!GanttCanvas->OnDragBlocked.IsAlreadyBound(this, &UMASkillAllocationViewer::OnGanttDragBlocked))
        {
            GanttCanvas->OnDragBlocked.AddDynamic(this, &UMASkillAllocationViewer::OnGanttDragBlocked);
            UE_LOG(LogMASkillAllocationViewer, Log, TEXT("GanttCanvas OnDragBlocked event bound"));
        }
        
        // Bind drag failed event (Requirements 7.4)
        if (!GanttCanvas->OnDragFailed.IsAlreadyBound(this, &UMASkillAllocationViewer::OnGanttDragFailed))
        {
            GanttCanvas->OnDragFailed.AddDynamic(this, &UMASkillAllocationViewer::OnGanttDragFailed);
            UE_LOG(LogMASkillAllocationViewer, Log, TEXT("GanttCanvas OnDragFailed event bound"));
        }
    }
    
    // Initialize status log
    AppendStatusLog(TEXT("Skill Allocation Workbench started"));
    AppendStatusLog(TEXT("Tip: Edit JSON and click 'Update' to load data"));
    
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("MASkillAllocationViewer NativeConstruct completed"));
}

TSharedRef<SWidget> UMASkillAllocationViewer::RebuildWidget()
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

FReply UMASkillAllocationViewer::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Intercept all mouse button events to prevent click-through to game scene
    FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    return FReply::Handled();
}

FReply UMASkillAllocationViewer::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Intercept all mouse button release events
    FReply Reply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    return FReply::Handled();
}

FReply UMASkillAllocationViewer::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Intercept mouse wheel events
    FReply Reply = Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    return FReply::Handled();
}

//=============================================================================
// UI Construction
//=============================================================================

void UMASkillAllocationViewer::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("BuildUI: Starting UI construction (top-bottom layout)..."));

    // Create data model
    AllocationModel = NewObject<UMASkillAllocationModel>(this, TEXT("AllocationModel"));

    // Create root CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
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

    // Create main background Border - windowed mode (80% width, 85% height, centered)
    UBorder* MainBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBackground"));
    MainBackground->SetPadding(FMargin(10.0f));
    
    // Apply rounded corners using MARoundedBorderUtils (Requirements 4.2)
    MARoundedBorderUtils::ApplyRoundedCorners(MainBackground, BackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainBackground);
    // Windowed mode: centered with 80% width and 85% height
    MainSlot->SetAnchors(FAnchors(0.1f, 0.075f, 0.9f, 0.925f));
    MainSlot->SetOffsets(FMargin(0.0f));

    // Create main vertical layout (top-bottom: title bar + gantt + bottom panel)
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    MainBackground->AddChild(MainVBox);

    // Create title bar with close button
    UHorizontalBox* TitleBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TitleBar"));
    UVerticalBoxSlot* TitleBarSlot = MainVBox->AddChildToVerticalBox(TitleBar);
    TitleBarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    TitleBarSlot->SetPadding(FMargin(0, 0, 0, 5));

    // Title text
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Skill Allocation Workbench")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    // 标题在深色背景上，使用 LabelTextColor（浅色）而不是 TextColor（黑色）
    FLinearColor TitleTextColor = Theme ? Theme->LabelTextColor : FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);
    TitleText->SetColorAndOpacity(FSlateColor(TitleTextColor));
    
    UHorizontalBoxSlot* TitleTextSlot = TitleBar->AddChildToHorizontalBox(TitleText);
    TitleTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TitleTextSlot->SetVerticalAlignment(VAlign_Center);

    // Hint text
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(TEXT("Press N to close the window")));
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
    
    FButtonStyle CloseButtonStyle;
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

    // Create top panel (Gantt chart) - takes 60% of space
    UBorder* TopPanel = CreateTopPanel();
    UVerticalBoxSlot* TopSlot = MainVBox->AddChildToVerticalBox(TopPanel);
    FSlateChildSize TopSize(ESlateSizeRule::Fill);
    TopSize.Value = 0.6f;
    TopSlot->SetSize(TopSize);
    TopSlot->SetPadding(FMargin(0, 0, 0, 5));

    // Create bottom panel (Status Log + JSON Editor + Buttons) - takes 40% of space
    UBorder* BottomPanel = CreateBottomPanel();
    UVerticalBoxSlot* BottomSlot = MainVBox->AddChildToVerticalBox(BottomPanel);
    FSlateChildSize BottomSize(ESlateSizeRule::Fill);
    BottomSize.Value = 0.4f;
    BottomSlot->SetSize(BottomSize);

    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("BuildUI: UI construction completed successfully (top-bottom layout)"));
}

UBorder* UMASkillAllocationViewer::CreateTopPanel()
{
    // Top panel background (Gantt chart area)
    UBorder* TopPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TopPanelBorder"));
    TopPanelBorder->SetPadding(FMargin(10.0f));
    
    // Apply rounded corners using MARoundedBorderUtils (Requirements 4.2)
    MARoundedBorderUtils::ApplyRoundedCorners(TopPanelBorder, PanelBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);

    // Gantt canvas - will auto-size based on content
    GanttCanvas = WidgetTree->ConstructWidget<UMAGanttCanvas>(UMAGanttCanvas::StaticClass(), TEXT("GanttCanvas"));
    GanttCanvas->BindToModel(AllocationModel);
    
    TopPanelBorder->AddChild(GanttCanvas);

    return TopPanelBorder;
}

UBorder* UMASkillAllocationViewer::CreateBottomPanel()
{
    // Bottom panel background
    UBorder* BottomPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BottomPanelBorder"));
    BottomPanelBorder->SetPadding(FMargin(10.0f));
    
    // Apply rounded corners using MARoundedBorderUtils (Requirements 4.2)
    MARoundedBorderUtils::ApplyRoundedCorners(BottomPanelBorder, PanelBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);

    // Horizontal layout (Status Log | JSON Editor + Buttons)
    UHorizontalBox* BottomHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomHBox"));
    BottomPanelBorder->AddChild(BottomHBox);

    // Status log section (left side of bottom panel)
    UVerticalBox* StatusLogSection = CreateStatusLogSection();
    UHorizontalBoxSlot* StatusSlot = BottomHBox->AddChildToHorizontalBox(StatusLogSection);
    FSlateChildSize StatusSize(ESlateSizeRule::Fill);
    StatusSize.Value = 0.4f;
    StatusSlot->SetSize(StatusSize);
    StatusSlot->SetPadding(FMargin(0, 0, 15, 0));  // 增加两个框之间的间距

    // JSON editor section (right side of bottom panel)
    UVerticalBox* JsonEditorSection = CreateJsonEditorSection();
    UHorizontalBoxSlot* JsonSlot = BottomHBox->AddChildToHorizontalBox(JsonEditorSection);
    FSlateChildSize JsonSize(ESlateSizeRule::Fill);
    JsonSize.Value = 0.6f;
    JsonSlot->SetSize(JsonSize);

    return BottomPanelBorder;
}

UBorder* UMASkillAllocationViewer::CreateLeftPanel()
{
    // Left panel background (deprecated - kept for compatibility)
    UBorder* LeftPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanelBorder"));
    LeftPanelBorder->SetBrushColor(PanelBackgroundColor);
    LeftPanelBorder->SetPadding(FMargin(10.0f));

    // Vertical layout
    UVerticalBox* LeftVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVBox"));
    LeftPanelBorder->AddChild(LeftVBox);

    // Title
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LeftPanelTitle"));
    TitleText->SetText(FText::FromString(TEXT("Skill Allocation Workbench")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    
    UVerticalBoxSlot* TitleSlot = LeftVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Hint text
    UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(TEXT("Press X to toggle visibility")));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 10;
    HintText->SetFont(HintFont);
    FLinearColor LeftHintTextColor = Theme ? Theme->HintTextColor : FLinearColor(0.5f, 0.5f, 0.6f);
    HintText->SetColorAndOpacity(FSlateColor(LeftHintTextColor));
    
    UVerticalBoxSlot* HintSlot = LeftVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 15));

    // Status log section
    UVerticalBox* StatusLogSection = CreateStatusLogSection();
    UVerticalBoxSlot* StatusSlot = LeftVBox->AddChildToVerticalBox(StatusLogSection);
    FSlateChildSize StatusSize(ESlateSizeRule::Fill);
    StatusSize.Value = StatusLogHeightRatio;
    StatusSlot->SetSize(StatusSize);
    StatusSlot->SetPadding(FMargin(0, 0, 0, 10));

    // JSON editor section
    UVerticalBox* JsonEditorSection = CreateJsonEditorSection();
    UVerticalBoxSlot* JsonSlot = LeftVBox->AddChildToVerticalBox(JsonEditorSection);
    FSlateChildSize JsonSize(ESlateSizeRule::Fill);
    JsonSize.Value = 1.0f - StatusLogHeightRatio;
    JsonSlot->SetSize(JsonSize);

    return LeftPanelBorder;
}

UVerticalBox* UMASkillAllocationViewer::CreateStatusLogSection()
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
    
    // 设置文本样式：使用主题颜色，字号 12，透明背景
    FEditableTextBoxStyle StatusLogStyle;
    FLinearColor StatusLogTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor StatusLogSlateColor = FSlateColor(StatusLogTextColor);
    StatusLogStyle.SetForegroundColor(StatusLogSlateColor);
    StatusLogStyle.SetFocusedForegroundColor(StatusLogSlateColor);
    StatusLogStyle.SetReadOnlyForegroundColor(StatusLogSlateColor);
    StatusLogStyle.TextStyle.ColorAndOpacity = BlackColor;
    FSlateFontInfo StatusLogFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    StatusLogStyle.SetFont(StatusLogFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush TransparentBrush1;
    TransparentBrush1.TintColor = FSlateColor(FLinearColor::Transparent);
    StatusLogStyle.SetBackgroundImageNormal(TransparentBrush1);
    StatusLogStyle.SetBackgroundImageHovered(TransparentBrush1);
    StatusLogStyle.SetBackgroundImageFocused(TransparentBrush1);
    StatusLogStyle.SetBackgroundImageReadOnly(TransparentBrush1);
    
    StatusLogBox->WidgetStyle = StatusLogStyle;
    
    // 创建圆角 Border 包装文本框
    UBorder* StatusLogBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StatusLogBorder"));
    StatusLogBorder->SetPadding(FMargin(8.0f, 4.0f));
    
    // 应用圆角效果 - 只读文本框使用主题次要颜色背景
    FLinearColor StatusLogBgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(StatusLogBorder, StatusLogBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    StatusLogBorder->AddChild(StatusLogBox);
    
    // Add border to section - let it fill available space
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(StatusLogBorder);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    return Section;
}

UVerticalBox* UMASkillAllocationViewer::CreateJsonEditorSection()
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
    LabelSlot->SetPadding(FMargin(0, 0, 0, 3));

    // JSON editor text box (editable) - wrapped in rounded border
    JsonEditorBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditorBox"));
    JsonEditorBox->SetIsReadOnly(false);
    JsonEditorBox->SetText(FText::FromString(TEXT("{\n  \"name\": \"\",\n  \"description\": \"\",\n  \"data\": {}\n}")));
    
    // 设置文本样式：使用主题颜色，字号 12，透明背景
    FEditableTextBoxStyle JsonEditorStyle;
    FLinearColor JsonEditorTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor JsonEditorSlateColor = FSlateColor(JsonEditorTextColor);
    JsonEditorStyle.SetForegroundColor(JsonEditorSlateColor);
    JsonEditorStyle.SetFocusedForegroundColor(JsonEditorSlateColor);
    FSlateFontInfo JsonEditorFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    JsonEditorStyle.SetFont(JsonEditorFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush TransparentBrush2;
    TransparentBrush2.TintColor = FSlateColor(FLinearColor::Transparent);
    JsonEditorStyle.SetBackgroundImageNormal(TransparentBrush2);
    JsonEditorStyle.SetBackgroundImageHovered(TransparentBrush2);
    JsonEditorStyle.SetBackgroundImageFocused(TransparentBrush2);
    JsonEditorStyle.SetBackgroundImageReadOnly(TransparentBrush2);
    
    JsonEditorBox->WidgetStyle = JsonEditorStyle;
    
    // 创建圆角 Border 包装文本框
    UBorder* JsonEditorBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonEditorBorder"));
    JsonEditorBorder->SetPadding(FMargin(8.0f, 4.0f));
    
    // 应用圆角效果 - 可编辑文本框使用主题输入背景颜色
    FLinearColor JsonEditorBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(JsonEditorBorder, JsonEditorBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    JsonEditorBorder->AddChild(JsonEditorBox);
    
    // Add border to section - let it fill available space
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(JsonEditorBorder);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 5));

    // Button row - horizontal layout for buttons
    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
    UVerticalBoxSlot* ButtonRowSlot = Section->AddChildToVerticalBox(ButtonRow);
    ButtonRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);  // 按钮靠右对齐

    // "Update" button - 使用 MAStyledButton，与 Save 按钮样式一致
    UpdateButton = WidgetTree->ConstructWidget<UMAStyledButton>(UMAStyledButton::StaticClass(), TEXT("UpdateButton"));
    UpdateButton->SetButtonText(FText::FromString(TEXT("Update")));
    UpdateButton->SetButtonStyle(EMAButtonStyle::Secondary);
    
    UHorizontalBoxSlot* UpdateButtonSlot = ButtonRow->AddChildToHorizontalBox(UpdateButton);
    UpdateButtonSlot->SetPadding(FMargin(0, 0, 10, 0));

    // "Save" button - 黄色保存按钮，使用 MAStyledButton
    SaveButton = WidgetTree->ConstructWidget<UMAStyledButton>(UMAStyledButton::StaticClass(), TEXT("SaveButton"));
    SaveButton->SetButtonText(FText::FromString(TEXT("Save")));
    SaveButton->SetButtonStyle(EMAButtonStyle::Success);

    UHorizontalBoxSlot* SaveButtonSlot = ButtonRow->AddChildToHorizontalBox(SaveButton);
    SaveButtonSlot->SetPadding(FMargin(0, 0, 0, 0));

    // Reset button removed - no functionality implemented

    return Section;
}

UBorder* UMASkillAllocationViewer::CreateRightPanel()
{
    // Right panel background (deprecated - kept for compatibility)
    UBorder* RightPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBorder"));
    RightPanelBorder->SetBrushColor(PanelBackgroundColor);
    RightPanelBorder->SetPadding(FMargin(10.0f));

    // Gantt canvas
    GanttCanvas = WidgetTree->ConstructWidget<UMAGanttCanvas>(UMAGanttCanvas::StaticClass(), TEXT("GanttCanvas"));
    GanttCanvas->BindToModel(AllocationModel);
    
    RightPanelBorder->AddChild(GanttCanvas);

    return RightPanelBorder;
}

//=============================================================================
// Public Interface
//=============================================================================

void UMASkillAllocationViewer::LoadSkillAllocation(const FMASkillAllocationData& Data)
{
    if (!AllocationModel)
    {
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("LoadSkillAllocation: AllocationModel is null!"));
        return;
    }
    
    AllocationModel->LoadFromData(Data);
    SyncJsonEditorFromModel();
    
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(FString::Printf(TEXT("Skill allocation loaded: %d time steps, %d robots"),
        AllocationModel->GetAllTimeSteps().Num(), AllocationModel->GetAllRobotIds().Num()));
    
    OnSkillAllocationChanged.Broadcast(Data);
}

bool UMASkillAllocationViewer::LoadSkillAllocationFromJson(const FString& JsonString)
{
    if (!AllocationModel)
    {
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("LoadSkillAllocationFromJson: AllocationModel is null!"));
        return false;
    }
    
    // Check for empty input
    if (JsonString.IsEmpty())
    {
        AppendStatusLog(TEXT("[Error] JSON input is empty"));
        return false;
    }
    
    FString ErrorMessage;
    if (!AllocationModel->LoadFromJsonWithError(JsonString, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] %s"), *ErrorMessage));
        return false;
    }
    
    // Check if there were warnings
    if (!ErrorMessage.IsEmpty() && ErrorMessage.StartsWith(TEXT("[Warning]")))
    {
        AppendStatusLog(ErrorMessage);
    }
    
    SyncJsonEditorFromModel();
    
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    FMASkillAllocationData Data = AllocationModel->GetWorkingData();
    AppendStatusLog(FString::Printf(TEXT("[Success] Skill allocation loaded: %d time steps, %d robots"),
        AllocationModel->GetTimeStepCount(), AllocationModel->GetRobotCount()));
    
    OnSkillAllocationChanged.Broadcast(Data);
    return true;
}

void UMASkillAllocationViewer::AppendStatusLog(const FString& Message)
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
    
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("StatusLog: %s"), *Message);
}

void UMASkillAllocationViewer::ClearStatusLog()
{
    if (StatusLogBox)
    {
        StatusLogBox->SetText(FText::GetEmpty());
    }
}

FString UMASkillAllocationViewer::GetJsonText() const
{
    if (JsonEditorBox)
    {
        return JsonEditorBox->GetText().ToString();
    }
    return FString();
}

void UMASkillAllocationViewer::SetJsonText(const FString& JsonText)
{
    if (JsonEditorBox)
    {
        JsonEditorBox->SetText(FText::FromString(JsonText));
    }
}

bool UMASkillAllocationViewer::LoadMockData()
{
    // 请使用 TempDataManager 从 skill_allocation_temp.json 读取数据
    AppendStatusLog(TEXT("[Warning] LoadMockData is deprecated. Use TempDataManager instead."));
    UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("LoadMockData is deprecated. Data should be loaded from skill_allocation_temp.json via TempDataManager."));
    return false;
}

//=============================================================================
// Event Handling
//=============================================================================

void UMASkillAllocationViewer::OnUpdateButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("UpdateButton clicked"));
    
    FString JsonText = GetJsonText();
    
    if (JsonText.IsEmpty())
    {
        AppendStatusLog(TEXT("[Warning] JSON editor is empty"));
        return;
    }
    
    // Trim whitespace and check if it's just whitespace
    FString TrimmedJson = JsonText.TrimStartAndEnd();
    if (TrimmedJson.IsEmpty())
    {
        AppendStatusLog(TEXT("[Warning] JSON editor contains only whitespace"));
        return;
    }
    
    FString ErrorMessage;
    if (!AllocationModel->LoadFromJsonWithError(JsonText, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] %s"), *ErrorMessage));
        return;
    }
    
    // Check if there were warnings
    if (!ErrorMessage.IsEmpty() && ErrorMessage.StartsWith(TEXT("[Warning]")))
    {
        AppendStatusLog(ErrorMessage);
    }
    
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(FString::Printf(TEXT("[Success] Skill allocation updated: %d time steps, %d robots"),
        AllocationModel->GetTimeStepCount(), AllocationModel->GetRobotCount()));
    
    OnSkillAllocationChanged.Broadcast(AllocationModel->GetWorkingData());
}

void UMASkillAllocationViewer::OnSaveButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SaveButton clicked"));
    SaveAndNavigateToModal();
}

void UMASkillAllocationViewer::SaveAndNavigateToModal()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SaveAndNavigateToModal: Saving data and navigating to modal"));
    
    // 1. 保存当前数据到 TempDataManager
    if (AllocationModel)
    {
        FMASkillAllocationData CurrentData = AllocationModel->GetWorkingData();
        
        if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld()))
        {
            if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
            {
                TempDataMgr->SaveSkillAllocation(CurrentData);
                AppendStatusLog(TEXT("[保存] 技能分配已保存"));
                UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SaveAndNavigateToModal: Skill allocation saved to TempDataManager"));
            }
            else
            {
                UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SaveAndNavigateToModal: TempDataManager not found"));
                AppendStatusLog(TEXT("[错误] 无法保存数据：TempDataManager 不可用"));
                return;
            }
        }
        else
        {
            UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SaveAndNavigateToModal: GameInstance not found"));
            AppendStatusLog(TEXT("[错误] 无法保存数据：GameInstance 不可用"));
            return;
        }
    }
    
    // 2. 导航到 SkillAllocationModal (通过 MAHUD 获取 UIManager)
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC)
    {
        if (AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD()))
        {
            if (UMAUIManager* UIManager = HUD->GetUIManager())
            {
                UIManager->NavigateFromViewerToSkillAllocationModal();
                UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SaveAndNavigateToModal: Navigation to SkillAllocationModal initiated"));
            }
            else
            {
                UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SaveAndNavigateToModal: UIManager not found"));
                AppendStatusLog(TEXT("[错误] 无法导航：UIManager 不可用"));
            }
        }
        else
        {
            UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SaveAndNavigateToModal: MAHUD not found"));
            AppendStatusLog(TEXT("[错误] 无法导航：HUD 不可用"));
        }
    }
}

void UMASkillAllocationViewer::OnResetButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("ResetButton clicked"));
    
    // 重新启用拖拽功能
    if (GanttCanvas)
    {
        GanttCanvas->SetDragEnabled(true);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnResetButtonClicked: Drag re-enabled"));
    }
    
    // 调用 AllocationModel->ResetToOriginal() 重置所有状态
    if (AllocationModel)
    {
        AllocationModel->ResetToOriginal();
        
        // 同步 JSON 编辑器
        SyncJsonEditorFromModel();
    }
    
    // 刷新甘特图
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    // 记录日志
    AppendStatusLog(TEXT("[重置] 所有技能状态已重置为待执行"));
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Reset completed - all skills set to Pending"));
}

void UMASkillAllocationViewer::OnCloseButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Close button clicked"));
    
    // 通过 UIManager 隐藏 widget，这样可以正确同步 HUDStateManager 状态
    // 直接调用 SetVisibility 会导致状态不同步
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        if (AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD()))
        {
            if (UMAUIManager* UIManager = HUD->GetUIManager())
            {
                UIManager->HideWidget(EMAWidgetType::SkillAllocation);
                UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SkillAllocationViewer hidden via UIManager"));
                return;
            }
        }
    }
    
    // Fallback: 如果无法获取 UIManager，直接隐藏（但状态可能不同步）
    UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("Could not get UIManager, hiding widget directly (state may be out of sync)"));
    SetVisibility(ESlateVisibility::Collapsed);
}

void UMASkillAllocationViewer::OnModelDataChanged()
{
    // Sync to JSON editor in real-time
    SyncJsonEditorFromModel();
}

void UMASkillAllocationViewer::OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus Status)
{
    // Validate inputs
    if (RobotId.IsEmpty())
    {
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("OnSkillStatusUpdated: Empty RobotId received, ignoring"));
        return;
    }
    
    if (TimeStep < 0)
    {
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("OnSkillStatusUpdated: Invalid TimeStep %d received, ignoring"), TimeStep);
        return;
    }
    
    // Verify the skill exists in the model
    if (AllocationModel)
    {
        FMASkillAssignment Skill;
        if (!AllocationModel->FindSkill(TimeStep, RobotId, Skill))
        {
            UE_LOG(LogMASkillAllocationViewer, Warning, 
                TEXT("OnSkillStatusUpdated: Skill not found for TimeStep=%d, RobotId=%s - status update ignored"), 
                TimeStep, *RobotId);
            AppendStatusLog(FString::Printf(TEXT("[Warning] Status update for unknown skill: TimeStep %d, Robot %s"), 
                TimeStep, *RobotId));
            return;
        }
    }
    
    // Update Gantt canvas colors
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    // Log status change
    FString StatusStr;
    switch (Status)
    {
        case ESkillExecutionStatus::Pending:
            StatusStr = TEXT("Pending");
            break;
        case ESkillExecutionStatus::InProgress:
            StatusStr = TEXT("In Progress");
            break;
        case ESkillExecutionStatus::Completed:
            StatusStr = TEXT("Completed");
            break;
        case ESkillExecutionStatus::Failed:
            StatusStr = TEXT("Failed");
            break;
        default:
            StatusStr = TEXT("Unknown");
            UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("OnSkillStatusUpdated: Unknown status value %d"), (int32)Status);
            break;
    }
    
    AppendStatusLog(FString::Printf(TEXT("[Status] TimeStep %d, Robot %s: %s"),
        TimeStep, *RobotId, *StatusStr));
}

void UMASkillAllocationViewer::OnTempSkillAllocationChanged(const FMASkillAllocationData& NewData)
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnTempSkillAllocationChanged: Received new skill allocation data"));
    
    // 加载新数据到模型
    if (AllocationModel)
    {
        AllocationModel->LoadFromData(NewData);
        
        // 同步 JSON 编辑器
        SyncJsonEditorFromModel();
        
        // 刷新甘特图
        if (GanttCanvas)
        {
            GanttCanvas->RefreshFromModel();
        }
        
        AppendStatusLog(FString::Printf(TEXT("[Info] Skill list auto-refreshed: %s (%d time steps, %d robots)"),
            *NewData.Name, NewData.Data.Num(), AllocationModel->GetRobotCount()));
    }
}

void UMASkillAllocationViewer::OnTempSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnTempSkillStatusUpdated: TimeStep=%d, RobotId=%s, Status=%d"),
        TimeStep, *RobotId, static_cast<int32>(NewStatus));
    
    // Update the AllocationModel with the new status
    if (AllocationModel)
    {
        AllocationModel->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
    
    // Refresh the Gantt canvas to show the updated status color
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
}

//=============================================================================
// Drag Event Handling (Requirements 5.1)
//=============================================================================

void UMASkillAllocationViewer::OnGanttDragStarted(const FString& SkillName, int32 TimeStep, const FString& RobotId)
{
    UE_LOG(LogMASkillAllocationViewer, Log, 
        TEXT("OnGanttDragStarted: Skill=%s, TimeStep=%d, RobotId=%s"),
        *SkillName, TimeStep, *RobotId);
    
    // 记录状态日志 (Requirements 7.1)
    AppendStatusLog(FString::Printf(TEXT("[拖拽] 开始移动技能: %s (T%d, %s)"),
        *SkillName, TimeStep, *RobotId));
}

void UMASkillAllocationViewer::OnGanttDragCompleted(int32 SourceTimeStep, const FString& SourceRobotId,
                                                     int32 TargetTimeStep, const FString& TargetRobotId)
{
    UE_LOG(LogMASkillAllocationViewer, Log, 
        TEXT("OnGanttDragCompleted: Skill moved from T%d/%s to T%d/%s"),
        SourceTimeStep, *SourceRobotId, TargetTimeStep, *TargetRobotId);
    
    // 记录状态日志 (Requirements 7.2)
    if (AllocationModel)
    {
        FMASkillAssignment Skill;
        if (AllocationModel->FindSkill(TargetTimeStep, TargetRobotId, Skill))
        {
            AppendStatusLog(FString::Printf(TEXT("[成功] 技能已移动: %s 从 T%d 到 T%d"),
                *Skill.SkillName, SourceTimeStep, TargetTimeStep));
        }
        else
        {
            AppendStatusLog(FString::Printf(TEXT("[成功] 技能已移动: 从 T%d/%s 到 T%d/%s"),
                SourceTimeStep, *SourceRobotId, TargetTimeStep, *TargetRobotId));
        }
    }
    
    // 同步 JSON 编辑器
    SyncJsonEditorFromModel();
    
    // 同步数据到临时文件 (Requirements 5.1)
    SyncDataToTempFile();
}

void UMASkillAllocationViewer::OnGanttDragCancelled()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnGanttDragCancelled: Drag operation cancelled"));
    
    // 记录状态日志 (Requirements 7.3)
    AppendStatusLog(TEXT("[取消] 拖拽操作已取消"));
}

void UMASkillAllocationViewer::OnGanttDragBlocked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnGanttDragBlocked: Drag attempt blocked during execution"));
    
    // 记录警告日志 (Requirements 8.2)
    AppendStatusLog(TEXT("[警告] 执行期间无法修改技能分配"));
}

void UMASkillAllocationViewer::OnGanttDragFailed()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnGanttDragFailed: Drag operation failed due to invalid target"));
    
    // 记录失败日志 (Requirements 7.4)
    AppendStatusLog(TEXT("[失败] 无法放置: 目标槽位已被占用"));
}

void UMASkillAllocationViewer::SyncDataToTempFile()
{
    // 获取 MATempDataManager (Requirements 5.1)
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
    if (!GameInstance)
    {
        AppendStatusLog(TEXT("[错误] 无法获取 GameInstance，数据同步失败"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SyncDataToTempFile: Failed to get GameInstance"));
        return;
    }
    
    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        AppendStatusLog(TEXT("[错误] TempDataManager 不可用，数据同步失败"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SyncDataToTempFile: TempDataManager not available"));
        return;
    }
    
    // 获取当前模型数据
    if (!AllocationModel)
    {
        AppendStatusLog(TEXT("[错误] AllocationModel 为空，数据同步失败"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SyncDataToTempFile: AllocationModel is null"));
        return;
    }
    
    FMASkillAllocationData Data = AllocationModel->GetWorkingData();
    
    // 验证数据完整性 (Requirements 5.2)
    if (Data.Data.Num() == 0)
    {
        AppendStatusLog(TEXT("[警告] 技能分配为空，跳过数据同步"));
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("SyncDataToTempFile: Skill allocation is empty, skipping sync"));
        return;
    }
    
    // 调用 SaveSkillAllocation() 保存数据 (Requirements 5.1)
    if (TempDataMgr->SaveSkillAllocation(Data))
    {
        // 记录成功日志 (Requirements 5.4, 7.5)
        AppendStatusLog(TEXT("[同步] 数据已保存到 skill_allocation_temp.json"));
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("SyncDataToTempFile: Data saved to skill_allocation_temp.json"));
    }
    else
    {
        // 记录失败日志 (Requirements 5.3)
        AppendStatusLog(TEXT("[错误] 文件写入失败，数据同步失败"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("SyncDataToTempFile: Failed to save data to temp file"));
    }
}

//=============================================================================
// Helper Methods
//=============================================================================

void UMASkillAllocationViewer::SyncJsonEditorFromModel()
{
    if (!AllocationModel || !JsonEditorBox)
    {
        return;
    }
    
    FString JsonText = AllocationModel->ToJson();
    JsonEditorBox->SetText(FText::FromString(JsonText));
}

FString UMASkillAllocationViewer::GetTimestamp() const
{
    FDateTime Now = FDateTime::Now();
    return FString::Printf(TEXT("%02d:%02d:%02d"), Now.GetHour(), Now.GetMinute(), Now.GetSecond());
}

//=============================================================================
// Data Conversion
//=============================================================================

bool UMASkillAllocationViewer::ConvertToSkillListMessage(
    const FMASkillAllocationData& InData,
    FMASkillListMessage& OutMessage,
    FString& OutErrorMessage)
{
    // Initialize output
    OutMessage = FMASkillListMessage();
    OutErrorMessage.Empty();
    
    // Check for empty data
    if (InData.Data.Num() == 0)
    {
        OutErrorMessage = TEXT("技能列表为空");
        return false;
    }
    
    // Get sorted time steps
    TArray<int32> TimeSteps;
    InData.Data.GetKeys(TimeSteps);
    TimeSteps.Sort();
    
    OutMessage.TotalTimeSteps = TimeSteps.Num();
    
    for (int32 TimeStep : TimeSteps)
    {
        FMATimeStepCommands TimeStepCmd;
        TimeStepCmd.TimeStep = TimeStep;
        
        const FMATimeStepData* TimeStepData = InData.Data.Find(TimeStep);
        if (!TimeStepData)
        {
            continue;
        }
        
        for (const auto& Pair : TimeStepData->RobotSkills)
        {
            const FString& RobotId = Pair.Key;
            const FMASkillAssignment& Assignment = Pair.Value;
            
            FMAAgentSkillCommand Cmd;
            Cmd.AgentId = RobotId;
            Cmd.SkillName = Assignment.SkillName;
            
            // Parse parameters from JSON string
            if (!Assignment.ParamsJson.IsEmpty())
            {
                TSharedPtr<FJsonObject> ParamsObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Assignment.ParamsJson);
                if (FJsonSerializer::Deserialize(Reader, ParamsObject) && ParamsObject.IsValid())
                {
                    // Store raw params JSON for later use
                    Cmd.Params.RawParamsJson = Assignment.ParamsJson;
                    
                    // Parse common parameters
                    
                    // dest_position (for navigate)
                    const TSharedPtr<FJsonObject>* DestPosObj;
                    if (ParamsObject->TryGetObjectField(TEXT("dest_position"), DestPosObj))
                    {
                        double X = 0, Y = 0, Z = 0;
                        (*DestPosObj)->TryGetNumberField(TEXT("x"), X);
                        (*DestPosObj)->TryGetNumberField(TEXT("y"), Y);
                        (*DestPosObj)->TryGetNumberField(TEXT("z"), Z);
                        Cmd.Params.DestPosition = FVector(X, Y, Z);
                        Cmd.Params.bHasDestPosition = true;
                    }
                    
                    // goal_type (for search)
                    FString GoalType;
                    if (ParamsObject->TryGetStringField(TEXT("goal_type"), GoalType))
                    {
                        Cmd.Params.GoalType = GoalType;
                    }
                    
                    // task_id
                    FString TaskId;
                    if (ParamsObject->TryGetStringField(TEXT("task_id"), TaskId))
                    {
                        Cmd.Params.TaskId = TaskId;
                    }
                    
                    // area_token
                    FString AreaToken;
                    if (ParamsObject->TryGetStringField(TEXT("area_token"), AreaToken))
                    {
                        Cmd.Params.AreaToken = AreaToken;
                    }
                    
                    // target_token
                    FString TargetToken;
                    if (ParamsObject->TryGetStringField(TEXT("target_token"), TargetToken))
                    {
                        Cmd.Params.TargetToken = TargetToken;
                    }
                    
                    // target_entity
                    FString TargetEntity;
                    if (ParamsObject->TryGetStringField(TEXT("target_entity"), TargetEntity))
                    {
                        Cmd.Params.TargetEntity = TargetEntity;
                    }
                    
                    // target (JSON object for search)
                    const TSharedPtr<FJsonObject>* TargetObj;
                    if (ParamsObject->TryGetObjectField(TEXT("target"), TargetObj))
                    {
                        FString TargetJsonStr;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TargetJsonStr);
                        FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
                        Cmd.Params.TargetJson = TargetJsonStr;
                    }
                    
                    // object1 / target (for Place skill)
                    const TSharedPtr<FJsonObject>* Object1Obj;
                    if (ParamsObject->TryGetObjectField(TEXT("object1"), Object1Obj))
                    {
                        FString Object1JsonStr;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Object1JsonStr);
                        FJsonSerializer::Serialize((*Object1Obj).ToSharedRef(), Writer);
                        Cmd.Params.Object1Json = Object1JsonStr;
                    }
                    
                    // object2 / surface_target (for Place skill)
                    const TSharedPtr<FJsonObject>* Object2Obj;
                    if (ParamsObject->TryGetObjectField(TEXT("object2"), Object2Obj))
                    {
                        FString Object2JsonStr;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Object2JsonStr);
                        FJsonSerializer::Serialize((*Object2Obj).ToSharedRef(), Writer);
                        Cmd.Params.Object2Json = Object2JsonStr;
                    }
                    
                    // search_area (polygon vertices)
                    // Supports two formats:
                    // 1. Array format: [[x, y], [x, y], ...] (from backend)
                    // 2. Object format: [{x, y}, {x, y}, ...] (legacy)
                    const TArray<TSharedPtr<FJsonValue>>* SearchAreaArray;
                    if (ParamsObject->TryGetArrayField(TEXT("search_area"), SearchAreaArray))
                    {
                        for (const TSharedPtr<FJsonValue>& PointValue : *SearchAreaArray)
                        {
                            // Try array format first: [x, y]
                            const TArray<TSharedPtr<FJsonValue>>* PointArray;
                            if (PointValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                            {
                                double X = (*PointArray)[0]->AsNumber();
                                double Y = (*PointArray)[1]->AsNumber();
                                Cmd.Params.SearchArea.Add(FVector2D(X, Y));
                            }
                            // Fallback to object format: {x, y}
                            else
                            {
                                const TSharedPtr<FJsonObject>* PointObj;
                                if (PointValue->TryGetObject(PointObj))
                                {
                                    double X = 0, Y = 0;
                                    (*PointObj)->TryGetNumberField(TEXT("x"), X);
                                    (*PointObj)->TryGetNumberField(TEXT("y"), Y);
                                    Cmd.Params.SearchArea.Add(FVector2D(X, Y));
                                }
                            }
                        }
                        UE_LOG(LogMASkillAllocationViewer, Log, 
                            TEXT("ConvertToSkillListMessage: Parsed search_area with %d vertices"),
                            Cmd.Params.SearchArea.Num());
                    }
                    
                    // target_features (key-value pairs)
                    const TSharedPtr<FJsonObject>* FeaturesObj;
                    if (ParamsObject->TryGetObjectField(TEXT("target_features"), FeaturesObj))
                    {
                        for (const auto& FeaturePair : (*FeaturesObj)->Values)
                        {
                            FString FeatureValue;
                            if (FeaturePair.Value->TryGetString(FeatureValue))
                            {
                                Cmd.Params.TargetFeatures.Add(FeaturePair.Key, FeatureValue);
                            }
                        }
                    }
                }
                else
                {
                    UE_LOG(LogMASkillAllocationViewer, Warning, 
                        TEXT("ConvertToSkillListMessage: Failed to parse params JSON for %s at TimeStep %d: %s"),
                        *RobotId, TimeStep, *Assignment.ParamsJson);
                    // Still add the command, but with raw params only
                    Cmd.Params.RawParamsJson = Assignment.ParamsJson;
                }
            }
            
            TimeStepCmd.Commands.Add(Cmd);
        }
        
        OutMessage.TimeSteps.Add(TimeStepCmd);
    }
    
    UE_LOG(LogMASkillAllocationViewer, Log, 
        TEXT("ConvertToSkillListMessage: Converted %d time steps with %d total commands"),
        OutMessage.TotalTimeSteps, 
        [&OutMessage]() {
            int32 Total = 0;
            for (const auto& TS : OutMessage.TimeSteps) { Total += TS.Commands.Num(); }
            return Total;
        }());
    
    return true;
}


//=============================================================================
// 主题应用
//=============================================================================

void UMASkillAllocationViewer::ApplyTheme(UMAUITheme* InTheme)
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
    
    if (BackgroundOverlay)
    {
        BackgroundOverlay->SetBrushColor(Theme->OverlayColor);
    }
    
    // 将主题传递给子组件
    if (GanttCanvas)
    {
        GanttCanvas->ApplyTheme(Theme);
    }
    
    UE_LOG(LogMASkillAllocationViewer, Verbose, TEXT("ApplyTheme: Theme applied to skill allocation viewer"));
}
