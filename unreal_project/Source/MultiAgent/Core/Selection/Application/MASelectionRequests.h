#pragma once

#include "CoreMinimal.h"

struct MULTIAGENT_API FSelectionRequest
{
    FString RequestId;
    FString Action;
    FString TargetId;

    bool IsValid() const
    {
        return !Action.IsEmpty();
    }
};
