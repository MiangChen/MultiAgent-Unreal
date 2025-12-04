// MAStateTreeComponent.cpp

#include "MAStateTreeComponent.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../Character/MACharacter.h"

UMAStateTreeComponent::UMAStateTreeComponent()
{
}

void UMAStateTreeComponent::BeginPlay()
{
    Super::BeginPlay();

    // 缓存 GAS 组件
    if (AActor* Owner = GetOwner())
    {
        CachedASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>();
    }
}

UMAAbilitySystemComponent* UMAStateTreeComponent::GetAbilitySystemComponent() const
{
    return CachedASC.Get();
}

bool UMAStateTreeComponent::RequestActivateAbility(FGameplayTag AbilityTag)
{
    if (UMAAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        return ASC->TryActivateAbilityByTag(AbilityTag);
    }
    return false;
}

void UMAStateTreeComponent::RequestCancelAbility(FGameplayTag AbilityTag)
{
    if (UMAAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->CancelAbilityByTag(AbilityTag);
    }
}
