#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAModifyWidgetModel.h"

class AActor;
class UWorld;

class MULTIAGENT_API FMAModifyWidgetSceneGraphAdapter
{
public:
    bool FindMatchingNodes(UWorld* World, const FString& ActorGuid, TArray<FMASceneGraphNode>& OutNodes, FString& OutError) const;
    bool FindSharedNodeForActors(UWorld* World, const TArray<AActor*>& Actors, FMASceneGraphNode& OutNode, FString& OutError) const;

    FMAModifyPreviewModel BuildSingleActorPreview(UWorld* World, AActor* Actor) const;
    FMAModifyPreviewModel BuildMultiActorPreview(const TArray<AActor*>& Actors) const;

    FString BuildEditNodeJson(UWorld* World, const FMAAnnotationInput& Input, const FString& NodeId) const;
};
