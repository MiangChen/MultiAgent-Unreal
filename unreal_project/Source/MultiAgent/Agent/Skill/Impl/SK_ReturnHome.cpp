// SK_ReturnHome.cpp
// 返航技能 - 使用 MANavigationService 统一接口

#include "SK_ReturnHome.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Component/MANavigationService.h"

USK_ReturnHome::USK_ReturnHome()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_ReturnHome::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    bReturnHomeSucceeded = false;
    ReturnHomeResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailAndEnd(TEXT("ReturnHome failed: Character not found"));
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailAndEnd(TEXT("ReturnHome failed: SkillComponent not found"));
        return;
    }
    
    UMANavigationService* NavService = Character->GetNavigationService();
    if (!NavService)
    {
        FailAndEnd(TEXT("ReturnHome failed: NavigationService not found"));
        return;
    }
    
    // 获取返航参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    HomeLocation = Params.HomeLocation;
    LandAltitude = Params.LandHeight;
    
    Character->ShowAbilityStatus(TEXT("ReturnHome"), TEXT("Flying home..."));
    Character->bIsMoving = true;
    
    // 绑定导航完成回调
    NavService->OnNavigationCompleted.AddDynamic(this, &USK_ReturnHome::OnNavigationCompleted);
    
    // 调用 NavigationService 的 ReturnHome
    if (!NavService->ReturnHome(HomeLocation, LandAltitude))
    {
        NavService->OnNavigationCompleted.RemoveDynamic(this, &USK_ReturnHome::OnNavigationCompleted);
        FailAndEnd(TEXT("ReturnHome failed: NavigationService rejected request"));
    }
}

void USK_ReturnHome::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    bReturnHomeSucceeded = bSuccess;
    ReturnHomeResultMessage = Message;
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("ReturnHome"), bSuccess ? TEXT("Complete!") : TEXT("Failed"));
        
        // 返航成功后停止螺旋桨
        if (bSuccess)
        {
            if (USkeletalMeshComponent* Mesh = Character->GetMesh())
            {
                Mesh->Stop();
            }
        }
    }
    
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, !bSuccess);
}

void USK_ReturnHome::FailAndEnd(const FString& Message)
{
    bReturnHomeSucceeded = false;
    ReturnHomeResultMessage = Message;
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

void USK_ReturnHome::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    UMASkillComponent* SkillCompToNotify = nullptr;
    bool bShouldNotify = false;
    
    if (Character)
    {
        // 解绑回调
        if (UMANavigationService* NavService = Character->GetNavigationService())
        {
            NavService->OnNavigationCompleted.RemoveDynamic(this, &USK_ReturnHome::OnNavigationCompleted);
            
            if (bWasCancelled)
            {
                NavService->CancelNavigation();
            }
        }
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (bWasCancelled && ReturnHomeResultMessage.IsEmpty())
        {
            FVector CurrentLocation = Character->GetActorLocation();
            float Distance2D = FVector::Dist2D(CurrentLocation, HomeLocation);
            ReturnHomeResultMessage = FString::Printf(
                TEXT("ReturnHome cancelled: Stopped %.0fm from home at (%.0f, %.0f, %.0f)"), 
                Distance2D / 100.f, CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
        }
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag ReturnHomeTag = FGameplayTag::RequestGameplayTag(FName("Command.ReturnHome"));
            if (SkillComp->HasMatchingGameplayTag(ReturnHomeTag))
            {
                SkillComp->RemoveLooseGameplayTag(ReturnHomeTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bReturnHomeSucceeded, ReturnHomeResultMessage);
    }
}
