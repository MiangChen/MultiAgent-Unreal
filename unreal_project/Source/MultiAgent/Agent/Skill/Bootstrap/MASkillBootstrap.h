#pragma once

#include "CoreMinimal.h"

class AActor;
class UMASkillComponent;

class MULTIAGENT_API FMASkillBootstrap
{
public:
    static void GrantDefaultAbilities(UMASkillComponent& SkillComponent, AActor* OwnerActor);
};
