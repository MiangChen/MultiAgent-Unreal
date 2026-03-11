// MASTTask_TakeOff.cpp
// StateTree 起飞任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_TakeOff.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_TakeOff::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    if (FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::TakeOff))
    {
        return EStateTreeRunStatus::Running;
    }
    
    return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FMASTTask_TakeOff::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr;
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    // 检查命令 Tag 是否还存在（由 GAS Ability 完成时清除）
    if (FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::TakeOff))
    {
        return EStateTreeRunStatus::Succeeded;
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_TakeOff::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    // 无需额外清理
}
