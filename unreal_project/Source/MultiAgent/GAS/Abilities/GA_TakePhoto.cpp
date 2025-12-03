// GA_TakePhoto.cpp
// 拍照技能实现

#include "GA_TakePhoto.h"
#include "../MAGameplayTags.h"
#include "../../AgentManager/MACameraAgent.h"

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
    
    // 检查是否是 Camera Agent
    return GetCameraAgent() != nullptr;
}

void UGA_TakePhoto::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    AMACameraAgent* Camera = GetCameraAgent();
    if (Camera)
    {
        bool bSuccess = Camera->TakePhoto(SavePath);
        
        if (bSuccess)
        {
            BroadcastGameplayEvent(FMAGameplayTags::Get().Event_Photo_End);
        }
        
        UE_LOG(LogTemp, Log, TEXT("[TakePhoto] %s took photo, success=%d"), 
            *Camera->AgentName, bSuccess ? 1 : 0);
    }
    
    // 瞬时技能，立即结束
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

AMACameraAgent* UGA_TakePhoto::GetCameraAgent() const
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (ActorInfo && ActorInfo->AvatarActor.IsValid())
    {
        return Cast<AMACameraAgent>(ActorInfo->AvatarActor.Get());
    }
    return nullptr;
}
