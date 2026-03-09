// L4 runtime bridge for HUD scene-edit actions.

#include "MAHUDSceneActionRuntimeAdapter.h"

#include "../MAHUD.h"
#include "Engine/World.h"

UWorld* FMAHUDSceneActionRuntimeAdapter::ResolveWorld(const AMAHUD* HUD) const
{
    return HUD ? HUD->GetWorld() : nullptr;
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::ApplySingleModify(
    AMAHUD* HUD,
    AActor* Actor,
    const FString& LabelText) const
{
    return ModifySceneActionAdapter.ApplySingleModify(ResolveWorld(HUD), Actor, LabelText);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::ApplyMultiModify(
    AMAHUD* HUD,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FString& GeneratedJson) const
{
    return ModifySceneActionAdapter.ApplyMultiModify(ResolveWorld(HUD), Actors, LabelText, GeneratedJson);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::ApplyNodeEdit(
    AMAHUD* HUD,
    AActor* Actor,
    const FString& JsonContent) const
{
    return EditSceneActionAdapter.ApplyNodeEdit(ResolveWorld(HUD), Actor, JsonContent);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::DeleteActor(AMAHUD* HUD, AActor* Actor) const
{
    return EditSceneActionAdapter.DeleteActor(ResolveWorld(HUD), Actor);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::CreateGoal(
    AMAHUD* HUD,
    AMAPointOfInterest* POI,
    const FString& Description) const
{
    return EditSceneActionAdapter.CreateGoal(ResolveWorld(HUD), POI, Description);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::CreateZone(
    AMAHUD* HUD,
    const TArray<AMAPointOfInterest*>& POIs,
    const FString& Description) const
{
    return EditSceneActionAdapter.CreateZone(ResolveWorld(HUD), POIs, Description);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::AddPresetActor(
    AMAHUD* HUD,
    AMAPointOfInterest* POI,
    const FString& ActorType) const
{
    return EditSceneActionAdapter.AddPresetActor(ResolveWorld(HUD), POI, ActorType);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::DeletePOIs(
    AMAHUD* HUD,
    const TArray<AMAPointOfInterest*>& POIs) const
{
    return EditSceneActionAdapter.DeletePOIs(ResolveWorld(HUD), POIs);
}

FMASceneActionResult FMAHUDSceneActionRuntimeAdapter::SetGoalState(
    AMAHUD* HUD,
    AActor* Actor,
    bool bShouldSetGoal) const
{
    return EditSceneActionAdapter.SetGoalState(ResolveWorld(HUD), Actor, bShouldSetGoal);
}
