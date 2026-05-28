#pragma once

#include "MAFeedbackGenerator.h"

#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"

namespace MAFeedbackGeneratorInternal
{
inline float FeedbackDistanceMeters(const float DistanceCm)
{
    return DistanceCm / 100.0f;
}

inline void AddDurationSecondsField(FMASkillExecutionFeedback& Feedback, const float DurationSeconds)
{
    if (DurationSeconds > 0.f)
    {
        Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), DurationSeconds));
    }
}

inline void AddSceneGraphNodeFields(
    FMASkillExecutionFeedback& Feedback,
    const TArray<FMASceneGraphNode>& AllNodes,
    const TCHAR* IdField,
    const TCHAR* LabelField,
    const FString& NodeId,
    const FString& FallbackLabel = FString())
{
    if (!NodeId.IsEmpty())
    {
        Feedback.Data.Add(IdField, NodeId);
        const FString NodeLabel = FMASceneGraphQueryUseCases::GetLabelById(AllNodes, NodeId);
        Feedback.Data.Add(LabelField, NodeLabel.IsEmpty() ? FallbackLabel : NodeLabel);
        return;
    }

    if (!FallbackLabel.IsEmpty())
    {
        Feedback.Data.Add(LabelField, FallbackLabel);
    }
}
}
