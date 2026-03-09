#pragma once

#include "CoreMinimal.h"

class AActor;

class MULTIAGENT_API FMAActorHighlightAdapter
{
public:
    AActor* FindRootActor(AActor* Actor) const;
    void CollectActorTree(AActor* RootActor, TArray<AActor*>& OutActors) const;
    void SetActorTreeHighlight(AActor* Actor, bool bHighlight) const;

private:
    void SetSingleActorHighlight(AActor* Actor, bool bHighlight) const;
};
