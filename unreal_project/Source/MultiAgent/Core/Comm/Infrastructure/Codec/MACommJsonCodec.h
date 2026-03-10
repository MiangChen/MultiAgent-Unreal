#pragma once

#include "CoreMinimal.h"
#include "../../Domain/MACommTypes.h"

namespace MACommJsonCodec
{
    // Envelope
    FString SerializeEnvelope(const FMAMessageEnvelope& Envelope);
    bool DeserializeEnvelope(const FString& Json, FMAMessageEnvelope& OutEnvelope);

    // Basic payloads
    FString SerializeUIInput(const FMAUIInputMessage& Message);
    bool DeserializeUIInput(const FString& Json, FMAUIInputMessage& OutMessage);
    FString SerializeButtonEvent(const FMAButtonEventMessage& Message);
    bool DeserializeButtonEvent(const FString& Json, FMAButtonEventMessage& OutMessage);
    FString SerializeTaskFeedback(const FMATaskFeedbackMessage& Message);
    bool DeserializeTaskFeedback(const FString& Json, FMATaskFeedbackMessage& OutMessage);

    // TaskPlan / WorldModel
    FString SerializeTaskPlan(const FMATaskPlan& TaskPlan);
    bool DeserializeTaskPlan(const FString& Json, FMATaskPlan& OutTaskPlan);
    bool ValidateTaskPlanDAG(const FMATaskPlan& TaskPlan);
    FString SerializeWorldModelGraph(const FMAWorldModelGraph& Graph);
    bool DeserializeWorldModelGraph(const FString& Json, FMAWorldModelGraph& OutGraph);

    // Skill list and feedback
    bool DeserializeSkillList(const FString& Json, FMASkillListMessage& OutSkillList);
    FString SerializeSkillList(const FMASkillListMessage& SkillList);
    const FMATimeStepCommands* FindSkillListTimeStep(const FMASkillListMessage& SkillList, int32 Step);
    FString SerializeTimeStepFeedback(const FMATimeStepFeedbackMessage& Message);
    FString SerializeSkillListCompleted(const FMASkillListCompletedMessage& Message);

    // Scene / allocation / response
    FString SceneChangeTypeToString(EMASceneChangeType Type);
    EMASceneChangeType SceneChangeTypeFromString(const FString& TypeStr);
    FString SerializeSceneChange(const FMASceneChangeMessage& Message);
    bool DeserializeSceneChange(const FString& Json, FMASceneChangeMessage& OutMessage);
    FString SerializeSkillAllocation(const FMASkillAllocationMessage& Message);
    bool DeserializeSkillAllocation(const FString& Json, FMASkillAllocationMessage& OutMessage);
    FString SerializeSkillStatusUpdate(const FMASkillStatusUpdateMessage& Message);
    bool DeserializeSkillStatusUpdate(const FString& Json, FMASkillStatusUpdateMessage& OutMessage);
    FString SerializeReviewResponse(const FMAReviewResponseMessage& Message);
    bool DeserializeReviewResponse(const FString& Json, FMAReviewResponseMessage& OutMessage);
    FString SerializeDecisionResponse(const FMADecisionResponseMessage& Message);
    bool DeserializeDecisionResponse(const FString& Json, FMADecisionResponseMessage& OutMessage);

    // Factory
    FMAMessageEnvelope MakeOutboundEnvelope(
        EMACommMessageType MessageType,
        EMAMessageCategory MessageCategory,
        const FString& PayloadJson,
        int64 Timestamp = 0,
        const FString& MessageId = TEXT(""));
}
