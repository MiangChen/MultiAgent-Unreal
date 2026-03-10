// MAUIRuntimeEventCoordinator.cpp
// UIManager 运行时事件桥接协调器实现（L1 Application）

#include "MAUIRuntimeEventCoordinator.h"
#include "../MAUIManager.h"
#include "../MAHUDStateManager.h"
#include "../../HUD/MAMainHUDWidget.h"
#include "../../Components/MANotificationWidget.h"
#include "../../Components/MAPreviewPanel.h"
#include "../../Modal/MADecisionModal.h"
#include "../../Modal/MASkillAllocationModal.h"
#include "../../../Core/Manager/MATempDataManager.h"
#include "../../../Core/Command/Runtime/MACommandManager.h"
#include "../../../Core/Comm/MACommSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void FMAUIRuntimeEventCoordinator::BindTempDataManagerEvents(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

    if (!GameInstance)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindTempDataManagerEvents: GameInstance not available"));
        return;
    }

    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindTempDataManagerEvents: TempDataManager not available"));
        return;
    }

    if (!TempDataMgr->OnTaskGraphChanged.IsAlreadyBound(UIManager, &UMAUIManager::OnTempTaskGraphChanged))
    {
        TempDataMgr->OnTaskGraphChanged.AddDynamic(UIManager, &UMAUIManager::OnTempTaskGraphChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnTaskGraphChanged event"));
    }

    if (!TempDataMgr->OnSkillAllocationChanged.IsAlreadyBound(UIManager, &UMAUIManager::OnTempSkillAllocationChanged))
    {
        TempDataMgr->OnSkillAllocationChanged.AddDynamic(UIManager, &UMAUIManager::OnTempSkillAllocationChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnSkillAllocationChanged event"));
    }

    if (!TempDataMgr->OnSkillStatusUpdated.IsAlreadyBound(UIManager, &UMAUIManager::OnSkillStatusUpdated))
    {
        TempDataMgr->OnSkillStatusUpdated.AddDynamic(UIManager, &UMAUIManager::OnSkillStatusUpdated);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnSkillStatusUpdated event"));
    }

    FMATaskGraphData TaskGraphData;
    if (TempDataMgr->LoadTaskGraph(TaskGraphData) && TaskGraphData.Nodes.Num() > 0)
    {
        HandleTempTaskGraphChanged(UIManager, TaskGraphData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing task graph data (%d nodes)"),
            TaskGraphData.Nodes.Num());
    }

    FMASkillAllocationData SkillAllocationData;
    if (TempDataMgr->LoadSkillAllocation(SkillAllocationData) && SkillAllocationData.Data.Num() > 0)
    {
        HandleTempSkillAllocationChanged(UIManager, SkillAllocationData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing skill allocation data (%d time steps)"),
            SkillAllocationData.Data.Num());
    }
}

void FMAUIRuntimeEventCoordinator::HandleTempTaskGraphChanged(UMAUIManager* UIManager, const FMATaskGraphData& Data) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnTempTaskGraphChanged: Received task graph update (%d nodes, %d edges)"),
        Data.Nodes.Num(), Data.Edges.Num());

    if (UMAPreviewPanel* PreviewPanel = UIManager->GetPreviewPanel())
    {
        PreviewPanel->UpdateTaskGraphPreview(Data);
    }
}

void FMAUIRuntimeEventCoordinator::HandleTempSkillAllocationChanged(UMAUIManager* UIManager, const FMASkillAllocationData& Data) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnTempSkillAllocationChanged: Received skill allocation update (%d time steps)"),
        Data.Data.Num());

    if (UMAPreviewPanel* PreviewPanel = UIManager->GetPreviewPanel())
    {
        PreviewPanel->UpdateSkillListPreview(Data);
    }
}

void FMAUIRuntimeEventCoordinator::HandleSkillStatusUpdated(
    UMAUIManager* UIManager,
    int32 TimeStep,
    const FString& RobotId,
    ESkillExecutionStatus NewStatus) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnSkillStatusUpdated: TimeStep=%d, RobotId=%s, Status=%d"),
        TimeStep, *RobotId, static_cast<int32>(NewStatus));

    if (UMAPreviewPanel* PreviewPanel = UIManager->GetPreviewPanel())
    {
        UE_LOG(LogMAUIManager, Log, TEXT("OnSkillStatusUpdated: Forwarding to PreviewPanel"));
        PreviewPanel->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnSkillStatusUpdated: PreviewPanel is null"));
    }

    if (UMASkillAllocationModal* SkillAllocationModal = UIManager->GetSkillAllocationModal())
    {
        SkillAllocationModal->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
}

void FMAUIRuntimeEventCoordinator::BindCommSubsystemEvents(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

    if (!GameInstance)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommSubsystemEvents: GameInstance not available"));
        return;
    }

    UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
    if (!CommSubsystem)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommSubsystemEvents: CommSubsystem not available"));
        return;
    }

    if (!CommSubsystem->OnRequestUserCommandReceived.IsAlreadyBound(UIManager, &UMAUIManager::OnRequestUserCommandReceived))
    {
        CommSubsystem->OnRequestUserCommandReceived.AddDynamic(UIManager, &UMAUIManager::OnRequestUserCommandReceived);
        UE_LOG(LogMAUIManager, Log, TEXT("BindCommSubsystemEvents: Bound OnRequestUserCommandReceived"));
    }

    if (!CommSubsystem->OnDecisionReceived.IsAlreadyBound(UIManager, &UMAUIManager::OnDecisionDataReceived))
    {
        CommSubsystem->OnDecisionReceived.AddDynamic(UIManager, &UMAUIManager::OnDecisionDataReceived);
        UE_LOG(LogMAUIManager, Log, TEXT("BindCommSubsystemEvents: Bound OnDecisionReceived"));
    }

    UE_LOG(LogMAUIManager, Log, TEXT("BindCommSubsystemEvents: CommSubsystem events bound"));
}

