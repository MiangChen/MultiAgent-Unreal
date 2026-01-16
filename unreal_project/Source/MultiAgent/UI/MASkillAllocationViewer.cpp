// MASkillAllocationViewer.cpp
// Skill Allocation Viewer main container Widget - Pure C++ implementation

#include "MASkillAllocationViewer.h"
#include "MAGanttCanvas.h"
#include "MASkillAllocationModel.h"
#include "../Core/Comm/MACommSubsystem.h"
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
    
    if (StartExecuteButton && !StartExecuteButton->OnClicked.IsAlreadyBound(this, &UMASkillAllocationViewer::OnStartExecuteButtonClicked))
    {
        StartExecuteButton->OnClicked.AddDynamic(this, &UMASkillAllocationViewer::OnStartExecuteButtonClicked);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("StartExecuteButton event bound"));
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
    
    // Initialize status log
    AppendStatusLog(TEXT("Skill Allocation Viewer started"));
    AppendStatusLog(TEXT("Tip: Edit JSON and click 'Update Skill List' to load data"));
    
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
    
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("BuildUI: Starting UI construction..."));

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

    // Create main background Border
    UBorder* MainBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBackground"));
    MainBackground->SetBrushColor(BackgroundColor);
    MainBackground->SetPadding(FMargin(10.0f));
    
    UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainBackground);
    // Full screen fill
    MainSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    MainSlot->SetOffsets(FMargin(0.0f));

    // Create main horizontal layout
    UHorizontalBox* MainHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainHBox"));
    MainBackground->AddChild(MainHBox);

    // Create left panel
    UBorder* LeftPanel = CreateLeftPanel();
    UHorizontalBoxSlot* LeftSlot = MainHBox->AddChildToHorizontalBox(LeftPanel);
    LeftSlot->SetPadding(FMargin(0, 0, 5, 0));
    // Set left panel ratio (using FillSize)
    FSlateChildSize LeftSize(ESlateSizeRule::Fill);
    LeftSize.Value = LeftPanelWidthRatio;
    LeftSlot->SetSize(LeftSize);

    // Create right panel
    UBorder* RightPanel = CreateRightPanel();
    UHorizontalBoxSlot* RightSlot = MainHBox->AddChildToHorizontalBox(RightPanel);
    // Set right panel ratio (using FillSize)
    FSlateChildSize RightSize(ESlateSizeRule::Fill);
    RightSize.Value = 1.0f - LeftPanelWidthRatio;
    RightSlot->SetSize(RightSize);

    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("BuildUI: UI construction completed successfully"));
}

UBorder* UMASkillAllocationViewer::CreateLeftPanel()
{
    // Left panel background
    UBorder* LeftPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanelBorder"));
    LeftPanelBorder->SetBrushColor(PanelBackgroundColor);
    LeftPanelBorder->SetPadding(FMargin(10.0f));

    // Vertical layout
    UVerticalBox* LeftVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVBox"));
    LeftPanelBorder->AddChild(LeftVBox);

    // Title
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LeftPanelTitle"));
    TitleText->SetText(FText::FromString(TEXT("Skill Allocation Viewer")));
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
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.6f)));
    
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

    // Label
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusLogLabel"));
    Label->SetText(FText::FromString(TEXT("Status Log:")));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // Use ScrollBox to wrap status log for auto-scrolling
    StatusLogScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("StatusLogScrollBox"));
    
    // Status log text box (read-only)
    StatusLogBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("StatusLogBox"));
    StatusLogBox->SetIsReadOnly(true);
    StatusLogBox->SetText(FText::GetEmpty());
    
    StatusLogScrollBox->AddChild(StatusLogBox);
    
    // Use SizeBox to set minimum height
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StatusLogSizeBox"));
    SizeBox->SetMinDesiredHeight(100.0f);
    SizeBox->AddChild(StatusLogScrollBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    return Section;
}

