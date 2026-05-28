#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"

enum class EMASceneGraphFeedbackKind : uint8
{
    None,
    Info,
    Success,
    Warning,
    Error
};

struct MULTIAGENT_API FMASceneGraphMutationFeedback
{
    EMASceneGraphFeedbackKind Kind = EMASceneGraphFeedbackKind::None;
    FString Message;
    bool bSuccess = false;
    bool bClearSelection = false;
    bool bRefreshSceneList = false;
    bool bRefreshSelection = false;
    bool bReloadSceneVisualization = false;
    bool bClearHighlightedActor = false;
};

struct MULTIAGENT_API FMASceneGraphNodesFeedback
{
    bool bSuccess = false;
    FString Message;
    TArray<FMASceneGraphNode> Nodes;
};

struct MULTIAGENT_API FMASceneGraphNodeFeedback
{
    bool bSuccess = false;
    FString Message;
    FMASceneGraphNode Node;
};

namespace MASceneGraphFeedback
{
inline FMASceneGraphMutationFeedback MakeMutationFailure(const FString& Message)
{
    FMASceneGraphMutationFeedback Feedback;
    Feedback.Kind = EMASceneGraphFeedbackKind::Error;
    Feedback.Message = Message;
    return Feedback;
}

inline FMASceneGraphMutationFeedback MakeMutationWarning(
    const FString& Message,
    const bool bClearSelection = false,
    const bool bClearHighlightedActor = false)
{
    FMASceneGraphMutationFeedback Feedback;
    Feedback.Kind = EMASceneGraphFeedbackKind::Warning;
    Feedback.Message = Message;
    Feedback.bClearSelection = bClearSelection;
    Feedback.bClearHighlightedActor = bClearHighlightedActor;
    return Feedback;
}

inline FMASceneGraphMutationFeedback MakeMutationSuccess(
    const FString& Message,
    const bool bClearSelection = false,
    const bool bRefreshSceneList = false,
    const bool bRefreshSelection = false,
    const bool bReloadSceneVisualization = false,
    const bool bClearHighlightedActor = false)
{
    FMASceneGraphMutationFeedback Feedback;
    Feedback.Kind = EMASceneGraphFeedbackKind::Success;
    Feedback.Message = Message;
    Feedback.bSuccess = true;
    Feedback.bClearSelection = bClearSelection;
    Feedback.bRefreshSceneList = bRefreshSceneList;
    Feedback.bRefreshSelection = bRefreshSelection;
    Feedback.bReloadSceneVisualization = bReloadSceneVisualization;
    Feedback.bClearHighlightedActor = bClearHighlightedActor;
    return Feedback;
}

inline FMASceneGraphNodesFeedback MakeNodesFailure(const FString& Message)
{
    FMASceneGraphNodesFeedback Feedback;
    Feedback.Message = Message;
    return Feedback;
}

inline FMASceneGraphNodesFeedback MakeNodesSuccess(const TArray<FMASceneGraphNode>& Nodes)
{
    FMASceneGraphNodesFeedback Feedback;
    Feedback.bSuccess = true;
    Feedback.Nodes = Nodes;
    return Feedback;
}

inline FMASceneGraphNodeFeedback MakeNodeFailure(const FString& Message)
{
    FMASceneGraphNodeFeedback Feedback;
    Feedback.Message = Message;
    return Feedback;
}

inline FMASceneGraphNodeFeedback MakeNodeSuccess(const FMASceneGraphNode& Node)
{
    FMASceneGraphNodeFeedback Feedback;
    Feedback.bSuccess = Node.IsValid();
    Feedback.Node = Node;
    if (!Feedback.bSuccess)
    {
        Feedback.Message = TEXT("Scene graph node not found");
    }
    return Feedback;
}
}
