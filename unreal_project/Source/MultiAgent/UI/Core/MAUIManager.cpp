// MAUIManager.cpp
// UI Manager 实现 - Widget 生命周期管理
// 集成 HUD 状态管理器，处理 UI 状态转换和模态窗口管理

#include "MAUIManager.h"
#include "MAUITheme.h"
#include "../HUD/Presentation/MAHUDWidget.h"
#include "../HUD/Presentation/MAMainHUDWidget.h"
#include "../TaskGraph/Presentation/MATaskGraphModal.h"
#include "../SkillAllocation/Presentation/MASkillAllocationModal.h"
#include "Modal/MADecisionModal.h"
#include "Modal/MABaseModalWidget.h"
#include "../TaskGraph/Presentation/MATaskPlannerWidget.h"
#include "../SkillAllocation/Presentation/MASkillAllocationViewer.h"
#include "../../Components/Presentation/MADirectControlIndicator.h"
#include "../SceneEditing/Presentation/MAModifyWidget.h"
#include "../SceneEditing/Presentation/MAEditWidget.h"
#include "../SceneEditing/Presentation/MASceneListWidget.h"
#include "../../Components/Presentation/MASystemLogPanel.h"
#include "../../Components/Presentation/MAPreviewPanel.h"
#include "../../Components/Presentation/MAInstructionPanel.h"
#include "MAHUDStateManager.h"

DEFINE_LOG_CATEGORY(LogMAUIManager);

//=============================================================================
// 初始化
//=============================================================================

void UMAUIManager::Initialize(APlayerController* InOwningPC)
{
    if (!InOwningPC)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("Initialize: PlayerController is null"));
        return;
    }

    OwningPC = InOwningPC;
    
    // 初始化 HUD 状态管理器
    InitializeHUDStateManager();
    
    UE_LOG(LogMAUIManager, Log, TEXT("UIManager initialized"));
}

void UMAUIManager::CreateAllWidgets()
{
    WidgetLifecycleCoordinator.CreateAllWidgets(this);
}

void UMAUIManager::SetHUDStateManagerInternal(UMAHUDStateManager* InHUDStateManager)
{
    HUDStateManager = InHUDStateManager;
}

void UMAUIManager::SetMainHUDWidgetInternal(UMAMainHUDWidget* InMainHUDWidget)
{
    MainHUDWidget = InMainHUDWidget;
}

void UMAUIManager::SetWidgetInstanceInternal(EMAWidgetType Type, UUserWidget* Widget)
{
    switch (Type)
    {
    case EMAWidgetType::HUD:
        HUDWidget = Cast<UMAHUDWidget>(Widget);
        break;
    case EMAWidgetType::TaskPlanner:
        TaskPlannerWidget = Cast<UMATaskPlannerWidget>(Widget);
        break;
    case EMAWidgetType::SkillAllocation:
        SkillAllocationViewer = Cast<UMASkillAllocationViewer>(Widget);
        break;
    case EMAWidgetType::SemanticMap:
        SemanticMapWidget = Widget;
        break;
    case EMAWidgetType::DirectControl:
        DirectControlIndicator = Cast<UMADirectControlIndicator>(Widget);
        break;
    case EMAWidgetType::Modify:
        ModifyWidget = Cast<UMAModifyWidget>(Widget);
        break;
    case EMAWidgetType::Edit:
        EditWidget = Cast<UMAEditWidget>(Widget);
        break;
    case EMAWidgetType::SceneList:
        SceneListWidget = Cast<UMASceneListWidget>(Widget);
        break;
    case EMAWidgetType::SystemLogPanel:
        SystemLogPanel = Cast<UMASystemLogPanel>(Widget);
        break;
    case EMAWidgetType::PreviewPanel:
        PreviewPanel = Cast<UMAPreviewPanel>(Widget);
        break;
    case EMAWidgetType::InstructionPanel:
        InstructionPanel = Cast<UMAInstructionPanel>(Widget);
        break;
    default:
        break;
    }
}

void UMAUIManager::SetModalWidgetInternal(EMAModalType ModalType, UMABaseModalWidget* ModalWidget)
{
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        TaskGraphModal = Cast<UMATaskGraphModal>(ModalWidget);
        break;
    case EMAModalType::SkillAllocation:
        SkillAllocationModal = Cast<UMASkillAllocationModal>(ModalWidget);
        break;
    case EMAModalType::Decision:
        DecisionModal = Cast<UMADecisionModal>(ModalWidget);
        break;
    default:
        break;
    }
}

