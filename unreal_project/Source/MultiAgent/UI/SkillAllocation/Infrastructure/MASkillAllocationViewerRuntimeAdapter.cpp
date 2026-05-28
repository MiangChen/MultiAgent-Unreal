#include "MASkillAllocationViewerRuntimeAdapter.h"

#include "UI/SkillAllocation/Presentation/MASkillAllocationViewer.h"
#include "Core/TempData/Runtime/MATempDataManager.h"
#include "UI/HUD/Runtime/MAHUD.h"
#include "UI/Core/MAUIManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

namespace
{
UGameInstance* ResolveSkillAllocationGameInstance(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    return World ? World->GetGameInstance() : nullptr;
}

UMATempDataManager* ResolveSkillAllocationTempDataManager(const UObject* WorldContextObject)
{
    if (UGameInstance* GameInstance = ResolveSkillAllocationGameInstance(WorldContextObject))
    {
        return GameInstance->GetSubsystem<UMATempDataManager>();
    }
    return nullptr;
}

UMAUIManager* ResolveSkillAllocationUIManager(const UObject* WorldContextObject)
{
    if (!WorldContextObject || !WorldContextObject->GetWorld())
    {
        return nullptr;
    }

    APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
    {
        return nullptr;
    }

    AMAHUD* HUD = Cast<AMAHUD>(PlayerController->GetHUD());
    return HUD ? HUD->GetUIManager() : nullptr;
}
}

bool FMASkillAllocationViewerRuntimeAdapter::TryLoadSkillAllocation(const UObject* WorldContextObject, FMASkillAllocationData& OutData, FString& OutError)
{
    UMATempDataManager* TempDataManager = ResolveSkillAllocationTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->LoadSkillAllocation(OutData))
    {
        OutError = TEXT("Failed to load skill allocation from runtime storage");
        return false;
    }

    return true;
}

bool FMASkillAllocationViewerRuntimeAdapter::TrySaveSkillAllocation(const UObject* WorldContextObject, const FMASkillAllocationData& Data, FString& OutError)
{
    UMATempDataManager* TempDataManager = ResolveSkillAllocationTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->SaveSkillAllocation(Data))
    {
        OutError = TEXT("Failed to save skill allocation to runtime storage");
        return false;
    }

    return true;
}

bool FMASkillAllocationViewerRuntimeAdapter::TryNavigateToSkillAllocationModal(const UObject* WorldContextObject, FString& OutError)
{
    UMAUIManager* UIManager = ResolveSkillAllocationUIManager(WorldContextObject);
    if (!UIManager)
    {
        OutError = TEXT("Unable to get UIManager");
        return false;
    }

    UIManager->NavigateFromViewerToSkillAllocationModal();
    return true;
}

bool FMASkillAllocationViewerRuntimeAdapter::TryHideSkillAllocationViewer(const UObject* WorldContextObject, FString& OutError)
{
    UMAUIManager* UIManager = ResolveSkillAllocationUIManager(WorldContextObject);
    if (!UIManager)
    {
        OutError = TEXT("Unable to get UIManager");
        return false;
    }

    UIManager->HideWidget(EMAWidgetType::SkillAllocation);
    return true;
}

bool FMASkillAllocationViewerRuntimeAdapter::BindSkillAllocationChanged(const UObject* WorldContextObject, UMASkillAllocationViewer* Viewer, FString& OutError)
{
    if (!Viewer)
    {
        OutError = TEXT("Skill allocation viewer is null");
        return false;
    }

    UMATempDataManager* TempDataManager = ResolveSkillAllocationTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->OnSkillAllocationChanged.IsAlreadyBound(Viewer, &UMASkillAllocationViewer::OnRuntimeSkillAllocationChanged))
    {
        TempDataManager->OnSkillAllocationChanged.AddDynamic(Viewer, &UMASkillAllocationViewer::OnRuntimeSkillAllocationChanged);
    }

    return true;
}

bool FMASkillAllocationViewerRuntimeAdapter::BindSkillStatusUpdated(const UObject* WorldContextObject, UMASkillAllocationViewer* Viewer, FString& OutError)
{
    if (!Viewer)
    {
        OutError = TEXT("Skill allocation viewer is null");
        return false;
    }

    UMATempDataManager* TempDataManager = ResolveSkillAllocationTempDataManager(WorldContextObject);
    if (!TempDataManager)
    {
        OutError = TEXT("Unable to get TempDataManager");
        return false;
    }

    if (!TempDataManager->OnSkillStatusUpdated.IsAlreadyBound(Viewer, &UMASkillAllocationViewer::OnRuntimeSkillStatusUpdated))
    {
        TempDataManager->OnSkillStatusUpdated.AddDynamic(Viewer, &UMASkillAllocationViewer::OnRuntimeSkillStatusUpdated);
    }

    return true;
}
