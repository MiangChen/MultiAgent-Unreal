#pragma once

#include "CoreMinimal.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "StateTreeExecutionContext.h"

namespace MASTTaskUtils
{
inline EStateTreeRunStatus ToRunStatus(const EMAStateTreeTaskDecision Decision)
{
    switch (Decision)
    {
        case EMAStateTreeTaskDecision::Succeeded:
            return EStateTreeRunStatus::Succeeded;
        case EMAStateTreeTaskDecision::Running:
            return EStateTreeRunStatus::Running;
        default:
            return EStateTreeRunStatus::Failed;
    }
}

inline AMACharacter* ResolveCharacter(FStateTreeExecutionContext& Context)
{
    return Cast<AMACharacter>(Cast<AActor>(Context.GetOwner()));
}

inline UMASkillComponent* ResolveSkillComponent(FStateTreeExecutionContext& Context)
{
    if (AMACharacter* Character = ResolveCharacter(Context))
    {
        return Character->GetSkillComponent();
    }

    return nullptr;
}

inline UMASkillComponent* ResolveSkillComponent(AActor* Owner)
{
    return Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr;
}

inline EStateTreeRunStatus BuildCommandEnterStatus(const bool bActivationSucceeded)
{
    return ToRunStatus(FMAStateTreeUseCases::BuildCommandEnterDecision(bActivationSucceeded));
}

inline EStateTreeRunStatus BuildCommandTickStatus(UMASkillComponent& SkillComp, const EMACommand Command)
{
    return ToRunStatus(FMAStateTreeUseCases::BuildCommandTickDecision(
        true,
        FMASkillExecutionUseCases::HasCommandCompleted(SkillComp, Command)));
}

inline void HandleInterruptedCommandExit(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition,
    const EMACommand Command)
{
    const FMAStateTreeTaskExitFeedback Feedback =
        FMAStateTreeUseCases::BuildInterruptedCommandExit(
            Transition.CurrentRunStatus == EStateTreeRunStatus::Running);
    if (!Feedback.bShouldCancelCommand)
    {
        return;
    }

    if (UMASkillComponent* SkillComp = ResolveSkillComponent(Cast<AActor>(Context.GetOwner())))
    {
        FMASkillExecutionUseCases::CancelCommandIfInterrupted(*SkillComp, Command, Transition.CurrentRunStatus);
    }
}

inline void HandleActivatedCommandExit(
    FStateTreeExecutionContext& Context,
    const bool bSkillActivated,
    const EMACommand Command,
    const bool bTransitionCommandToIdle)
{
    const FMAStateTreeTaskExitFeedback Feedback =
        FMAStateTreeUseCases::BuildActivatedCommandExit(bSkillActivated, bTransitionCommandToIdle);
    if (!Feedback.bShouldCancelCommand)
    {
        return;
    }

    if (UMASkillComponent* SkillComp = ResolveSkillComponent(Cast<AActor>(Context.GetOwner())))
    {
        FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, Command, bSkillActivated);
        if (Feedback.bShouldTransitionCommandToIdle)
        {
            FMASkillExecutionUseCases::TransitionCommandToIdle(*SkillComp, Command);
        }
    }
}

inline void ClearOwnerStatus(FStateTreeExecutionContext& Context)
{
    if (AMACharacter* Character = ResolveCharacter(Context))
    {
        Character->ShowStatus(TEXT(""), 0.f);
    }
}
}
