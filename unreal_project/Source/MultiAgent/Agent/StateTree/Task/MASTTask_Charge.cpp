// MASTTask_Charge.cpp

#include "MASTTask_Charge.h"
#include "MASTTaskUtils.h"
#include "../../Character/MACharacter.h"
#include "../../GAS/MAAbilitySystemComponent.h"
#include "../../../Environment/MAChargingStation.h"
#include "../../Interface/MAAgentInterfaces.h"
#include "../../Component/Capability/MACapabilityComponents.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameplayTagContainer.h"

using namespace MASTTaskUtils;

EStateTreeRunStatus FMASTTask_Charge::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_ChargeInstanceData& Data = Context.GetInstanceData<FMASTTask_ChargeInstanceData>(*this);
    Data.bFoundStation = false;
    Data.bIsMoving = false;
    Data.bIsCharging = false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] No owner actor"));
        return EStateTreeRunStatus::Failed;
    }

    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] Owner is not AMACharacter"));
        return EStateTreeRunStatus::Failed;
    }

    // 使用 Interface 检查是否支持充电 (支持 Component 模式)
    IMAChargeable* Chargeable = GetCapabilityInterface<IMAChargeable>(Owner);
    if (!Chargeable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] Owner does not have IMAChargeable capability"));
        return EStateTreeRunStatus::Failed;
    }

    // 查找最近的充电站
    AMAChargingStation* Station = FindNearestChargingStation(Owner->GetWorld(), Character->GetActorLocation(), Data.StationLocation);
    if (!Station)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] %s: No ChargingStation found"), *Character->AgentName);
        return EStateTreeRunStatus::Failed;
    }

    Data.bFoundStation = true;
    Data.ChargingStation = Station;
    Data.InteractionRadius = Station->InteractionRadius;
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s found charging station at (%.0f, %.0f, %.0f), radius=%.0f"),
        *Character->AgentName, Data.StationLocation.X, Data.StationLocation.Y, Data.StationLocation.Z, Data.InteractionRadius);

    // 检查是否已经在充电站范围内
    float Distance = FVector::Dist(Character->GetActorLocation(), Data.StationLocation);
    if (Distance <= Data.InteractionRadius)
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s already at charging station, starting charge"), *Character->AgentName);
        Data.bIsCharging = true;
        
        // 调用 Character 的 TryCharge (如果有)
        if (UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent()))
        {
            ASC->TryActivateCharge();
        }
        return EStateTreeRunStatus::Running;
    }

    // 导航到充电站
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (ASC && ASC->TryActivateNavigate(Data.StationLocation))
    {
        Data.bIsMoving = true;
        UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s navigating to charging station"), *Character->AgentName);
        return EStateTreeRunStatus::Running;
    }

    UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] Failed to start navigation"));
    return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FMASTTask_Charge::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_ChargeInstanceData& Data = Context.GetInstanceData<FMASTTask_ChargeInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        return EStateTreeRunStatus::Failed;
    }

    IMAChargeable* Chargeable = GetCapabilityInterface<IMAChargeable>(Owner);
    if (!Chargeable)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 检查 Command.Charge Tag 是否还存在
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (ASC && !ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge"))))
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s: Command.Charge removed, exiting charge"), *Character->AgentName);
        return EStateTreeRunStatus::Succeeded;
    }

    // 如果正在充电
    if (Data.bIsCharging)
    {
        // 检查是否充满
        if (Chargeable->GetEnergyPercent() >= 100.f)
        {
            UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s fully charged!"), *Character->AgentName);
            
            if (ASC)
            {
                ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
                ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
            }
            
            return EStateTreeRunStatus::Succeeded;
        }
        return EStateTreeRunStatus::Running;
    }

    // 如果正在移动
    if (Data.bIsMoving)
    {
        float Distance = FVector::Dist(Character->GetActorLocation(), Data.StationLocation);
        
        // 到达充电站
        if (Distance <= Data.InteractionRadius)
        {
            UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s arrived at charging station, starting charge"), *Character->AgentName);
            Data.bIsMoving = false;
            Data.bIsCharging = true;
            
            if (ASC)
            {
                ASC->TryActivateCharge();
            }
            return EStateTreeRunStatus::Running;
        }

        // 检查移动是否失败
        if (!Character->bIsMoving)
        {
            UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] %s stopped moving but not at station (dist=%.0f)"), 
                *Character->AgentName, Distance);
            
            // 重试导航
            if (ASC)
            {
                ASC->TryActivateNavigate(Data.StationLocation);
            }
        }
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Charge::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (Owner)
    {
        if (UMAAbilitySystemComponent* ASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>())
        {
            ASC->CancelNavigate();
            
            FGameplayTag IdleTag = FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
            FGameplayTag FollowTag = FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
            FGameplayTag ChargeTag = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
            FGameplayTag CoverageTag = FGameplayTag::RequestGameplayTag(FName("Command.Coverage"));
            FGameplayTag PatrolTag = FGameplayTag::RequestGameplayTag(FName("Command.Patrol"));
            
            bool bHasOtherCommand = ASC->HasMatchingGameplayTag(FollowTag) ||
                                    ASC->HasMatchingGameplayTag(ChargeTag) ||
                                    ASC->HasMatchingGameplayTag(CoverageTag) ||
                                    ASC->HasMatchingGameplayTag(PatrolTag);
            
            if (!bHasOtherCommand && !ASC->HasMatchingGameplayTag(IdleTag))
            {
                ASC->AddLooseGameplayTag(IdleTag);
                UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] Added Command.Idle for state transition"));
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] Charge ended"));
}

AMAChargingStation* FMASTTask_Charge::FindNearestChargingStation(UWorld* World, const FVector& FromLocation, FVector& OutStationLocation) const
{
    if (!World) return nullptr;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundActors);

    if (FoundActors.Num() == 0)
    {
        return nullptr;
    }

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
        FVector StationLoc = NearestStation->GetActorLocation();
        
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(StationLoc, NavLocation, FVector(500.f, 500.f, 200.f)))
            {
                OutStationLocation = NavLocation.Location;
                return NearestStation;
            }
        }
        
        OutStationLocation = StationLoc;
        return NearestStation;
    }

    return nullptr;
}