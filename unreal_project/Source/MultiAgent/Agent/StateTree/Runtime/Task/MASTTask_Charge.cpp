// MASTTask_Charge.cpp
// StateTree 充电任务 - 从 SkillComponent 获取能量信息

#include "MASTTask_Charge.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/StateTree/Infrastructure/MAStateTreeRuntimeBridge.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "MASTTaskUtils.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Charge::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_ChargeInstanceData& Data = Context.GetInstanceData<FMASTTask_ChargeInstanceData>(*this);
    Data.bFoundStation = false;
    Data.bIsMoving = false;
    Data.bIsCharging = false;

    AMACharacter* Character = MASTTaskUtils::ResolveCharacter(Context);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMAStateTreeChargeStationFeedback StationFeedback =
        FMAStateTreeRuntimeBridge::ResolveNearestChargingStation(Character, Character->GetActorLocation());
    if (!StationFeedback.bFoundStation)
    {
        return EStateTreeRunStatus::Failed;
    }

    Data.bFoundStation = true;
    Data.ChargingStation = StationFeedback.ChargingStation;
    Data.StationLocation = StationFeedback.StationLocation;
    Data.InteractionRadius = StationFeedback.InteractionRadius;

    return FMASkillExecutionUseCases::StartChargeFlow(
        *SkillComp,
        Character->GetActorLocation(),
        Data.StationLocation,
        Data.InteractionRadius,
        Data.bIsMoving,
        Data.bIsCharging);
}

EStateTreeRunStatus FMASTTask_Charge::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_ChargeInstanceData& Data = Context.GetInstanceData<FMASTTask_ChargeInstanceData>(*this);

    AMACharacter* Character = MASTTaskUtils::ResolveCharacter(Context);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context);
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const EMAStateTreeTaskDecision Decision =
        FMAStateTreeUseCases::BuildCommandTickDecision(
            true,
            FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Charge));
    if (Decision == EMAStateTreeTaskDecision::Succeeded)
    {
        return EStateTreeRunStatus::Succeeded;
    }

    return FMASkillExecutionUseCases::TickChargeFlow(
        *SkillComp,
        Character->GetActorLocation(),
        Character->bIsMoving,
        Data.StationLocation,
        Data.InteractionRadius,
        Data.bIsMoving,
        Data.bIsCharging);
}

void FMASTTask_Charge::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_ChargeInstanceData& Data = Context.GetInstanceData<FMASTTask_ChargeInstanceData>(*this);

    MASTTaskUtils::ClearOwnerStatus(Context);

    if (UMASkillComponent* SkillComp = MASTTaskUtils::ResolveSkillComponent(Context))
    {
        const FMAStateTreeChargeExitFeedback Feedback =
            FMAStateTreeUseCases::BuildChargeExitFeedback(Data.bIsMoving, Data.bIsCharging);
        if (Feedback.bShouldCancelNavigate)
        {
            FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Navigate, Data.bIsMoving);
        }
        if (Feedback.bShouldCancelCharge)
        {
            FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Charge, Data.bIsCharging);
        }
    }
}
