// Edit widget application-layer orchestration.

#include "MAEditWidgetCoordinator.h"

#include "../../HUD/MAHUD.h"
#include "../MAEditWidget.h"
#include "../Domain/MASceneSelectionDisplay.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"

namespace
{
    const FString EditDefaultHintText = TEXT("Select an Actor or POI to operate");
    const FString ActorSelectedHintText = TEXT("Edit JSON and click Confirm to save");
    const FString POISingleHintText = TEXT("Can create Goal or add preset Actor");
    const FString POIMultiHintText = TEXT("Select 3+ POIs to create a zone");
}

void FMAEditWidgetCoordinator::RefreshFromEditModeSelection(AMAHUD* HUD, UMAEditWidget* Widget)
{
    if (!HUD || !Widget)
    {
        return;
    }

    TWeakObjectPtr<AActor> SelectedActor;
    TArray<AMAPointOfInterest*> SelectedPOIs;
    if (!HUD->RuntimeResolveCurrentEditSelection(SelectedActor, SelectedPOIs))
    {
        ClearSelection();
        ApplyCurrentViewModel(Widget);
        return;
    }

    if (SelectedActor.IsValid())
    {
        SetActorSelection(HUD, SelectedActor.Get());
    }
    else if (!SelectedPOIs.IsEmpty())
    {
        SetPOISelection(SelectedPOIs);
    }
    else
    {
        ClearSelection();
    }

    ApplyCurrentViewModel(Widget);
}

void FMAEditWidgetCoordinator::RefreshCurrentSelection(AMAHUD* HUD, UMAEditWidget* Widget)
{
    if (!HUD || !Widget)
    {
        return;
    }

    if (SelectionState.HasActor())
    {
        SetActorSelection(HUD, SelectionState.SelectedActor.Get());
    }

    ApplyCurrentViewModel(Widget);
}

bool FMAEditWidgetCoordinator::HandleConfirmRequested(
    UMAEditWidget* Widget,
    const FString& JsonContent,
    FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return false;
    }

    FString ValidationError;
    if (!Widget->ValidateJsonDocument(JsonContent, ValidationError))
    {
        ApplyCurrentViewModel(Widget, ValidationError);
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::ApplyNodeEdit;
    OutRequest.Actor = SelectionState.SelectedActor.Get();
    OutRequest.JsonContent = JsonContent;
    return true;
}

