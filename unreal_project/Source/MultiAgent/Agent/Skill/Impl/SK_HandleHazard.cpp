// SK_HandleHazard.cpp

#include "SK_HandleHazard.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "Containers/Ticker.h"

USK_HandleHazard::USK_HandleHazard()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
}

void USK_HandleHazard::ActivateAbility(
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

    // 初始化处理状态
    HandleProgress = 0.f;
    StartTime = Character->GetWorld()->GetTimeSeconds();

    // 显示处理状态
    ShowHandleEffect();

    // 启动 Ticker 更新进度
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &USK_HandleHazard::TickHandle),
        0.0f
    );
}

void USK_HandleHazard::ShowHandleEffect()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    // 显示初始状态
    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Processing... 0%"));
}

bool USK_HandleHazard::TickHandle(float DeltaTime)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        OnHandleComplete();
        return false;  // 停止 Ticker
    }

    // 更新进度
    float CurrentTime = Character->GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - StartTime;
    HandleProgress = FMath::Clamp(ElapsedTime / HandleDuration, 0.f, 1.f);

    // 更新进度显示
    UpdateProgressDisplay();

    // 检查是否完成
    if (HandleProgress >= 1.f)
    {
        OnHandleComplete();
        return false;  // 停止 Ticker
    }

    return true;  // 继续 Ticker
}

void USK_HandleHazard::UpdateProgressDisplay()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    int32 ProgressPercent = FMath::RoundToInt(HandleProgress * 100.f);
    FString StatusText = FString::Printf(TEXT("Processing... %d%%"), ProgressPercent);
    Character->ShowAbilityStatus(TEXT("HandleHazard"), StatusText);
}

void USK_HandleHazard::OnHandleComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
    }

    // 通知技能完成
    bool bShouldNotify = false;
    bool bSuccessToNotify = true;
    FString MessageToNotify = TEXT("处理成功");
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag HandleHazardTag = FGameplayTag::RequestGameplayTag(FName("Command.HandleHazard"));
            if (SkillComp->HasMatchingGameplayTag(HandleHazardTag))
            {
                SkillComp->RemoveLooseGameplayTag(HandleHazardTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;

                // 记录处理持续时间到反馈上下文
                FMAFeedbackContext& FeedbackCtx = SkillComp->GetFeedbackContextMutable();
                FeedbackCtx.HazardHandleDurationSeconds = HandleDuration;
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

void USK_HandleHazard::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 清理 Ticker
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
