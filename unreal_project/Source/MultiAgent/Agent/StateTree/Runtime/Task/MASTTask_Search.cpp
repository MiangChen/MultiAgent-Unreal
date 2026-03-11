// MASTTask_Search.cpp
// StateTree 搜索任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Search.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Search::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandEnterStatus(
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::Search));
}

EStateTreeRunStatus FMASTTask_Search::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandTickStatus(*SkillComp, EMACommand::Search);
}

void FMASTTask_Search::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    MASTTaskUtils::HandleInterruptedCommandExit(Context, Transition, EMACommand::Search);
}
