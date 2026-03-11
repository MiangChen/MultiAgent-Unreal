// MASTTask_Navigate.cpp
// StateTree 导航任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Navigate.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
EStateTreeRunStatus ToNavigateTaskRunStatus(const EMAStateTreeTaskDecision Decision)
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

EStateTreeRunStatus FMASTTask_Navigate::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    FVector TargetLocation = Params.TargetLocation;

    return ToNavigateTaskRunStatus(FMAStateTreeUseCases::BuildCommandEnterDecision(
        FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComp, TargetLocation)));
}

EStateTreeRunStatus FMASTTask_Navigate::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return ToNavigateTaskRunStatus(FMAStateTreeUseCases::BuildCommandTickDecision(
        true,
        FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Navigate)));
}

void FMASTTask_Navigate::ExitState(
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
            FMASkillExecutionUseCases::CancelCommandIfInterrupted(*SkillComp, EMACommand::Navigate, Transition.CurrentRunStatus);
        }
    }
}
