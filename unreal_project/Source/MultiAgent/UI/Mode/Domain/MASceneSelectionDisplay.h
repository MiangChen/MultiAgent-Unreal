#pragma once

#include "CoreMinimal.h"

class AActor;

// Shared selection display text helpers used by edit/modify flows.
struct FMASceneSelectionDisplay
{
    static FString BuildActorSelectionHint(const TArray<AActor*>& Actors, const FString& DefaultHintText)
    {
        if (Actors.Num() == 1 && Actors[0])
        {
            return FString::Printf(TEXT("Selected: %s"), *Actors[0]->GetName());
        }

        if (Actors.Num() > 1)
        {
            return FString::Printf(TEXT("Selected: %d Actors"), Actors.Num());
        }

        return DefaultHintText;
    }

    static FString BuildPOISelectionHint(
        int32 POICount,
        const FString& DefaultHintText,
        const FString& SingleHintText,
        const FString& MultiHintText)
    {
        if (POICount <= 0)
        {
            return DefaultHintText;
        }

        if (POICount == 1)
        {
            return SingleHintText;
        }

        if (POICount >= 3)
        {
            return FString::Printf(TEXT("%d POIs selected, can create zone"), POICount);
        }

        return MultiHintText;
    }
};