void UMAUIManager::RegisterWidgetMappingInternal(EMAWidgetType Type, UUserWidget* Widget)
{
    if (Type == EMAWidgetType::None || !Widget)
    {
        return;
    }

    Widgets.Add(Type, Widget);
}

void UMAUIManager::BindRuntimeEventSourcesInternal()
{
    BindTempDataManagerEvents();
    BindCommSubsystemEvents();
    BindCommandManagerEvents();
}

bool UMAUIManager::BindTempDataEventsInternal(FString& OutError)
{
    return RuntimeBridge.BindTempDataEvents(this, OutError);
}

bool UMAUIManager::BindCommEventsInternal(FString& OutError)
{
    return RuntimeBridge.BindCommEvents(this, OutError);
}

bool UMAUIManager::BindCommandEventsInternal(FString& OutError)
{
    return RuntimeBridge.BindCommandEvents(this, OutError);
}

bool UMAUIManager::LoadTaskGraphDataInternal(FMATaskGraphData& OutData, FString& OutError) const
{
    return RuntimeBridge.TryLoadTaskGraphData(this, OutData, OutError);
}

bool UMAUIManager::LoadSkillAllocationDataInternal(FMASkillAllocationData& OutData, FString& OutError) const
{
    return RuntimeBridge.TryLoadSkillAllocationData(this, OutData, OutError);
}

bool UMAUIManager::SendDecisionResponseInternal(const FString& SelectedOption, const FString& UserFeedback, FString& OutError) const
{
    return RuntimeBridge.TrySendDecisionResponse(this, SelectedOption, UserFeedback, OutError);
}

bool UMAUIManager::ClearResumeNotificationTimerInternal(FString& OutError)
{
    return RuntimeBridge.TryClearResumeNotificationTimer(this, ResumeNotificationTimerHandle, OutError);
}

bool UMAUIManager::ScheduleResumeNotificationAutoHideInternal(float DelaySeconds, FString& OutError)
{
    return RuntimeBridge.TryScheduleResumeNotificationAutoHide(this, ResumeNotificationTimerHandle, DelaySeconds, OutError);
}

bool UMAUIManager::IsPhaseNotificationEnabledInternal() const
{
    return RuntimeBridge.IsPhaseNotificationEnabled(this);
}

void UMAUIManager::CreateModalWidgetsInternal()
{
    CreateModalWidgets();
}

void UMAUIManager::RegisterViewportWidgetInternal(EMAWidgetType Type, UUserWidget* Widget, ESlateVisibility Visibility)
{
    WidgetRegistryCoordinator.RegisterViewportWidget(this, Type, Widget, Visibility);
}

void UMAUIManager::RegisterManagedWidgetInternal(EMAWidgetType Type, UUserWidget* Widget, ESlateVisibility Visibility)
{
    WidgetRegistryCoordinator.RegisterManagedWidget(this, Type, Widget, Visibility);
}

void UMAUIManager::SetCurrentThemeInternal(UMAUITheme* InTheme)
{
    CurrentTheme = InTheme;
}

void UMAUIManager::SetDefaultThemeInternal(UMAUITheme* InTheme)
{
    DefaultTheme = InTheme;
}


//=============================================================================
// Widget 访问 - 通用接口
//=============================================================================

UUserWidget* UMAUIManager::GetWidget(EMAWidgetType Type) const
{
    if (Type == EMAWidgetType::None)
    {
        return nullptr;
    }

    const UUserWidget* const* FoundWidget = Widgets.Find(Type);
    if (FoundWidget && *FoundWidget)
    {
        return const_cast<UUserWidget*>(*FoundWidget);
    }

    return nullptr;
}

//=============================================================================
// Widget 访问 - 类型安全 Getter
//=============================================================================

UMATaskPlannerWidget* UMAUIManager::GetTaskPlannerWidget() const
{
    return TaskPlannerWidget;
}

UMASkillAllocationViewer* UMAUIManager::GetSkillAllocationViewer() const
{
    return SkillAllocationViewer;
}

UMADirectControlIndicator* UMAUIManager::GetDirectControlIndicator() const
{
    return DirectControlIndicator;
}

UMAModifyWidget* UMAUIManager::GetModifyWidget() const
{
    return ModifyWidget;
}

