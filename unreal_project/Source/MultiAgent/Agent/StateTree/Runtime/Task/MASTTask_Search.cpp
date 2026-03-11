// MASTTask_Search.cpp
// StateTree 搜索任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Search.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
EStateTreeRunStatus ToSearchTaskRunStatus(const EMAStateTreeTaskDecision Decision)
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
}

EStateTreeRunStatus FMASTTask_Search::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return ToSearchTaskRunStatus(FMAStateTreeUseCases::BuildCommandEnterDecision(
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::Search)));
}

EStateTreeRunStatus FMASTTask_Search::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr;
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return ToSearchTaskRunStatus(FMAStateTreeUseCases::BuildCommandTickDecision(
        true,
        FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Search)));
}

void FMASTTask_Search::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    const FMAStateTreeTaskExitFeedback Feedback =
        FMAStateTreeUseCases::BuildInterruptedCommandExit(
            Transition.CurrentRunStatus == EStateTreeRunStatus::Running);
    if (Feedback.bShouldCancelCommand)
    {
        AActor* Owner = Cast<AActor>(Context.GetOwner());
        if (UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr)
        {
            FMASkillExecutionUseCases::CancelCommandIfInterrupted(*SkillComp, EMACommand::Search, Transition.CurrentRunStatus);
        }
    }
}
