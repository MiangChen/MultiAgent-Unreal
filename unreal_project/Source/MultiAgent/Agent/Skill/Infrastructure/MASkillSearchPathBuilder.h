#pragma once

#include "CoreMinimal.h"
#include "Agent/Skill/Domain/MASkillTypes.h"

class AMACharacter;
class UMASkillComponent;

struct MULTIAGENT_API FMASkillSearchPathBuilder
{
    static TArray<FVector> BuildPath(
        AMACharacter& Character,
        const UMASkillComponent& SkillComponent,
        ESearchMode SearchMode,
        float ScanWidth);
};
