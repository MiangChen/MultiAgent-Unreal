#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"

struct MULTIAGENT_API FMACommandNames
{
    static FString ToString(EMACommand Command);
    static EMACommand FromString(const FString& CommandString);
};
