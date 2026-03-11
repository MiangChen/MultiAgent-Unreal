// MASTTask_TakeOff.cpp
// StateTree 起飞任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_TakeOff.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_TakeOff::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandEnterStatus(
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::TakeOff));
}

EStateTreeRunStatus FMASTTask_TakeOff::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandTickStatus(*SkillComp, EMACommand::TakeOff);
}

void FMASTTask_TakeOff::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    MASTTaskUtils::HandleInterruptedCommandExit(Context, Transition, EMACommand::TakeOff);
}
