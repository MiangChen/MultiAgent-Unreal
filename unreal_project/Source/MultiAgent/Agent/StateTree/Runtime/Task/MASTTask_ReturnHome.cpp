// MASTTask_ReturnHome.cpp
// StateTree 返航任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_ReturnHome.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_ReturnHome::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandEnterStatus(
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::ReturnHome));
}

EStateTreeRunStatus FMASTTask_ReturnHome::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandTickStatus(*SkillComp, EMACommand::ReturnHome);
}

void FMASTTask_ReturnHome::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    MASTTaskUtils::HandleInterruptedCommandExit(Context, Transition, EMACommand::ReturnHome);
}
