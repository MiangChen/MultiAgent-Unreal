// SK_TakePhoto.cpp

#include "SK_TakePhoto.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "TimerManager.h"

USK_TakePhoto::USK_TakePhoto()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
}

void USK_TakePhoto::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 显示拍照状态
    Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Taking photo..."));

    // 显示拍照特效
    ShowPhotoEffect();

    // 设置定时器，2秒后完成拍照
    if (UWorld* World = Character->GetWorld())
    {
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &USK_TakePhoto::OnPhotoComplete);
        
        World->GetTimerManager().SetTimer(
            PhotoTimerHandle,
            TimerDelegate,
            PhotoDuration,
            false
        );
    }
}

void USK_TakePhoto::ShowPhotoEffect()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    // 简单的视觉反馈：显示相机图标文本
    // 在实际项目中，这里可以添加粒子特效、声音等
    Character->ShowStatus(TEXT("📷"), 2.0f);
}

void USK_TakePhoto::OnPhotoComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
    }

    // 通知技能完成
    bool bShouldNotify = false;
    bool bSuccessToNotify = true;
    FString MessageToNotify;
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag TakePhotoTag = FGameplayTag::RequestGameplayTag(FName("Command.TakePhoto"));
            if (SkillComp->HasMatchingGameplayTag(TakePhotoTag))
            {
                SkillComp->RemoveLooseGameplayTag(TakePhotoTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;

                // 根据 FeedbackContext 生成消息
                const FMAFeedbackContext& FeedbackCtx = SkillComp->GetFeedbackContext();
                if (FeedbackCtx.bPhotoTargetFound)
                {
                    MessageToNotify = FString::Printf(TEXT("拍照成功，且发现了%s"), *FeedbackCtx.PhotoTargetName);
                }
                else
                {
                    FString TargetDesc = SkillComp->GetSkillParams().CommonTarget.Type.IsEmpty() 
                        ? TEXT("目标") 
                        : SkillComp->GetSkillParams().CommonTarget.Type;
                    MessageToNotify = FString::Printf(TEXT("拍照成功，但是没发现%s"), *TargetDesc);
                }
            }
        }
    }

    // 结束技能
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);

    // 在技能完全结束后通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}

void USK_TakePhoto::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 清理定时器
    AMACharacter* Character = GetOwningCharacter();
    if (Character && PhotoTimerHandle.IsValid())
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(PhotoTimerHandle);
        }
        PhotoTimerHandle.Invalidate();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
