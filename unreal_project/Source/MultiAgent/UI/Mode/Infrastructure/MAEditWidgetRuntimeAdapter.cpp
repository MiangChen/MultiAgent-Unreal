// L4 runtime bridge for edit widget selection and scene graph resolution.

#include "MAEditWidgetRuntimeAdapter.h"

#include "../../HUD/MAHUD.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"
#include "Engine/World.h"

UWorld* FMAEditWidgetRuntimeAdapter::ResolveWorld(const AMAHUD* HUD) const
{
    return HUD ? HUD->GetWorld() : nullptr;
}

UMAEditModeManager* FMAEditWidgetRuntimeAdapter::ResolveEditModeManager(const AMAHUD* HUD) const
{
    if (UWorld* World = ResolveWorld(HUD))
    {
        return World->GetSubsystem<UMAEditModeManager>();
    }

    return nullptr;
}

bool FMAEditWidgetRuntimeAdapter::ResolveCurrentSelection(
    AMAHUD* HUD,
    TWeakObjectPtr<AActor>& OutSelectedActor,
    TArray<AMAPointOfInterest*>& OutSelectedPOIs) const
{
    OutSelectedActor = nullptr;
    OutSelectedPOIs.Reset();

    if (UMAEditModeManager* EditModeManager = ResolveEditModeManager(HUD))
    {
        if (EditModeManager->HasSelectedActor())
        {
            OutSelectedActor = EditModeManager->GetSelectedActor();
            return true;
        }

        if (EditModeManager->HasSelectedPOIs())
        {
            OutSelectedPOIs = EditModeManager->GetSelectedPOIs();
            return true;
        }
    }

    return false;
}

bool FMAEditWidgetRuntimeAdapter::ResolveActorNodes(
    AMAHUD* HUD,
    AActor* Actor,
    TArray<FMASceneGraphNode>& OutNodes,
    FString& OutError) const
{
    return SceneGraphAdapter.ResolveActorNodes(ResolveWorld(HUD), Actor, OutNodes, OutError);
}
