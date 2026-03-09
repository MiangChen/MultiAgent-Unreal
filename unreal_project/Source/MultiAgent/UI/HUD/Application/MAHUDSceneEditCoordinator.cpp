// HUD scene-edit flow coordination.

#include "MAHUDSceneEditCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../Mode/MAModifyWidget.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDSceneEditCoordinator, Log, All);

void FMAHUDSceneEditCoordinator::HandleModifyConfirmed(AMAHUD* HUD, AActor* Actor, const FString& LabelText) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyConfirmed: Actor=%s, LabelText=%s"),
        Actor ? *Actor->GetName() : TEXT("null"), *LabelText);
    if (!Actor)
    {
        HUD->ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    TArray<AActor*> Actors;
    if (Actor)
    {
        Actors.Add(Actor);
    }
    FMASceneActionResult Result = ModifySceneActionAdapter.ApplySingleModify(HUD->GetWorld(), Actor, LabelText);
    ActionResultCoordinator.ApplyModifyResult(HUD, ModifyWidget, Actors, LabelText, Result);
}

void FMAHUDSceneEditCoordinator::HandleModifyActorSelected(AMAHUD* HUD, AActor* SelectedActor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyActorSelected: Actor=%s"),
        SelectedActor ? *SelectedActor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyModifySelection(HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr, SelectedActor);
}

void FMAHUDSceneEditCoordinator::HandleModifyActorsSelected(AMAHUD* HUD, const TArray<AActor*>& SelectedActors) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyActorsSelected: %d actors"), SelectedActors.Num());

    ActionResultCoordinator.ApplyModifySelection(HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr, SelectedActors);
}

void FMAHUDSceneEditCoordinator::HandleMultiSelectModifyConfirmed(
    AMAHUD* HUD,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FString& GeneratedJson) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnMultiSelectModifyConfirmed: %d actors, LabelText=%s"),
        Actors.Num(), *LabelText);
    if (Actors.Num() == 0)
    {
        HUD->ShowNotification(TEXT("Please select at least one Actor first"), true);
        return;
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    FMASceneActionResult Result = ModifySceneActionAdapter.ApplyMultiModify(HUD->GetWorld(), Actors, LabelText, GeneratedJson);
    ActionResultCoordinator.ApplyModifyResult(HUD, ModifyWidget, Actors, LabelText, Result);
}

void FMAHUDSceneEditCoordinator::HandleEditConfirmed(AMAHUD* HUD, AActor* Actor, const FString& JsonContent) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditConfirmed: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.ApplyNodeEdit(HUD->GetWorld(), Actor, JsonContent));
}

void FMAHUDSceneEditCoordinator::HandleEditDeleteActor(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeleteActor: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.DeleteActor(HUD->GetWorld(), Actor));
}

void FMAHUDSceneEditCoordinator::HandleEditCreateGoal(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& Description) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditCreateGoal: POI=%s, Description=%s"),
        POI ? *POI->GetName() : TEXT("null"), *Description);

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.CreateGoal(HUD->GetWorld(), POI, Description));
}

void FMAHUDSceneEditCoordinator::HandleEditCreateZone(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditCreateZone: %d POIs, Description=%s"), POIs.Num(), *Description);

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.CreateZone(HUD->GetWorld(), POIs, Description));
}

void FMAHUDSceneEditCoordinator::HandleEditAddPresetActor(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& ActorType) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditAddPresetActor: POI=%s, ActorType=%s"),
        POI ? *POI->GetName() : TEXT("null"), *ActorType);

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.AddPresetActor(HUD->GetWorld(), POI, ActorType));
}

void FMAHUDSceneEditCoordinator::HandleEditDeletePOIs(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeletePOIs: %d POIs to delete"), POIs.Num());

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.DeletePOIs(HUD->GetWorld(), POIs));
}

void FMAHUDSceneEditCoordinator::HandleEditSetAsGoal(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditSetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.SetGoalState(HUD->GetWorld(), Actor, true));
}

void FMAHUDSceneEditCoordinator::HandleEditUnsetAsGoal(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditUnsetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, EditSceneActionAdapter.SetGoalState(HUD->GetWorld(), Actor, false));
}

void FMAHUDSceneEditCoordinator::HandleSceneListGoalClicked(AMAHUD* HUD, const FString& GoalId) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListGoalClicked: GoalId=%s"), *GoalId);
    SceneListSelectionCoordinator.HandleGoalClicked(HUD, GoalId);
}

void FMAHUDSceneEditCoordinator::HandleSceneListZoneClicked(AMAHUD* HUD, const FString& ZoneId) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListZoneClicked: ZoneId=%s"), *ZoneId);
    SceneListSelectionCoordinator.HandleZoneClicked(HUD, ZoneId);
}
