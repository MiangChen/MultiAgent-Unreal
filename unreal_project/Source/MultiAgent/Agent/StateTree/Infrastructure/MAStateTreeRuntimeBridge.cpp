#include "Agent/StateTree/Infrastructure/MAStateTreeRuntimeBridge.h"

#include "Agent/StateTree/Runtime/MAStateTreeComponent.h"
#include "Environment/Entity/MAChargingStation.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

void FMAStateTreeRuntimeBridge::RestartLogicNextTick(UMAStateTreeComponent& StateTreeComponent)
{
    if (UWorld* World = StateTreeComponent.GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick([&StateTreeComponent]()
        {
            StateTreeComponent.StopLogic(TEXT("Resetting for new StateTree"));
            StateTreeComponent.StartLogic();
        });
    }
}

FMAStateTreeChargeStationFeedback FMAStateTreeRuntimeBridge::ResolveNearestChargingStation(
    const AActor* WorldContext,
    const FVector& FromLocation)
{
    FMAStateTreeChargeStationFeedback Feedback;

    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    if (!World)
    {
        return Feedback;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundActors);
    if (FoundActors.IsEmpty())
    {
        return Feedback;
    }

    float MinDistance = FLT_MAX;
    AMAChargingStation* NearestStation = nullptr;
    for (AActor* Actor : FoundActors)
    {
        AMAChargingStation* Station = Cast<AMAChargingStation>(Actor);
        if (!Station)
        {
            continue;
        }

        const float Distance = FVector::Dist(FromLocation, Station->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestStation = Station;
        }
    }

    if (!NearestStation)
    {
        return Feedback;
    }

    Feedback.bFoundStation = true;
    Feedback.ChargingStation = NearestStation;
    Feedback.InteractionRadius = NearestStation->InteractionRadius;
    Feedback.StationLocation = NearestStation->GetActorLocation();

    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation NavLocation;
        if (NavSys->ProjectPointToNavigation(Feedback.StationLocation, NavLocation, FVector(500.f, 500.f, 200.f)))
        {
            Feedback.StationLocation = NavLocation.Location;
        }
    }

    return Feedback;
}
