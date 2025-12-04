// GA_TakePhoto.cpp
// 拍照技能实现

#include "GA_TakePhoto.h"
#include "../MAGameplayTags.h"
#include "../../Characters/MACharacter.h"
#include "../../Actors/MACameraSensor.h"

UGA_TakePhoto::UGA_TakePhoto()
{
    // 设置 AssetTags
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_TakePhoto);
    SetAssetTags(AssetTags);
    
    // 瞬时技能，不需要持续
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UGA_TakePhoto::CanActivateAbility(
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
    
    // 检查是否有附着的 Camera Sensor
    return GetAttachedCameraSensor() != nullptr;
}

void UGA_TakePhoto::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    AMACameraSensor* Camera = GetAttachedCameraSensor();
    AMACharacter* Character = GetOwningCharacter();
    
    if (Camera)
    {
        bool bSuccess = Camera->TakePhoto(SavePath);
        
        if (bSuccess)
        {
            BroadcastGameplayEvent(FMAGameplayTags::Get().Event_Photo_End);
            
            if (Character)
            {
                Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("OK"));
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("[TakePhoto] %s took photo via %s, success=%d"), 
            Character ? *Character->ActorName : TEXT("Unknown"),
            *Camera->SensorName, bSuccess ? 1 : 0);
    }
    
    // 瞬时技能，立即结束
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

AMACameraSensor* UGA_TakePhoto::GetAttachedCameraSensor() const
{
    AMACharacter* Character = const_cast<UGA_TakePhoto*>(this)->GetOwningCharacter();
    if (!Character) return nullptr;
    
    // 查找附着的 Camera Sensor
    TArray<AActor*> AttachedActors;
    Character->GetAttachedActors(AttachedActors);
    
    for (AActor* Actor : AttachedActors)
    {
        if (AMACameraSensor* Camera = Cast<AMACameraSensor>(Actor))
        {
            return Camera;
        }
    }
    
    return nullptr;
}
