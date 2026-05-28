#pragma once

#include "CoreMinimal.h"
#include "MACommandTypes.generated.h"

UENUM(BlueprintType)
enum class EMACommand : uint8
{
    None         UMETA(DisplayName = "None"),
    Idle         UMETA(DisplayName = "Idle"),
    Navigate     UMETA(DisplayName = "Navigate"),
    Follow       UMETA(DisplayName = "Follow"),
    Charge       UMETA(DisplayName = "Charge"),
    Search       UMETA(DisplayName = "Search"),
    Place        UMETA(DisplayName = "Place"),
    TakeOff      UMETA(DisplayName = "TakeOff"),
    Land         UMETA(DisplayName = "Land"),
    ReturnHome   UMETA(DisplayName = "ReturnHome"),
    TakePhoto    UMETA(DisplayName = "TakePhoto"),
    Broadcast    UMETA(DisplayName = "Broadcast"),
    HandleHazard UMETA(DisplayName = "HandleHazard"),
    Guide        UMETA(DisplayName = "Guide")
};
