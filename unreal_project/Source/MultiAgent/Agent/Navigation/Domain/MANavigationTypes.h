#pragma once

#include "CoreMinimal.h"
#include "MANavigationTypes.generated.h"

UENUM(BlueprintType)
enum class EMANavigationState : uint8
{
    Idle,
    Navigating,
    TakingOff,
    Landing,
    Arrived,
    Failed,
    Cancelled
};

UENUM()
enum class EMANavigationPauseMode : uint8
{
    None,
    Following,
    Flying,
    Manual,
    Ground
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnNavigationCompleted,
    bool,
    bSuccess,
    const FString&,
    Message
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnNavigationStateChanged,
    EMANavigationState,
    NewState
);
