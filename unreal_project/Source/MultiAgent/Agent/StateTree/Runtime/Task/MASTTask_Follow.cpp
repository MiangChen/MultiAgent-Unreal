// MASTTask_Follow.cpp
// StateTree 跟随任务 - 从 SkillComponent 读取参数

#include "MASTTask_Follow.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
EStateTreeRunStatus ToFollowTaskRunStatus(const EMAStateTreeTaskDecision Decision)
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

EStateTreeRunStatus FMASTTask_Follow::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);
    Data.TargetActor.Reset();
    Data.bSkillActivated = false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    const FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargets();

    AActor* TargetActor = RuntimeTargets.FollowTarget.Get();
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: No FollowTarget"), *Character->AgentLabel);
        return EStateTreeRunStatus::Failed;
    }
    
    Data.TargetActor = TargetActor;

    const bool bActivated =
        FMASkillActivationUseCases::PrepareAndActivateFollow(*SkillComp, TargetActor, Params.FollowDistance);
    if (!bActivated)
    {
        return EStateTreeRunStatus::Failed;
    }

    Data.bSkillActivated = true;
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s following %s"), 
        *Character->AgentLabel, *TargetActor->GetName());

    return ToFollowTaskRunStatus(FMAStateTreeUseCases::BuildCommandEnterDecision(bActivated));
}

EStateTreeRunStatus FMASTTask_Follow::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    return ToFollowTaskRunStatus(FMAStateTreeUseCases::BuildFollowTickDecision(
        SkillComp != nullptr,
        SkillComp && FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Follow),
        Data.TargetActor.IsValid()));
}

void FMASTTask_Follow::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return;
    
    const FMAStateTreeTaskExitFeedback Feedback =
        FMAStateTreeUseCases::BuildActivatedCommandExit(Data.bSkillActivated, false);
    if (Feedback.bShouldCancelCommand)
    {
        if (UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>())
        {
            FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Follow, Data.bSkillActivated);
        }
    }
}
