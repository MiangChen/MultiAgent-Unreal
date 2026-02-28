// MASceneGraphCommandUseCases.h
// SceneGraph 写用例 (P2a 进行中)

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphNodeTypes.h"

class FMASceneGraphCommandUseCases
{
public:
    enum class ESavePrecondition : uint8
    {
        Ok,
        DisabledByRunMode,
        InvalidWorkingCopy,
        EmptySourcePath
    };

    enum class ESaveResult : uint8
    {
        Saved,
        SkippedByRunMode,
        InvalidWorkingCopy,
        EmptySourcePath,
        PersistFailed
    };

    struct FStaticGraphContext
    {
        TSharedPtr<FJsonObject>& WorkingCopy;
        TArray<FMASceneGraphNode>& StaticNodes;
    };

    struct FDynamicGraphContext
    {
        TArray<FMASceneGraphNode>& DynamicNodes;
    };

    static bool AddStaticNode(
        FStaticGraphContext& Context,
        const TSharedPtr<FJsonObject>& NodeObject,
        FString& OutError);

    static bool DeleteStaticNodeById(
        FStaticGraphContext& Context,
        const FString& NodeId,
        FString& OutDeletedNodeJson,
        FString& OutError);

    static bool ReplaceStaticNodeById(
        FStaticGraphContext& Context,
        const FString& NodeId,
        const TSharedPtr<FJsonObject>& NewNodeObject,
        FString& OutError);

    static bool BuildNodeJsonWithGoalFlag(
        FStaticGraphContext& Context,
        const FString& NodeId,
        bool bIsGoal,
        FString& OutModifiedNodeJson,
        FString& OutError);

    static bool UpdateDynamicNodePosition(
        FDynamicGraphContext& Context,
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& NodeId,
        const FVector& NewPosition,
        FString& OutLocationLabel,
        FString& OutError);

    static bool UpdateDynamicNodeFeature(
        FDynamicGraphContext& Context,
        const FString& NodeId,
        const FString& Key,
        const FString& Value,
        FString& OutError);

    static ESavePrecondition CheckSavePreconditions(
        bool bCanPersistToSource,
        const TSharedPtr<FJsonObject>& WorkingCopy,
        const FString& SourcePath);

    static ESaveResult SaveWorkingCopy(
        bool bCanPersistToSource,
        const TSharedPtr<FJsonObject>& WorkingCopy,
        const FString& SourcePath,
        const TFunctionRef<bool(const TSharedPtr<FJsonObject>&)>& PersistFn);

private:
    static TArray<TSharedPtr<FJsonValue>>* GetNodesArrayMutable(const TSharedPtr<FJsonObject>& WorkingCopy);
    static FMASceneGraphNode* FindDynamicNodeByIdMutable(
        FDynamicGraphContext& Context,
        const FString& NodeId);
};
