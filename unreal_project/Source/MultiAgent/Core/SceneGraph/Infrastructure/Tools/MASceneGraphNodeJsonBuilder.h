// MASceneGraphNodeJsonBuilder.h
// 统一构建 SceneGraph 节点 JSON

#pragma once

#include "CoreMinimal.h"

class FMASceneGraphNodeJsonBuilder
{
public:
    static FString BuildPointNodeJson(
        const FString& Id,
        const FString& Type,
        const FString& Label,
        const FVector& WorldLocation,
        const FString& Guid);
};
