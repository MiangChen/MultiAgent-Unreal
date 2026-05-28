#include "Agent/Skill/Infrastructure/MAFeedbackGeneratorInternal.h"

#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"

void FMAFeedbackGenerator::GenerateTakePhotoFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);

        Feedback.Data.Add(TEXT("target_found"), Context.bPhotoTargetFound ? TEXT("true") : TEXT("false"));

        if (Context.bPhotoTargetFound)
        {
            MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
                Feedback,
                AllNodes,
                TEXT("target_id"),
                TEXT("target_label"),
                Context.PhotoTargetId,
                Context.PhotoTargetName);

            if (Context.PhotoRobotTargetDistance >= 0.f)
            {
                Feedback.Data.Add(TEXT("robot_target_distance"), FString::Printf(TEXT("%.2f"), Context.PhotoRobotTargetDistance));
            }
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FString TargetLabel = Feedback.Data.FindRef(TEXT("target_label"));

        if (bSuccess)
        {
            if (Context.bPhotoTargetFound && !TargetLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Photo taken successfully, found %s"), *TargetLabel);
            }
            else if (Context.bPhotoTargetFound)
            {
                Feedback.Message = TEXT("Photo taken successfully, found target");
            }
            else
            {
                Feedback.Message = TEXT("Photo taken successfully, but target not found");
            }
        }
        else
        {
            Feedback.Message = TEXT("Photo failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Photo taken successfully") : TEXT("Photo failed");
    }
}

void FMAFeedbackGenerator::GenerateBroadcastFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        if (!Context.BroadcastMessage.IsEmpty())
        {
            Feedback.Data.Add(TEXT("message"), Context.BroadcastMessage);
        }

        Feedback.Data.Add(TEXT("target_found"), Context.bBroadcastTargetFound ? TEXT("true") : TEXT("false"));

        if (Context.bBroadcastTargetFound && !Context.BroadcastTargetName.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_name"), Context.BroadcastTargetName);
        }

        if (Context.BroadcastRobotTargetDistance >= 0.f)
        {
            Feedback.Data.Add(TEXT("robot_target_distance"), FString::Printf(TEXT("%.2f"), Context.BroadcastRobotTargetDistance));
        }

        MAFeedbackGeneratorInternal::AddDurationSecondsField(Feedback, Context.BroadcastDurationSeconds);
    }

    Feedback.Message = Message.IsEmpty() ? (bSuccess ? TEXT("Broadcast succeeded") : TEXT("Broadcast failed")) : Message;
}

void FMAFeedbackGenerator::GenerateHandleHazardFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);

        Feedback.Data.Add(TEXT("target_found"), Context.bHazardTargetFound ? TEXT("true") : TEXT("false"));

        if (Context.bHazardTargetFound)
        {
            MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
                Feedback,
                AllNodes,
                TEXT("target_id"),
                TEXT("target_label"),
                Context.HazardTargetId,
                Context.HazardTargetName);
        }

        MAFeedbackGeneratorInternal::AddDurationSecondsField(Feedback, Context.HazardHandleDurationSeconds);
    }

    Feedback.Message = Message.IsEmpty()
        ? (bSuccess ? TEXT("Hazard handled successfully") : TEXT("Hazard handling failed"))
        : Message;
}

void FMAFeedbackGenerator::GenerateGuideFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);

        Feedback.Data.Add(TEXT("target_found"), Context.bGuideTargetFound ? TEXT("true") : TEXT("false"));

        if (Context.bGuideTargetFound)
        {
            MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
                Feedback,
                AllNodes,
                TEXT("target_id"),
                TEXT("target_label"),
                Context.GuideTargetId,
                Context.GuideTargetName);
        }

        if (!Context.GuideDestination.IsZero())
        {
            Feedback.Data.Add(TEXT("destination"), FString::Printf(TEXT("(%.1f, %.1f, %.1f)"),
                Context.GuideDestination.X, Context.GuideDestination.Y, Context.GuideDestination.Z));
        }

        MAFeedbackGeneratorInternal::AddDurationSecondsField(Feedback, Context.GuideDurationSeconds);
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        Feedback.Message = (bSuccess && !Context.GuideDestination.IsZero())
            ? FString::Printf(TEXT("Guide succeeded, target guided to (%.0f, %.0f, %.0f)"),
                Context.GuideDestination.X, Context.GuideDestination.Y, Context.GuideDestination.Z)
            : (bSuccess ? TEXT("Guide succeeded") : TEXT("Guide failed"));
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Guide succeeded") : TEXT("Guide failed");
    }
}
