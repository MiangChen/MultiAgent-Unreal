// MASkillTags.cpp

#include "MASkillTags.h"
#include "Core/Command/Domain/MACommandTags.h"
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
    SkillTags.Command_Idle = FMACommandTags::ToTag(EMACommand::Idle);
    SkillTags.Command_Navigate = FMACommandTags::ToTag(EMACommand::Navigate);
    SkillTags.Command_Follow = FMACommandTags::ToTag(EMACommand::Follow);
    SkillTags.Command_Charge = FMACommandTags::ToTag(EMACommand::Charge);
    SkillTags.Command_Search = FMACommandTags::ToTag(EMACommand::Search);
    SkillTags.Command_Place = FMACommandTags::ToTag(EMACommand::Place);
    SkillTags.Command_TakeOff = FMACommandTags::ToTag(EMACommand::TakeOff);
    SkillTags.Command_Land = FMACommandTags::ToTag(EMACommand::Land);
    SkillTags.Command_ReturnHome = FMACommandTags::ToTag(EMACommand::ReturnHome);
    SkillTags.Command_TakePhoto = FMACommandTags::ToTag(EMACommand::TakePhoto);
    SkillTags.Command_Broadcast = FMACommandTags::ToTag(EMACommand::Broadcast);
    SkillTags.Command_HandleHazard = FMACommandTags::ToTag(EMACommand::HandleHazard);
    SkillTags.Command_Guide = FMACommandTags::ToTag(EMACommand::Guide);

    UE_LOG(LogTemp, Log, TEXT("MASkillTags initialized"));
}
