// MASceneGraphCommandUseCases.h
// SceneGraph 写用例 (P2a 进行中)

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphNodeTypes.h"

class IMASceneGraphEventPublisherPort;
enum class EMASceneChangeType : uint8;

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

    struct FSaveResultReport
    {
        bool bSuccess = false;
        ELogVerbosity::Type Verbosity = ELogVerbosity::Error;
        FString Message;
    };

    struct FSceneChangePublishReport
    {
        bool bPublished = false;
        ELogVerbosity::Type Verbosity = ELogVerbosity::Warning;
        FString Message;
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

    struct FAddPointSceneNodeInput
    {
        FString Id;
        FString Type;
        FString Label;
        FVector WorldLocation = FVector::ZeroVector;
        FString Guid;
    };

    struct FResolvedLabelInput
    {
        FString Id;
        FString Type;
        bool bHasCategory = false;
        bool bUsedAutoId = false;
    };

    static bool ResolveLabelInput(
        const FString& InputText,
        const TFunctionRef<FString()>& NextIdProvider,
        FResolvedLabelInput& OutResolved,
        FString& OutError);

    static bool ContainsNodeId(
        const TArray<FMASceneGraphNode>& StaticNodes,
        const FString& NodeId);

    static int32 CountNodeType(
        const TArray<FMASceneGraphNode>& StaticNodes,
        const FString& Type);

    static FString BuildGeneratedLabel(
        const TArray<FMASceneGraphNode>& StaticNodes,
        const FString& Type);

    static FString CalculateNextNumericId(const TArray<FMASceneGraphNode>& StaticNodes);

    static bool AddPointSceneNode(
        const FAddPointSceneNodeInput& Input,
        const TFunctionRef<bool(const FString&, FString&)>& AddNodeFn,
        const TFunctionRef<bool()>& SaveToSourceFn,
        FString& OutError);

    static bool AddStaticNode(
        FStaticGraphContext& Context,
        const TSharedPtr<FJsonObject>& NodeObject,
        FString& OutError);

    static bool AddStaticNodeFromJson(
        FStaticGraphContext& Context,
        const FString& NodeJson,
        const TFunctionRef<bool(const FString&)>& IsIdExistsFn,
        FString& OutNodeId,
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

    static bool ReplaceStaticNodeByIdFromJson(
        FStaticGraphContext& Context,
        const FString& NodeId,
        const FString& NewNodeJson,
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

    static FMASceneGraphNode* FindDynamicNodeByIdOrLabelMutable(
        FDynamicGraphContext& Context,
        const FString& NodeIdOrLabel);

    static bool HasDynamicNode(
        FDynamicGraphContext& Context,
        const FString& NodeIdOrLabel);

    static bool BindDynamicNodeGuid(
        FDynamicGraphContext& Context,
        const FString& NodeIdOrLabel,
        const FString& ActorGuid,
        FString& OutBoundNodeId,
        FString& OutBoundNodeLabel,
        FString& OutError);

    static bool ParseAndValidateNodeJson(
        const FString& NodeJson,
        TSharedPtr<FJsonObject>& OutNodeObject,
        FString& OutError);

    static bool PrepareStaticNodeForAdd(
        const FString& NodeJson,
        const TFunctionRef<bool(const FString&)>& IsIdExistsFn,
        TSharedPtr<FJsonObject>& OutNodeObject,
        FString& OutNodeId,
        FString& OutError);

    static bool EditDynamicNodeFromJson(
        FDynamicGraphContext& Context,
        const FString& NodeId,
        const TSharedPtr<FJsonObject>& NewNodeObject,
        FString& OutError);

    static bool ValidateEditDynamicNodeInput(
        const FString& NodeId,
        const FString& NewNodeJson,
        FString& OutError);

    static bool EditDynamicNodeFromJsonString(
        FDynamicGraphContext& Context,
        const FString& NodeId,
        const FString& NewNodeJson,
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

    static FSaveResultReport BuildSaveResultReport(
        ESaveResult SaveResult,
        const FString& SourcePath);

    static FSceneChangePublishReport PublishSceneChange(
        IMASceneGraphEventPublisherPort* EventPublisherPort,
        EMASceneChangeType ChangeType,
        const FString& Payload);

private:
    static FString CapitalizeFirstLetter(const FString& Type);
    static TArray<TSharedPtr<FJsonValue>>* GetNodesArrayMutable(const TSharedPtr<FJsonObject>& WorkingCopy);
    static FMASceneGraphNode* FindDynamicNodeByIdMutable(
        FDynamicGraphContext& Context,
        const FString& NodeId);
};
