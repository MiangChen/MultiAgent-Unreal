// SK_Broadcast.cpp

#include "SK_Broadcast.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "Components/TextRenderComponent.h"
#include "TimerManager.h"

USK_Broadcast::USK_Broadcast()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
    TextBubble = nullptr;
}

void USK_Broadcast::ActivateAbility(
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

    // 获取喊话内容
    FString Message = TEXT("...");
    if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
    {
        Message = SkillComp->GetSkillParams().BroadcastMessage;
        if (Message.IsEmpty())
        {
            Message = TEXT("Hello!");
        }
    }

    // 显示喊话状态
    Character->ShowAbilityStatus(TEXT("Broadcast"), Message);

    // 显示喊话特效（文本气泡）
    ShowBroadcastEffect(Message);

    // 设置定时器，3秒后完成喊话
    if (UWorld* World = Character->GetWorld())
    {
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &USK_Broadcast::OnBroadcastComplete);
        
        World->GetTimerManager().SetTimer(
            BroadcastTimerHandle,
            TimerDelegate,
            BroadcastDuration,
            false
        );
    }
}

void USK_Broadcast::ShowBroadcastEffect(const FString& Message)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    // 创建文本渲染组件作为喊话气泡
    TextBubble = NewObject<UTextRenderComponent>(Character);
    if (TextBubble)
    {
        TextBubble->RegisterComponent();
        TextBubble->AttachToComponent(
            Character->GetRootComponent(),
            FAttachmentTransformRules::KeepRelativeTransform
        );

        // 设置文本内容和样式
        TextBubble->SetText(FText::FromString(Message));
        TextBubble->SetTextRenderColor(FColor::Yellow);
        TextBubble->SetWorldSize(50.0f);
        TextBubble->SetHorizontalAlignment(EHTA_Center);
        TextBubble->SetVerticalAlignment(EVRTA_TextBottom);

        // 设置位置（在角色头顶上方）
        FVector Offset(0.f, 0.f, 150.f);
        TextBubble->SetRelativeLocation(Offset);

        // 让文本面向摄像机
        TextBubble->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    }
}

void USK_Broadcast::HideBroadcastEffect()
{
    if (TextBubble && TextBubble->IsValidLowLevel())
    {
        TextBubble->DestroyComponent();
        TextBubble = nullptr;
    }
}

void USK_Broadcast::OnBroadcastComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
    }

    // 隐藏文本气泡
    HideBroadcastEffect();

    // 通知技能完成
    bool bShouldNotify = false;
    bool bSuccessToNotify = true;
    FString MessageToNotify = TEXT("喊话成功");
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag BroadcastTag = FGameplayTag::RequestGameplayTag(FName("Command.Broadcast"));
            if (SkillComp->HasMatchingGameplayTag(BroadcastTag))
            {
                SkillComp->RemoveLooseGameplayTag(BroadcastTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
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

void USK_Broadcast::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 清理定时器
    AMACharacter* Character = GetOwningCharacter();
    if (Character && BroadcastTimerHandle.IsValid())
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(BroadcastTimerHandle);
        }
        BroadcastTimerHandle.Invalidate();
    }

    // 清理文本气泡
    HideBroadcastEffect();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
