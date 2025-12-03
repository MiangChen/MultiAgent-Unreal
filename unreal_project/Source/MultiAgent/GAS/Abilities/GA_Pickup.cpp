// GA_Pickup.cpp
// UE 5.5 风格

#include "GA_Pickup.h"
#include "../MAGameplayTags.h"
#include "../MAAbilitySystemComponent.h"
#include "../../AgentManager/MAAgent.h"
#include "../../Interaction/MAPickupItem.h"
#include "Kismet/GameplayStatics.h"

UGA_Pickup::UGA_Pickup()
{
    // UE 5.5: 使用 SetAssetTags 设置 Ability 标识
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Pickup);
    SetAssetTags(AssetTags);
    
    // 激活时添加的 Tags
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().State_Interaction_Pickup);
    
    // 阻止同时激活的 Tags
    BlockAbilitiesWithTag.AddTag(FMAGameplayTags::Get().Ability_Drop);
    
    // 激活需要的 Tags (必须在可拾取范围内)
    ActivationRequiredTags.AddTag(FMAGameplayTags::Get().Status_CanPickup);
    
    // 阻止激活的 Tags (已经持有物品时不能再拾取)
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

    // 检查是否有可拾取物品
    return FindNearestPickupItem() != nullptr;
}

void UGA_Pickup::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 发送开始事件 (通知 State Tree)
    BroadcastGameplayEvent(FMAGameplayTags::Get().Event_Pickup_Start);

    // 查找并拾取物品
    if (AMAPickupItem* Item = FindNearestPickupItem())
    {
        PerformPickup(Item);
        
        // 添加 Holding Tag
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
        {
            ASC->AddLooseGameplayTag(FMAGameplayTags::Get().Status_Holding);
        }
        
        // 发送完成事件
        BroadcastGameplayEvent(FMAGameplayTags::Get().Event_Pickup_End);
    }

    // 结束技能
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
    AMAAgent* Agent = GetOwningAgent();
    if (!Agent) return nullptr;

    FVector AgentLocation = Agent->GetActorLocation();
    AMAPickupItem* NearestItem = nullptr;
    float NearestDistance = PickupRadius;

    // 查找所有 PickupItem
    TArray<AActor*> FoundItems;
    UGameplayStatics::GetAllActorsOfClass(Agent->GetWorld(), AMAPickupItem::StaticClass(), FoundItems);

    for (AActor* Actor : FoundItems)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (Item && Item->bCanBePickedUp)
        {
            float Distance = FVector::Dist(AgentLocation, Item->GetActorLocation());
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
    AMAAgent* Agent = GetOwningAgent();
    if (!Agent || !Item) return;

    // 通知物品被拾取
    Item->OnPickedUp(Agent);

    // 附着到角色的手部 Socket
    USkeletalMeshComponent* Mesh = Agent->GetMesh();
    if (Mesh && Mesh->DoesSocketExist(AttachSocketName))
    {
        Item->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
    }
    else
    {
        // 如果没有 Socket，附着到 Actor 本身
        Item->AttachToActor(Agent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Item->SetActorRelativeLocation(FVector(50.f, 0.f, 50.f)); // 前方偏移
    }

    UE_LOG(LogTemp, Log, TEXT("%s picked up %s"), *Agent->AgentName, *Item->ItemName);
    
    // 屏幕提示
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("%s picked up %s"), *Agent->AgentName, *Item->ItemName));
    }
}
