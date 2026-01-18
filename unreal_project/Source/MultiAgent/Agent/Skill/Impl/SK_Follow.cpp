// SK_Follow.cpp

#include "SK_Follow.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "AIController.h"
#include "TimerManager.h"

USK_Follow::USK_Follow()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    LastTargetLocation = FVector::ZeroVector;
    bFollowSucceeded = false;
    FollowResultMessage = TEXT("");
}

void USK_Follow::SetTargetCharacter(AMACharacter* InTargetCharacter)
{
    TargetActor = InTargetCharacter;
}

void USK_Follow::SetTargetActor(AActor* InTargetActor)
{
    TargetActor = InTargetActor;
}

void USK_Follow::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置结果状态
    bFollowSucceeded = false;
    FollowResultMessage = TEXT("");
    
    if (!TargetActor.IsValid())
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: Target not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: Owner character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    FString TargetName = TargetActor->GetName();
    Character->ShowAbilityStatus(TEXT("Following"), FString::Printf(TEXT("-> %s"), *TargetName));
    UpdateFollow();
    
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USK_Follow::UpdateFollow, UpdateInterval, true);
    }
}

void USK_Follow::UpdateFollow()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !TargetActor.IsValid())
    {
        bFollowSucceeded = false;
        FollowResultMessage = TargetActor.IsValid() ? 
            TEXT("Follow failed: Owner character lost") : 
            TEXT("Follow failed: Target lost");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    FVector CurrentTargetLocation = TargetActor->GetActorLocation();
    FVector FollowLocation = CalculateFollowLocation();
    
    float DistanceToFollow = FVector::Dist(Character->GetActorLocation(), FollowLocation);
    if (DistanceToFollow < FollowDistance * 1.1f)
    {
        Character->ShowStatus(TEXT(""), 0.f);
        Character->bIsMoving = false;
        // 跟随成功（保持在跟随距离内）
        bFollowSucceeded = true;
        FString TargetName = TargetActor->GetName();
        FollowResultMessage = FString::Printf(TEXT("Follow succeeded: Following %s at distance %.0f"), 
            *TargetName, DistanceToFollow);
        return;
    }
    
    float DistanceMoved = FVector::Dist(CurrentTargetLocation, LastTargetLocation);
    if (DistanceMoved < RepathThreshold && Character->bIsMoving) return;
    
    AAIController* AICtrl = Cast<AAIController>(Character->GetController());
    if (AICtrl)
    {
        EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(FollowLocation, 50.f, true, true, true, true);
        if ((int32)Result == 0)
        {
            Result = AICtrl->MoveToLocation(CurrentTargetLocation, FollowDistance, true, true, true, true);
        }
        Character->bIsMoving = ((int32)Result != 0);
        if (Character->bIsMoving) LastTargetLocation = CurrentTargetLocation;
    }
}

FVector USK_Follow::CalculateFollowLocation() const
{
    if (!TargetActor.IsValid()) return FVector::ZeroVector;
    
    AMACharacter* Character = const_cast<USK_Follow*>(this)->GetOwningCharacter();
    if (!Character) return TargetActor->GetActorLocation();
    
    FVector TargetLoc = TargetActor->GetActorLocation();
    FVector MyLocation = Character->GetActorLocation();
    
    FVector Direction = (MyLocation - TargetLoc).GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        // 如果目标是 AMACharacter，使用其朝向
        if (AMACharacter* TargetChar = Cast<AMACharacter>(TargetActor.Get()))
        {
            Direction = -TargetChar->GetActorForwardVector();
        }
        else
        {
            // 否则使用目标 Actor 的朝向
            Direction = -TargetActor->GetActorForwardVector();
        }
    }
    
    FVector FollowLoc = TargetLoc + Direction * FollowDistance;
    FollowLoc.Z = TargetLoc.Z;
    return FollowLoc;
}

void USK_Follow::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bFollowSucceeded;
    FString MessageToNotify = FollowResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
        UpdateTimerHandle.Invalidate();
        
        Character->bIsMoving = false;
        if (AAIController* AICtrl = Cast<AAIController>(Character->GetController()))
        {
            AICtrl->StopMovement();
        }
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && FollowResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FString TargetName = TargetActor.IsValid() ? TargetActor->GetName() : TEXT("unknown");
            MessageToNotify = FString::Printf(TEXT("Follow cancelled: Stopped following %s"), *TargetName);
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag FollowTag = FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
            if (SkillComp->HasMatchingGameplayTag(FollowTag))
            {
                SkillComp->RemoveLooseGameplayTag(FollowTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    TargetActor.Reset();
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