UVerticalBox* UMASkillAllocationViewer::CreateJsonEditorSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JsonEditorSection"));

    // Label
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonEditorLabel"));
    Label->SetText(FText::FromString(TEXT("JSON Editor:")));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // JSON editor text box (editable)
    JsonEditorBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditorBox"));
    JsonEditorBox->SetIsReadOnly(false);
    JsonEditorBox->SetText(FText::FromString(TEXT("{\n  \"name\": \"\",\n  \"description\": \"\",\n  \"data\": {}\n}")));
    
    // Use SizeBox to set minimum height
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditorSizeBox"));
    SizeBox->SetMinDesiredHeight(200.0f);
    SizeBox->AddChild(JsonEditorBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 10));

    // "Update Skill List" button
    UpdateButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpdateButton"));
    
    UTextBlock* UpdateButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpdateButtonText"));
    UpdateButtonText->SetText(FText::FromString(TEXT(" Update Skill List ")));
    UpdateButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    UpdateButton->AddChild(UpdateButtonText);
    
    UVerticalBoxSlot* UpdateButtonSlot = Section->AddChildToVerticalBox(UpdateButton);
    UpdateButtonSlot->SetHorizontalAlignment(HAlign_Left);
    UpdateButtonSlot->SetPadding(FMargin(0, 0, 0, 10));

    // "Start Executing" button
    StartExecuteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartExecuteButton"));

    UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
    StartButtonText->SetText(FText::FromString(TEXT(" Start Executing ")));
    StartButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    StartExecuteButton->AddChild(StartButtonText);

    UVerticalBoxSlot* StartButtonSlot = Section->AddChildToVerticalBox(StartExecuteButton);
    StartButtonSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UBorder* UMASkillAllocationViewer::CreateRightPanel()
{
    // Right panel background
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
    
    // Auto-scroll to latest content
    if (StatusLogScrollBox)
    {
        StatusLogScrollBox->ScrollToEnd();
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

void UMASkillAllocationViewer::StartExecution()
{
    if (!AllocationModel)
    {
        AppendStatusLog(TEXT("[Error] Internal error: AllocationModel is null"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: AllocationModel is null!"));
        return;
    }
    
    if (!AllocationModel->IsValidData())
    {
        AppendStatusLog(TEXT("[Error] Cannot start execution: No valid skill allocation loaded"));
        return;
    }
    
    if (bIsExecuting)
    {
        AppendStatusLog(TEXT("[Warning] Execution already in progress"));
        return;
    }
    
    // 使用模拟执行模式
    StartSimulatedExecution();
}

void UMASkillAllocationViewer::StartSimulatedExecution()
{
    // Set executing flag
    bIsExecuting = true;
    
    // Disable start button
    if (StartExecuteButton)
    {
        StartExecuteButton->SetIsEnabled(false);
    }
    
    // Reset simulation state
    CurrentSimulationTimeStep = 0;
    CurrentSimulationPhase = 0;
    
    // Log
    AppendStatusLog(FString::Printf(TEXT("[Simulating] Starting simulated execution (%d time steps, %d robots)"),
        AllocationModel->GetTimeStepCount(), AllocationModel->GetRobotCount()));
    
    OnExecutionStarted.Broadcast();
    
    // Start simulation timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            SimulationTimerHandle,
            this,
            &UMASkillAllocationViewer::OnSimulationTick,
            SimulationTickInterval,
            true,  // Loop
            0.0f   // Start immediately
        );
    }
    
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Simulated execution started"));
}

void UMASkillAllocationViewer::OnSimulationTick()
{
    AdvanceSimulation();
}

void UMASkillAllocationViewer::AdvanceSimulation()
{
    if (!AllocationModel || !AllocationModel->IsValidData())
    {
        // Stop simulation
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(SimulationTimerHandle);
        }
        bIsExecuting = false;
        return;
    }
    
    TArray<int32> TimeSteps = AllocationModel->GetAllTimeSteps();
    TimeSteps.Sort();
    
    if (CurrentSimulationTimeStep >= TimeSteps.Num())
    {
        // All time steps completed
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(SimulationTimerHandle);
        }
        OnExecutionCompleted();
        return;
    }
    
    int32 TimeStep = TimeSteps[CurrentSimulationTimeStep];
    TArray<FString> RobotIds = AllocationModel->GetAllRobotIds();
    
    if (CurrentSimulationPhase == 0)
    {
        // Phase 0: Set all skills at this time step to InProgress
        for (const FString& RobotId : RobotIds)
        {
            FMASkillAssignment Skill;
            if (AllocationModel->FindSkill(TimeStep, RobotId, Skill))
            {
                AllocationModel->UpdateSkillStatus(TimeStep, RobotId, ESkillExecutionStatus::InProgress);
            }
        }
        AppendStatusLog(FString::Printf(TEXT("[T%d] Skills started (InProgress)"), TimeStep));
        CurrentSimulationPhase = 1;
    }
    else
    {
        // Phase 1: Set all skills at this time step to Completed
        for (const FString& RobotId : RobotIds)
        {
            FMASkillAssignment Skill;
            if (AllocationModel->FindSkill(TimeStep, RobotId, Skill))
            {
                AllocationModel->UpdateSkillStatus(TimeStep, RobotId, ESkillExecutionStatus::Completed);
            }
        }
        AppendStatusLog(FString::Printf(TEXT("[T%d] Skills completed"), TimeStep));
        
        // Move to next time step
        CurrentSimulationTimeStep++;
        CurrentSimulationPhase = 0;
    }
    
    // Refresh Gantt canvas
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
}

