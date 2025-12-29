// MASTTask_Follow.cpp
// StateTree 跟随任务 - 从 SkillComponent 读取参数

#include "MASTTask_Follow.h"
#include "../../Character/MACharacter.h"
#include "../../Skill/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Follow::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);
    Data.TargetCharacter.Reset();
    Data.bSkillActivated = false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    
    AMACharacter* TargetCharacter = Params.FollowTarget.Get();
    if (!TargetCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: No FollowTarget"), *Character->AgentName);
        return EStateTreeRunStatus::Failed;
    }
    
    Data.TargetCharacter = TargetCharacter;

    if (!SkillComp->TryActivateFollow(TargetCharacter, Params.FollowDistance))
    {
        return EStateTreeRunStatus::Failed;
    }
    
    Data.bSkillActivated = true;
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s following %s"), 
        *Character->AgentName, *TargetCharacter->AgentName);

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
    if (SkillComp && !SkillComp->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow"))))
    {
        return EStateTreeRunStatus::Succeeded;
    }

    if (!Data.TargetCharacter.IsValid())
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
        if (Data.bSkillActivated)
        {
            SkillComp->CancelFollow();
        }
    }
}
