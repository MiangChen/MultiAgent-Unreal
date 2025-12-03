// MAAbilitySystemComponent.cpp

#include "MAAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UMAAbilitySystemComponent::UMAAbilitySystemComponent()
{
}

void UMAAbilitySystemComponent::InitializeAbilities(AActor* InOwnerActor)
{
    if (!InOwnerActor) return;

    // 授予默认 Abilities
    for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, InOwnerActor));
        }
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

bool UMAAbilitySystemComponent::HasGameplayTagFromContainer(FGameplayTag TagToCheck) const
{
    // 使用父类的方法检查 Tag
    FGameplayTagContainer OwnedTags;
    GetOwnedGameplayTags(OwnedTags);
    return OwnedTags.HasTag(TagToCheck);
}
