#include "Agent/Skill/Application/MASkillActivationUseCases.h"

#include "Agent/Skill/Runtime/MASkillRuntimeGateway.h"

bool FMASkillActivationUseCases::PrepareAndActivateNavigate(UMASkillComponent& SkillComponent, const FVector TargetLocation)
{
    return FMASkillRuntimeGateway::PrepareAndActivateNavigate(SkillComponent, TargetLocation);
}

bool FMASkillActivationUseCases::PrepareAndActivateFollow(
    UMASkillComponent& SkillComponent,
    AActor* TargetActor,
    const float FollowDistance)
{
    if (!TargetActor)
    {
        return false;
    }

    return FMASkillRuntimeGateway::PrepareAndActivateFollow(SkillComponent, TargetActor, FollowDistance);
}

bool FMASkillActivationUseCases::PrepareAndActivateCharge(UMASkillComponent& SkillComponent)
{
    return FMASkillRuntimeGateway::PrepareAndActivateCharge(SkillComponent);
}

bool FMASkillActivationUseCases::ActivatePreparedCommand(UMASkillComponent& SkillComponent, const EMACommand Command)
{
    return FMASkillRuntimeGateway::ActivatePreparedCommand(SkillComponent, Command);
}

void FMASkillActivationUseCases::CancelCommand(UMASkillComponent& SkillComponent, const EMACommand Command)
{
    FMASkillRuntimeGateway::CancelCommand(SkillComponent, Command);
}
