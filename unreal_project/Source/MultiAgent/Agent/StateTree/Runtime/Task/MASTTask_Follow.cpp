// MASTTask_Follow.cpp
// StateTree 跟随任务 - 从 SkillComponent 读取参数

#include "MASTTask_Follow.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Follow::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);
    Data.TargetActor.Reset();
    Data.bSkillActivated = false;

    AMACharacter* Character = MASTTaskUtils::ResolveCharacter(Context);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
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

    return MASTTaskUtils::BuildCommandEnterStatus(bActivated);
}

EStateTreeRunStatus FMASTTask_Follow::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AMACharacter* Character = MASTTaskUtils::ResolveCharacter(Context);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    return MASTTaskUtils::ToRunStatus(FMAStateTreeUseCases::BuildFollowTickDecision(
        SkillComp != nullptr,
        SkillComp && FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Follow),
        Data.TargetActor.IsValid()));
}

void FMASTTask_Follow::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    MASTTaskUtils::HandleActivatedCommandExit(Context, Data.bSkillActivated, EMACommand::Follow, false);
}
