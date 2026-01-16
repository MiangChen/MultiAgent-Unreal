// MASkillAllocationViewer.cpp
// Skill Allocation Viewer main container Widget - Pure C++ implementation

#include "MASkillAllocationViewer.h"
#include "MAGanttCanvas.h"
#include "MASkillAllocationModel.h"
#include "../../Core/Comm/MACommSubsystem.h"
#include "../../Core/Manager/MATempDataManager.h"
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

    if (ResetButton && !ResetButton->OnClicked.IsAlreadyBound(this, &UMASkillAllocationViewer::OnResetButtonClicked))
    {
        ResetButton->OnClicked.AddDynamic(this, &UMASkillAllocationViewer::OnResetButtonClicked);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("ResetButton event bound"));
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
    
    // Bind TempDataManager skill list change event
    if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld()))
    {
        if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
        {
            if (!TempDataMgr->OnSkillListChanged.IsAlreadyBound(this, &UMASkillAllocationViewer::OnTempSkillListChanged))
            {
                TempDataMgr->OnSkillListChanged.AddDynamic(this, &UMASkillAllocationViewer::OnTempSkillListChanged);
                UE_LOG(LogMASkillAllocationViewer, Log, TEXT("TempDataManager OnSkillListChanged event bound"));
            }
        }
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
    StartButtonSlot->SetPadding(FMargin(0, 0, 0, 10));

    // "Reset" button
    ResetButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResetButton"));

    UTextBlock* ResetButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResetButtonText"));
    ResetButtonText->SetText(FText::FromString(TEXT(" Reset ")));
    ResetButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ResetButton->AddChild(ResetButtonText);

    UVerticalBoxSlot* ResetButtonSlot = Section->AddChildToVerticalBox(ResetButton);
    ResetButtonSlot->SetHorizontalAlignment(HAlign_Left);

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
    // 1. 验证 AllocationModel 数据有效性
    if (!AllocationModel)
    {
        AppendStatusLog(TEXT("[错误] 内部错误: AllocationModel 为空"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: AllocationModel is null!"));
        return;
    }
    
    if (bIsExecuting)
    {
        AppendStatusLog(TEXT("[警告] 执行已在进行中"));
        return;
    }
    
    // 从 TempDataManager 读取技能列表
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
    if (!GameInstance)
    {
        AppendStatusLog(TEXT("[错误] 无法获取 GameInstance"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: Failed to get GameInstance"));
        return;
    }
    
    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        AppendStatusLog(TEXT("[错误] TempDataManager 不可用"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: TempDataManager not available"));
        return;
    }
    
    // 检查技能列表文件是否存在
    if (!TempDataMgr->SkillListFileExists())
    {
        AppendStatusLog(TEXT("[错误] 技能列表文件不存在，请先从后端发送技能列表"));
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("StartExecution: skill_list_temp.json does not exist"));
        return;
    }
    
    // 从临时文件加载技能列表
    FMASkillAllocationData Data;
    if (!TempDataMgr->LoadSkillList(Data))
    {
        AppendStatusLog(TEXT("[错误] 从临时文件加载技能列表失败"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: Failed to load skill list from temp file"));
        return;
    }
    
    // 检查数据是否为空
    if (Data.Data.Num() == 0)
    {
        AppendStatusLog(TEXT("[错误] 技能列表为空，请先从后端发送技能列表"));
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("StartExecution: Skill list is empty"));
        return;
    }
    
    // 加载数据到模型
    AllocationModel->LoadFromData(Data);
    
    // 同步 JSON 编辑器
    SyncJsonEditorFromModel();
    
    // 刷新甘特图
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(FString::Printf(TEXT("[信息] 从临时文件加载技能列表: %s"), *Data.Name));
    
    // 2. 获取 MACommandManager 子系统
    UWorld* World = GetWorld();
    if (!World)
    {
        AppendStatusLog(TEXT("[错误] 无法获取 World"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: World is null"));
        return;
    }
    
    UMACommandManager* CommandMgr = World->GetSubsystem<UMACommandManager>();
    if (!CommandMgr)
    {
        AppendStatusLog(TEXT("[错误] 无法获取 MACommandManager"));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: MACommandManager not available"));
        return;
    }
    
    // 3. 调用 ConvertToSkillListMessage 转换数据
    FMASkillListMessage SkillListMsg;
    FString ErrorMessage;
    if (!ConvertToSkillListMessage(AllocationModel->GetWorkingData(), SkillListMsg, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] 数据转换失败: %s"), *ErrorMessage));
        UE_LOG(LogMASkillAllocationViewer, Error, TEXT("StartExecution: Data conversion failed: %s"), *ErrorMessage);
        return;
    }
    
    // 4. 如果 MACommandManager 正在执行中，先中断当前执行
    if (CommandMgr->IsExecuting())
    {
        AppendStatusLog(TEXT("[警告] 中断当前执行以启动新执行"));
        CommandMgr->InterruptCurrentExecution();
    }
    
    // 5. 绑定事件
    BindCommandManagerEvents();
    
    // 6. 设置 bIsExecuting = true
    bIsExecuting = true;
    
    // 7. 禁用 StartExecuteButton
    if (StartExecuteButton)
    {
        StartExecuteButton->SetIsEnabled(false);
    }
    
    // 8. 将所有技能设置为 Pending 状态 (通过重置到原始数据)
    AllocationModel->ResetToOriginal();
    
    // 刷新甘特图显示 Pending 状态
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    // 9. 记录执行开始日志
    AppendStatusLog(FString::Printf(TEXT("[执行] 开始执行技能列表 (%d 个时间步)"), SkillListMsg.TotalTimeSteps));
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("StartExecution: Starting real execution with %d time steps"), SkillListMsg.TotalTimeSteps);
    
    // 广播执行开始事件
    OnExecutionStarted.Broadcast();
    
    // 10. 调用 CommandMgr->ExecuteSkillList() 启动真实执行
    CommandMgr->ExecuteSkillList(SkillListMsg);
}

// StartSimulatedExecution, OnSimulationTick, AdvanceSimulation 已移除
// 现在使用真实执行模式，通过 MACommandManager::ExecuteSkillList() 执行技能

bool UMASkillAllocationViewer::LoadMockData()
{
    // 已废弃: 不再从 datasets/skill_allocation_example.json 读取
    // 请使用 TempDataManager 从 skill_list_temp.json 读取数据
    AppendStatusLog(TEXT("[Warning] LoadMockData is deprecated. Use TempDataManager instead."));
    UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("LoadMockData is deprecated. Data should be loaded from skill_list_temp.json via TempDataManager."));
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

void UMASkillAllocationViewer::OnStartExecuteButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("StartExecuteButton clicked"));
    StartExecution();
}

void UMASkillAllocationViewer::OnResetButtonClicked()
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("ResetButton clicked"));
    
    // 如果正在执行，先中断执行
    if (bIsExecuting)
    {
        // 获取 MACommandManager 并中断当前执行
        UWorld* World = GetWorld();
        if (World)
        {
            UMACommandManager* CommandMgr = World->GetSubsystem<UMACommandManager>();
            if (CommandMgr && CommandMgr->IsExecuting())
            {
                CommandMgr->InterruptCurrentExecution();
                AppendStatusLog(TEXT("[警告] 执行被中断"));
            }
        }
        
        // 解绑事件
        UnbindCommandManagerEvents();
        
        // 重置执行状态
        bIsExecuting = false;
        
        // 重新启用 StartExecuteButton
        if (StartExecuteButton)
        {
            StartExecuteButton->SetIsEnabled(true);
        }
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

void UMASkillAllocationViewer::OnTempSkillListChanged(const FMASkillAllocationData& NewData)
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnTempSkillListChanged: Received new skill list data"));
    
    // 如果正在执行，不自动刷新
    if (bIsExecuting)
    {
        AppendStatusLog(TEXT("[Info] Skill list updated in temp file (execution in progress, not auto-refreshing)"));
        return;
    }
    
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
    
    // 广播执行完成事件
    OnExecutionCompletedDelegate.Broadcast();
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

//=============================================================================
// MACommandManager Event Binding
//=============================================================================

void UMASkillAllocationViewer::BindCommandManagerEvents()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("BindCommandManagerEvents: World is null"));
        return;
    }
    
    UMACommandManager* CommandMgr = World->GetSubsystem<UMACommandManager>();
    if (!CommandMgr)
    {
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("BindCommandManagerEvents: MACommandManager not found"));
        return;
    }
    
    // Bind OnTimeStepCompleted delegate
    if (!CommandMgr->OnTimeStepCompleted.IsAlreadyBound(this, &UMASkillAllocationViewer::OnCommandManagerTimeStepCompleted))
    {
        CommandMgr->OnTimeStepCompleted.AddDynamic(this, &UMASkillAllocationViewer::OnCommandManagerTimeStepCompleted);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Bound OnTimeStepCompleted delegate"));
    }
    
    // Bind OnSkillListCompleted delegate
    if (!CommandMgr->OnSkillListCompleted.IsAlreadyBound(this, &UMASkillAllocationViewer::OnCommandManagerSkillListCompleted))
    {
        CommandMgr->OnSkillListCompleted.AddDynamic(this, &UMASkillAllocationViewer::OnCommandManagerSkillListCompleted);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Bound OnSkillListCompleted delegate"));
    }
}

