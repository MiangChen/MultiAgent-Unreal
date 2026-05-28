#pragma once

#include "CoreMinimal.h"

struct FMAInstructionPanelActionResult
{
    bool bAccepted = false;
    bool bDismissRequestNotification = false;
    FString Command;
};