bool FMAEditWidgetCoordinator::HandleDeleteActorRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return false;
    }

    const FMASceneGraphNode* CurrentNode = SelectionState.GetCurrentNode();
    if (!Widget->IsGoalOrZoneActor(SelectionState.SelectedActor.Get()) &&
        (!CurrentNode || !Widget->IsPointTypeNode(*CurrentNode)))
    {
        ApplyCurrentViewModel(Widget, TEXT("Only point type nodes can be deleted"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::DeleteActor;
    OutRequest.Actor = SelectionState.SelectedActor.Get();
    return true;
}

bool FMAEditWidgetCoordinator::HandleCreateGoalRequested(
    UMAEditWidget* Widget,
    const FString& Description,
    FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() == 0)
    {
        ApplyCurrentViewModel(Widget, TEXT("No POI selected"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::CreateGoal;
    OutRequest.POIs = MoveTemp(POIs);
    OutRequest.Description = Description.IsEmpty() ? TEXT("New Goal") : Description;
    return true;
}

bool FMAEditWidgetCoordinator::HandleCreateZoneRequested(
    UMAEditWidget* Widget,
    const FString& Description,
    FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() < 3)
    {
        ApplyCurrentViewModel(Widget, TEXT("Creating a zone requires at least 3 POIs"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::CreateZone;
    OutRequest.POIs = MoveTemp(POIs);
    OutRequest.Description = Description.IsEmpty() ? TEXT("New Zone") : Description;
    return true;
}

bool FMAEditWidgetCoordinator::HandleAddPresetActorRequested(
    UMAEditWidget* Widget,
    const FString& ActorType,
    FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() != 1)
    {
        ApplyCurrentViewModel(Widget, TEXT("Please select a single POI"));
        return false;
    }

    if (ActorType.IsEmpty() || ActorType == TEXT("(No preset Actors)"))
    {
        ApplyCurrentViewModel(Widget, TEXT("Please select a preset Actor type"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::AddPresetActor;
    OutRequest.POIs = MoveTemp(POIs);
    OutRequest.ActorType = ActorType;
    return true;
}

bool FMAEditWidgetCoordinator::HandleDeletePOIsRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() == 0)
    {
        ApplyCurrentViewModel(Widget, TEXT("No POI selected"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::DeletePOIs;
    OutRequest.POIs = MoveTemp(POIs);
    return true;
}

bool FMAEditWidgetCoordinator::HandleSetAsGoalRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::SetGoalState;
    OutRequest.Actor = SelectionState.SelectedActor.Get();
    OutRequest.bGoalState = true;
    return true;
}

bool FMAEditWidgetCoordinator::HandleUnsetAsGoalRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest)
{
    OutRequest = FMAEditWidgetActionRequest();
    if (!Widget)
    {
        return false;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return false;
    }

    ApplyCurrentViewModel(Widget);
    OutRequest.Kind = EMAEditWidgetActionKind::SetGoalState;
    OutRequest.Actor = SelectionState.SelectedActor.Get();
    OutRequest.bGoalState = false;
    return true;
}

void FMAEditWidgetCoordinator::HandleNodeSwitchRequested(AMAHUD* HUD, UMAEditWidget* Widget, int32 NodeIndex)
{
    if (!HUD || !Widget)
    {
        return;
    }

    if (!SelectionState.ActorNodes.IsValidIndex(NodeIndex))
    {
        ApplyCurrentViewModel(Widget, FString::Printf(TEXT("Invalid node index: %d"), NodeIndex));
        return;
    }

    SelectionState.CurrentNodeIndex = NodeIndex;
    ApplyCurrentViewModel(Widget);
}

void FMAEditWidgetCoordinator::ApplyCurrentViewModel(UMAEditWidget* Widget, const FString& ErrorMessage)
{
    if (!Widget)
    {
        return;
    }

    WidgetStateCoordinator.ApplyViewModel(Widget, BuildViewModel(Widget, ErrorMessage));
}

void FMAEditWidgetCoordinator::ClearSelection()
{
    SelectionState.Reset();
}

void FMAEditWidgetCoordinator::SetActorSelection(AMAHUD* HUD, AActor* Actor)
{
    SelectionState.Reset();
    SelectionState.SelectedActor = Actor;

    FString OutError;
    if (HUD && Actor)
    {
        TArray<FMASceneGraphNode> ResolvedNodes;
        if (HUD->RuntimeResolveEditActorNodes(Actor, ResolvedNodes, OutError))
        {
            SelectionState.ActorNodes = MoveTemp(ResolvedNodes);
        }
    }
}

void FMAEditWidgetCoordinator::SetPOISelection(const TArray<AMAPointOfInterest*>& POIs)
{
    SelectionState.Reset();
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            SelectionState.SelectedPOIs.Add(POI);
        }
    }
}

FMAEditWidgetViewModel FMAEditWidgetCoordinator::BuildViewModel(const UMAEditWidget* Widget, const FString& ErrorMessage) const
{
    FMAEditWidgetViewModel ViewModel;
    ViewModel.ErrorText = ErrorMessage;

    if (SelectionState.HasActor())
    {
        ViewModel.bShowActorOperations = true;
        ViewModel.bJsonReadOnly = false;
        ViewModel.CurrentNodeIndex = SelectionState.GetSafeNodeIndex();

        const FMASceneGraphNode* CurrentNode = SelectionState.GetCurrentNode();
        if (CurrentNode)
        {
            ViewModel.JsonContent = Widget ? Widget->BuildEditableJson(*CurrentNode) : CurrentNode->RawJson;
            ViewModel.bShowDeleteActor =
                (Widget && Widget->IsGoalOrZoneActor(SelectionState.SelectedActor.Get())) ||
                (Widget && Widget->IsPointTypeNode(*CurrentNode));

            const bool bIsGoalOrZoneActor = Widget && Widget->IsGoalOrZoneActor(SelectionState.SelectedActor.Get());
            const bool bIsCurrentNodeGoal = Widget && Widget->IsNodeMarkedGoal(*CurrentNode);

            ViewModel.bShowSetAsGoal = !bIsGoalOrZoneActor && !bIsCurrentNodeGoal;
            ViewModel.bShowUnsetAsGoal = !bIsGoalOrZoneActor && bIsCurrentNodeGoal;

            if (!Widget || !Widget->IsPointTypeNode(*CurrentNode))
            {
                ViewModel.HintText = TEXT("polygon/linestring type: only properties field modifications will be saved");
                ViewModel.HintTone = EMAEditWidgetHintTone::Warning;
            }
            else
            {
                ViewModel.HintText = ActorSelectedHintText;
                ViewModel.HintTone = EMAEditWidgetHintTone::Success;
            }
        }
        else
        {
            ViewModel.JsonContent = TEXT("No matching scene graph node found");
            ViewModel.bJsonReadOnly = true;
            ViewModel.HintText = TEXT("No matching scene graph node found");
            ViewModel.HintTone = EMAEditWidgetHintTone::Warning;
        }

        for (int32 Index = 0; Index < SelectionState.ActorNodes.Num(); ++Index)
        {
            FMAEditWidgetNodeTabViewModel NodeTab;
            NodeTab.Label = FString::Printf(TEXT(" Node %d "), Index + 1);
            NodeTab.bSelected = Index == ViewModel.CurrentNodeIndex;
            ViewModel.NodeTabs.Add(NodeTab);
        }
    }
    else if (SelectionState.HasPOIs())
    {
        const int32 POICount = SelectionState.GetSelectedPOIRaw().Num();
        ViewModel.bShowPOIOperations = true;
        ViewModel.bShowCreateGoal = POICount > 0;
        ViewModel.bShowCreateZone = POICount >= 3;
        ViewModel.bShowPresetActorControls = POICount == 1;
        ViewModel.HintText = FMASceneSelectionDisplay::BuildPOISelectionHint(
            POICount,
            EditDefaultHintText,
            POISingleHintText,
            POIMultiHintText);
        ViewModel.HintTone = EMAEditWidgetHintTone::Info;
    }
    else
    {
        ViewModel.HintText = EditDefaultHintText;
        ViewModel.HintTone = EMAEditWidgetHintTone::Default;
    }

    if (!ErrorMessage.IsEmpty())
    {
        ViewModel.HintTone = EMAEditWidgetHintTone::Error;
    }

    return ViewModel;
}
