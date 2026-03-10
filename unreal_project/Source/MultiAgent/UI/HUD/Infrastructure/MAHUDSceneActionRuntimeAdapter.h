#pragma once

#include "CoreMinimal.h"
#include "../../SceneEditing/Domain/MASceneActionResult.h"
#include "../../SceneEditing/Infrastructure/MAEditSceneActionAdapter.h"
#include "../../SceneEditing/Infrastructure/MAModifySceneActionAdapter.h"

class AActor;
class AMAHUD;
class AMAPointOfInterest;

class MULTIAGENT_API FMAHUDSceneActionRuntimeAdapter
{
public:
    FMASceneActionResult ApplySingleModify(AMAHUD* HUD, AActor* Actor, const FString& LabelText) const;
    FMASceneActionResult ApplyMultiModify(
        AMAHUD* HUD,
        const TArray<AActor*>& Actors,
        const FString& LabelText,
        const FString& GeneratedJson) const;
    FMASceneActionResult ApplyNodeEdit(AMAHUD* HUD, AActor* Actor, const FString& JsonContent) const;
    FMASceneActionResult DeleteActor(AMAHUD* HUD, AActor* Actor) const;
    FMASceneActionResult CreateGoal(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& Description) const;
    FMASceneActionResult CreateZone(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const;
    FMASceneActionResult AddPresetActor(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& ActorType) const;
    FMASceneActionResult DeletePOIs(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs) const;
    FMASceneActionResult SetGoalState(AMAHUD* HUD, AActor* Actor, bool bShouldSetGoal) const;

private:
    class UWorld* ResolveWorld(const AMAHUD* HUD) const;

    FMAModifySceneActionAdapter ModifySceneActionAdapter;
    FMAEditSceneActionAdapter EditSceneActionAdapter;
};
