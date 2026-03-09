// Scene-list selection resolution for edit-mode actors.

#include "MAHUDSceneListSelectionCoordinator.h"

#include "../MAHUD.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "Engine/GameInstance.h"

void FMAHUDSceneListSelectionCoordinator::HandleGoalClicked(AMAHUD* HUD, const FString& GoalId) const
{
    if (!HUD)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    if (AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalId))
    {
        EditModeManager->SelectActor(GoalActor);
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        return;
    }

    const FMASceneGraphNode Node = SceneGraphManager->GetNodeById(GoalId);
    if (!Node.IsValid() || Node.Guid.IsEmpty())
    {
        return;
    }

    if (AActor* FoundActor = EditModeManager->FindActorByGuid(Node.Guid))
    {
        EditModeManager->SelectActor(FoundActor);
    }
}

void FMAHUDSceneListSelectionCoordinator::HandleZoneClicked(AMAHUD* HUD, const FString& ZoneId) const
{
    if (!HUD)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    if (AMAZoneActor* ZoneActor = EditModeManager->GetZoneActorByNodeId(ZoneId))
    {
        EditModeManager->SelectActor(ZoneActor);
        return;
    }

    HUD->ShowNotification(FString::Printf(TEXT("Zone %s has no visualization Actor"), *ZoneId), false, true);
}
