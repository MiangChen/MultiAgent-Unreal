// SK_Land.cpp
// 降落技能 - 使用 MANavigationService 统一接口

#include "SK_Land.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"

USK_Land::USK_Land()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_Land::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    bLandSucceeded = false;
    LandResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailAndEnd(TEXT("Land failed: Character not found"));
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailAndEnd(TEXT("Land failed: SkillComponent not found"));
        return;
    }
    
    UMANavigationService* NavService = Character->GetNavigationService();
    if (!NavService)
    {
        FailAndEnd(TEXT("Land failed: NavigationService not found"));
        return;
    }
    
    // 获取降落高度参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    TargetAltitude = Params.LandHeight;
    
    Character->ShowAbilityStatus(TEXT("Land"), FString::Printf(TEXT("-> %.0fm"), TargetAltitude / 100.f));
    Character->bIsMoving = true;
    
    // 绑定导航完成回调
    NavService->OnNavigationCompleted.AddDynamic(this, &USK_Land::OnNavigationCompleted);
    
    // 调用 NavigationService 的 Land
    if (!NavService->Land(TargetAltitude))
    {
        NavService->OnNavigationCompleted.RemoveDynamic(this, &USK_Land::OnNavigationCompleted);
        FailAndEnd(TEXT("Land failed: NavigationService rejected request"));
    }
}

void USK_Land::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    bLandSucceeded = bSuccess;
    LandResultMessage = Message;
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("Land"), bSuccess ? TEXT("Complete!") : TEXT("Failed"));
        
        // 降落成功后停止螺旋桨
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

void USK_Land::FailAndEnd(const FString& Message)
{
    bLandSucceeded = false;
    LandResultMessage = Message;
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

void USK_Land::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        // 解绑回调
        if (UMANavigationService* NavService = Character->GetNavigationService())
        {
            NavService->OnNavigationCompleted.RemoveDynamic(this, &USK_Land::OnNavigationCompleted);
            
            if (bWasCancelled)
            {
                NavService->CancelNavigation();
            }
        }
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (bWasCancelled && LandResultMessage.IsEmpty())
        {
            FVector CurrentLocation = Character->GetActorLocation();
            LandResultMessage = FString::Printf(TEXT("Land cancelled: Stopped at altitude %.0fm (target: %.0fm)"), 
                CurrentLocation.Z / 100.f, TargetAltitude / 100.f);
        }
        
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Land, bLandSucceeded, LandResultMessage);
        }
    }
}
