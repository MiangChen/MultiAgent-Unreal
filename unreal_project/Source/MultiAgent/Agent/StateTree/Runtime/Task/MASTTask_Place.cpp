// MASTTask_Place.cpp
// StateTree Place 任务

#include "MASTTask_Place.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
EStateTreeRunStatus ToPlaceTaskRunStatus(const EMAStateTreeTaskDecision Decision)
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

    const FMASearchRuntimeResults& Results = SkillComp->GetSearchResults();

    const bool bActivated =
        Results.Object1Actor.IsValid() &&
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComp, EMACommand::Place);
    const EMAStateTreeTaskDecision Decision =
        FMAStateTreeUseCases::BuildPlaceEnterDecision(Results.Object1Actor.IsValid(), bActivated);
    if (Decision == EMAStateTreeTaskDecision::Failed)
    {
        return EStateTreeRunStatus::Failed;
    }

    Data.bSkillActivated = bActivated;
    return ToPlaceTaskRunStatus(Decision);
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

    return ToPlaceTaskRunStatus(FMAStateTreeUseCases::BuildCommandTickDecision(
        true,
        FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Place)));
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
    
    const FMAStateTreeTaskExitFeedback Feedback =
        FMAStateTreeUseCases::BuildActivatedCommandExit(Data.bSkillActivated, true);
    if (Feedback.bShouldCancelCommand)
    {
        if (UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>())
        {
            FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Place, Data.bSkillActivated);
            if (Feedback.bShouldTransitionCommandToIdle)
            {
                FMASkillExecutionUseCases::TransitionCommandToIdle(*SkillComp, EMACommand::Place);
            }
        }
    }
}
