// MAGameplayTags.cpp

#include "MAGameplayTags.h"
#include "GameplayTagsManager.h"

FMAGameplayTags FMAGameplayTags::GameplayTags;

void FMAGameplayTags::InitializeNativeTags()
{
    UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

    // State Tags
    GameplayTags.State_Exploration = Manager.AddNativeGameplayTag(
        FName("State.Exploration"), 
        FString("Default exploration state"));
    
    GameplayTags.State_PhotoMode = Manager.AddNativeGameplayTag(
        FName("State.PhotoMode"), 
        FString("Photo mode state"));
    
    GameplayTags.State_Interaction = Manager.AddNativeGameplayTag(
        FName("State.Interaction"), 
        FString("Interaction state"));
    
    GameplayTags.State_Interaction_Pickup = Manager.AddNativeGameplayTag(
        FName("State.Interaction.Pickup"), 
        FString("Pickup sub-state"));
    
    GameplayTags.State_Interaction_Dialogue = Manager.AddNativeGameplayTag(
        FName("State.Interaction.Dialogue"), 
        FString("Dialogue sub-state"));

    // Ability Tags
    GameplayTags.Ability_Pickup = Manager.AddNativeGameplayTag(
        FName("Ability.Pickup"), 
        FString("Pickup ability"));
    
    GameplayTags.Ability_Drop = Manager.AddNativeGameplayTag(
        FName("Ability.Drop"), 
        FString("Drop ability"));
    
    GameplayTags.Ability_TakePhoto = Manager.AddNativeGameplayTag(
        FName("Ability.TakePhoto"), 
        FString("Take photo ability"));
    
    GameplayTags.Ability_Navigate = Manager.AddNativeGameplayTag(
        FName("Ability.Navigate"), 
        FString("Navigate ability"));
    
    GameplayTags.Ability_Interact = Manager.AddNativeGameplayTag(
        FName("Ability.Interact"), 
        FString("Interact ability"));

    // Event Tags
    GameplayTags.Event_Pickup_Start = Manager.AddNativeGameplayTag(
        FName("Event.Pickup.Start"), 
        FString("Pickup started event"));
    
    GameplayTags.Event_Pickup_End = Manager.AddNativeGameplayTag(
        FName("Event.Pickup.End"), 
        FString("Pickup ended event"));
    
    GameplayTags.Event_Photo_Start = Manager.AddNativeGameplayTag(
        FName("Event.Photo.Start"), 
        FString("Photo started event"));
    
    GameplayTags.Event_Photo_End = Manager.AddNativeGameplayTag(
        FName("Event.Photo.End"), 
        FString("Photo ended event"));

    // Status Tags
    GameplayTags.Status_CanPickup = Manager.AddNativeGameplayTag(
        FName("Status.CanPickup"), 
        FString("Can pickup nearby item"));
    
    GameplayTags.Status_Holding = Manager.AddNativeGameplayTag(
        FName("Status.Holding"), 
        FString("Currently holding an item"));
    
    GameplayTags.Status_Moving = Manager.AddNativeGameplayTag(
        FName("Status.Moving"), 
        FString("Currently moving"));
}
