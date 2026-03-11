#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/Command/Domain/MACommandTypes.h"

struct MULTIAGENT_API FMACommandTags
{
    static FGameplayTag ToTag(EMACommand Command);
};
