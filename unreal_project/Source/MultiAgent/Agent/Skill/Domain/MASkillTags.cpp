// MASkillTags.cpp

#include "MASkillTags.h"
#include "GameplayTagsManager.h"

FMASkillTags FMASkillTags::SkillTags;
static bool bTagsInitialized = false;

void FMASkillTags::InitializeNativeTags()
{
    if (bTagsInitialized) return;
    bTagsInitialized = true;

    // Skill Tags
    SkillTags.Skill_Navigate = FGameplayTag::RequestGameplayTag(FName("Skill.Navigate"));
    SkillTags.Skill_Follow = FGameplayTag::RequestGameplayTag(FName("Skill.Follow"));
    SkillTags.Skill_Charge = FGameplayTag::RequestGameplayTag(FName("Skill.Charge"));
    SkillTags.Skill_Search = FGameplayTag::RequestGameplayTag(FName("Skill.Search"));
    SkillTags.Skill_Place = FGameplayTag::RequestGameplayTag(FName("Skill.Place"));
    SkillTags.Skill_TakePhoto = FGameplayTag::RequestGameplayTag(FName("Skill.TakePhoto"));
    SkillTags.Skill_Broadcast = FGameplayTag::RequestGameplayTag(FName("Skill.Broadcast"));
    SkillTags.Skill_HandleHazard = FGameplayTag::RequestGameplayTag(FName("Skill.HandleHazard"));
    SkillTags.Skill_Guide = FGameplayTag::RequestGameplayTag(FName("Skill.Guide"));

    // Status Tags
    SkillTags.Status_Moving = FGameplayTag::RequestGameplayTag(FName("Status.Moving"));
    SkillTags.Status_Searching = FGameplayTag::RequestGameplayTag(FName("Status.Searching"));
    SkillTags.Status_Charging = FGameplayTag::RequestGameplayTag(FName("Status.Charging"));

    // Command Tags
    SkillTags.Command_Idle = FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
    SkillTags.Command_Navigate = FGameplayTag::RequestGameplayTag(FName("Command.Navigate"));
    SkillTags.Command_Follow = FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
    SkillTags.Command_Charge = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
    SkillTags.Command_Search = FGameplayTag::RequestGameplayTag(FName("Command.Search"));
    SkillTags.Command_Place = FGameplayTag::RequestGameplayTag(FName("Command.Place"));
    SkillTags.Command_TakePhoto = FGameplayTag::RequestGameplayTag(FName("Command.TakePhoto"));
    SkillTags.Command_Broadcast = FGameplayTag::RequestGameplayTag(FName("Command.Broadcast"));
    SkillTags.Command_HandleHazard = FGameplayTag::RequestGameplayTag(FName("Command.HandleHazard"));
    SkillTags.Command_Guide = FGameplayTag::RequestGameplayTag(FName("Command.Guide"));

    UE_LOG(LogTemp, Log, TEXT("MASkillTags initialized"));
}
