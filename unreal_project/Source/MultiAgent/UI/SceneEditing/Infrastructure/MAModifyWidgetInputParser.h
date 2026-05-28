#pragma once

#include "CoreMinimal.h"
#include "../MAModifyTypes.h"
#include "MAModifyWidgetNodeBuilder.h"

class UWorld;

class MULTIAGENT_API FMAModifyWidgetInputParser
{
public:
    bool ParseAnnotationInput(UWorld* World, const FString& Input, FMAAnnotationInput& OutResult, FString& OutError) const;
    bool ParseAnnotationInputV2(UWorld* World, const FString& Input, FMAAnnotationInput& OutResult, FString& OutError) const;
    bool ValidateSelectionForCategory(EMANodeCategory Category, int32 SelectionCount, FString& OutError) const;

private:
    FMAModifyWidgetNodeBuilder NodeBuilder;
};
