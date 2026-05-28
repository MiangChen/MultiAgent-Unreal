#pragma once

#include "CoreMinimal.h"

struct FMASelectionSnapshot
{
    TArray<FString> SelectedAgentIds;
    int32 HighlightedActorCount = 0;
    int32 SelectedPOICount = 0;
    bool bHasPrimaryActorSelection = false;
};
