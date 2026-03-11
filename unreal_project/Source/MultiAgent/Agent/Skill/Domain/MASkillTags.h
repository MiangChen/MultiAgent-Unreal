// MASkillTags.h
// 技能系统 Gameplay Tags 定义

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct MULTIAGENT_API FMASkillTags
{
public:
    static const FMASkillTags& Get() { return SkillTags; }
    static void InitializeNativeTags();

    // ========== Skill Tags ==========
    FGameplayTag Skill_Navigate;
    FGameplayTag Skill_Follow;
    FGameplayTag Skill_Charge;
    FGameplayTag Skill_Search;
    FGameplayTag Skill_Place;
    FGameplayTag Skill_TakePhoto;
    FGameplayTag Skill_Broadcast;
    FGameplayTag Skill_HandleHazard;
    FGameplayTag Skill_Guide;

    // ========== Status Tags ==========
    FGameplayTag Status_Moving;
    FGameplayTag Status_Searching;
    FGameplayTag Status_Charging;

    // ========== Command Tags ==========
    FGameplayTag Command_Idle;
    FGameplayTag Command_Navigate;
    FGameplayTag Command_Follow;
    FGameplayTag Command_Charge;
    FGameplayTag Command_Search;
    FGameplayTag Command_Place;
    FGameplayTag Command_TakeOff;
    FGameplayTag Command_Land;
    FGameplayTag Command_ReturnHome;
    FGameplayTag Command_TakePhoto;
    FGameplayTag Command_Broadcast;
    FGameplayTag Command_HandleHazard;
    FGameplayTag Command_Guide;

private:
    static FMASkillTags SkillTags;
};
