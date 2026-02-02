// SK_Navigate.cpp
// 导航技能 - 委托到 NavigationService 实现

#include "SK_Navigate.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Component/MANavigationService.h"
#include "AIController.h"

USK_Navigate::USK_Navigate()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    TargetLocation = FVector::ZeroVector;
    bNavigationSucceeded = false;
    NavigationResultMessage = TEXT("");
}

void USK_Navigate::SetTargetLocation(FVector InTargetLocation)
{
    TargetLocation = InTargetLocation;
}

bool USK_Navigate::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
    
    AMACharacter* Character = const_cast<USK_Navigate*>(this)->GetOwningCharacter();
    if (!Character) return false;
    
    return Cast<AAIController>(Character->GetController()) != nullptr;
}

void USK_Navigate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置结果状态
    bNavigationSucceeded = false;
    NavigationResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: Character not found");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 获取 NavigationService
    NavigationService = Character->GetNavigationService();
    if (!NavigationService)
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Navigate] %s: NavigationService not found"), *Character->AgentLabel);
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: NavigationService not available");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Navigate::OnNavigationCompleted);
    
    // 显示状态
    Character->ShowAbilityStatus(TEXT("Navigate"), FString::Printf(TEXT("-> (%.0f, %.0f, %.0f)"), TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
    Character->bIsMoving = true;
    
    // 启动导航
    bool bNavStarted = NavigationService->NavigateTo(TargetLocation, AcceptanceRadius);
    
    if (!bNavStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: NavigateTo failed to start"), *Character->AgentLabel);
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: Could not start navigation");
        
        // 解绑回调
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Navigate::OnNavigationCompleted);
        
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
}

void USK_Navigate::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    AMACharacter* Character = GetOwningCharacter();
    UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: OnNavigationCompleted - Success=%s, Message=%s"),
        Character ? *Character->AgentLabel : TEXT("NULL"),
        bSuccess ? TEXT("true") : TEXT("false"),
        *Message);
    
    // 设置结果
    bNavigationSucceeded = bSuccess;
    NavigationResultMessage = Message;
    
    // 结束技能
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, !bSuccess);
}

void USK_Navigate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 解绑导航回调并取消导航
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Navigate::OnNavigationCompleted);
        
        // 如果被取消，取消导航
        if (bWasCancelled && NavigationService->IsNavigating())
        {
            NavigationService->CancelNavigation();
        }
        
        NavigationService = nullptr;
    }
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bNavigationSucceeded;
    FString MessageToNotify = NavigationResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && NavigationResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            MessageToNotify = TEXT("Navigate failed: Cancelled by user");
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag NavigateTag = FGameplayTag::RequestGameplayTag(FName("Command.Navigate"));
            if (SkillComp->HasMatchingGameplayTag(NavigateTag))
            {
                SkillComp->RemoveLooseGameplayTag(NavigateTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    // 先调用父类 EndAbility，确保技能完全结束
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