bool UMASkillAllocationViewer::LoadMockData()
{
    // Build mock data file path
    // datasets folder is in the parent directory of the UE project
    FString FilePath = FPaths::ProjectDir() / TEXT("../datasets/skill_allocation_example.json");
    
    // Check if file exists
    if (!FPaths::FileExists(FilePath))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Mock data file not found: %s"), *FilePath));
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("Mock data file not found: %s"), *FilePath);
        return false;
    }
    
    // Read file content
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        AppendStatusLog(FString::Printf(TEXT("[Error] Unable to read file: %s"), *FilePath));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }
    
    // Load JSON
    if (!LoadSkillAllocationFromJson(JsonContent))
    {
        return false;
    }
    
    AppendStatusLog(TEXT("[Success] Mock data loaded"));
    
    return true;
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

void UMASkillAllocationViewer::OnStartExecuteButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("StartExecuteButton clicked"));
    StartExecution();
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
            // Check if all skills are completed
            if (AllocationModel && AreAllSkillsCompleted())
            {
                OnExecutionCompleted();
            }
            break;
        case ESkillExecutionStatus::Failed:
            StatusStr = TEXT("Failed");
            // Handle execution failure
            OnExecutionFailed(TimeStep, RobotId);
            break;
        default:
            StatusStr = TEXT("Unknown");
            UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("OnSkillStatusUpdated: Unknown status value %d"), (int32)Status);
            break;
    }
    
    AppendStatusLog(FString::Printf(TEXT("[Status] TimeStep %d, Robot %s: %s"),
        TimeStep, *RobotId, *StatusStr));
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

bool UMASkillAllocationViewer::AreAllSkillsCompleted() const
{
    if (!AllocationModel || !AllocationModel->IsValidData())
    {
        return false;
    }
    
    TArray<int32> TimeSteps = AllocationModel->GetAllTimeSteps();
    TArray<FString> RobotIds = AllocationModel->GetAllRobotIds();
    
    for (int32 TimeStep : TimeSteps)
    {
        for (const FString& RobotId : RobotIds)
        {
            FMASkillAssignment Skill;
            if (AllocationModel->FindSkill(TimeStep, RobotId, Skill))
            {
                if (Skill.Status != ESkillExecutionStatus::Completed)
                {
                    return false;
                }
            }
        }
    }
    
    return true;
}

void UMASkillAllocationViewer::OnExecutionCompleted()
{
    bIsExecuting = false;
    
    // Re-enable start button
    if (StartExecuteButton)
    {
        StartExecuteButton->SetIsEnabled(true);
    }
    
    AppendStatusLog(TEXT("[Success] All skills completed successfully"));
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Skill allocation execution completed successfully"));
}

void UMASkillAllocationViewer::OnExecutionFailed(int32 TimeStep, const FString& RobotId)
{
    bIsExecuting = false;
    
    // Re-enable start button to allow retry
    if (StartExecuteButton)
    {
        StartExecuteButton->SetIsEnabled(true);
    }
    
    AppendStatusLog(FString::Printf(TEXT("[Error] Execution failed at TimeStep %d, Robot %s"), TimeStep, *RobotId));
    UE_LOG(LogMASkillAllocationViewer, Error, TEXT("Skill allocation execution failed: TimeStep=%d, RobotId=%s"), TimeStep, *RobotId);
}

void UMASkillAllocationViewer::ResetExecution()
{
    bIsExecuting = false;
    
    // Re-enable start button
    if (StartExecuteButton)
    {
        StartExecuteButton->SetIsEnabled(true);
    }
    
    // Reset all skill statuses to Pending
    if (AllocationModel)
    {
        AllocationModel->ResetToOriginal();
    }
    
    // Refresh the Gantt canvas
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(TEXT("[Info] Execution reset - all skills set to Pending"));
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Execution reset"));
}
