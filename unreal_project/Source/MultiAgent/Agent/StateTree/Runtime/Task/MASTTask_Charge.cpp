// MASTTask_Charge.cpp
// StateTree 充电任务 - 从 SkillComponent 获取能量信息

#include "MASTTask_Charge.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/StateTree/Infrastructure/MAStateTreeRuntimeBridge.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Charge::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_ChargeInstanceData& Data = Context.GetInstanceData<FMASTTask_ChargeInstanceData>(*this);
    Data.bFoundStation = false;
    Data.bIsMoving = false;
    Data.bIsCharging = false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    const FMAStateTreeChargeStationFeedback StationFeedback =
        FMAStateTreeRuntimeBridge::ResolveNearestChargingStation(Owner, Character->GetActorLocation());
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

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
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

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return;
    
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
    }
    
    if (UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>())
    {
        const FMAStateTreeTaskExitFeedback Feedback =
            FMAStateTreeUseCases::BuildActivatedCommandExit(Data.bIsMoving, false);
        if (Feedback.bShouldCancelCommand)
        {
            FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Navigate, Data.bIsMoving);
        }
    }
}