UMAEditWidget* UMAUIManager::GetEditWidget() const
{
    return EditWidget;
}

UMASceneListWidget* UMAUIManager::GetSceneListWidget() const
{
    return SceneListWidget;
}

UMAHUDWidget* UMAUIManager::GetHUDWidget() const
{
    return HUDWidget;
}

//=============================================================================
// Widget 访问 - 右侧边栏拆分面板
//=============================================================================

UMASystemLogPanel* UMAUIManager::GetSystemLogPanel() const
{
    return SystemLogPanel;
}

UMAPreviewPanel* UMAUIManager::GetPreviewPanel() const
{
    return PreviewPanel;
}

UMAInstructionPanel* UMAUIManager::GetInstructionPanel() const
{
    return InstructionPanel;
}

UMABaseModalWidget* UMAUIManager::GetModalByType(EMAModalType ModalType) const
{
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        return TaskGraphModal;
    case EMAModalType::SkillAllocation:
        return SkillAllocationModal;
    case EMAModalType::Decision:
        return DecisionModal;
    default:
        return nullptr;
    }
}

//=============================================================================
// Widget 可见性控制
//=============================================================================

bool UMAUIManager::ShowWidget(EMAWidgetType Type, bool bSetFocus)
{
    return WidgetInteractionCoordinator.ShowWidget(this, Type, bSetFocus);
}

bool UMAUIManager::HideWidget(EMAWidgetType Type)
{
    return WidgetInteractionCoordinator.HideWidget(this, Type);
}

void UMAUIManager::ToggleWidget(EMAWidgetType Type)
{
    WidgetInteractionCoordinator.ToggleWidget(this, Type);
}

bool UMAUIManager::IsWidgetVisible(EMAWidgetType Type) const
{
    return WidgetInteractionCoordinator.IsWidgetVisible(this, Type);
}

bool UMAUIManager::IsAnyFullscreenWidgetVisible() const
{
    return WidgetInteractionCoordinator.IsAnyFullscreenWidgetVisible(this);
}

void UMAUIManager::NavigateFromViewerToSkillAllocationModal()
{
    WidgetInteractionCoordinator.NavigateFromViewerToSkillAllocationModal(this);
}

void UMAUIManager::NavigateFromWorkbenchToTaskGraphModal()
{
    WidgetInteractionCoordinator.NavigateFromWorkbenchToTaskGraphModal(this);
}

void UMAUIManager::SetWidgetFocus(EMAWidgetType Type)
{
    WidgetInteractionCoordinator.SetWidgetFocus(this, Type);
}


void UMAUIManager::SetInputModeGameAndUI(UUserWidget* Widget)
{
    InputModeAdapter.SetInputModeGameAndUI(this, Widget);
}

void UMAUIManager::SetInputModeGameOnly()
{
    InputModeAdapter.SetInputModeGameOnly(this);
}


//=============================================================================
// UI 主题
//=============================================================================

void UMAUIManager::LoadTheme(UMAUITheme* ThemeAsset)
{
    ThemeCoordinator.LoadTheme(this, ThemeAsset);
}

UMAUITheme* UMAUIManager::GetTheme() const
{
    if (CurrentTheme)
    {
        return CurrentTheme;
    }
    
    // 如果没有加载主题，返回默认主题
    if (DefaultTheme)
    {
        return DefaultTheme;
    }
    
    // 创建临时默认主题 (const_cast 用于延迟初始化)
    UMAUIManager* MutableThis = const_cast<UMAUIManager*>(this);
    MutableThis->DefaultTheme = NewObject<UMAUITheme>(MutableThis);
    return MutableThis->DefaultTheme;
}

bool UMAUIManager::ValidateTheme(UMAUITheme* InTheme) const
{
    return ThemeCoordinator.ValidateTheme(InTheme);
}

void UMAUIManager::DistributeThemeToWidgets()
{
    ThemeCoordinator.DistributeThemeToWidgets(this);
}

//=============================================================================
// HUD 状态管理
//=============================================================================

void UMAUIManager::InitializeHUDStateManager()
{
    StateModalCoordinator.InitializeHUDStateManager(this);
}

void UMAUIManager::CreateModalWidgets()
{
    StateModalCoordinator.CreateModalWidgets(this);
}

void UMAUIManager::BindStateManagerDelegates()
{
    StateModalCoordinator.BindStateManagerDelegates(this);
}

