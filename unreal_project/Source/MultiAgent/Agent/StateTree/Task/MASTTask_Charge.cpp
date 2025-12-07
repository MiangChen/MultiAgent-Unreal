// MASTTask_Charge.cpp

#include "MASTTask_Charge.h"
#include "../../Character/MARobotDogCharacter.h"
#include "../../GAS/MAAbilitySystemComponent.h"
#include "MAChargingStation.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameplayTagContainer.h"

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

    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] Owner is not RobotDog"));
        return EStateTreeRunStatus::Failed;
    }

    // 查找最近的充电站
    AMAChargingStation* Station = FindNearestChargingStation(Owner->GetWorld(), Robot->GetActorLocation(), Data.StationLocation);
    if (!Station)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] %s: No ChargingStation found"), *Robot->ActorName);
        return EStateTreeRunStatus::Failed;
    }

    Data.bFoundStation = true;
    Data.ChargingStation = Station;
    Data.InteractionRadius = Station->InteractionRadius;
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s found charging station at (%.0f, %.0f, %.0f), radius=%.0f"),
        *Robot->ActorName, Data.StationLocation.X, Data.StationLocation.Y, Data.StationLocation.Z, Data.InteractionRadius);

    // 检查是否已经在充电站范围内
    float Distance = FVector::Dist(Robot->GetActorLocation(), Data.StationLocation);
    if (Distance <= Data.InteractionRadius)
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s already at charging station, starting charge"), *Robot->ActorName);
        Data.bIsCharging = true;
        Robot->TryCharge();
        return EStateTreeRunStatus::Running;
    }

    // 导航到充电站
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent());
    if (ASC && ASC->TryActivateNavigate(Data.StationLocation))
    {
        Data.bIsMoving = true;
        UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s navigating to charging station"), *Robot->ActorName);
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
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 检查 Command.Charge Tag 是否还存在（命令可能被其他命令覆盖）
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent());
    if (ASC && !ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge"))))
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s: Command.Charge removed, exiting charge"), *Robot->ActorName);
        return EStateTreeRunStatus::Succeeded;
    }

    // 如果正在充电
    if (Data.bIsCharging)
    {
        // 检查是否充满
        if (Robot->GetEnergyPercent() >= 1.0f)
        {
            UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s fully charged!"), *Robot->ActorName);
            
            // 清除 Command.Charge Tag，防止重复进入 Charge 状态
            if (UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent()))
            {
                ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
                // 添加 Idle 命令，让机器人回到 Idle 状态
                ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
            }
            
            return EStateTreeRunStatus::Succeeded;
        }
        return EStateTreeRunStatus::Running;
    }

    // 如果正在移动
    if (Data.bIsMoving)
    {
        float Distance = FVector::Dist(Robot->GetActorLocation(), Data.StationLocation);
        
        // 到达充电站
        if (Distance <= Data.InteractionRadius)
        {
            UE_LOG(LogTemp, Log, TEXT("[STTask_Charge] %s arrived at charging station, starting charge"), *Robot->ActorName);
            Data.bIsMoving = false;
            Data.bIsCharging = true;
            Robot->TryCharge();
            return EStateTreeRunStatus::Running;
        }

        // 检查移动是否失败
        if (!Robot->bIsMoving)
        {
            UE_LOG(LogTemp, Warning, TEXT("[STTask_Charge] %s stopped moving but not at station (dist=%.0f)"), 
                *Robot->ActorName, Distance);
            
            // 重试导航
            UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent());
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
            
            // 如果没有其他命令，设置 Idle 命令以便 StateTree 能正确转换
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

    // 直接查找 ChargingStation 类
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundActors);

    if (FoundActors.Num() == 0)
    {
        return nullptr;
    }

    // 找最近的
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
        
        // 将目标位置投影到 NavMesh 上
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        if (NavSys)
        {
            FNavLocation NavLocation;
            // 在充电站周围 500 单位范围内查找可导航点
            if (NavSys->ProjectPointToNavigation(StationLoc, NavLocation, FVector(500.f, 500.f, 200.f)))
            {
                OutStationLocation = NavLocation.Location;
                return NearestStation;
            }
        }
        
        // 如果投影失败，使用原始位置
        OutStationLocation = StationLoc;
        return NearestStation;
    }

    return nullptr;
}
