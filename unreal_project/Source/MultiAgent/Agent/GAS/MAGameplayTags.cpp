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
    GameplayTags.Ability_Patrol = FGameplayTag::RequestGameplayTag(FName("Ability.Patrol"));
    GameplayTags.Ability_Search = FGameplayTag::RequestGameplayTag(FName("Ability.Search"));
    GameplayTags.Ability_Observe = FGameplayTag::RequestGameplayTag(FName("Ability.Observe"));
    GameplayTags.Ability_Report = FGameplayTag::RequestGameplayTag(FName("Ability.Report"));
    GameplayTags.Ability_Charge = FGameplayTag::RequestGameplayTag(FName("Ability.Charge"));
    GameplayTags.Ability_Formation = FGameplayTag::RequestGameplayTag(FName("Ability.Formation"));
    GameplayTags.Ability_Avoid = FGameplayTag::RequestGameplayTag(FName("Ability.Avoid"));

    // Event Tags
    GameplayTags.Event_Pickup_Start = FGameplayTag::RequestGameplayTag(FName("Event.Pickup.Start"));
    GameplayTags.Event_Pickup_End = FGameplayTag::RequestGameplayTag(FName("Event.Pickup.End"));
    GameplayTags.Event_Photo_Start = FGameplayTag::RequestGameplayTag(FName("Event.Photo.Start"));
    GameplayTags.Event_Photo_End = FGameplayTag::RequestGameplayTag(FName("Event.Photo.End"));
    GameplayTags.Event_Target_Found = FGameplayTag::RequestGameplayTag(FName("Event.Target.Found"));
    GameplayTags.Event_Target_Lost = FGameplayTag::RequestGameplayTag(FName("Event.Target.Lost"));
    GameplayTags.Event_Charge_Complete = FGameplayTag::RequestGameplayTag(FName("Event.Charge.Complete"));
    GameplayTags.Event_Patrol_WaypointReached = FGameplayTag::RequestGameplayTag(FName("Event.Patrol.WaypointReached"));

    // Status Tags
    GameplayTags.Status_CanPickup = FGameplayTag::RequestGameplayTag(FName("Status.CanPickup"));
    GameplayTags.Status_Holding = FGameplayTag::RequestGameplayTag(FName("Status.Holding"));
    GameplayTags.Status_Moving = FGameplayTag::RequestGameplayTag(FName("Status.Moving"));
    GameplayTags.Status_Patrolling = FGameplayTag::RequestGameplayTag(FName("Status.Patrolling"));
    GameplayTags.Status_Searching = FGameplayTag::RequestGameplayTag(FName("Status.Searching"));
    GameplayTags.Status_Observing = FGameplayTag::RequestGameplayTag(FName("Status.Observing"));
    GameplayTags.Status_Charging = FGameplayTag::RequestGameplayTag(FName("Status.Charging"));
    GameplayTags.Status_InFormation = FGameplayTag::RequestGameplayTag(FName("Status.InFormation"));
    GameplayTags.Status_Avoiding = FGameplayTag::RequestGameplayTag(FName("Status.Avoiding"));
    GameplayTags.Status_LowEnergy = FGameplayTag::RequestGameplayTag(FName("Status.LowEnergy"));

    // Command Tags (外部命令)
    GameplayTags.Command_Patrol = FGameplayTag::RequestGameplayTag(FName("Command.Patrol"));
    GameplayTags.Command_Charge = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
    GameplayTags.Command_Idle = FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
    GameplayTags.Command_Search = FGameplayTag::RequestGameplayTag(FName("Command.Search"));
    
    UE_LOG(LogTemp, Log, TEXT("MAGameplayTags initialized"));
}
