#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MASceneEditingFeedback.h"
#include "MAEditWidgetSceneGraphAdapter.h"

class AActor;
class AMAPointOfInterest;
class UWorld;

class MULTIAGENT_API FMAEditSceneActionAdapter
{
public:
    FMASceneActionResult ApplyNodeEdit(UWorld* World, AActor* Actor, const FString& JsonContent) const;
    FMASceneActionResult DeleteActor(UWorld* World, AActor* Actor) const;
    FMASceneActionResult CreateGoal(UWorld* World, AMAPointOfInterest* POI, const FString& Description) const;
    FMASceneActionResult CreateZone(UWorld* World, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const;
    FMASceneActionResult AddPresetActor(UWorld* World, AMAPointOfInterest* POI, const FString& ActorType) const;
    FMASceneActionResult DeletePOIs(UWorld* World, const TArray<AMAPointOfInterest*>& POIs) const;
    FMASceneActionResult SetGoalState(UWorld* World, AActor* Actor, bool bShouldSetGoal) const;

private:
    FMAEditWidgetSceneGraphAdapter SceneGraphAdapter;
};
