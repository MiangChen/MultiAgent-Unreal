// MASTTask_Land.cpp
// StateTree 降落任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Land.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Land::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandEnterStatus(
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::Land));
}

EStateTreeRunStatus FMASTTask_Land::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandTickStatus(*SkillComp, EMACommand::Land);
}

void FMASTTask_Land::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    MASTTaskUtils::HandleInterruptedCommandExit(Context, Transition, EMACommand::Land);
}