void UMAUIManager::OnHUDStateChanged(EMAHUDState OldState, EMAHUDState NewState)
{
    StateModalCoordinator.HandleHUDStateChanged(this, OldState, NewState);
}

void UMAUIManager::OnNotificationReceived(EMANotificationType Type)
{
    StateModalCoordinator.HandleNotificationReceived(this, Type);
}

void UMAUIManager::OnModalConfirmed(EMAModalType ModalType)
{
    StateModalCoordinator.HandleModalConfirmed(ModalType);
}

void UMAUIManager::OnModalRejected(EMAModalType ModalType)
{
    StateModalCoordinator.HandleModalRejected(ModalType);
}

void UMAUIManager::OnModalEditRequested(EMAModalType ModalType)
{
    StateModalCoordinator.HandleModalEditRequested(this, ModalType);
}

void UMAUIManager::OnModalHideAnimationComplete()
{
    StateModalCoordinator.HandleModalHideAnimationComplete(this);
}

void UMAUIManager::ShowModal(EMAModalType ModalType, bool bEditMode)
{
    StateModalCoordinator.ShowModal(this, ModalType, bEditMode);
}

void UMAUIManager::HideCurrentModal()
{
    StateModalCoordinator.HideCurrentModal(this);
}

void UMAUIManager::LoadDataIntoModal(EMAModalType ModalType)
{
    StateModalCoordinator.LoadDataIntoModal(this, ModalType);
}

int32 UMAUIManager::GetModalZOrder() const
{
    return StateModalCoordinator.GetModalZOrder();
}

//=============================================================================
// 通知处理
//=============================================================================

void UMAUIManager::ShowNotification(EMANotificationType Type)
{
    NotificationCoordinator.ShowNotification(this, Type);
}

void UMAUIManager::DismissNotification()
{
    NotificationCoordinator.DismissNotification(this);
}

void UMAUIManager::DismissRequestUserCommandNotification()
{
    NotificationCoordinator.DismissRequestUserCommandNotification(this);
}

EMAModalType UMAUIManager::GetModalTypeForNotification(EMANotificationType NotificationType) const
{
    return NotificationCoordinator.GetModalTypeForNotification(NotificationType);
}


//=============================================================================
// TempDataManager 事件绑定
//=============================================================================

void UMAUIManager::BindTempDataManagerEvents()
{
    RuntimeEventCoordinator.BindTempDataManagerEvents(this);
}

void UMAUIManager::OnTempTaskGraphChanged(const FMATaskGraphData& Data)
{
    RuntimeEventCoordinator.HandleTempTaskGraphChanged(this, Data);
}

void UMAUIManager::OnTempSkillAllocationChanged(const FMASkillAllocationData& Data)
{
    RuntimeEventCoordinator.HandleTempSkillAllocationChanged(this, Data);
}

void UMAUIManager::OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    RuntimeEventCoordinator.HandleSkillStatusUpdated(this, TimeStep, RobotId, NewStatus);
}

//=============================================================================
// CommSubsystem 事件绑定
//=============================================================================

void UMAUIManager::BindCommSubsystemEvents()
{
    RuntimeEventCoordinator.BindCommSubsystemEvents(this);
}

void UMAUIManager::OnRequestUserCommandReceived()
{
    RuntimeEventCoordinator.HandleRequestUserCommandReceived(this);
}

//=============================================================================
// CommandManager 事件绑定
//=============================================================================

void UMAUIManager::BindCommandManagerEvents()
{
    RuntimeEventCoordinator.BindCommandManagerEvents(this);
}

void UMAUIManager::OnExecutionPauseStateChanged(bool bPaused)
{
    RuntimeEventCoordinator.HandleExecutionPauseStateChanged(this, bPaused);
}


//=============================================================================
// Decision 事件处理
//=============================================================================

void UMAUIManager::OnDecisionDataReceived(const FString& Description, const FString& ContextJson)
{
    RuntimeEventCoordinator.HandleDecisionDataReceived(this, Description, ContextJson);
}

void UMAUIManager::OnDecisionModalConfirmed(const FString& SelectedOption, const FString& UserFeedback)
{
    RuntimeEventCoordinator.HandleDecisionModalConfirmed(this, SelectedOption, UserFeedback);
}

void UMAUIManager::OnDecisionModalRejected(const FString& SelectedOption, const FString& UserFeedback)
{
    RuntimeEventCoordinator.HandleDecisionModalRejected(this, SelectedOption, UserFeedback);
}
