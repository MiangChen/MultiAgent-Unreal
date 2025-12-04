// GA_Pickup.cpp
// 标准拾取流程：禁用物理 -> 禁用碰撞 -> Attach to Socket

#include "GA_Pickup.h"
#include "../MAGameplayTags.h"
#include "../MAAbilitySystemComponent.h"
#include "../../Characters/MACharacter.h"
#include "../../Actors/MAPickupItem.h"
#include "Kismet/GameplayStatics.h"

UGA_Pickup::UGA_Pickup()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Pickup);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().State_Interaction_Pickup);
    BlockAbilitiesWithTag.AddTag(FMAGameplayTags::Get().Ability_Drop);
    ActivationBlockedTags.AddTag(FMAGameplayTags::Get().Status_Holding);
}

bool UGA_Pickup::CanActivateAbility(
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
    return FindNearestPickupItem() != nullptr;
}

void UGA_Pickup::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    BroadcastGameplayEvent(FMAGameplayTags::Get().Event_Pickup_Start);

    if (AMAPickupItem* Item = FindNearestPickupItem())
    {
        PerformPickup(Item);
        
        // 显示头顶状态
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowAbilityStatus(TEXT("Pickup"), Item->ItemName);
        }
        
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->AddLooseGameplayTag(FMAGameplayTags::Get().Status_Holding);
        }
        
        BroadcastGameplayEvent(FMAGameplayTags::Get().Event_Pickup_End);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Pickup::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AMAPickupItem* UGA_Pickup::FindNearestPickupItem() const
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return nullptr;

    FVector CharacterLocation = Character->GetActorLocation();
    AMAPickupItem* NearestItem = nullptr;
    float NearestDistance = PickupRadius;

    TArray<AActor*> FoundItems;
    UGameplayStatics::GetAllActorsOfClass(Character->GetWorld(), AMAPickupItem::StaticClass(), FoundItems);

    for (AActor* Actor : FoundItems)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (Item && Item->bCanBePickedUp)
        {
            float Distance = FVector::Dist(CharacterLocation, Item->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestItem = Item;
            }
        }
    }

    return NearestItem;
}

void UGA_Pickup::PerformPickup(AMAPickupItem* Item)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Item) return;

    UStaticMeshComponent* MeshComp = Item->GetMeshComponent();
    USkeletalMeshComponent* CharMesh = Character->GetMesh();
    
    if (!MeshComp || !CharMesh) return;
    
    if (!CharMesh->DoesSocketExist(AttachSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Pickup] Socket '%s' not found"), *AttachSocketName.ToString());
        return;
    }

    // 关闭物理和碰撞
    MeshComp->SetSimulatePhysics(false);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Item->bCanBePickedUp = false;

    // 附着到角色 Socket
    FAttachmentTransformRules AttachRules(
        EAttachmentRule::SnapToTarget,
        EAttachmentRule::SnapToTarget,
        EAttachmentRule::KeepWorld,
        true
    );
    Item->AttachToComponent(CharMesh, AttachRules, AttachSocketName);
    
    UE_LOG(LogTemp, Log, TEXT("[Pickup] %s picked up %s"), *Character->ActorName, *Item->ItemName);
}
