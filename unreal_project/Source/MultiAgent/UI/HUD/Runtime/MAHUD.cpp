// MAHUD.cpp
// HUD 管理器实现
// 继承自 MASelectionHUD 以保留框选绘制功能
// Widget 管理职责已委托给 UMAUIManager

#include "MAHUD.h"
#include "../../../Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "../../Core/MAUIManager.h"
#include "../../../Input/MAPlayerController.h"
#include "../../../Core/Comm/Domain/MACommTypes.h"
#include "../../../Core/Shared/Types/MASimTypes.h"
#include "../../../Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"
#include "../../../Core/Interaction/Infrastructure/MAFeedback21Applier.h"
#include "../../SceneEditing/Infrastructure/MAEditWidgetRuntimeAdapter.h"
#include "../Infrastructure/MAHUDBackendRuntimeAdapter.h"
#include "../Infrastructure/MAHUDEditRuntimeAdapter.h"
#include "../Infrastructure/MAHUDSceneActionRuntimeAdapter.h"
#include "../Bootstrap/MAHUDBootstrap.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUD, Log, All);

namespace
{
const FMAHUDBootstrap& HUDBootstrap()
{
    static const FMAHUDBootstrap Bootstrap;
    return Bootstrap;
}

const FMAFeedback21Applier& HUDFeedbackApplier()
{
    static const FMAFeedback21Applier Applier;
    return Applier;
}

const FMAHUDBackendRuntimeAdapter& HUDBackendRuntimeAdapter()
{
    static const FMAHUDBackendRuntimeAdapter Adapter;
    return Adapter;
}

const FMAHUDEditRuntimeAdapter& HUDEditRuntimeAdapter()
{
    static const FMAHUDEditRuntimeAdapter Adapter;
    return Adapter;
}

const FMAHUDSceneActionRuntimeAdapter& HUDSceneActionRuntimeAdapter()
{
    static const FMAHUDSceneActionRuntimeAdapter Adapter;
    return Adapter;
}

const FMAEditWidgetRuntimeAdapter& EditWidgetRuntimeAdapter()
{
    static const FMAEditWidgetRuntimeAdapter Adapter;
    return Adapter;
}
}

//=============================================================================
// 构造函数
//=============================================================================

AMAHUD::AMAHUD()
{
    // 默认值
    bMainUIVisible = false;
    UIManager = nullptr;
    ViewCoordinator.SetUIManager(nullptr);
}

//=============================================================================
// AHUD 重写
//=============================================================================

void AMAHUD::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogMAHUD, Log, TEXT("MAHUD BeginPlay"));

    HUDBootstrap().InitializeHUD(this);
}

//=============================================================================
// 公共接口
//=============================================================================

void AMAHUD::ToggleMainUI()
{
    PanelCoordinator.ToggleMainUI(this);
}

void AMAHUD::ShowMainUI()
{
    PanelCoordinator.ShowMainUI(this);
}

void AMAHUD::HideMainUI()
{
    PanelCoordinator.HideMainUI(this);
}

void AMAHUD::ToggleSemanticMap()
{
    PanelCoordinator.ToggleSemanticMap(this);
}


void AMAHUD::DrawHUD()
{
    Super::DrawHUD();


    // Draw scene labels (Modify mode) - 保留，使用 DrawDebugString (世界空间)
    DrawSceneLabels();

    // Edit 模式指示器 - 数据收集并更新 HUDWidget
    DrawEditModeIndicator();

    // 绘制画中画相机
    DrawPIPCameras();
}

void AMAHUD::ShowDirectControlIndicator(AMACharacter* Agent)
{
    WidgetCoordinator.ShowDirectControlIndicator(UIManager, Agent ? Agent->AgentLabel : TEXT(""));
}

void AMAHUD::HideDirectControlIndicator()
{
    WidgetCoordinator.HideDirectControlIndicator(UIManager);
}

bool AMAHUD::IsDirectControlIndicatorVisible() const
{
    return WidgetCoordinator.IsDirectControlIndicatorVisible(UIManager);
}

