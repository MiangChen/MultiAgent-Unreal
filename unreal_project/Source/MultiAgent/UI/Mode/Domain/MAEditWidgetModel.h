#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Manager/MASceneGraphNodeTypes.h"

class AMAPointOfInterest;

enum class EMAEditWidgetHintTone : uint8
{
    Default,
    Info,
    Success,
    Warning,
    Error
};

struct FMAEditWidgetNodeTabViewModel
{
    FString Label;
    bool bSelected = false;
};

struct FMAEditWidgetViewModel
{
    FString HintText = TEXT("Select an Actor or POI to operate");
    EMAEditWidgetHintTone HintTone = EMAEditWidgetHintTone::Default;
    FString ErrorText;
    FString JsonContent;
    bool bJsonReadOnly = true;

    bool bShowActorOperations = false;
    bool bShowPOIOperations = false;
    bool bShowCreateGoal = false;
    bool bShowCreateZone = false;
    bool bShowPresetActorControls = false;
    bool bShowDeleteActor = false;
    bool bShowSetAsGoal = false;
    bool bShowUnsetAsGoal = false;

    TArray<FMAEditWidgetNodeTabViewModel> NodeTabs;
    int32 CurrentNodeIndex = 0;
};

struct FMAEditWidgetSelectionState
{
    TWeakObjectPtr<AActor> SelectedActor;
    TArray<TWeakObjectPtr<AMAPointOfInterest>> SelectedPOIs;
    TArray<FMASceneGraphNode> ActorNodes;
    int32 CurrentNodeIndex = 0;

    void Reset()
    {
        SelectedActor.Reset();
        SelectedPOIs.Empty();
        ActorNodes.Empty();
        CurrentNodeIndex = 0;
    }

    bool HasActor() const
    {
        return SelectedActor.IsValid();
    }

    bool HasPOIs() const
    {
        for (const TWeakObjectPtr<AMAPointOfInterest>& POI : SelectedPOIs)
        {
            if (POI.IsValid())
            {
                return true;
            }
        }
        return false;
    }

    int32 GetSafeNodeIndex() const
    {
        return ActorNodes.IsValidIndex(CurrentNodeIndex) ? CurrentNodeIndex : 0;
    }

    const FMASceneGraphNode* GetCurrentNode() const
    {
        return ActorNodes.IsValidIndex(GetSafeNodeIndex()) ? &ActorNodes[GetSafeNodeIndex()] : nullptr;
    }

    TArray<AMAPointOfInterest*> GetSelectedPOIRaw() const
    {
        TArray<AMAPointOfInterest*> Result;
        for (const TWeakObjectPtr<AMAPointOfInterest>& POI : SelectedPOIs)
        {
            if (POI.IsValid())
            {
                Result.Add(POI.Get());
            }
        }
        return Result;
    }
};
