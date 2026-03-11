// MASTTask_Follow.cpp
// StateTree 跟随任务 - 从 SkillComponent 读取参数

#include "MASTTask_Follow.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

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

    if (!FMASkillActivationUseCases::PrepareAndActivateFollow(*SkillComp, TargetActor, Params.FollowDistance))
    {
        return EStateTreeRunStatus::Failed;
    }
    
    Data.bSkillActivated = true;
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s following %s"), 
        *Character->AgentLabel, *TargetActor->GetName());

    return EStateTreeRunStatus::Running;
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
    if (SkillComp && FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Follow))
    {
        return EStateTreeRunStatus::Succeeded;
    }

    if (!Data.TargetActor.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Follow::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return;
    
    if (UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>())
    {
        FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Follow, Data.bSkillActivated);
    }
}