//=============================================================================
// 技能分配查看器控制
//=============================================================================

void AMAHUD::ToggleSkillAllocationViewer()
{
    PanelCoordinator.ToggleSkillAllocationViewer(this);
}

void AMAHUD::ShowSkillAllocationViewer()
{
    PanelCoordinator.ShowSkillAllocationViewer(this);
}

void AMAHUD::HideSkillAllocationViewer()
{
    PanelCoordinator.HideSkillAllocationViewer(this);
}

bool AMAHUD::IsSkillAllocationViewerVisible() const
{
    return PanelCoordinator.IsSkillAllocationViewerVisible(this);
}

//=============================================================================
// 事件回调
//=============================================================================

void AMAHUD::OnMainUIToggled(bool bVisible)
{
    UE_LOG(LogMAHUD, Verbose, TEXT("OnMainUIToggled: %s"), bVisible ? TEXT("Show") : TEXT("Hide"));
    PanelCoordinator.HandleMainUIToggled(this, bVisible);
}

void AMAHUD::OnRefocusMainUI()
{
    UE_LOG(LogMAHUD, Verbose, TEXT("OnRefocusMainUI"));
    PanelCoordinator.HandleRefocusMainUI(this);
}

// 内部方法
//=============================================================================

void AMAHUD::BindWidgetDelegates()
{
    DelegateCoordinator.BindWidgetDelegates(this);
}

void AMAHUD::BindControllerEvents()
{
    DelegateCoordinator.BindControllerEvents(this);
}

AMAPlayerController* AMAHUD::GetMAPlayerController() const
{
    return Cast<AMAPlayerController>(GetOwningPlayerController());
}

void AMAHUD::ApplyInteractionFeedback(const FMAFeedback21Batch& Feedback)
{
    HUDFeedbackApplier().ApplyToHUD(this, Feedback);
}

bool AMAHUD::RuntimeBindBackendEvents()
{
    return HUDBackendRuntimeAdapter().BindBackendEvents(GetWorld(), this);
}

bool AMAHUD::RuntimeSendNaturalLanguageCommand(const FString& Command)
{
    return HUDBackendRuntimeAdapter().SendNaturalLanguageCommand(GetWorld(), this, Command);
}

bool AMAHUD::RuntimeHasCommSubsystem() const
{
    return HUDBackendRuntimeAdapter().ResolveCommSubsystem(GetWorld()) != nullptr;
}

void AMAHUD::RuntimeSendTaskGraphSubmit(const FString& TaskGraphJson)
{
    HUDBackendRuntimeAdapter().SendTaskGraphSubmit(GetWorld(), TaskGraphJson);
}

void AMAHUD::RuntimeSendReviewResponse(
    const bool bApproved,
    const FString& DataJson,
    const FString& RejectionReason)
{
    HUDBackendRuntimeAdapter().SendReviewResponse(GetWorld(), bApproved, DataJson, RejectionReason);
}

void AMAHUD::RuntimeSendButtonEvent(
    const FString& WidgetName,
    const FString& EventType,
    const FString& Label)
{
    HUDBackendRuntimeAdapter().SendButtonEvent(GetWorld(), WidgetName, EventType, Label);
}

bool AMAHUD::RuntimeLoadSceneGraphNodes(TArray<FMASceneGraphNode>& OutNodes)
{
    return HUDEditRuntimeAdapter().LoadSceneGraphNodes(this, OutNodes);
}

void AMAHUD::RuntimeDrawSceneLabels(const TArray<FMASceneGraphNode>& Nodes)
{
    HUDEditRuntimeAdapter().DrawSceneLabels(this, Nodes);
}

void AMAHUD::RuntimeDrawPIPCameras()
{
    HUDEditRuntimeAdapter().DrawPIPCameras(this);
}

bool AMAHUD::RuntimeBindEditModeSelectionChanged()
{
    return HUDEditRuntimeAdapter().BindEditModeSelectionChanged(this);
}

