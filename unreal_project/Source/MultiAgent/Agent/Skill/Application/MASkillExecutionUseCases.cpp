#include "Agent/Skill/Application/MASkillExecutionUseCases.h"

#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Core/Command/Domain/MACommandTags.h"

bool FMASkillExecutionUseCases::HasCommandCompleted(const UMASkillComponent& SkillComponent, const EMACommand Command)
{
    const FGameplayTag CommandTag = FMACommandTags::ToTag(Command);
    return !CommandTag.IsValid() || !SkillComponent.HasMatchingGameplayTag(CommandTag);
}

void FMASkillExecutionUseCases::CancelCommandIfActivated(
    UMASkillComponent& SkillComponent,
    const EMACommand Command,
    const bool bShouldCancel)
{
    if (bShouldCancel)
    {
        FMASkillActivationUseCases::CancelCommand(SkillComponent, Command);
    }
}

void FMASkillExecutionUseCases::CancelCommandIfInterrupted(
    UMASkillComponent& SkillComponent,
    const EMACommand Command,
    const EStateTreeRunStatus CurrentRunStatus)
{
    CancelCommandIfActivated(SkillComponent, Command, CurrentRunStatus == EStateTreeRunStatus::Running);
}

void FMASkillExecutionUseCases::TransitionCommandToIdle(UMASkillComponent& SkillComponent, const EMACommand Command)
{
    const FGameplayTag CommandTag = FMACommandTags::ToTag(Command);
    if (CommandTag.IsValid())
    {
        SkillComponent.RemoveLooseGameplayTag(CommandTag);
    }
    SkillComponent.AddLooseGameplayTag(FMACommandTags::ToTag(EMACommand::Idle));
}

EStateTreeRunStatus FMASkillExecutionUseCases::StartChargeFlow(
    UMASkillComponent& SkillComponent,
    const FVector& CharacterLocation,
    const FVector& StationLocation,
    const float InteractionRadius,
    bool& bOutIsMoving,
    bool& bOutIsCharging)
{
    SkillComponent.GetFeedbackContextMutable().EnergyBefore = SkillComponent.GetEnergyPercent();

    const float Distance = FVector::Dist(CharacterLocation, StationLocation);
    if (Distance <= InteractionRadius)
    {
        bOutIsMoving = false;
        bOutIsCharging = true;
        return FMASkillActivationUseCases::PrepareAndActivateCharge(SkillComponent)
            ? EStateTreeRunStatus::Running
            : EStateTreeRunStatus::Failed;
    }

    if (FMASkillActivationUseCases::PrepareAndActivateNavigate(SkillComponent, StationLocation))
    {
        bOutIsMoving = true;
        bOutIsCharging = false;
        return EStateTreeRunStatus::Running;
    }

    bOutIsMoving = false;
    bOutIsCharging = false;
    return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FMASkillExecutionUseCases::TickChargeFlow(
    UMASkillComponent& SkillComponent,
    const FVector& CharacterLocation,
    const bool bIsCharacterMoving,
    const FVector& StationLocation,
    const float InteractionRadius,
    bool& bInOutIsMoving,
    bool& bInOutIsCharging)
{
    SkillComponent.GetFeedbackContextMutable().EnergyAfter = SkillComponent.GetEnergyPercent();

    if (bInOutIsCharging)
    {
        if (SkillComponent.IsFullEnergy())
        {
            TransitionCommandToIdle(SkillComponent, EMACommand::Charge);
            return EStateTreeRunStatus::Succeeded;
        }
        return EStateTreeRunStatus::Running;
    }

    if (bInOutIsMoving)
    {
        const float Distance = FVector::Dist(CharacterLocation, StationLocation);
        if (Distance <= InteractionRadius)
        {
            bInOutIsMoving = false;
            bInOutIsCharging = true;
            if (!FMASkillActivationUseCases::PrepareAndActivateCharge(SkillComponent))
            {
                return EStateTreeRunStatus::Failed;
            }
        }
        else if (!bIsCharacterMoving)
        {
            FMASkillActivationUseCases::PrepareAndActivateNavigate(SkillComponent, StationLocation);
        }
    }

    return EStateTreeRunStatus::Running;
}
