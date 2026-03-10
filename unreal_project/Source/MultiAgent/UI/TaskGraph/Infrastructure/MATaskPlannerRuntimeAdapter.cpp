#include "MATaskPlannerRuntimeAdapter.h"
#include "UI/TaskGraph/MATaskPlannerWidget.h"
#include "Core/Comm/Runtime/MACommSubsystem.h"
#include "Core/TempData/Runtime/MATempDataManager.h"
#include "UI/HUD/MAHUD.h"
#include "UI/Core/MAUIManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

namespace
{
UGameInstance* ResolveTaskPlannerGameInstance(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    return World ? World->GetGameInstance() : nullptr;
}

UMATempDataManager* ResolveTaskPlannerTempDataManager(const UObject* WorldContextObject)
{
    if (UGameInstance* GameInstance = ResolveTaskPlannerGameInstance(WorldContextObject))
    {
        return GameInstance->GetSubsystem<UMATempDataManager>();
    }
    return nullptr;
}

UMACommSubsystem* ResolveCommSubsystem(const UObject* WorldContextObject)
{
    if (UGameInstance* GameInstance = ResolveTaskPlannerGameInstance(WorldContextObject))
    {
        return GameInstance->GetSubsystem<UMACommSubsystem>();
    }
    return nullptr;
}

UMAUIManager* ResolveTaskPlannerUIManager(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    APlayerController* PlayerController = WorldContextObject->GetWorld() ? WorldContextObject->GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PlayerController)
    {
        return nullptr;
    }

    AMAHUD* HUD = Cast<AMAHUD>(PlayerController->GetHUD());
    return HUD ? HUD->GetUIManager() : nullptr;
}
}

bool FMATaskPlannerRuntimeAdapter::TryLoadTaskGraph(const UObject* WorldContextObject, FMATaskGraphData& OutData, FString& OutError)
{
    UMATempDataManager* TempDataManager = ResolveTaskPlannerTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->TaskGraphFileExists())
    {
        OutError = TEXT("Task graph temp file does not exist");
        return false;
    }

    if (!TempDataManager->LoadTaskGraph(OutData))
    {
        OutError = TEXT("Failed to load task graph from temp storage");
        return false;
    }

    return true;
}

bool FMATaskPlannerRuntimeAdapter::TrySaveTaskGraph(const UObject* WorldContextObject, const FMATaskGraphData& Data, FString& OutError)
{
    UMATempDataManager* TempDataManager = ResolveTaskPlannerTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->SaveTaskGraph(Data))
    {
        OutError = TEXT("Failed to save task graph to temp storage");
        return false;
    }

    return true;
}

bool FMATaskPlannerRuntimeAdapter::TrySubmitTaskGraph(const UObject* WorldContextObject, const FString& TaskGraphJson, FString& OutError)
{
    UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(WorldContextObject);
    if (!CommSubsystem)
    {
        OutError = TEXT("Unable to get communication subsystem");
        return false;
    }

    CommSubsystem->SendTaskGraphSubmitMessage(TaskGraphJson);
    return true;
}

bool FMATaskPlannerRuntimeAdapter::TryNavigateToTaskGraphModal(const UObject* WorldContextObject, FString& OutError)
{
    UMAUIManager* UIManager = ResolveTaskPlannerUIManager(WorldContextObject);
    if (!UIManager)
    {
        OutError = TEXT("Unable to get UIManager");
        return false;
    }

    UIManager->NavigateFromWorkbenchToTaskGraphModal();
    return true;
}

bool FMATaskPlannerRuntimeAdapter::TryHideTaskPlanner(const UObject* WorldContextObject, FString& OutError)
{
    UMAUIManager* UIManager = ResolveTaskPlannerUIManager(WorldContextObject);
    if (!UIManager)
    {
        OutError = TEXT("Unable to get UIManager");
        return false;
    }

    UIManager->HideWidget(EMAWidgetType::TaskPlanner);
    return true;
}

bool FMATaskPlannerRuntimeAdapter::BindTaskGraphChanged(const UObject* WorldContextObject, UMATaskPlannerWidget* Widget, FString& OutError)
{
    if (!Widget)
    {
        OutError = TEXT("Task planner widget is null");
        return false;
    }

    UMATempDataManager* TempDataManager = ResolveTaskPlannerTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->OnTaskGraphChanged.IsAlreadyBound(Widget, &UMATaskPlannerWidget::OnRuntimeTaskGraphChanged))
    {
        TempDataManager->OnTaskGraphChanged.AddDynamic(Widget, &UMATaskPlannerWidget::OnRuntimeTaskGraphChanged);
    }

    return true;
}