bool AMAHUD::RuntimeBuildEditModeIndicatorModel(FMAHUDEditModeIndicatorModel& OutModel) const
{
    return HUDEditRuntimeAdapter().BuildEditModeIndicatorModel(const_cast<AMAHUD*>(this), OutModel);
}

bool AMAHUD::RuntimeSelectGoalById(const FString& GoalId)
{
    return HUDEditRuntimeAdapter().SelectGoalById(this, GoalId);
}

bool AMAHUD::RuntimeSelectZoneById(const FString& ZoneId)
{
    return HUDEditRuntimeAdapter().SelectZoneById(this, ZoneId);
}

bool AMAHUD::RuntimeResolveCurrentEditSelection(
    TWeakObjectPtr<AActor>& OutSelectedActor,
    TArray<AMAPointOfInterest*>& OutSelectedPOIs)
{
    return EditWidgetRuntimeAdapter().ResolveCurrentSelection(this, OutSelectedActor, OutSelectedPOIs);
}

bool AMAHUD::RuntimeResolveEditActorNodes(
    AActor* Actor,
    TArray<FMASceneGraphNode>& OutNodes,
    FString& OutError)
{
    return EditWidgetRuntimeAdapter().ResolveActorNodes(this, Actor, OutNodes, OutError);
}

FMASceneActionResult AMAHUD::RuntimeApplySingleModify(AActor* Actor, const FString& LabelText)
{
    return HUDSceneActionRuntimeAdapter().ApplySingleModify(this, Actor, LabelText);
}

FMASceneActionResult AMAHUD::RuntimeApplyMultiModify(
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FString& GeneratedJson)
{
    return HUDSceneActionRuntimeAdapter().ApplyMultiModify(this, Actors, LabelText, GeneratedJson);
}

FMASceneActionResult AMAHUD::RuntimeApplyNodeEdit(AActor* Actor, const FString& JsonContent)
{
    return HUDSceneActionRuntimeAdapter().ApplyNodeEdit(this, Actor, JsonContent);
}

FMASceneActionResult AMAHUD::RuntimeDeleteActor(AActor* Actor)
{
    return HUDSceneActionRuntimeAdapter().DeleteActor(this, Actor);
}

FMASceneActionResult AMAHUD::RuntimeCreateGoal(AMAPointOfInterest* POI, const FString& Description)
{
    return HUDSceneActionRuntimeAdapter().CreateGoal(this, POI, Description);
}

FMASceneActionResult AMAHUD::RuntimeCreateZone(
    const TArray<AMAPointOfInterest*>& POIs,
    const FString& Description)
{
    return HUDSceneActionRuntimeAdapter().CreateZone(this, POIs, Description);
}

FMASceneActionResult AMAHUD::RuntimeAddPresetActor(AMAPointOfInterest* POI, const FString& ActorType)
{
    return HUDSceneActionRuntimeAdapter().AddPresetActor(this, POI, ActorType);
}

FMASceneActionResult AMAHUD::RuntimeDeletePOIs(const TArray<AMAPointOfInterest*>& POIs)
{
    return HUDSceneActionRuntimeAdapter().DeletePOIs(this, POIs);
}

FMASceneActionResult AMAHUD::RuntimeSetGoalState(AActor* Actor, const bool bShouldSetGoal)
{
    return HUDSceneActionRuntimeAdapter().SetGoalState(this, Actor, bShouldSetGoal);
}

void AMAHUD::OnSimpleCommandSubmitted(const FString& Command)
{
    UE_LOG(LogMAHUD, Log, TEXT("Command submitted: %s"), *Command);
    BackendCoordinator.HandleSimpleCommandSubmitted(GetWorld(), UIManager, this, Command);
}

void AMAHUD::OnPlannerResponse(const FMAPlannerResponse& Response)
{
    UE_LOG(LogMAHUD, Log, TEXT("Planner response received: %s"), *Response.Message);
    BackendCoordinator.HandlePlannerResponse(this, UIManager, Response);
}

