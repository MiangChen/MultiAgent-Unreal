// SK_TakeOff.cpp
// 起飞技能 - 使用 MANavigationService 统一接口

#include "SK_TakeOff.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Component/MANavigationService.h"

USK_TakeOff::USK_TakeOff()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_TakeOff::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    bTakeOffSucceeded = false;
    TakeOffResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailAndEnd(TEXT("TakeOff failed: Character not found"));
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailAndEnd(TEXT("TakeOff failed: SkillComponent not found"));
        return;
    }
    
    UMANavigationService* NavService = Character->GetNavigationService();
    if (!NavService)
    {
        FailAndEnd(TEXT("TakeOff failed: NavigationService not found"));
        return;
    }
    
    // 获取起飞高度参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    TargetAltitude = Params.TakeOffHeight;
    
    Character->ShowAbilityStatus(TEXT("TakeOff"), FString::Printf(TEXT("-> %.0fm"), TargetAltitude / 100.f));
    Character->bIsMoving = true;
    
    // 启动螺旋桨动画
    if (USkeletalMeshComponent* Mesh = Character->GetMesh())
    {
        Mesh->Play(true);
    }
    
    // 绑定导航完成回调
    NavService->OnNavigationCompleted.AddDynamic(this, &USK_TakeOff::OnNavigationCompleted);
    
    // 调用 NavigationService 的 TakeOff
    if (!NavService->TakeOff(TargetAltitude))
    {
        NavService->OnNavigationCompleted.RemoveDynamic(this, &USK_TakeOff::OnNavigationCompleted);
        FailAndEnd(TEXT("TakeOff failed: NavigationService rejected request"));
    }
}

void USK_TakeOff::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    bTakeOffSucceeded = bSuccess;
    TakeOffResultMessage = Message;
    
    if (AMACharacter* Character = GetOwningCharacter())
    {
        Character->ShowAbilityStatus(TEXT("TakeOff"), bSuccess ? TEXT("Complete!") : TEXT("Failed"));
    }
    
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, !bSuccess);
}

void USK_TakeOff::FailAndEnd(const FString& Message)
{
    bTakeOffSucceeded = false;
    TakeOffResultMessage = Message;
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

void USK_TakeOff::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    UMASkillComponent* SkillCompToNotify = nullptr;
    bool bShouldNotify = false;
    
    if (Character)
    {
        // 解绑回调
        if (UMANavigationService* NavService = Character->GetNavigationService())
        {
            NavService->OnNavigationCompleted.RemoveDynamic(this, &USK_TakeOff::OnNavigationCompleted);
            
            // 如果被取消，也取消导航
            if (bWasCancelled)
            {
                NavService->CancelNavigation();
            }
        }
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息
        if (bWasCancelled && TakeOffResultMessage.IsEmpty())
        {
            FVector CurrentLocation = Character->GetActorLocation();
            TakeOffResultMessage = FString::Printf(TEXT("TakeOff cancelled: Stopped at altitude %.0fm (target: %.0fm)"), 
                CurrentLocation.Z / 100.f, TargetAltitude / 100.f);
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
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bTakeOffSucceeded, TakeOffResultMessage);
    }
}
