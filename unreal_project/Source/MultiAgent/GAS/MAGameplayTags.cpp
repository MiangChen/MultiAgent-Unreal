// MAGameplayTags.cpp

#include "MAGameplayTags.h"
#include "GameplayTagsManager.h"

FMAGameplayTags FMAGameplayTags::GameplayTags;

// 静态初始化标记
static bool bTagsInitialized = false;

void FMAGameplayTags::InitializeNativeTags()
{
    // 防止重复初始化
    if (bTagsInitialized)
    {
        return;
    }
    bTagsInitialized = true;

    // 使用 RequestGameplayTag 而不是 AddNativeGameplayTag
    // 这样更安全，不会在 Native Tags 阶段结束后报错
    
    // State Tags
    GameplayTags.State_Exploration = FGameplayTag::RequestGameplayTag(FName("State.Exploration"));
    GameplayTags.State_PhotoMode = FGameplayTag::RequestGameplayTag(FName("State.PhotoMode"));
    GameplayTags.State_Interaction = FGameplayTag::RequestGameplayTag(FName("State.Interaction"));
    GameplayTags.State_Interaction_Pickup = FGameplayTag::RequestGameplayTag(FName("State.Interaction.Pickup"));
    GameplayTags.State_Interaction_Dialogue = FGameplayTag::RequestGameplayTag(FName("State.Interaction.Dialogue"));

    // Ability Tags
    GameplayTags.Ability_Pickup = FGameplayTag::RequestGameplayTag(FName("Ability.Pickup"));
    GameplayTags.Ability_Drop = FGameplayTag::RequestGameplayTag(FName("Ability.Drop"));
    GameplayTags.Ability_TakePhoto = FGameplayTag::RequestGameplayTag(FName("Ability.TakePhoto"));
    GameplayTags.Ability_Navigate = FGameplayTag::RequestGameplayTag(FName("Ability.Navigate"));
    GameplayTags.Ability_Interact = FGameplayTag::RequestGameplayTag(FName("Ability.Interact"));

    // Event Tags
    GameplayTags.Event_Pickup_Start = FGameplayTag::RequestGameplayTag(FName("Event.Pickup.Start"));
    GameplayTags.Event_Pickup_End = FGameplayTag::RequestGameplayTag(FName("Event.Pickup.End"));
    GameplayTags.Event_Photo_Start = FGameplayTag::RequestGameplayTag(FName("Event.Photo.Start"));
    GameplayTags.Event_Photo_End = FGameplayTag::RequestGameplayTag(FName("Event.Photo.End"));

    // Status Tags
    GameplayTags.Status_CanPickup = FGameplayTag::RequestGameplayTag(FName("Status.CanPickup"));
    GameplayTags.Status_Holding = FGameplayTag::RequestGameplayTag(FName("Status.Holding"));
    GameplayTags.Status_Moving = FGameplayTag::RequestGameplayTag(FName("Status.Moving"));
    
    UE_LOG(LogTemp, Log, TEXT("MAGameplayTags initialized"));
}
