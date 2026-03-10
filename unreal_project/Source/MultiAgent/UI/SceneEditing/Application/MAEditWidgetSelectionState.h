#pragma once

#include "CoreMinimal.h"
#include "../../../Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"

class AMAPointOfInterest;

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
