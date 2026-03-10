// MASceneGraphNodeJsonValidator.h
// SceneGraph 节点 JSON 结构校验器

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMASceneGraphNodeJsonValidator
{
public:
    static bool ValidateNodeJsonStructure(
        const TSharedPtr<FJsonObject>& NodeObject,
        FString& OutErrorMessage);
};

