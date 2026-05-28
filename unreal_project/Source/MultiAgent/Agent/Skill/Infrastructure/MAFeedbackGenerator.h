// MAFeedbackGenerator.h
// 技能反馈生成器 - 运行时反馈拼装与场景图投影

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Agent/Skill/Feedback/MASkillFeedbackTypes.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"

class AMACharacter;
class UMASkillComponent;

class MULTIAGENT_API FMAFeedbackGenerator
{
public:
    static FMASkillExecutionFeedback Generate(AMACharacter* Agent, EMACommand Command, bool bSuccess, const FString& Message);

    static TSharedPtr<FJsonObject> BuildNodeJsonFromSceneGraph(const FMASceneGraphNode& Node);

    static TSharedPtr<FJsonObject> BuildNodeJsonFromUE5Data(
        const FString& Id,
        const FString& Label,
        const FVector& Location,
        const FString& Type,
        const FString& Category = TEXT(""),
        const TMap<FString, FString>& Features = TMap<FString, FString>());

private:
    static void GenerateNavigateFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateSearchFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateFollowFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateChargeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GeneratePlaceFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateTakeOffFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateLandFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateReturnHomeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateIdleFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, bool bSuccess, const FString& Message);
    static void GenerateTakePhotoFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateBroadcastFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateHandleHazardFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateGuideFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);

    static TArray<FMASceneGraphNode> LoadSceneGraphNodes(AMACharacter* Agent);
    static void AddCommonFieldsToFeedback(FMASkillExecutionFeedback& Feedback, UMASkillComponent* SkillComp, AMACharacter* Agent);
};
