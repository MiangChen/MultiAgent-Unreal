// GA_Drop.cpp
// UE 5.5 风格

#include "GA_Drop.h"
#include "../MAGameplayTags.h"
#include "../MAAbilitySystemComponent.h"
#include "../../AgentManager/MAAgent.h"
#include "../../Interaction/MAPickupItem.h"

UGA_Drop::UGA_Drop()
{
    // UE 5.5: 使用 SetAssetTags 设置 Ability 标识
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Drop);
    SetAssetTags(AssetTags);
    
    // 激活需要的 Tags (需要持有物品才能放下)
    ActivationRequiredTags.AddTag(FMAGameplayTags::Get().Status_Holding);
}

bool UGA_Drop::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    return FindHeldItem() != nullptr;
}

void UGA_Drop::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (AMAPickupItem* Item = FindHeldItem())
    {
        PerformDrop(Item);
        
        // 移除 Holding Tag
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->RemoveLooseGameplayTag(FMAGameplayTags::Get().Status_Holding);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

AMAPickupItem* UGA_Drop::FindHeldItem() const
{
    AMAAgent* Agent = GetOwningAgent();
    if (!Agent) return nullptr;

    // 查找附着在 Agent 上的 PickupItem
    TArray<AActor*> AttachedActors;
    Agent->GetAttachedActors(AttachedActors);

    for (AActor* Actor : AttachedActors)
    {
        if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
        {
            return Item;
        }
    }

    return nullptr;
}

void UGA_Drop::PerformDrop(AMAPickupItem* Item)
{
    AMAAgent* Agent = GetOwningAgent();
    if (!Agent || !Item) return;

    // 计算放下位置 (角色前方)
    FVector DropLocation = Agent->GetActorLocation() + 
        Agent->GetActorForwardVector() * DropForwardOffset;
    DropLocation.Z += 50.f; // 稍微抬高避免穿地

    // 执行放下
    Item->OnDropped(DropLocation);

    UE_LOG(LogTemp, Log, TEXT("%s dropped %s"), *Agent->AgentName, *Item->ItemName);
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("%s dropped %s"), *Agent->AgentName, *Item->ItemName));
    }
}
