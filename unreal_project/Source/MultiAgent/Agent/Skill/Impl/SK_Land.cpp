// SK_Land.cpp

#include "SK_Land.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAUAVCharacter.h"
#include "TimerManager.h"

USK_Land::USK_Land()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    bLandSucceeded = false;
    LandResultMessage = TEXT("");
}

void USK_Land::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置结果状态
    bLandSucceeded = false;
    LandResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bLandSucceeded = false;
        LandResultMessage = TEXT("Land failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bLandSucceeded = false;
        LandResultMessage = TEXT("Land failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 获取降落高度参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    float LandHeight = Params.LandHeight;
    
    StartLocation = Character->GetActorLocation();
    TargetLocation = StartLocation;
    TargetLocation.Z = LandHeight;
    
    Character->ShowAbilityStatus(TEXT("Land"), FString::Printf(TEXT("-> %.0fm"), LandHeight / 100.f));
    Character->bIsMoving = true;
    
    // 开始降落更新
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USK_Land::UpdateLand, 0.05f, true);
    }
}

void USK_Land::UpdateLand()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bLandSucceeded = false;
        LandResultMessage = TEXT("Land failed: Character lost during landing");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    FVector CurrentLocation = Character->GetActorLocation();
    float DistanceToTarget = FMath::Abs(CurrentLocation.Z - TargetLocation.Z);
    
    if (DistanceToTarget < 10.f)
    {
        // 到达目标高度
        Character->SetActorLocation(TargetLocation);
        
        // 更新 UAV 的飞行状态为 Landed
        if (AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(Character))
        {
            UAV->SetFlightState(EMAFlightState::Landed);
        }
        
        bLandSucceeded = true;
        LandResultMessage = FString::Printf(TEXT("Land succeeded: Landed at altitude %.0fm"), TargetLocation.Z / 100.f);
        Character->ShowAbilityStatus(TEXT("Land"), TEXT("Complete!"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        return;
    }
    
    // 向下移动
    FVector NewLocation = CurrentLocation;
    float DeltaZ = LandSpeed * 0.05f;
    NewLocation.Z = FMath::Max(NewLocation.Z - DeltaZ, TargetLocation.Z);
    Character->SetActorLocation(NewLocation);
}

void USK_Land::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bLandSucceeded;
    FString MessageToNotify = LandResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
        UpdateTimerHandle.Invalidate();
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && LandResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FVector CurrentLocation = Character->GetActorLocation();
            MessageToNotify = FString::Printf(TEXT("Land cancelled: Stopped at altitude %.0fm (target: %.0fm)"), 
                CurrentLocation.Z / 100.f, TargetLocation.Z / 100.f);
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag LandTag = FGameplayTag::RequestGameplayTag(FName("Command.Land"));
            if (SkillComp->HasMatchingGameplayTag(LandTag))
            {
                SkillComp->RemoveLooseGameplayTag(LandTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
