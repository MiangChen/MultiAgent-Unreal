#pragma once

#include "CoreMinimal.h"

struct FMASelectionHUDCircleEntry
{
    FVector WorldLocation = FVector::ZeroVector;
    bool bSelected = false;
};

struct FMASelectionHUDControlGroupEntry
{
    int32 GroupIndex = 0;
    int32 UnitCount = 0;
};

struct FMASelectionHUDStatusTextEntry
{
    FVector2D ScreenPosition = FVector2D::ZeroVector;
    FString Text;
};

struct FMASelectionHUDFrameModel
{
    bool bHasFullscreenWidget = false;
    bool bHasBlockingVisibleWidget = false;
    TArray<FMASelectionHUDCircleEntry> CircleEntries;
    TArray<FMASelectionHUDControlGroupEntry> ControlGroupEntries;
    TArray<FMASelectionHUDStatusTextEntry> StatusEntries;
};
