// MASTTask_Coverage.cpp

#include "MASTTask_Coverage.h"
#include "MACharacter.h"
#include "MAAbilitySystemComponent.h"
#include "MACoverageArea.h"
#include "../Interface/MAAgentInterfaces.h"
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

    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] Owner is not AMACharacter"));
        return EStateTreeRunStatus::Failed;
    }

    // 使用 Interface 获取 CoverageArea 和 ScanRadius
    IMACoverable* Coverable = Cast<IMACoverable>(Owner);
    if (!Coverable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] Owner does not implement IMACoverable"));
        return EStateTreeRunStatus::Failed;
    }

    AMACoverageArea* CoverageArea = Cast<AMACoverageArea>(Coverable->GetCoverageArea());
    if (!CoverageArea)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Coverage] %s: No CoverageArea set"), *Character->ActorName);
        return EStateTreeRunStatus::Failed;
    }

    float ScanRadius = Coverable->GetScanRadius();

    // 从 Area 获取原始路径点
    TArray<FVector> RawPath = CoverageArea->GenerateCoveragePath(ScanRadius);
    
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

    const float ProjectionRadius = ScanRadius;
    const FVector ProjectionExtent(ProjectionRadius, ProjectionRadius, 500.f);
    int32 SkippedCount = 0;

    // 4个方向偏移（前后左右）
    const FVector Directions[4] = {
        FVector(1.f, 0.f, 0.f),
        FVector(-1.f, 0.f, 0.f),
        FVector(0.f, 1.f, 0.f),
        FVector(0.f, -1.f, 0.f)
    };

    for (const FVector& RawPoint : RawPath)
    {
        FNavLocation NavLoc;
        bool bFound = false;

        const float RadiusMultipliers[] = { 1.0f, 2.0f, 3.0f };
        
        for (float Multiplier : RadiusMultipliers)
        {
            if (bFound) break;
            
            float CurrentRadius = ProjectionRadius * Multiplier;
            FVector CurrentExtent(CurrentRadius, CurrentRadius, 500.f);

            if (NavSys->ProjectPointToNavigation(RawPoint, NavLoc, CurrentExtent))
            {
                Data.CoveragePath.Add(NavLoc.Location);
                bFound = true;
                break;
            }

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
        *Character->ActorName, Data.TotalPathPoints, SkippedCount, ScanRadius);

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
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 先检查 Command Tag
    if (UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent()))
    {
        if (!ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage"))))
        {
            return EStateTreeRunStatus::Succeeded;
        }
    }

    // 检查是否已完成所有点
    if (Data.bIsCompleted)
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Coverage] %s completed coverage!"), *Character->ActorName);
        
        if (UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent()))
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
    FVector CharacterLoc = Character->GetActorLocation();
    float Distance = FVector::Dist2D(CharacterLoc, CurrentTarget);

    // 使用 Interface 获取 ScanRadius
    float ActualAcceptRadius = 100.f;
    if (IMACoverable* Coverable = Cast<IMACoverable>(Owner))
    {
        ActualAcceptRadius = Coverable->GetScanRadius();
    }

    // 调试日志（减少频率）
    static int32 DebugCounter = 0;
    if (++DebugCounter % 30 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Coverage DEBUG] %s: Index=%d, Pos=(%.0f,%.0f), Target=(%.0f,%.0f), Dist=%.0f, Accept=%.0f"),
            *Character->ActorName, Data.CurrentPathIndex,
            CharacterLoc.X, CharacterLoc.Y, CurrentTarget.X, CurrentTarget.Y, Distance, ActualAcceptRadius);
    }

    // 检查是否到达当前目标点
    if (Distance <= ActualAcceptRadius)
    {
        Data.CurrentPathIndex++;
        Data.bIsMoving = false;

        UE_LOG(LogTemp, Warning, TEXT("[Coverage DEBUG] %s REACHED point [%d/%d], moving to next"),
            *Character->ActorName, Data.CurrentPathIndex, Data.TotalPathPoints);

        if (Data.CurrentPathIndex >= Data.TotalPathPoints)
        {
            Data.bIsCompleted = true;
            return EStateTreeRunStatus::Running;
        }

        MoveToNextPoint(Context, Data);
    }
    else if (!Data.bIsMoving || !Character->bIsMoving)
    {
        if (!MoveToNextPoint(Context, Data))
        {
            UE_LOG(LogTemp, Warning, TEXT("[Coverage] %s: No more reachable points, completing"),
                *Character->ActorName);
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
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        return false;
    }

    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (!ASC)
    {
        return false;
    }

    // 尝试移动到当前点，如果失败则跳过
    while (Data.CurrentPathIndex < Data.CoveragePath.Num())
    {
        FVector TargetLocation = Data.CoveragePath[Data.CurrentPathIndex];
        
        // 保持当前高度 (对 Drone 很重要)
        TargetLocation.Z = Character->GetActorLocation().Z;
        
        if (ASC->TryActivateNavigate(TargetLocation))
        {
            Data.bIsMoving = true;
            return true;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[Coverage] %s: Point %d unreachable, skipping"),
            *Character->ActorName, Data.CurrentPathIndex);
        Data.CurrentPathIndex++;
    }

    return false;
}