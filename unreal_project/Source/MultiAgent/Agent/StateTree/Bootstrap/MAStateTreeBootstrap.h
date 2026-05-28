#pragma once

#include "CoreMinimal.h"

class UStateTree;
class UMAStateTreeComponent;
class UWorld;

class MULTIAGENT_API FMAStateTreeBootstrap
{
public:
    static bool ShouldUseStateTree(UWorld* World);
    static void RestartWithAsset(UMAStateTreeComponent& StateTreeComponent, UStateTree* StateTree);
};
