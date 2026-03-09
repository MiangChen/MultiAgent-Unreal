#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAEditWidgetModel.h"

class AActor;
class UWorld;

class MULTIAGENT_API FMAEditWidgetSceneGraphAdapter
{
public:
    bool ResolveActorNodes(UWorld* World, AActor* Actor, TArray<FMASceneGraphNode>& OutNodes, FString& OutError) const;
    bool ValidateJsonDocument(const FString& Json, FString& OutError) const;

    FString BuildEditableJson(const FMASceneGraphNode& Node) const;
    bool IsPointTypeNode(const FMASceneGraphNode& Node) const;
    bool IsGoalOrZoneActor(const AActor* Actor) const;
    bool IsNodeMarkedGoal(const FMASceneGraphNode& Node) const;
};
