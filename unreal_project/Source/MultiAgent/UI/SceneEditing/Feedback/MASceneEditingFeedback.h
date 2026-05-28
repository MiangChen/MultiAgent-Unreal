#pragma once

#include "CoreMinimal.h"

struct FMASceneActionResult
{
    bool bSuccess = false;
    bool bIsWarning = false;
    bool bClearSelection = false;
    bool bRefreshSceneList = false;
    bool bRefreshSelection = false;
    bool bReloadSceneVisualization = false;
    bool bClearHighlightedActor = false;
    FString Message;
};
