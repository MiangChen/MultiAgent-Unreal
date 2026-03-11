// MASTTask_Charge.cpp
// StateTree 充电任务 - 从 SkillComponent 获取能量信息

#include "MASTTask_Charge.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Application/MASkillExecutionUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "../../../Environment/Entity/MAChargingStation.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

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

    // 查找最近的充电站
    AMAChargingStation* Station = FindNearestChargingStation(Owner->GetWorld(), Character->GetActorLocation(), Data.StationLocation);
    if (!Station)
    {
        return EStateTreeRunStatus::Failed;
    }

    Data.bFoundStation = true;
    Data.ChargingStation = Station;
    Data.InteractionRadius = Station->InteractionRadius;

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

    if (FMASkillExecutionUseCases::HasCommandCompleted(*SkillComp, EMACommand::Charge))
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
        FMASkillExecutionUseCases::CancelCommandIfActivated(*SkillComp, EMACommand::Navigate, Data.bIsMoving);
    }
}

AMAChargingStation* FMASTTask_Charge::FindNearestChargingStation(UWorld* World, const FVector& FromLocation, FVector& OutStationLocation) const
{
    if (!World) return nullptr;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundActors);
    if (FoundActors.Num() == 0) return nullptr;

    float MinDistance = FLT_MAX;
    AMAChargingStation* NearestStation = nullptr;

    for (AActor* Actor : FoundActors)
    {
        AMAChargingStation* Station = Cast<AMAChargingStation>(Actor);
        if (!Station) continue;
        
        float Distance = FVector::Dist(FromLocation, Actor->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestStation = Station;
        }
    }

    if (NearestStation)
    {
        OutStationLocation = NearestStation->GetActorLocation();
        if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(OutStationLocation, NavLocation, FVector(500.f, 500.f, 200.f)))
            {
                OutStationLocation = NavLocation.Location;
            }
        }
    }

    return NearestStation;
}
