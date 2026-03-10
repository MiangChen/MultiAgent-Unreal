#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASceneActionResult.h"

class AActor;
class UWorld;

class MULTIAGENT_API FMAModifySceneActionAdapter
{
public:
    FMASceneActionResult ApplySingleModify(UWorld* World, AActor* Actor, const FString& LabelText) const;
    FMASceneActionResult ApplyMultiModify(
        UWorld* World,
        const TArray<AActor*>& Actors,
        const FString& LabelText,
        const FString& GeneratedJson) const;
};