void AMAHUD::LoadTaskGraph(const FMATaskGraphData& Data)
{
    WidgetCoordinator.LoadTaskGraph(UIManager, Data);
}

//=============================================================================
// Widget Getter 方法 (委托到 UIManager)
//=============================================================================

UMATaskPlannerWidget* AMAHUD::GetTaskPlannerWidget() const
{
    return UIManager ? UIManager->GetTaskPlannerWidget() : nullptr;
}

UMAHUDStateManager* AMAHUD::GetHUDStateManager() const
{
    return UIManager ? UIManager->GetHUDStateManager() : nullptr;
}

UMAMainHUDWidget* AMAHUD::GetMainHUDWidget() const
{
    return UIManager ? UIManager->GetMainHUDWidget() : nullptr;
}

//=============================================================================
// Modify 模式 UI 控制
//=============================================================================

void AMAHUD::ShowModifyWidget()
{
    PanelCoordinator.ShowModifyWidget(this);
}

void AMAHUD::HideModifyWidget()
{
    PanelCoordinator.HideModifyWidget(this);
}

bool AMAHUD::IsModifyWidgetVisible() const
{
    return PanelCoordinator.IsModifyWidgetVisible(this);
}

//=============================================================================
// Edit 模式 UI 控制
//=============================================================================

void AMAHUD::ShowEditWidget()
{
    PanelCoordinator.ShowEditWidget(this);
}

void AMAHUD::HideEditWidget()
{
    PanelCoordinator.HideEditWidget(this);
}

bool AMAHUD::IsEditWidgetVisible() const
{
    return PanelCoordinator.IsEditWidgetVisible(this);
}

bool AMAHUD::IsMouseOverEditWidget() const
{
    return PanelCoordinator.IsMouseOverEditWidget(this);
}

//=============================================================================
// System Log Panel 控制
//=============================================================================

