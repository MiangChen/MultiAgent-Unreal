#pragma once

#include "CoreMinimal.h"

// Shared selection display text helpers used by edit/modify flows.
struct FMASceneSelectionDisplay
{
    static FString BuildActorSelectionHint(
        int32 ActorCount,
        const FString& SingleActorName,
        const FString& DefaultHintText)
    {
        if (ActorCount == 1 && !SingleActorName.IsEmpty())
        {
            return FString::Printf(TEXT("Selected: %s"), *SingleActorName);
        }

        if (ActorCount > 1)
        {
            return FString::Printf(TEXT("Selected: %d Actors"), ActorCount);
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
