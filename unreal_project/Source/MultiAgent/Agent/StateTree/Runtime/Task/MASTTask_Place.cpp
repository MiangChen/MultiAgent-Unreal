// MASTTask_Place.cpp
// StateTree Place 任务

#include "MASTTask_Place.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Place::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_PlaceInstanceData& Data = Context.GetInstanceData<FMASTTask_PlaceInstanceData>(*this);
    Data.bSkillActivated = false;

    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
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
    return MASTTaskUtils::ToRunStatus(Decision);
}

EStateTreeRunStatus FMASTTask_Place::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_PlaceInstanceData& Data = Context.GetInstanceData<FMASTTask_PlaceInstanceData>(*this);

    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    return MASTTaskUtils::BuildCommandTickStatus(*SkillComp, EMACommand::Place);
}

void FMASTTask_Place::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_PlaceInstanceData& Data = Context.GetInstanceData<FMASTTask_PlaceInstanceData>(*this);

    MASTTaskUtils::ClearOwnerStatus(Context);
    MASTTaskUtils::HandleActivatedCommandExit(Context, Data.bSkillActivated, EMACommand::Place, true);
}
