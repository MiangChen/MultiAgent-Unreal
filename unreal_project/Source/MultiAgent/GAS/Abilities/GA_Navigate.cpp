// GA_Navigate.cpp
// 异步导航技能实现

#include "GA_Navigate.h"
#include "../MAGameplayTags.h"
#include "../MAAbilitySystemComponent.h"
#include "../../AgentManager/MAAgent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

UGA_Navigate::UGA_Navigate()
{
    // AssetTags 用于资产分类
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Navigate);
    SetAssetTags(AssetTags);
    
    // 导航时添加 Moving 状态
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Moving);
    
    TargetLocation = FVector::ZeroVector;
}

void UGA_Navigate::SetTargetLocation(FVector InTargetLocation)
{
    TargetLocation = InTargetLocation;
}

bool UGA_Navigate::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    
    // 检查是否有有效的 AI Controller
    AMAAgent* Agent = const_cast<UGA_Navigate*>(this)->GetOwningAgent();
    if (!Agent) return false;
    
    AAIController* AICtrl = Cast<AAIController>(Agent->GetController());
    return AICtrl != nullptr;
}

void UGA_Navigate::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    // 缓存用于 EndAbility
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 从 EventData 获取目标位置 (如果有)
    if (TriggerEventData && TriggerEventData->TargetData.IsValid(0))
    {
        const FGameplayAbilityTargetData* Data = TriggerEventData->TargetData.Get(0);
        if (Data)
        {
            // 尝试从 TargetData 获取位置
            if (Data->HasHitResult())
            {
                TargetLocation = Data->GetHitResult()->Location;
            }
        }
    }
    
    StartNavigation();
}

void UGA_Navigate::StartNavigation()
{
    AMAAgent* Agent = GetOwningAgent();
    if (!Agent)
    {
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    AAIController* AICtrl = Cast<AAIController>(Agent->GetController());
    if (!AICtrl)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Navigate] No AIController for %s"), *Agent->AgentName);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 绑定导航完成回调
    UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent();
    if (PathComp)
    {
        PathComp->OnRequestFinished.AddUObject(this, &UGA_Navigate::OnMoveCompleted);
    }
    
    // 显示头顶状态
    Agent->ShowAbilityStatus(TEXT("Navigate"), 
        FString::Printf(TEXT("→ (%.0f, %.0f)"), TargetLocation.X, TargetLocation.Y));
    
    // 开始导航
    Agent->bIsMoving = true;
    EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(
        TargetLocation,
        AcceptanceRadius,
        true,   // bStopOnOverlap
        true,   // bUsePathfinding
        false,  // bProjectDestinationToNavigation
        true,   // bCanStrafe
        nullptr // FilterClass
    );
    
    UE_LOG(LogTemp, Log, TEXT("[Navigate] %s moving to (%.0f, %.0f, %.0f), Result=%d"), 
        *Agent->AgentName, TargetLocation.X, TargetLocation.Y, TargetLocation.Z, (int32)Result);
    
    if (Result == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Navigate] MoveToLocation failed for %s"), *Agent->AgentName);
        CleanupNavigation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
    else if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        UE_LOG(LogTemp, Log, TEXT("[Navigate] %s already at goal"), *Agent->AgentName);
        CleanupNavigation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
    }
    // RequestSuccessful: 等待 OnMoveCompleted 回调
}

void UGA_Navigate::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    AMAAgent* Agent = GetOwningAgent();
    FString AgentName = Agent ? Agent->AgentName : TEXT("Unknown");
    
    bool bWasCancelled = !Result.IsSuccess();
    
    UE_LOG(LogTemp, Log, TEXT("[Navigate] %s move completed, Code=%d, Cancelled=%d"), 
        *AgentName, (int32)Result.Code, bWasCancelled ? 1 : 0);
    
    // 到达目标后清除头顶状态
    if (Agent)
    {
        Agent->ShowStatus(TEXT(""), 0.f);
    }
    
    CleanupNavigation();
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, bWasCancelled);
}

void UGA_Navigate::CleanupNavigation()
{
    AMAAgent* Agent = GetOwningAgent();
    if (Agent)
    {
        Agent->bIsMoving = false;
        
        // 解绑回调
        if (AAIController* AICtrl = Cast<AAIController>(Agent->GetController()))
        {
            if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
            {
                PathComp->OnRequestFinished.RemoveAll(this);
            }
        }
    }
}

void UGA_Navigate::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 确保清理
    CleanupNavigation();
    
    // 如果被取消，停止移动
    if (bWasCancelled)
    {
        AMAAgent* Agent = GetOwningAgent();
        if (Agent)
        {
            if (AAIController* AICtrl = Cast<AAIController>(Agent->GetController()))
            {
                AICtrl->StopMovement();
            }
        }
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
