// HUD scene-edit flow coordination.

#include "MAHUDSceneEditCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../Mode/MAModifyWidget.h"

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
        FMAFeedback21Batch Feedback;
        Feedback.AddMessage(TEXT("Please select an Actor first"), EMAFeedback21MessageSeverity::Error);
        ActionResultCoordinator.ApplyFeedback(HUD, Feedback);
        return;
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    TArray<AActor*> Actors;
    if (Actor)
    {
        Actors.Add(Actor);
    }
    FMASceneActionResult Result = SceneActionRuntimeAdapter.ApplySingleModify(HUD, Actor, LabelText);
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
        FMAFeedback21Batch Feedback;
        Feedback.AddMessage(TEXT("Please select at least one Actor first"), EMAFeedback21MessageSeverity::Error);
        ActionResultCoordinator.ApplyFeedback(HUD, Feedback);
        return;
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    FMASceneActionResult Result = SceneActionRuntimeAdapter.ApplyMultiModify(HUD, Actors, LabelText, GeneratedJson);
    ActionResultCoordinator.ApplyModifyResult(HUD, ModifyWidget, Actors, LabelText, Result);
}

void FMAHUDSceneEditCoordinator::HandleEditConfirmed(AMAHUD* HUD, AActor* Actor, const FString& JsonContent) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditConfirmed: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.ApplyNodeEdit(HUD, Actor, JsonContent));
}

void FMAHUDSceneEditCoordinator::HandleEditDeleteActor(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeleteActor: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.DeleteActor(HUD, Actor));
}

void FMAHUDSceneEditCoordinator::HandleEditCreateGoal(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& Description) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditCreateGoal: POI=%s, Description=%s"),
        POI ? TEXT("set") : TEXT("null"), *Description);

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.CreateGoal(HUD, POI, Description));
}

void FMAHUDSceneEditCoordinator::HandleEditCreateZone(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditCreateZone: %d POIs, Description=%s"), POIs.Num(), *Description);

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.CreateZone(HUD, POIs, Description));
}

void FMAHUDSceneEditCoordinator::HandleEditAddPresetActor(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& ActorType) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditAddPresetActor: POI=%s, ActorType=%s"),
        POI ? TEXT("set") : TEXT("null"), *ActorType);

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.AddPresetActor(HUD, POI, ActorType));
}

void FMAHUDSceneEditCoordinator::HandleEditDeletePOIs(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeletePOIs: %d POIs to delete"), POIs.Num());

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.DeletePOIs(HUD, POIs));
}

void FMAHUDSceneEditCoordinator::HandleEditSetAsGoal(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditSetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.SetGoalState(HUD, Actor, true));
}

void FMAHUDSceneEditCoordinator::HandleEditUnsetAsGoal(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditUnsetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.SetGoalState(HUD, Actor, false));
}

void FMAHUDSceneEditCoordinator::HandleEditActionRequest(AMAHUD* HUD, const FMAEditWidgetActionRequest& Request) const
{
    if (!HUD)
    {
        return;
    }

    switch (Request.Kind)
    {
    case EMAEditWidgetActionKind::ApplyNodeEdit:
        HandleEditConfirmed(HUD, Request.Actor, Request.JsonContent);
        break;
    case EMAEditWidgetActionKind::DeleteActor:
        HandleEditDeleteActor(HUD, Request.Actor);
        break;
    case EMAEditWidgetActionKind::CreateGoal:
        if (Request.POIs.Num() > 0)
        {
            HandleEditCreateGoal(HUD, Request.POIs[0], Request.Description);
        }
        break;
    case EMAEditWidgetActionKind::CreateZone:
        HandleEditCreateZone(HUD, Request.POIs, Request.Description);
        break;
    case EMAEditWidgetActionKind::AddPresetActor:
        if (Request.POIs.Num() > 0)
        {
            HandleEditAddPresetActor(HUD, Request.POIs[0], Request.ActorType);
        }
        break;
    case EMAEditWidgetActionKind::DeletePOIs:
        HandleEditDeletePOIs(HUD, Request.POIs);
        break;
    case EMAEditWidgetActionKind::SetGoalState:
        ActionResultCoordinator.ApplyResult(HUD, SceneActionRuntimeAdapter.SetGoalState(HUD, Request.Actor, Request.bGoalState));
        break;
    default:
        break;
    }
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