void UMASkillAllocationViewer::UnbindCommandManagerEvents()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    UMACommandManager* CommandMgr = World->GetSubsystem<UMACommandManager>();
    if (!CommandMgr)
    {
        return;
    }
    
    // Unbind OnTimeStepCompleted delegate
    if (CommandMgr->OnTimeStepCompleted.IsAlreadyBound(this, &UMASkillAllocationViewer::OnCommandManagerTimeStepCompleted))
    {
        CommandMgr->OnTimeStepCompleted.RemoveDynamic(this, &UMASkillAllocationViewer::OnCommandManagerTimeStepCompleted);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Unbound OnTimeStepCompleted delegate"));
    }
    
    // Unbind OnSkillListCompleted delegate
    if (CommandMgr->OnSkillListCompleted.IsAlreadyBound(this, &UMASkillAllocationViewer::OnCommandManagerSkillListCompleted))
    {
        CommandMgr->OnSkillListCompleted.RemoveDynamic(this, &UMASkillAllocationViewer::OnCommandManagerSkillListCompleted);
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("Unbound OnSkillListCompleted delegate"));
    }
}

void UMASkillAllocationViewer::OnCommandManagerTimeStepCompleted(const FMATimeStepFeedback& Feedback)
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnCommandManagerTimeStepCompleted: TimeStep %d with %d skill feedbacks"), 
        Feedback.TimeStep, Feedback.SkillFeedbacks.Num());
    
    if (!AllocationModel)
    {
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("OnCommandManagerTimeStepCompleted: AllocationModel is null"));
        return;
    }
    
    int32 TimeStep = Feedback.TimeStep;
    
    // 解析 FMATimeStepFeedback 中的技能反馈
    for (const FMASkillExecutionFeedback& SkillFeedback : Feedback.SkillFeedbacks)
    {
        // 根据 bSuccess 确定新状态
        ESkillExecutionStatus NewStatus = SkillFeedback.bSuccess 
            ? ESkillExecutionStatus::Completed 
            : ESkillExecutionStatus::Failed;
        
        // 调用 AllocationModel->UpdateSkillStatus() 更新模型中的状态
        bool bUpdated = AllocationModel->UpdateSkillStatus(TimeStep, SkillFeedback.AgentId, NewStatus);
        
        if (!bUpdated)
        {
            UE_LOG(LogMASkillAllocationViewer, Warning, 
                TEXT("OnCommandManagerTimeStepCompleted: Failed to update status for TimeStep=%d, AgentId=%s"),
                TimeStep, *SkillFeedback.AgentId);
        }
        
        // 记录状态日志
        FString StatusStr = SkillFeedback.bSuccess ? TEXT("完成") : TEXT("失败");
        FString LogMessage = FString::Printf(TEXT("[T%d] %s [%s]: %s"), 
            TimeStep, *SkillFeedback.AgentId, *SkillFeedback.SkillName, *StatusStr);
        
        // 如果有额外消息，附加到日志
        if (!SkillFeedback.Message.IsEmpty())
        {
            LogMessage += FString::Printf(TEXT(" - %s"), *SkillFeedback.Message);
        }
        
        AppendStatusLog(LogMessage);
        
        // 如果技能失败，记录错误日志
        if (!SkillFeedback.bSuccess)
        {
            AppendStatusLog(FString::Printf(TEXT("[错误] 技能失败: %s - %s"), 
                *SkillFeedback.AgentId, *SkillFeedback.SkillName));
        }
    }
    
    // 刷新甘特图
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    // 记录时间步进度日志
    if (AllocationModel->IsValidData())
    {
        int32 TotalTimeSteps = AllocationModel->GetTimeStepCount();
        AppendStatusLog(FString::Printf(TEXT("[进度] 正在执行 TimeStep %d/%d"), TimeStep + 1, TotalTimeSteps));
    }
}

