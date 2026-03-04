// MAHUDSceneEditCoordinator.h
// HUD 场景编辑协调器
// 封装 Modify/Edit/SceneList 相关流程，降低 MAHUD 体积与耦合

#pragma once

#include "CoreMinimal.h"

class AActor;
class AMAHUD;
class AMAPointOfInterest;

class MULTIAGENT_API FMAHUDSceneEditCoordinator
{
public:
    void HandleModifyConfirmed(AMAHUD* HUD, AActor* Actor, const FString& LabelText) const;
    void HandleModifyActorSelected(AMAHUD* HUD, AActor* SelectedActor) const;
    void HandleModifyActorsSelected(AMAHUD* HUD, const TArray<AActor*>& SelectedActors) const;
    void HandleMultiSelectModifyConfirmed(
        AMAHUD* HUD,
        const TArray<AActor*>& Actors,
        const FString& LabelText,
        const FString& GeneratedJson) const;

    void HandleEditConfirmed(AMAHUD* HUD, AActor* Actor, const FString& JsonContent) const;
    void HandleEditDeleteActor(AMAHUD* HUD, AActor* Actor) const;
    void HandleEditCreateGoal(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& Description) const;
    void HandleEditCreateZone(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const;
    void HandleEditAddPresetActor(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& ActorType) const;
    void HandleEditDeletePOIs(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs) const;
    void HandleEditSetAsGoal(AMAHUD* HUD, AActor* Actor) const;
    void HandleEditUnsetAsGoal(AMAHUD* HUD, AActor* Actor) const;

    void HandleSceneListGoalClicked(AMAHUD* HUD, const FString& GoalId) const;
    void HandleSceneListZoneClicked(AMAHUD* HUD, const FString& ZoneId) const;
};