void FMAUIRuntimeEventCoordinator::HandleRequestUserCommandReceived(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnRequestUserCommandReceived: Received request for user command"));
    UIManager->ShowNotification(EMANotificationType::RequestUserCommand);
}

void FMAUIRuntimeEventCoordinator::BindCommandManagerEvents(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommandManagerEvents: World not available"));
        return;
    }

    UMACommandManager* CommandManager = World->GetSubsystem<UMACommandManager>();
    if (!CommandManager)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommandManagerEvents: CommandManager not available"));
        return;
    }

    if (!CommandManager->OnExecutionPauseStateChanged.IsAlreadyBound(UIManager, &UMAUIManager::OnExecutionPauseStateChanged))
    {
        CommandManager->OnExecutionPauseStateChanged.AddDynamic(UIManager, &UMAUIManager::OnExecutionPauseStateChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindCommandManagerEvents: Bound OnExecutionPauseStateChanged"));
    }

    UE_LOG(LogMAUIManager, Log, TEXT("BindCommandManagerEvents: CommandManager events bound"));
}

void FMAUIRuntimeEventCoordinator::HandleExecutionPauseStateChanged(UMAUIManager* UIManager, bool bPaused) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnExecutionPauseStateChanged: bPaused=%s"), bPaused ? TEXT("true") : TEXT("false"));

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    if (World)
    {
        World->GetTimerManager().ClearTimer(UIManager->GetResumeNotificationTimerHandleInternal());
    }

    if (bPaused)
    {
        UIManager->ShowNotification(EMANotificationType::SkillListPaused);
        return;
    }

    UIManager->ShowNotification(EMANotificationType::SkillListResumed);
    if (!World)
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        UIManager->GetResumeNotificationTimerHandleInternal(),
        [UIManager]()
        {
            if (!UIManager)
            {
                return;
            }

            UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
            UMANotificationWidget* NotificationWidget = MainHUDWidget ? MainHUDWidget->GetNotification() : nullptr;
            if (NotificationWidget && NotificationWidget->GetCurrentNotificationType() == EMANotificationType::SkillListResumed)
            {
                NotificationWidget->HideNotification();
                UE_LOG(LogMAUIManager, Log, TEXT("OnExecutionPauseStateChanged: Auto-hiding SkillListResumed notification"));
            }
        },
        2.0f,
        false
    );
}

void FMAUIRuntimeEventCoordinator::HandleDecisionDataReceived(
    UMAUIManager* UIManager,
    const FString& Description,
    const FString& ContextJson) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionDataReceived: Description='%s'"), *Description);

    if (UMADecisionModal* DecisionModal = UIManager->GetDecisionModal())
    {
        DecisionModal->LoadDecisionData(Description, ContextJson);
        UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionDataReceived: Decision data loaded into DecisionModal"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionDataReceived: DecisionModal is null"));
    }

    UIManager->ShowNotification(EMANotificationType::DecisionUpdate);
}

void FMAUIRuntimeEventCoordinator::HandleDecisionModalConfirmed(
    UMAUIManager* UIManager,
    const FString& SelectedOption,
    const FString& UserFeedback) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalConfirmed: SelectedOption=%s, UserFeedback=%s"),
        *SelectedOption, *UserFeedback);

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

    if (GameInstance)
    {
        UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
        if (CommSubsystem)
        {
            CommSubsystem->GetOutbound()->SendDecisionResponseSimple(
                SelectedOption,
                TEXT(""),
                UserFeedback
            );
            UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalConfirmed: Decision response sent to backend"));
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionModalConfirmed: CommSubsystem not available"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionModalConfirmed: GameInstance not available"));
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->DismissNotification();
    }
    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
        MainHUDWidget && MainHUDWidget->GetNotification())
    {
        MainHUDWidget->GetNotification()->HideNotification();
    }
}

void FMAUIRuntimeEventCoordinator::HandleDecisionModalRejected(
    UMAUIManager* UIManager,
    const FString& SelectedOption,
    const FString& UserFeedback) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalRejected: SelectedOption=%s, UserFeedback=%s"),
        *SelectedOption, *UserFeedback);

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

    if (GameInstance)
    {
        UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
        if (CommSubsystem)
        {
            CommSubsystem->GetOutbound()->SendDecisionResponseSimple(
                SelectedOption,
                TEXT(""),
                UserFeedback
            );
            UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalRejected: Decision response sent to backend"));
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionModalRejected: CommSubsystem not available"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionModalRejected: GameInstance not available"));
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->DismissNotification();
    }
    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
        MainHUDWidget && MainHUDWidget->GetNotification())
    {
        MainHUDWidget->GetNotification()->HideNotification();
    }
}
