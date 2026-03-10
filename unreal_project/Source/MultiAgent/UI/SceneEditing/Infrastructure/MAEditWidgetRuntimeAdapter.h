#pragma once

#include "CoreMinimal.h"
#include "MAEditWidgetSceneGraphAdapter.h"

class AActor;
class AMAHUD;
class AMAPointOfInterest;

class MULTIAGENT_API FMAEditWidgetRuntimeAdapter
{
public:
    bool ResolveCurrentSelection(
        AMAHUD* HUD,
        TWeakObjectPtr<AActor>& OutSelectedActor,
        TArray<AMAPointOfInterest*>& OutSelectedPOIs) const;
    bool ResolveActorNodes(
        AMAHUD* HUD,
        AActor* Actor,
        TArray<FMASceneGraphNode>& OutNodes,
        FString& OutError) const;

private:
    class UWorld* ResolveWorld(const AMAHUD* HUD) const;
    class UMAEditModeManager* ResolveEditModeManager(const AMAHUD* HUD) const;

    FMAEditWidgetSceneGraphAdapter SceneGraphAdapter;
};