void UMASkillAllocationViewer::OnCommandManagerSkillListCompleted(const TArray<FMATimeStepFeedback>& AllFeedbacks)
{
    UE_LOG(LogMASkillAllocationViewer, Log, TEXT("OnCommandManagerSkillListCompleted: %d time steps completed"), AllFeedbacks.Num());
    
    // 设置 bIsExecuting = false
    bIsExecuting = false;
    
    // 重新启用 StartExecuteButton
    if (StartExecuteButton)
    {
        StartExecuteButton->SetIsEnabled(true);
    }
    
    // 解绑事件
    UnbindCommandManagerEvents();
    
    // 检查是否全部成功
    bool bAllSuccess = true;
    int32 FailedCount = 0;
    FString FirstFailedAgent;
    FString FirstFailedSkill;
    
    for (const FMATimeStepFeedback& TSFeedback : AllFeedbacks)
    {
        for (const FMASkillExecutionFeedback& SkillFeedback : TSFeedback.SkillFeedbacks)
        {
            if (!SkillFeedback.bSuccess)
            {
                bAllSuccess = false;
                FailedCount++;
                
                // 记录第一个失败的技能信息
                if (FirstFailedAgent.IsEmpty())
                {
                    FirstFailedAgent = SkillFeedback.AgentId;
                    FirstFailedSkill = SkillFeedback.SkillName;
                }
            }
        }
    }
    
    // 记录日志
    if (bAllSuccess)
    {
        AppendStatusLog(TEXT("[成功] 所有技能已完成"));
        UE_LOG(LogMASkillAllocationViewer, Log, TEXT("All skills completed successfully"));
    }
    else
    {
        if (FailedCount == 1)
        {
            AppendStatusLog(FString::Printf(TEXT("[错误] 技能失败: %s - %s"), 
                *FirstFailedAgent, *FirstFailedSkill));
        }
        else
        {
            AppendStatusLog(FString::Printf(TEXT("[警告] %d 个技能执行失败"), FailedCount));
        }
        UE_LOG(LogMASkillAllocationViewer, Warning, TEXT("Skill list execution completed with %d failures"), FailedCount);
    }
    
    // 广播 OnExecutionCompleted 事件
    OnExecutionCompletedDelegate.Broadcast();
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
