// MASTTask_Patrol.cpp

#include "MASTTask_Patrol.h"
#include "../../GAS/MAAbilitySystemComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MARobotDogCharacter.h"
#include "../../Actor/MAPatrolPath.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_Patrol::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_PatrolInstanceData& Data = Context.GetInstanceData<FMASTTask_PatrolInstanceData>(*this);
    
    // 重置状态
    Data.CurrentWaypointIndex = 0;
    Data.bIsMoving = false;
    Data.bIsWaiting = false;
    Data.WaitTimer = 0.f;
    Data.Waypoints.Empty();

    // 获取 Robot
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] Owner is not RobotDog"));
        return EStateTreeRunStatus::Failed;
    }

    // 从 Robot 获取 PatrolPath
    AMAPatrolPath* FoundPatrolPath = Robot->GetPatrolPath();
    if (!FoundPatrolPath || !FoundPatrolPath->IsValidPath())
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] %s: No PatrolPath set"), *Robot->ActorName);
        return EStateTreeRunStatus::Failed;
    }

    // 获取路径点
    Data.Waypoints = FoundPatrolPath->GetWaypoints();
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] Starting patrol with %d waypoints"), Data.Waypoints.Num());

    // 开始移动到第一个点
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_Patrol::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    FMASTTask_PatrolInstanceData& Data = Context.GetInstanceData<FMASTTask_PatrolInstanceData>(*this);

    // 获取 Character
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] No Character!"));
        return EStateTreeRunStatus::Failed;
    }

    // 获取 ASC
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] %s has no ASC!"), *Character->ActorName);
        return EStateTreeRunStatus::Failed;
    }

    // 检查 Command.Patrol Tag 是否还存在（命令可能被其他命令覆盖）
    if (!ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol"))))
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] %s: Command.Patrol removed, exiting patrol"), *Character->ActorName);
        return EStateTreeRunStatus::Succeeded;
    }

    // 检查能量 (RobotDog)
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character))
    {
        if (!Robot->HasEnergy())
        {
            UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] %s ran out of energy"), *Character->ActorName);
            return EStateTreeRunStatus::Failed;
        }
    }

    // 调试：每秒输出一次状态
    static float DebugTimer = 0.f;
    DebugTimer += DeltaTime;
    if (DebugTimer >= 1.0f)
    {
        DebugTimer = 0.f;
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] %s - WP[%d/%d], bIsMoving=%d, bIsWaiting=%d, CharMoving=%d"), 
            *Character->ActorName, 
            Data.CurrentWaypointIndex + 1, Data.Waypoints.Num(),
            Data.bIsMoving ? 1 : 0, 
            Data.bIsWaiting ? 1 : 0,
            Character->bIsMoving ? 1 : 0);
    }

    // 等待状态
    if (Data.bIsWaiting)
    {
        Data.WaitTimer -= DeltaTime;
        if (Data.WaitTimer <= 0.f)
        {
            Data.bIsWaiting = false;
            // 移动到下一个点
            Data.CurrentWaypointIndex = (Data.CurrentWaypointIndex + 1) % Data.Waypoints.Num();
        }
        else
        {
            return EStateTreeRunStatus::Running;
        }
    }

    // 移动状态
    if (Data.bIsMoving)
    {
        // 检查是否到达
        FVector CurrentLocation = Character->GetActorLocation();
        FVector TargetLocation = Data.Waypoints[Data.CurrentWaypointIndex];
        float Distance = FVector::Dist2D(CurrentLocation, TargetLocation);

        // 从 Robot 获取 ScanRadius 作为到达判定
        AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character);
        float ActualAcceptRadius = Robot ? Robot->ScanRadius : 100.f;

        // 移动完成检测: Character->bIsMoving 由 GA_Navigate 在完成时设为 false
        // 当 GA_Navigate 成功完成 (包括 AlreadyAtGoal)，我们认为已到达
        if (!Character->bIsMoving)
        {
            // 移动已停止，认为到达了目标点
            UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] %s reached WP[%d/%d] (Distance=%.1f, move stopped)"), 
                *Character->ActorName, Data.CurrentWaypointIndex + 1, Data.Waypoints.Num(), Distance);
            
            Data.bIsMoving = false;
            Data.bIsWaiting = true;
            Data.WaitTimer = 0.5f;  // 固定等待时间
            
            // 显示状态
            Character->ShowAbilityStatus(TEXT("Patrol"), TEXT("Waiting..."));
        }
        else if (Distance <= ActualAcceptRadius)
        {
            // 距离检查也通过 (备用检测)
            UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] %s reached WP[%d/%d] (Distance=%.1f)"), 
                *Character->ActorName, Data.CurrentWaypointIndex + 1, Data.Waypoints.Num(), Distance);
            
            Data.bIsMoving = false;
            Data.bIsWaiting = true;
            Data.WaitTimer = 0.5f;  // 固定等待时间
            
            Character->ShowAbilityStatus(TEXT("Patrol"), TEXT("Waiting..."));
        }
        
        return EStateTreeRunStatus::Running;
    }

    // 开始移动到当前路径点
    if (Data.CurrentWaypointIndex < Data.Waypoints.Num())
    {
        FVector TargetLocation = Data.Waypoints[Data.CurrentWaypointIndex];
        TargetLocation.Z = Character->GetActorLocation().Z;

        UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] %s moving to WP[%d/%d] (%.0f, %.0f)"), 
            *Character->ActorName, Data.CurrentWaypointIndex + 1, Data.Waypoints.Num(),
            TargetLocation.X, TargetLocation.Y);

        // 显示状态
        Character->ShowAbilityStatus(TEXT("Patrol"), 
            FString::Printf(TEXT("WP %d/%d"), Data.CurrentWaypointIndex + 1, Data.Waypoints.Num()));

        if (ASC->TryActivateNavigate(TargetLocation))
        {
            Data.bIsMoving = true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[STTask_Patrol] Failed to activate navigate, skipping waypoint"));
            Data.CurrentWaypointIndex = (Data.CurrentWaypointIndex + 1) % Data.Waypoints.Num();
        }
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Patrol::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (Owner)
    {
        if (AMACharacter* Character = Cast<AMACharacter>(Owner))
        {
            Character->ShowStatus(TEXT(""), 0.f);
        }
        
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
                UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] Added Command.Idle for state transition"));
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Patrol] Patrol ended"));
}
