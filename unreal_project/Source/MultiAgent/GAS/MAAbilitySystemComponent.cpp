// MAAbilitySystemComponent.cpp

#include "MAAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GA_Pickup.h"
#include "Abilities/GA_Drop.h"
#include "Abilities/GA_Navigate.h"
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

    // 始终授予 Pickup、Drop、Navigate Ability，并保存 Handle
    PickupAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Pickup::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    DropAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Drop::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    NavigateAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Navigate::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    
    UE_LOG(LogTemp, Log, TEXT("Initialized GAS abilities for %s (Pickup=%d, Drop=%d, Navigate=%d)"), 
        *InOwnerActor->GetName(), PickupAbilityHandle.IsValid(), DropAbilityHandle.IsValid(), NavigateAbilityHandle.IsValid());
}

bool UMAAbilitySystemComponent::TryActivatePickup()
{
    UE_LOG(LogTemp, Warning, TEXT("[ASC] TryActivatePickup: Handle valid=%d"), PickupAbilityHandle.IsValid() ? 1 : 0);
    
    if (PickupAbilityHandle.IsValid())
    {
        bool bResult = TryActivateAbility(PickupAbilityHandle);
        UE_LOG(LogTemp, Warning, TEXT("[ASC] TryActivateAbility(Handle) result=%d"), bResult ? 1 : 0);
        return bResult;
    }
    
    // Fallback: 尝试通过 Class 激活
    UE_LOG(LogTemp, Warning, TEXT("[ASC] Fallback to TryActivateAbilityByClass"));
    bool bResult = TryActivateAbilityByClass(UGA_Pickup::StaticClass());
    UE_LOG(LogTemp, Warning, TEXT("[ASC] TryActivateAbilityByClass result=%d"), bResult ? 1 : 0);
    return bResult;
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
    CancelAbilityByTag(FMAGameplayTags::Get().Ability_Navigate);
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

bool UMAAbilitySystemComponent::HasGameplayTagFromContainer(FGameplayTag TagToCheck) const
{
    FGameplayTagContainer OwnedTags;
    GetOwnedGameplayTags(OwnedTags);
    return OwnedTags.HasTag(TagToCheck);
}
