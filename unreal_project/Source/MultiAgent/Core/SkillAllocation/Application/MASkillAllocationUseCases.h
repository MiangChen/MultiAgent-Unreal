#pragma once

#include "CoreMinimal.h"
#include "Core/Comm/Domain/MACommSceneTypes.h"
#include "Core/Comm/Domain/MACommSkillTypes.h"
#include "Core/SkillAllocation/Feedback/MASkillAllocationFeedback.h"

struct MULTIAGENT_API FMASkillAllocationUseCases
{
    static FMASkillAllocationData BuildEmptyData(
        const FString& Name = TEXT("Skill Allocation"),
        const FString& Description = TEXT(""));
    static FMASkillAllocationParseFeedback ParseJson(const FString& JsonString);
    static bool ParseJson(const FString& JsonString, FMASkillAllocationData& OutData, FString& OutError);
    static FString SerializeJson(const FMASkillAllocationData& Data);
    static FMASkillAllocationMessage BuildMessage(const FMASkillAllocationData& Data);
    static FMASkillAllocationSkillListBuildFeedback BuildSkillListMessage(const FMASkillAllocationData& Data);
    static bool TryBuildSkillListMessage(
        const FMASkillAllocationData& Data,
        FMASkillListMessage& OutMessage,
        FString& OutError);
};
