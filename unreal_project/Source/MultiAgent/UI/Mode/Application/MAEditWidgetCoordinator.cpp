// Edit widget application-layer orchestration.

#include "MAEditWidgetCoordinator.h"

#include "../../HUD/MAHUD.h"
#include "../MAEditWidget.h"
#include "../Domain/MASceneSelectionDisplay.h"
#include "../../../Core/Manager/MAEditModeManager.h"
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

    UWorld* World = HUD->GetWorld();
    UMAEditModeManager* EditModeManager = World ? World->GetSubsystem<UMAEditModeManager>() : nullptr;
    if (!EditModeManager)
    {
        ClearSelection();
        ApplyCurrentViewModel(Widget, TEXT("EditModeManager not found"));
        return;
    }

    if (EditModeManager->HasSelectedActor())
    {
        SetActorSelection(HUD, EditModeManager->GetSelectedActor());
    }
    else if (EditModeManager->HasSelectedPOIs())
    {
        SetPOISelection(EditModeManager->GetSelectedPOIs());
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

void FMAEditWidgetCoordinator::HandleConfirmRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& JsonContent)
{
    if (!HUD || !Widget)
    {
        return;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return;
    }

    FString ValidationError;
    if (!SceneGraphAdapter.ValidateJsonDocument(JsonContent, ValidationError))
    {
        ApplyCurrentViewModel(Widget, ValidationError);
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditConfirmed(SelectionState.SelectedActor.Get(), JsonContent);
}

void FMAEditWidgetCoordinator::HandleDeleteActorRequested(AMAHUD* HUD, UMAEditWidget* Widget)
{
    if (!HUD || !Widget)
    {
        return;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return;
    }

    const FMASceneGraphNode* CurrentNode = SelectionState.GetCurrentNode();
    if (!SceneGraphAdapter.IsGoalOrZoneActor(SelectionState.SelectedActor.Get()) &&
        (!CurrentNode || !SceneGraphAdapter.IsPointTypeNode(*CurrentNode)))
    {
        ApplyCurrentViewModel(Widget, TEXT("Only point type nodes can be deleted"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditDeleteActor(SelectionState.SelectedActor.Get());
}

void FMAEditWidgetCoordinator::HandleCreateGoalRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& Description)
{
    if (!HUD || !Widget)
    {
        return;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() == 0)
    {
        ApplyCurrentViewModel(Widget, TEXT("No POI selected"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditCreateGoal(POIs[0], Description.IsEmpty() ? TEXT("New Goal") : Description);
}

void FMAEditWidgetCoordinator::HandleCreateZoneRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& Description)
{
    if (!HUD || !Widget)
    {
        return;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() < 3)
    {
        ApplyCurrentViewModel(Widget, TEXT("Creating a zone requires at least 3 POIs"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditCreateZone(POIs, Description.IsEmpty() ? TEXT("New Zone") : Description);
}

void FMAEditWidgetCoordinator::HandleAddPresetActorRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& ActorType)
{
    if (!HUD || !Widget)
    {
        return;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() != 1)
    {
        ApplyCurrentViewModel(Widget, TEXT("Please select a single POI"));
        return;
    }

    if (ActorType.IsEmpty() || ActorType == TEXT("(No preset Actors)"))
    {
        ApplyCurrentViewModel(Widget, TEXT("Please select a preset Actor type"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditAddPresetActor(POIs[0], ActorType);
}

void FMAEditWidgetCoordinator::HandleDeletePOIsRequested(AMAHUD* HUD, UMAEditWidget* Widget)
{
    if (!HUD || !Widget)
    {
        return;
    }

    TArray<AMAPointOfInterest*> POIs = SelectionState.GetSelectedPOIRaw();
    if (POIs.Num() == 0)
    {
        ApplyCurrentViewModel(Widget, TEXT("No POI selected"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditDeletePOIs(POIs);
}

void FMAEditWidgetCoordinator::HandleSetAsGoalRequested(AMAHUD* HUD, UMAEditWidget* Widget)
{
    if (!HUD || !Widget)
    {
        return;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditSetAsGoal(SelectionState.SelectedActor.Get());
}

void FMAEditWidgetCoordinator::HandleUnsetAsGoalRequested(AMAHUD* HUD, UMAEditWidget* Widget)
{
    if (!HUD || !Widget)
    {
        return;
    }

    if (!SelectionState.HasActor())
    {
        ApplyCurrentViewModel(Widget, TEXT("No Actor selected"));
        return;
    }

    ApplyCurrentViewModel(Widget);
    HUD->OnEditUnsetAsGoal(SelectionState.SelectedActor.Get());
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

    WidgetStateCoordinator.ApplyViewModel(Widget, BuildViewModel(ErrorMessage));
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
        if (SceneGraphAdapter.ResolveActorNodes(HUD->GetWorld(), Actor, ResolvedNodes, OutError))
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

FMAEditWidgetViewModel FMAEditWidgetCoordinator::BuildViewModel(const FString& ErrorMessage) const
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
            ViewModel.JsonContent = SceneGraphAdapter.BuildEditableJson(*CurrentNode);
            ViewModel.bShowDeleteActor =
                SceneGraphAdapter.IsGoalOrZoneActor(SelectionState.SelectedActor.Get()) ||
                SceneGraphAdapter.IsPointTypeNode(*CurrentNode);

            const bool bIsGoalOrZoneActor = SceneGraphAdapter.IsGoalOrZoneActor(SelectionState.SelectedActor.Get());
            const bool bIsCurrentNodeGoal = SceneGraphAdapter.IsNodeMarkedGoal(*CurrentNode);

            ViewModel.bShowSetAsGoal = !bIsGoalOrZoneActor && !bIsCurrentNodeGoal;
            ViewModel.bShowUnsetAsGoal = !bIsGoalOrZoneActor && bIsCurrentNodeGoal;

            if (!SceneGraphAdapter.IsPointTypeNode(*CurrentNode))
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
