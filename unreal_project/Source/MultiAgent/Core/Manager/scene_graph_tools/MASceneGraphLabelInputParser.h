// MASceneGraphLabelInputParser.h
// 解析 AddSceneNode 输入标签格式: id/type/cate

#pragma once

#include "CoreMinimal.h"

struct FMASceneGraphLabelInputParseResult
{
    FString Id;
    FString Type;
    bool bHasId = false;
    bool bHasCategory = false;
};

class FMASceneGraphLabelInputParser
{
public:
    static bool Parse(
        const FString& InputText,
        FMASceneGraphLabelInputParseResult& OutResult,
        FString& OutErrorMessage);
};
