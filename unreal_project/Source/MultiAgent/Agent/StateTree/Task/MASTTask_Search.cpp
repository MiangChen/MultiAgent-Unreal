// MASTTask_Search.cpp
// StateTree 搜索任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Search.h"
#include "../../Character/MACharacter.h"
#include "../../Skill/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Search::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    if (SkillComp->TryActivateSearch())
    {
        return EStateTreeRunStatus::Running;
    }
    
    return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FMASTTask_Search::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr;
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    // 检查命令 Tag 是否还存在（由 GAS Ability 完成时清除）
    if (!SkillComp->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Search"))))
    {
        return EStateTreeRunStatus::Succeeded;
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Search::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    // 如果是被中断（不是正常完成），取消搜索
    if (Transition.CurrentRunStatus == EStateTreeRunStatus::Running)
    {
        AActor* Owner = Cast<AActor>(Context.GetOwner());
        if (UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr)
        {
            SkillComp->CancelSearch();
        }
    }
}
