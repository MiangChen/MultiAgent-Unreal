#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAEditWidgetModel.h"

class AActor;
class UWorld;

class MULTIAGENT_API FMAEditWidgetSceneGraphAdapter
{
public:
    bool ResolveActorNodes(UWorld* World, AActor* Actor, TArray<FMASceneGraphNode>& OutNodes, FString& OutError) const;
};
