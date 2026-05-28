#pragma once

#include "CoreMinimal.h"

struct MULTIAGENT_API FEnvironmentCoreRequest
{
    FString RequestId;
    FString Action;
    FString TargetId;

    bool IsValid() const
    {
        return !Action.IsEmpty();
    }
};
