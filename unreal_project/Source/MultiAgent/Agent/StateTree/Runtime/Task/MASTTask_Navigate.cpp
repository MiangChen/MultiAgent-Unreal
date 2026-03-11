// MASTTask_Navigate.cpp
// StateTree 导航任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Navigate.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Navigate::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    return MASTTaskUtils::BuildCommandEnterStatus(
        FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComp, Params.TargetLocation));
}

EStateTreeRunStatus FMASTTask_Navigate::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandTickStatus(*SkillComp, EMACommand::Navigate);
}

void FMASTTask_Navigate::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    MASTTaskUtils::HandleInterruptedCommandExit(Context, Transition, EMACommand::Navigate);
}
