#pragma once

#include "CoreMinimal.h"
#include "../MAModifyTypes.h"

class AActor;
class UWorld;

class MULTIAGENT_API FMAModifyWidgetNodeBuilder
{
public:
    FString GetNextAvailableId(UWorld* World) const;
    TMap<FString, FString> GetDefaultPropertiesForCategory(EMANodeCategory Category) const;

    FString GenerateSceneGraphNode(UWorld* World, const FMAAnnotationInput& Input, const TArray<AActor*>& Actors) const;
    FString GenerateSceneGraphNodeV2(UWorld* World, const FMAAnnotationInput& Input, const TArray<AActor*>& Actors) const;

private:
    FString ResolveGeneratedLabel(UWorld* World, const FString& Type) const;
    TArray<FString> CollectActorGuids(const TArray<AActor*>& Actors) const;
    TArray<FVector2D> ComputeConvexHull(const TArray<AActor*>& Actors) const;
    TArray<FVector2D> ComputeLineString(const TArray<AActor*>& Actors) const;

    void ApplyNodeProperties(
        TSharedPtr<FJsonObject> PropertiesObject,
        const FMAAnnotationInput& Input,
        const TMap<FString, FString>& DefaultProps,
        bool bSkipCategoryInDefaults) const;
};
