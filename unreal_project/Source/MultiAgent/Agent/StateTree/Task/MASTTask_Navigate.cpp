// MASTTask_Navigate.cpp

#include "MASTTask_Navigate.h"
#include "MAAbilitySystemComponent.h"
#include "MACharacter.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

EStateTreeRunStatus FMASTTask_Navigate::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    // 获取实例数据
    FMASTTask_NavigateInstanceData& InstanceData = Context.GetInstanceData<FMASTTask_NavigateInstanceData>(*this);
    InstanceData.bNavigationStarted = false;
    InstanceData.bNavigationCompleted = false;
    InstanceData.bNavigationSucceeded = false;

    // 获取 Owner Actor
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Navigate] No owner actor"));
        return EStateTreeRunStatus::Failed;
    }

    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Navigate] Owner is not AMACharacter"));
        return EStateTreeRunStatus::Failed;
    }

    // 获取 ASC
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Navigate] No ASC"));
        return EStateTreeRunStatus::Failed;
    }

    // 调整目标位置的 Z 坐标
    FVector AdjustedTarget = TargetLocation;
    AdjustedTarget.Z = Character->GetActorLocation().Z;

    UE_LOG(LogTemp, Log, TEXT("[STTask_Navigate] %s navigating to (%.0f, %.0f, %.0f)"),
        *Character->AgentName, AdjustedTarget.X, AdjustedTarget.Y, AdjustedTarget.Z);

    // 激活导航
    if (ASC->TryActivateNavigate(AdjustedTarget))
    {
        InstanceData.bNavigationStarted = true;
        return EStateTreeRunStatus::Running;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Navigate] Failed to activate navigate"));
        return EStateTreeRunStatus::Failed;
    }
}

EStateTreeRunStatus FMASTTask_Navigate::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    FMASTTask_NavigateInstanceData& InstanceData = Context.GetInstanceData<FMASTTask_NavigateInstanceData>(*this);
    
    if (!InstanceData.bNavigationStarted)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 获取 Owner
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        return EStateTreeRunStatus::Failed;
    }

    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 检查是否到达目标
    FVector CurrentLocation = Character->GetActorLocation();
    float Distance = FVector::Dist2D(CurrentLocation, TargetLocation);
    
    if (Distance <= AcceptanceRadius)
    {
        UE_LOG(LogTemp, Log, TEXT("[STTask_Navigate] %s reached destination, Distance=%.1f"),
            *Character->AgentName, Distance);
        InstanceData.bNavigationCompleted = true;
        InstanceData.bNavigationSucceeded = true;
        return EStateTreeRunStatus::Succeeded;
    }

    // 检查 Navigate Ability 是否还在运行
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (ASC)
    {
        // 检查是否有 Moving 状态
        if (!Character->bIsMoving)
        {
            // 移动已停止但未到达目标，可能是路径失败
            UE_LOG(LogTemp, Warning, TEXT("[STTask_Navigate] %s stopped moving but not at destination"),
                *Character->AgentName);
            InstanceData.bNavigationCompleted = true;
            InstanceData.bNavigationSucceeded = false;
            return EStateTreeRunStatus::Failed;
        }
    }

    return EStateTreeRunStatus::Running;
}

void FMASTTask_Navigate::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    // 如果状态被中断，取消导航
    if (Transition.CurrentRunStatus == EStateTreeRunStatus::Running)
    {
        AActor* Owner = Cast<AActor>(Context.GetOwner());
        if (Owner)
        {
            if (UMAAbilitySystemComponent* ASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>())
            {
                UE_LOG(LogTemp, Log, TEXT("[STTask_Navigate] Cancelling navigation due to state exit"));
                ASC->CancelNavigate();
            }
        }
    }
}
