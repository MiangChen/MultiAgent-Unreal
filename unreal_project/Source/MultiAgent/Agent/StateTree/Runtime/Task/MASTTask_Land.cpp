// MASTTask_Land.cpp
// StateTree 降落任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Land.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
EStateTreeRunStatus ToLandTaskRunStatus(const EMAStateTreeTaskDecision Decision)
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

EStateTreeRunStatus FMASTTask_Land::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return ToLandTaskRunStatus(FMAStateTreeUseCases::BuildCommandEnterDecision(
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::Land)));
}

EStateTreeRunStatus FMASTTask_Land::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr;
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return ToLandTaskRunStatus(FMAStateTreeUseCases::BuildCommandTickDecision(
        true,
        FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Land)));
}

void FMASTTask_Land::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    // 无需额外清理
}
