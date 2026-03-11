// MASTTask_Place.cpp
// StateTree Place 任务

#include "MASTTask_Place.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Place::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_PlaceInstanceData& Data = Context.GetInstanceData<FMASTTask_PlaceInstanceData>(*this);
    Data.bSkillActivated = false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    // 检查搜索结果是否有效
    const FMASearchRuntimeResults& Results = SkillComp->GetSearchResults();
    if (!Results.Object1Actor.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    // 激活 Place 技能
    if (!FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::Place))
    {
        return EStateTreeRunStatus::Failed;
    }
    
    Data.bSkillActivated = true;
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_Place::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_PlaceInstanceData& Data = Context.GetInstanceData<FMASTTask_PlaceInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    // 检查命令是否被取消
    if (!SkillComp->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Place"))))
    {
        return EStateTreeRunStatus::Succeeded;
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Place::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_PlaceInstanceData& Data = Context.GetInstanceData<FMASTTask_PlaceInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return;
    
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
    }
    
    if (UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>())
    {
        if (Data.bSkillActivated)
        {
            FMASkillActivationUseCases::CancelCommand(*SkillComp, EMACommand::Place);
        }
        
        // 完成后切换到 Idle
        SkillComp->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Place")));
        SkillComp->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
    }
}
