// GA_Drop.cpp
// 标准丢弃流程：Detach -> 恢复物理 -> 恢复碰撞

#include "GA_Drop.h"
#include "../MAGameplayTags.h"
#include "../MAAbilitySystemComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Actor/MAPickupItem.h"

UGA_Drop::UGA_Drop()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Drop);
    SetAssetTags(AssetTags);
    
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
        // 显示头顶状态
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowAbilityStatus(TEXT("Drop"), Item->ItemName);
        }
        
        PerformDrop(Item);
        
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->RemoveLooseGameplayTag(FMAGameplayTags::Get().Status_Holding);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

AMAPickupItem* UGA_Drop::FindHeldItem() const
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return nullptr;

    TArray<AActor*> AttachedActors;
    Character->GetAttachedActors(AttachedActors);

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
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Item) return;

    // ========== 第一步：Detach，保持世界位置 ==========
    FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
    Item->DetachFromActor(DetachRules);

    // ========== 第二步：设置放下位置 ==========
    FVector DropLocation = Character->GetActorLocation() + 
        Character->GetActorForwardVector() * DropForwardOffset;
    DropLocation.Z += 50.f;
    Item->SetActorLocation(DropLocation);

    // ========== 第三步：恢复物理和碰撞 ==========
    UStaticMeshComponent* MeshComp = Item->GetMeshComponent();
    if (MeshComp)
    {
        // 重新开启碰撞
        MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        // 重新开启物理模拟
        MeshComp->SetSimulatePhysics(true);
    }
    
    // 标记为可拾取
    Item->bCanBePickedUp = true;

    UE_LOG(LogTemp, Log, TEXT("[Drop] %s dropped %s at (%.0f, %.0f, %.0f)"), 
        *Character->ActorName, *Item->ItemName, DropLocation.X, DropLocation.Y, DropLocation.Z);
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("%s dropped %s"), *Character->ActorName, *Item->ItemName));
    }
}
