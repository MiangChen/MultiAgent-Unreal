// MAAbilitySystemComponent.cpp

#include "MAAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GA_Pickup.h"
#include "Abilities/GA_Drop.h"
#include "Abilities/GA_Navigate.h"
#include "Abilities/GA_Follow.h"
#include "Abilities/GA_TakePhoto.h"
#include "MAGameplayTags.h"

UMAAbilitySystemComponent::UMAAbilitySystemComponent()
{
}

void UMAAbilitySystemComponent::InitializeAbilities(AActor* InOwnerActor)
{
    if (!InOwnerActor) return;

    // 授予默认 Abilities (从 DefaultAbilities 数组)
    for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, InOwnerActor));
        }
    }

    // 始终授予 Pickup、Drop、Navigate、TakePhoto Ability，并保存 Handle
    PickupAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Pickup::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    DropAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Drop::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    NavigateAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Navigate::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    FollowAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Follow::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    TakePhotoAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_TakePhoto::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    
    UE_LOG(LogTemp, Log, TEXT("Initialized GAS abilities for %s (Pickup=%d, Drop=%d, Navigate=%d, Follow=%d, TakePhoto=%d)"), 
        *InOwnerActor->GetName(), PickupAbilityHandle.IsValid(), DropAbilityHandle.IsValid(), NavigateAbilityHandle.IsValid(), FollowAbilityHandle.IsValid(), TakePhotoAbilityHandle.IsValid());
}

bool UMAAbilitySystemComponent::TryActivatePickup()
{
    if (PickupAbilityHandle.IsValid())
    {
        return TryActivateAbility(PickupAbilityHandle);
    }
    
    // Fallback: 尝试通过 Class 激活
    return TryActivateAbilityByClass(UGA_Pickup::StaticClass());
}

bool UMAAbilitySystemComponent::TryActivateDrop()
{
    if (DropAbilityHandle.IsValid())
    {
        return TryActivateAbility(DropAbilityHandle);
    }
    
    // Fallback: 尝试通过 Class 激活
    return TryActivateAbilityByClass(UGA_Drop::StaticClass());
}

bool UMAAbilitySystemComponent::TryActivateNavigate(FVector TargetLocation)
{
    // 先取消正在进行的导航
    CancelNavigate();
    
    if (!NavigateAbilityHandle.IsValid())
    {
        return false;
    }
    
    // 先设置目标位置到 Ability 实例
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(NavigateAbilityHandle);
    if (Spec)
    {
        // 获取或创建实例
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            // 强制创建实例
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Navigate* NavigateAbility = Cast<UGA_Navigate>(Instance))
        {
            NavigateAbility->SetTargetLocation(TargetLocation);
        }
    }
    
    return TryActivateAbility(NavigateAbilityHandle);
}

void UMAAbilitySystemComponent::CancelNavigate()
{
    // 通过 Handle 直接取消，避免使用已废弃的 AbilityTags
    if (NavigateAbilityHandle.IsValid())
    {
        CancelAbilityHandle(NavigateAbilityHandle);
    }
}

bool UMAAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    return TryActivateAbilitiesByTag(TagContainer);
}

void UMAAbilitySystemComponent::CancelAbilityByTag(FGameplayTag AbilityTag)
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    CancelAbilities(&TagContainer);
}

bool UMAAbilitySystemComponent::TryActivateTakePhoto()
{
    if (TakePhotoAbilityHandle.IsValid())
    {
        return TryActivateAbility(TakePhotoAbilityHandle);
    }
    
    // Fallback: 尝试通过 Class 激活
    return TryActivateAbilityByClass(UGA_TakePhoto::StaticClass());
}

bool UMAAbilitySystemComponent::HasGameplayTagFromContainer(FGameplayTag TagToCheck) const
{
    FGameplayTagContainer OwnedTags;
    GetOwnedGameplayTags(OwnedTags);
    return OwnedTags.HasTag(TagToCheck);
}

bool UMAAbilitySystemComponent::TryActivateFollow(AMACharacter* TargetCharacter, float FollowDistance)
{
    // 先取消正在进行的追踪和导航
    CancelFollow();
    CancelNavigate();
    
    if (!FollowAbilityHandle.IsValid() || !TargetCharacter)
    {
        return false;
    }
    
    // 设置目标到 Ability 实例
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(FollowAbilityHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Follow* FollowAbility = Cast<UGA_Follow>(Instance))
        {
            FollowAbility->SetTargetCharacter(TargetCharacter);
            FollowAbility->FollowDistance = FollowDistance;
        }
    }
    
    return TryActivateAbility(FollowAbilityHandle);
}

void UMAAbilitySystemComponent::CancelFollow()
{
    if (FollowAbilityHandle.IsValid())
    {
        CancelAbilityHandle(FollowAbilityHandle);
    }
}