void AMAHUD::ShowSystemLogPanel()
{
    if (ViewCoordinator.ShowWidget(EMAWidgetType::SystemLogPanel, false, TEXT("SystemLogPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("SystemLogPanel shown via UIManager"));
    }
}

void AMAHUD::HideSystemLogPanel()
{
    if (ViewCoordinator.HideWidget(EMAWidgetType::SystemLogPanel, TEXT("SystemLogPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("SystemLogPanel hidden via UIManager"));
    }
}

void AMAHUD::ToggleSystemLogPanel()
{
    if (ViewCoordinator.ToggleWidget(EMAWidgetType::SystemLogPanel, TEXT("SystemLogPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("SystemLogPanel toggled via UIManager"));
    }
}

bool AMAHUD::IsSystemLogPanelVisible() const
{
    return ViewCoordinator.IsWidgetVisible(EMAWidgetType::SystemLogPanel);
}

//=============================================================================
// Preview Panel 控制
//=============================================================================

void AMAHUD::ShowPreviewPanel()
{
    if (ViewCoordinator.ShowWidget(EMAWidgetType::PreviewPanel, false, TEXT("PreviewPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("PreviewPanel shown via UIManager"));
    }
}

void AMAHUD::HidePreviewPanel()
{
    if (ViewCoordinator.HideWidget(EMAWidgetType::PreviewPanel, TEXT("PreviewPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("PreviewPanel hidden via UIManager"));
    }
}

void AMAHUD::TogglePreviewPanel()
{
    if (ViewCoordinator.ToggleWidget(EMAWidgetType::PreviewPanel, TEXT("PreviewPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("PreviewPanel toggled via UIManager"));
    }
}

bool AMAHUD::IsPreviewPanelVisible() const
{
    return ViewCoordinator.IsWidgetVisible(EMAWidgetType::PreviewPanel);
}

//=============================================================================
// Instruction Panel 控制
//=============================================================================

void AMAHUD::ShowInstructionPanel()
{
    if (ViewCoordinator.ShowWidget(EMAWidgetType::InstructionPanel, false, TEXT("InstructionPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("InstructionPanel shown via UIManager"));
    }
}

void AMAHUD::HideInstructionPanel()
{
    if (ViewCoordinator.HideWidget(EMAWidgetType::InstructionPanel, TEXT("InstructionPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("InstructionPanel hidden via UIManager"));
    }
}

void AMAHUD::ToggleInstructionPanel()
{
    if (ViewCoordinator.ToggleWidget(EMAWidgetType::InstructionPanel, TEXT("InstructionPanel")))
    {
        UE_LOG(LogMAHUD, Log, TEXT("InstructionPanel toggled via UIManager"));
    }
}

bool AMAHUD::IsInstructionPanelVisible() const
{
    return ViewCoordinator.IsWidgetVisible(EMAWidgetType::InstructionPanel);
}

bool AMAHUD::IsMouseOverRightSidebar() const
{
    return ViewCoordinator.IsMouseOverRightSidebar(GetOwningPlayerController());
}

bool AMAHUD::IsMouseOverPersistentUI() const
{
    return ViewCoordinator.IsMouseOverPersistentUI(GetOwningPlayerController());
}

void AMAHUD::OnModifyConfirmed(AActor* Actor, const FString& LabelText)
{
    SceneEditCoordinator.HandleModifyConfirmed(this, Actor, LabelText);
}

void AMAHUD::OnModifyActorSelected(AActor* SelectedActor)
{
    SceneEditCoordinator.HandleModifyActorSelected(this, SelectedActor);
}

void AMAHUD::OnModifyActorsSelected(const TArray<AActor*>& SelectedActors)
{
    SceneEditCoordinator.HandleModifyActorsSelected(this, SelectedActors);
}

void AMAHUD::OnMultiSelectModifyConfirmed(const TArray<AActor*>& Actors, const FString& LabelText, const FString& GeneratedJson)
{
    SceneEditCoordinator.HandleMultiSelectModifyConfirmed(this, Actors, LabelText, GeneratedJson);
}

//=============================================================================
// 通知系统
//=============================================================================

void AMAHUD::ShowNotification(const FString& Message, bool bIsError, bool bIsWarning)
{
    ViewCoordinator.ShowNotification(Message, bIsError, bIsWarning);

    UE_LOG(LogMAHUD, Log, TEXT("ShowNotification: %s (Error=%d, Warning=%d)"),
        *Message, bIsError, bIsWarning);
}

void AMAHUD::ClearHighlightedActor()
{
    AMAPlayerController* MAPC = GetMAPlayerController();
    if (MAPC)
    {
        // 调用 PlayerController 的方法清除高亮
        // 这会触发 OnModifyActorSelected(nullptr)
        MAPC->ClearModifyHighlight();
    }
}

//=============================================================================
// 场景标签可视化
//=============================================================================

void AMAHUD::LoadSceneGraphForVisualization()
{
    OverlayCoordinator.LoadSceneGraphForVisualization(this);
}

void AMAHUD::DrawSceneLabels()
{
    OverlayCoordinator.DrawSceneLabels(this);
}

void AMAHUD::DrawPIPCameras()
{
    OverlayCoordinator.DrawPIPCameras(this);
}

void AMAHUD::StartSceneLabelVisualization()
{
    OverlayCoordinator.StartSceneLabelVisualization(this);
}

void AMAHUD::StopSceneLabelVisualization()
{
    OverlayCoordinator.StopSceneLabelVisualization(this);
}

//=============================================================================
// Edit 模式集成
//=============================================================================

void AMAHUD::BindEditModeManagerEvents()
{
    OverlayCoordinator.BindEditModeManagerEvents(this);
}

void AMAHUD::BindEditWidgetDelegates()
{
    OverlayCoordinator.BindEditWidgetDelegates(this);
}

void AMAHUD::DrawEditModeIndicator()
{
    OverlayCoordinator.DrawEditModeIndicator(this);
}

void AMAHUD::OnEditModeSelectionChanged()
{
    OverlayCoordinator.HandleEditModeSelectionChanged(this);
}

void AMAHUD::OnTempSceneGraphChanged()
{
    OverlayCoordinator.HandleTempSceneGraphChanged(this);
}

void AMAHUD::OnEditConfirmRequested(const FString& JsonContent)
{
    OverlayCoordinator.HandleEditConfirmRequested(this, JsonContent);
}

void AMAHUD::OnEditDeleteRequested()
{
    OverlayCoordinator.HandleEditDeleteRequested(this);
}

void AMAHUD::OnEditCreateGoalRequested(const FString& Description)
{
    OverlayCoordinator.HandleEditCreateGoalRequested(this, Description);
}

void AMAHUD::OnEditCreateZoneRequested(const FString& Description)
{
    OverlayCoordinator.HandleEditCreateZoneRequested(this, Description);
}

void AMAHUD::OnEditAddPresetActorRequested(const FString& ActorType)
{
    OverlayCoordinator.HandleEditAddPresetActorRequested(this, ActorType);
}

void AMAHUD::OnEditDeletePOIsRequested()
{
    OverlayCoordinator.HandleEditDeletePOIsRequested(this);
}

void AMAHUD::OnEditSetAsGoalRequested()
{
    OverlayCoordinator.HandleEditSetAsGoalRequested(this);
}

void AMAHUD::OnEditUnsetAsGoalRequested()
{
    OverlayCoordinator.HandleEditUnsetAsGoalRequested(this);
}

void AMAHUD::OnEditNodeSwitchRequested(int32 NodeIndex)
{
    OverlayCoordinator.HandleEditNodeSwitchRequested(this, NodeIndex);
}

void AMAHUD::OnSceneListGoalClicked(const FString& GoalId)
{
    SceneEditCoordinator.HandleSceneListGoalClicked(this, GoalId);
}

void AMAHUD::OnSceneListZoneClicked(const FString& ZoneId)
{
    SceneEditCoordinator.HandleSceneListZoneClicked(this, ZoneId);
}

//=============================================================================
// 后端事件绑定
//=============================================================================

void AMAHUD::BindBackendEvents()
{
    BackendCoordinator.BindBackendEvents(GetWorld(), UIManager, this);
}

//=============================================================================
// 后端事件回调
//=============================================================================

void AMAHUD::OnTaskGraphReceived(const FMATaskPlan& TaskPlan)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnTaskGraphReceived: Received task graph update with %d nodes"), TaskPlan.Nodes.Num());
    BackendCoordinator.HandleTaskGraphReceived(this, UIManager, TaskPlan);
}

void AMAHUD::OnSkillAllocationReceived(const FMASkillAllocationData& AllocationData)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnSkillAllocationReceived: Received skill allocation data, Name=%s"), *AllocationData.Name);
    BackendCoordinator.HandleSkillAllocationReceived(this, UIManager, AllocationData);
}


void AMAHUD::OnSkillListReceived(const FMASkillListMessage& SkillList, bool bExecutable)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnSkillListReceived: Received skill list update with %d time steps, executable=%s"), 
        SkillList.TotalTimeSteps, bExecutable ? TEXT("true") : TEXT("false"));
    BackendCoordinator.HandleSkillListReceived(this, UIManager, SkillList, bExecutable);
}

//=============================================================================
// 模态操作回调
//=============================================================================

void AMAHUD::OnModalConfirmedHandler(EMAModalType ModalType)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModalConfirmedHandler: Modal confirmed, type=%s"),
        *UEnum::GetValueAsString(ModalType));
    BackendCoordinator.HandleModalConfirmed(this, UIManager, ModalType);

    // 显示确认通知
    ShowNotification(TEXT("Changes confirmed"), false);
}

void AMAHUD::OnModalRejectedHandler(EMAModalType ModalType)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModalRejectedHandler: Modal rejected, type=%s"),
        *UEnum::GetValueAsString(ModalType));
    const bool bRejectedHandled = BackendCoordinator.HandleModalRejected(this, UIManager, ModalType);

    if (bRejectedHandled)
    {
        // 显示拒绝通知
        ShowNotification(TEXT("Changes discarded"), false, true);
    }
}
