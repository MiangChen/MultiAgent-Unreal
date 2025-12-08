// GA_Follow.cpp
// 追踪技能实现 - 使用 Ground Truth 位置持续跟随目标

#include "GA_Follow.h"
#include "../MAGameplayTags.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.h"
#include "AIController.h"
#include "TimerManager.h"

UGA_Follow::UGA_Follow()
{
    // 设置 Tag
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Navigate);  // 复用 Navigate tag
    SetAssetTags(AssetTags);
    
    // 追踪时添加 Moving 状态
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Moving);
    
    LastTargetLocation = FVector::ZeroVector;
}

void UGA_Follow::SetTargetCharacter(AMACharacter* InTargetCharacter)
{
    TargetCharacter = InTargetCharacter;
}

void UGA_Follow::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 检查目标是否有效
    if (!TargetCharacter.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Follow] No target character set"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 从 RobotDog 获取 ScanRadius 作为跟随距离
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character))
    {
        FollowDistance = Robot->ScanRadius;
        UE_LOG(LogTemp, Log, TEXT("[Follow] Using Robot ScanRadius as FollowDistance: %.0f"), FollowDistance);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[Follow] %s starts following %s"), 
        *Character->AgentName, *TargetCharacter->AgentName);
    
    // 显示头顶状态（持续显示，设置很长的时间）
    Character->ShowAbilityStatus(TEXT("Following"), 
        FString::Printf(TEXT("→ %s"), *TargetCharacter->AgentName));
    Character->ShowStatus(FString::Printf(TEXT("[Following] → %s"), *TargetCharacter->AgentName), 9999.f);
    
    // 立即执行一次
    UpdateFollow();
    
    // 启动定时器持续更新
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            UpdateTimerHandle,
            this,
            &UGA_Follow::UpdateFollow,
            UpdateInterval,
            true  // 循环
        );
    }
}

void UGA_Follow::UpdateFollow()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !TargetCharacter.IsValid())
    {
        // 目标丢失，结束追踪
        UE_LOG(LogTemp, Warning, TEXT("[Follow] Target lost, stopping follow"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // Ground Truth: 直接获取目标位置
    FVector CurrentTargetLocation = TargetCharacter->GetActorLocation();
    
    // 计算跟随位置
    FVector FollowLocation = CalculateFollowLocation();
    
    // 检查是否已经足够近（1.1 倍跟随距离内就清除显示）
    float DistanceToFollow = FVector::Dist(Character->GetActorLocation(), FollowLocation);
    if (DistanceToFollow < FollowDistance * 1.1f)
    {
        // 已经很近了，不需要移动，清除头顶状态
        Character->ShowStatus(TEXT(""), 0.f);
        Character->bIsMoving = false;
        return;
    }
    
    // 检查目标是否移动了足够距离（优化：避免频繁重新导航）
    float DistanceMoved = FVector::Dist(CurrentTargetLocation, LastTargetLocation);
    if (DistanceMoved < Repath_Threshold && Character->bIsMoving)
    {
        // 目标没怎么动，继续当前导航，但保持显示状态
        return;
    }
    
    // 需要移动，显示追踪状态
    Character->ShowStatus(FString::Printf(TEXT("[Following] → %s"), *TargetCharacter->AgentName), 9999.f);
    
    // 导航到跟随位置
    AAIController* AICtrl = Cast<AAIController>(Character->GetController());
    if (AICtrl)
    {
        EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(
            FollowLocation,
            50.f,   // AcceptanceRadius
            true,   // bStopOnOverlap
            true,   // bUsePathfinding
            true,   // bProjectDestinationToNavigation - 自动投影到 NavMesh
            true    // bCanStrafe
        );
        
        // 根据结果设置移动状态 (0 = Failed)
        if ((int32)Result == 0)
        {
            // 跟随位置导航失败，尝试直接导航到目标位置
            Result = AICtrl->MoveToLocation(
                CurrentTargetLocation,  // 直接去目标位置
                FollowDistance,         // 用跟随距离作为接受半径
                true,
                true,
                true,
                true
            );
            
            if ((int32)Result == 0)
            {
                // 还是失败，真的无法到达
                Character->bIsMoving = false;
                Character->ShowStatus(TEXT("[Follow] Cannot reach!"), 2.0f);
                UE_LOG(LogTemp, Warning, TEXT("[Follow] %s cannot reach %s - path failed"), 
                    *Character->AgentName, *TargetCharacter->AgentName);
            }
            else
            {
                // 直接导航到目标成功
                Character->bIsMoving = true;
                LastTargetLocation = CurrentTargetLocation;
            }
        }
        else
        {
            Character->bIsMoving = true;
            LastTargetLocation = CurrentTargetLocation;
        }
        
    }
}

FVector UGA_Follow::CalculateFollowLocation() const
{
    if (!TargetCharacter.IsValid())
    {
        return FVector::ZeroVector;
    }
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        return TargetCharacter->GetActorLocation();
    }
    
    FVector TargetLocation = TargetCharacter->GetActorLocation();
    FVector MyLocation = Character->GetActorLocation();
    
    // 计算方向：从目标指向自己
    FVector Direction = (MyLocation - TargetLocation).GetSafeNormal();
    
    // 如果方向为零（重叠），使用目标的反方向
    if (Direction.IsNearlyZero())
    {
        Direction = -TargetCharacter->GetActorForwardVector();
    }
    
    // 跟随位置 = 目标位置 + 方向 * 跟随距离
    FVector FollowLocation = TargetLocation + Direction * FollowDistance;
    FollowLocation.Z = TargetLocation.Z;  // 保持同一高度
    
    return FollowLocation;
}

void UGA_Follow::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 停止定时器
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
        
        // 停止移动
        Character->bIsMoving = false;
        if (AAIController* AICtrl = Cast<AAIController>(Character->GetController()))
        {
            AICtrl->StopMovement();
        }
        
        UE_LOG(LogTemp, Log, TEXT("[Follow] %s stopped following"), *Character->AgentName);
        
        // 清除头顶状态
        Character->ShowStatus(TEXT(""), 0.f);
    }
    
    TargetCharacter.Reset();
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
