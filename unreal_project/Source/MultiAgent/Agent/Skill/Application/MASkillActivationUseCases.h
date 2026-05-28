#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"

class AActor;
class UMASkillComponent;

struct MULTIAGENT_API FMASkillActivationUseCases
{
    static bool PrepareAndActivateNavigate(UMASkillComponent& SkillComponent, FVector TargetLocation);
    static bool PrepareAndActivateFollow(UMASkillComponent& SkillComponent, AActor* TargetActor, float FollowDistance = 300.f);
    static bool PrepareAndActivateCharge(UMASkillComponent& SkillComponent);
    static bool ActivatePreparedCommand(UMASkillComponent& SkillComponent, EMACommand Command);
    static void CancelCommand(UMASkillComponent& SkillComponent, EMACommand Command);
};
