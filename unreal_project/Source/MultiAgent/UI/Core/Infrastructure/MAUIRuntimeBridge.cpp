#include "MAUIRuntimeBridge.h"

#include "UI/Core/MAUIManager.h"
#include "UI/HUD/Presentation/MAMainHUDWidget.h"
#include "UI/Components/Presentation/MANotificationWidget.h"
#include "Core/TempData/Runtime/MATempDataManager.h"
#include "Core/Command/Runtime/MACommandManager.h"
#include "Core/Comm/Runtime/MACommSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
UWorld* ResolveWorld(const UMAUIManager* UIManager)
{
    APlayerController* OwningPC = UIManager ? UIManager->GetOwningPlayerController() : nullptr;
    return OwningPC ? OwningPC->GetWorld() : nullptr;
}

UGameInstance* ResolveGameInstance(const UMAUIManager* UIManager)
{
    UWorld* World = ResolveWorld(UIManager);
    return World ? World->GetGameInstance() : nullptr;
}

UMATempDataManager* ResolveTempDataManager(const UMAUIManager* UIManager)
{
    UGameInstance* GameInstance = ResolveGameInstance(UIManager);
    return GameInstance ? GameInstance->GetSubsystem<UMATempDataManager>() : nullptr;
}

UMACommSubsystem* ResolveCommSubsystem(const UMAUIManager* UIManager)
{
    UGameInstance* GameInstance = ResolveGameInstance(UIManager);
    return GameInstance ? GameInstance->GetSubsystem<UMACommSubsystem>() : nullptr;
}

UMACommandManager* ResolveCommandManager(const UMAUIManager* UIManager)
{
    UWorld* World = ResolveWorld(UIManager);
    return World ? World->GetSubsystem<UMACommandManager>() : nullptr;
}
}

bool FMAUIRuntimeBridge::BindTempDataEvents(UMAUIManager* UIManager, FString& OutError) const
{
    UMATempDataManager* TempDataMgr = ResolveTempDataManager(UIManager);
    if (!TempDataMgr)
    {
        OutError = TEXT("TempDataManager not available");
        return false;
    }

    if (!TempDataMgr->OnTaskGraphChanged.IsAlreadyBound(UIManager, &UMAUIManager::OnTempTaskGraphChanged))
    {
        TempDataMgr->OnTaskGraphChanged.AddDynamic(UIManager, &UMAUIManager::OnTempTaskGraphChanged);
    }

    if (!TempDataMgr->OnSkillAllocationChanged.IsAlreadyBound(UIManager, &UMAUIManager::OnTempSkillAllocationChanged))
    {
        TempDataMgr->OnSkillAllocationChanged.AddDynamic(UIManager, &UMAUIManager::OnTempSkillAllocationChanged);
    }

    if (!TempDataMgr->OnSkillStatusUpdated.IsAlreadyBound(UIManager, &UMAUIManager::OnSkillStatusUpdated))
    {
        TempDataMgr->OnSkillStatusUpdated.AddDynamic(UIManager, &UMAUIManager::OnSkillStatusUpdated);
    }

    return true;
}

bool FMAUIRuntimeBridge::BindCommEvents(UMAUIManager* UIManager, FString& OutError) const
{
    UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(UIManager);
    if (!CommSubsystem)
    {
        OutError = TEXT("CommSubsystem not available");
        return false;
    }

    if (!CommSubsystem->OnRequestUserCommandReceived.IsAlreadyBound(UIManager, &UMAUIManager::OnRequestUserCommandReceived))
    {
        CommSubsystem->OnRequestUserCommandReceived.AddDynamic(UIManager, &UMAUIManager::OnRequestUserCommandReceived);
    }

    if (!CommSubsystem->OnDecisionReceived.IsAlreadyBound(UIManager, &UMAUIManager::OnDecisionDataReceived))
    {
        CommSubsystem->OnDecisionReceived.AddDynamic(UIManager, &UMAUIManager::OnDecisionDataReceived);
    }

    return true;
}

bool FMAUIRuntimeBridge::BindCommandEvents(UMAUIManager* UIManager, FString& OutError) const
{
    UMACommandManager* CommandManager = ResolveCommandManager(UIManager);
    if (!CommandManager)
    {
        OutError = TEXT("CommandManager not available");
        return false;
    }

    if (!CommandManager->OnExecutionPauseStateChanged.IsAlreadyBound(UIManager, &UMAUIManager::OnExecutionPauseStateChanged))
    {
        CommandManager->OnExecutionPauseStateChanged.AddDynamic(UIManager, &UMAUIManager::OnExecutionPauseStateChanged);
    }

    return true;
}

bool FMAUIRuntimeBridge::TryLoadTaskGraphData(const UMAUIManager* UIManager, FMATaskGraphData& OutData, FString& OutError) const
{
    UMATempDataManager* TempDataMgr = ResolveTempDataManager(UIManager);
    if (!TempDataMgr)
    {
        OutError = TEXT("TempDataManager not available");
        return false;
    }

    if (!TempDataMgr->TaskGraphFileExists())
    {
        OutError = TEXT("Task graph runtime data not available");
        return false;
    }

    if (!TempDataMgr->LoadTaskGraph(OutData))
    {
        OutError = TEXT("Failed to load task graph from runtime storage");
        return false;
    }

    return true;
}

bool FMAUIRuntimeBridge::TryLoadSkillAllocationData(const UMAUIManager* UIManager, FMASkillAllocationData& OutData, FString& OutError) const
{
    UMATempDataManager* TempDataMgr = ResolveTempDataManager(UIManager);
    if (!TempDataMgr)
    {
        OutError = TEXT("TempDataManager not available");
        return false;
    }

    if (!TempDataMgr->SkillAllocationFileExists())
    {
        OutError = TEXT("Skill allocation runtime data not available");
        return false;
    }

    if (!TempDataMgr->LoadSkillAllocation(OutData))
    {
        OutError = TEXT("Failed to load skill allocation from runtime storage");
        return false;
    }

    return true;
}

bool FMAUIRuntimeBridge::TrySendDecisionResponse(
    const UMAUIManager* UIManager,
    const FString& SelectedOption,
    const FString& UserFeedback,
    FString& OutError) const
{
    UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(UIManager);
    if (!CommSubsystem)
    {
        OutError = TEXT("CommSubsystem not available");
        return false;
    }

    if (!CommSubsystem->GetOutbound())
    {
        OutError = TEXT("Comm outbound channel not available");
        return false;
    }

    CommSubsystem->GetOutbound()->SendDecisionResponseSimple(SelectedOption, TEXT(""), UserFeedback);
    return true;
}

bool FMAUIRuntimeBridge::TryClearResumeNotificationTimer(const UMAUIManager* UIManager, FTimerHandle& TimerHandle, FString& OutError) const
{
    UWorld* World = ResolveWorld(UIManager);
    if (!World)
    {
        OutError = TEXT("World not available");
        return false;
    }

    World->GetTimerManager().ClearTimer(TimerHandle);
    return true;
}

bool FMAUIRuntimeBridge::TryScheduleResumeNotificationAutoHide(
    const UMAUIManager* UIManager,
    FTimerHandle& TimerHandle,
    float DelaySeconds,
    FString& OutError) const
{
    UWorld* World = ResolveWorld(UIManager);
    if (!World)
    {
        OutError = TEXT("World not available");
        return false;
    }

    World->GetTimerManager().SetTimer(
        TimerHandle,
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
        DelaySeconds,
        false
    );

    return true;
}
