// SK_TakeOff.cpp

#include "SK_TakeOff.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAUAVCharacter.h"
#include "TimerManager.h"

USK_TakeOff::USK_TakeOff()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    bTakeOffSucceeded = false;
    TakeOffResultMessage = TEXT("");
}

void USK_TakeOff::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置结果状态
    bTakeOffSucceeded = false;
    TakeOffResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bTakeOffSucceeded = false;
        TakeOffResultMessage = TEXT("TakeOff failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bTakeOffSucceeded = false;
        TakeOffResultMessage = TEXT("TakeOff failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 获取起飞高度参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    float TakeOffHeight = Params.TakeOffHeight;
    
    StartLocation = Character->GetActorLocation();
    TargetLocation = StartLocation;
    TargetLocation.Z = TakeOffHeight;
    
    Character->ShowAbilityStatus(TEXT("TakeOff"), FString::Printf(TEXT("-> %.0fm"), TakeOffHeight / 100.f));
    Character->bIsMoving = true;
    
    // 立即启动螺旋桨动画
    if (USkeletalMeshComponent* Mesh = Character->GetMesh())
    {
        Mesh->Play(true);
    }
    
    // 开始起飞更新
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USK_TakeOff::UpdateTakeOff, 0.05f, true);
    }
}

void USK_TakeOff::UpdateTakeOff()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bTakeOffSucceeded = false;
        TakeOffResultMessage = TEXT("TakeOff failed: Character lost during takeoff");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    FVector CurrentLocation = Character->GetActorLocation();
    float DistanceToTarget = FMath::Abs(TargetLocation.Z - CurrentLocation.Z);
    
    if (DistanceToTarget < 10.f)
    {
        // 到达目标高度
        Character->SetActorLocation(TargetLocation);
        
        // 更新 UAV 的飞行状态为 Hovering
        if (AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(Character))
        {
            UAV->SetFlightState(EMAFlightState::Hovering);
        }
        
        bTakeOffSucceeded = true;
        TakeOffResultMessage = FString::Printf(TEXT("TakeOff succeeded: Reached altitude %.0fm"), TargetLocation.Z / 100.f);
        Character->ShowAbilityStatus(TEXT("TakeOff"), TEXT("Complete!"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        return;
    }
    
    // 向上移动
    FVector NewLocation = CurrentLocation;
    float DeltaZ = TakeOffSpeed * 0.05f;
    NewLocation.Z = FMath::Min(NewLocation.Z + DeltaZ, TargetLocation.Z);
    Character->SetActorLocation(NewLocation);
}

void USK_TakeOff::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bTakeOffSucceeded;
    FString MessageToNotify = TakeOffResultMessage;
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
        if (bWasCancelled && TakeOffResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FVector CurrentLocation = Character->GetActorLocation();
            MessageToNotify = FString::Printf(TEXT("TakeOff cancelled: Stopped at altitude %.0fm (target: %.0fm)"), 
                CurrentLocation.Z / 100.f, TargetLocation.Z / 100.f);
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag TakeOffTag = FGameplayTag::RequestGameplayTag(FName("Command.TakeOff"));
            if (SkillComp->HasMatchingGameplayTag(TakeOffTag))
            {
                SkillComp->RemoveLooseGameplayTag(TakeOffTag);
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
