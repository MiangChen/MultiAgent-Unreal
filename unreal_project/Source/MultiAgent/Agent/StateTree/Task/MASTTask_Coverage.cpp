// MASTTask_Coverage.cpp

#include "MASTTask_Coverage.h"
#include "MARobotDogCharacter.h"
#include "MAAbilitySystemComponent.h"
#include "MACoverageArea.h"
#include "StateTreeExecutionContext.h"
#include "GameplayTagContainer.h"
#include "NavigationSystem.h"

EStateTreeRunStatus FMASTTask_Coverage::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_CoverageInstanceData& Data = Context.GetInstanceData<FMASTTask_CoverageInstanceData>(*this);
    Data.CurrentPathIndex = 0;
    Data.bIsMoving = false;
    Data.bIsCompleted = false;
    Data.CoveragePath.Empty();

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] No owner actor"));
        return EStateTreeRunStatus::Failed;
    }

    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] Owner is not RobotDog"));
        return EStateTreeRunStatus::Failed;
    }

    // 从 Robot 获取覆盖区域
    AMACoverageArea* CoverageArea = Cast<AMACoverageArea>(Robot->GetCoverageArea());
    if (!CoverageArea)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] %s: No CoverageArea set"), *Robot->ActorName);
        return EStateTreeRunStatus::Failed;
    }

    // 从 Area 获取原始路径点
    TArray<FVector> RawPath = CoverageArea->GenerateCoveragePath(Robot->ScanRadius);
    
    if (RawPath.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] Coverage path has less than 2 points"));
        return EStateTreeRunStatus::Failed;
    }

    // NavMesh 投影：确保所有点都在可导航区域
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Owner->GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] No NavigationSystem found"));
        return EStateTreeRunStatus::Failed;
    }

    const float ProjectionRadius = Robot->ScanRadius;
    const FVector ProjectionExtent(ProjectionRadius, ProjectionRadius, 500.f);  // Z轴大范围，适应高低地形
    int32 SkippedCount = 0;

    // 4个方向偏移（前后左右）
    const FVector Directions[4] = {
        FVector(1.f, 0.f, 0.f),   // +X
        FVector(-1.f, 0.f, 0.f),  // -X
        FVector(0.f, 1.f, 0.f),   // +Y
        FVector(0.f, -1.f, 0.f)   // -Y
    };

    for (const FVector& RawPoint : RawPath)
    {
        FNavLocation NavLoc;
        bool bFound = false;

        // 多轮尝试，逐步扩大搜索半径
        const float RadiusMultipliers[] = { 1.0f, 2.0f, 3.0f };
        
        for (float Multiplier : RadiusMultipliers)
        {
            if (bFound) break;
            
            float CurrentRadius = ProjectionRadius * Multiplier;
            FVector CurrentExtent(CurrentRadius, CurrentRadius, 500.f);

            // 首先尝试直接投影
            if (NavSys->ProjectPointToNavigation(RawPoint, NavLoc, CurrentExtent))
            {
                Data.CoveragePath.Add(NavLoc.Location);
                bFound = true;
                break;
            }

            // 投影失败，尝试4个方向偏移
            for (const FVector& Dir : Directions)
            {
                FVector OffsetPoint = RawPoint + Dir * CurrentRadius;
                if (NavSys->ProjectPointToNavigation(OffsetPoint, NavLoc, CurrentExtent))
                {
                    Data.CoveragePath.Add(NavLoc.Location);
                    bFound = true;
                    break;
                }
            }
        }

        if (!bFound)
        {
            SkippedCount++;
            UE_LOG(LogTemp, Warning, TEXT("[Coverage] Point (%.0f, %.0f) unreachable after all attempts"),
                RawPoint.X, RawPoint.Y);
        }
    }

    Data.TotalPathPoints = Data.CoveragePath.Num();

    UE_LOG(LogTemp, Log, TEXT("[Coverage] %s: Generated %d navigable points (skipped %d), ScanRadius=%.0f"),
        *Robot->ActorName, Data.TotalPathPoints, SkippedCount, Robot->ScanRadius);

    if (Data.TotalPathPoints < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] Not enough navigable points"));
        return EStateTreeRunStatus::Failed;
    }

    // 保存 CoverageArea 引用并设置可视化路径
    Data.CoverageAreaRef = CoverageArea;
    CoverageArea->SetActivePath(Data.CoveragePath);

    // 开始移动到第一个点
    if (!MoveToNextPoint(Context, Data))
    {
        return EStateTreeRunStatus::Failed;
    }

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_Coverage::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_CoverageInstanceData& Data = Context.GetInstanceData<FMASTTask_CoverageInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 先检查 Command Tag，如果没有 Coverage 命令则退出
    if (UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent()))
    {
        if (!ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage"))))
        {
            return EStateTreeRunStatus::Succeeded;
        }
    }

    // 检查是否已完成所有点
    if (Data.bIsCompleted)
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Coverage] %s completed coverage!"), *Robot->ActorName);
        
        // 清除命令 Tag
        if (UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent()))
        {
            ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
            ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        }
        
        return EStateTreeRunStatus::Succeeded;
    }

    // 检查当前路径点
    if (Data.CurrentPathIndex >= Data.CoveragePath.Num())
    {
        Data.bIsCompleted = true;
        return EStateTreeRunStatus::Running;
    }

    FVector CurrentTarget = Data.CoveragePath[Data.CurrentPathIndex];
    FVector RobotLoc = Robot->GetActorLocation();
    float Distance = FVector::Dist2D(RobotLoc, CurrentTarget);

    // 调试日志（减少频率）
    static int32 DebugCounter = 0;
    if (++DebugCounter % 30 == 0)  // 每30帧打印一次
    {
        UE_LOG(LogTemp, Warning, TEXT("[Coverage DEBUG] %s: Index=%d, Pos=(%.0f,%.0f), Target=(%.0f,%.0f), Dist=%.0f, Accept=%.0f"),
            *Robot->ActorName, Data.CurrentPathIndex,
            RobotLoc.X, RobotLoc.Y, CurrentTarget.X, CurrentTarget.Y, Distance, Robot->ScanRadius);
    }

    // 使用机器人的 ScanRadius 作为到达判定（进入扫描范围即可）
    float ActualAcceptRadius = Robot->ScanRadius;
    
    // 检查是否到达当前目标点
    if (Distance <= ActualAcceptRadius)
    {
        // 到达当前点，推进到下一个
        Data.CurrentPathIndex++;
        Data.bIsMoving = false;

        UE_LOG(LogTemp, Warning, TEXT("[Coverage DEBUG] %s REACHED point [%d/%d], moving to next"),
            *Robot->ActorName, Data.CurrentPathIndex, Data.TotalPathPoints);

        // 检查是否完成
        if (Data.CurrentPathIndex >= Data.TotalPathPoints)
        {
            Data.bIsCompleted = true;
            return EStateTreeRunStatus::Running;
        }

        // 移动到下一个点
        MoveToNextPoint(Context, Data);
    }
    else if (!Data.bIsMoving || !Robot->bIsMoving)
    {
        // 未在移动状态，启动移动
        if (!MoveToNextPoint(Context, Data))
        {
            // 所有剩余点都无法到达，完成任务
            UE_LOG(LogTemp, Warning, TEXT("[Coverage] %s: No more reachable points, completing"),
                *Robot->ActorName);
            Data.bIsCompleted = true;
        }
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Coverage::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_CoverageInstanceData& Data = Context.GetInstanceData<FMASTTask_CoverageInstanceData>(*this);

    // 清除可视化路径
    if (Data.CoverageAreaRef.IsValid())
    {
        Data.CoverageAreaRef->ClearActivePath();
    }

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
                UE_LOG(LogTemp, Log, TEXT("[STTask_Coverage] Added Command.Idle for state transition"));
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Coverage] Coverage ended"));
}

bool FMASTTask_Coverage::MoveToNextPoint(
    FStateTreeExecutionContext& Context,
    FMASTTask_CoverageInstanceData& Data) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        return false;
    }

    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent());
    if (!ASC)
    {
        return false;
    }

    // 尝试移动到当前点，如果失败则跳过
    while (Data.CurrentPathIndex < Data.CoveragePath.Num())
    {
        FVector TargetLocation = Data.CoveragePath[Data.CurrentPathIndex];
        
        if (ASC->TryActivateNavigate(TargetLocation))
        {
            Data.bIsMoving = true;
            return true;
        }
        
        // 导航失败，跳过这个点
        UE_LOG(LogTemp, Warning, TEXT("[Coverage] %s: Point %d unreachable, skipping"),
            *Robot->ActorName, Data.CurrentPathIndex);
        Data.CurrentPathIndex++;
    }

    // 所有点都无法到达
    return false;
}
