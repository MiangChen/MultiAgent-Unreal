// MASTTask_Navigate.cpp
// StateTree 导航任务 - 只负责启动技能，完成由 GAS Ability 处理

#include "MASTTask_Navigate.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Navigate::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    FVector TargetLocation = Params.TargetLocation;

    if (FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComp, TargetLocation))
    {
        return EStateTreeRunStatus::Running;
    }
    
    return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FMASTTask_Navigate::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    // 检查命令 Tag 是否还存在（由 GAS Ability 完成时清除）
    if (!SkillComp->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Navigate"))))
    {
        return EStateTreeRunStatus::Succeeded;
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Navigate::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    // 如果是被中断（不是正常完成），取消导航
    if (Transition.CurrentRunStatus == EStateTreeRunStatus::Running)
    {
        AActor* Owner = Cast<AActor>(Context.GetOwner());
        if (UMASkillComponent* SkillComp = Owner ? Owner->FindComponentByClass<UMASkillComponent>() : nullptr)
        {
            FMASkillActivationUseCases::CancelCommand(*SkillComp, EMACommand::Navigate);
        }
    }
}
