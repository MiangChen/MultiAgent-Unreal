#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "StateTreeTaskBase.h"

class UMASkillComponent;

struct MULTIAGENT_API FMASkillExecutionUseCases
{
    static bool HasCommandCompleted(const UMASkillComponent& SkillComponent, EMACommand Command);
    static void CancelCommandIfActivated(UMASkillComponent& SkillComponent, EMACommand Command, bool bShouldCancel);
    static void CancelCommandIfInterrupted(UMASkillComponent& SkillComponent, EMACommand Command, EStateTreeRunStatus CurrentRunStatus);
    static void TransitionCommandToIdle(UMASkillComponent& SkillComponent, EMACommand Command);

    static EStateTreeRunStatus StartChargeFlow(
        UMASkillComponent& SkillComponent,
        const FVector& CharacterLocation,
        const FVector& StationLocation,
        float InteractionRadius,
        bool& bOutIsMoving,
        bool& bOutIsCharging);

    static EStateTreeRunStatus TickChargeFlow(
        UMASkillComponent& SkillComponent,
        const FVector& CharacterLocation,
        bool bIsCharacterMoving,
        const FVector& StationLocation,
        float InteractionRadius,
        bool& bInOutIsMoving,
        bool& bInOutIsCharging);
};
