// MASceneGraphLabelInputParser.cpp

#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphLabelInputParser.h"

bool FMASceneGraphLabelInputParser::Parse(
    const FString& InputText,
    FMASceneGraphLabelInputParseResult& OutResult,
    FString& OutErrorMessage)
{
    OutResult = FMASceneGraphLabelInputParseResult{};
    OutErrorMessage.Empty();

    if (InputText.IsEmpty())
    {
        OutErrorMessage = TEXT("输入格式错误，请使用: cate:building/trans_facility/prop,type:值");
        return false;
    }

    TArray<FString> Parts;
    InputText.ParseIntoArray(Parts, TEXT(","), true);

    bool bFoundType = false;
    for (const FString& Part : Parts)
    {
        FString Key;
        FString Value;
        if (!Part.Split(TEXT(":"), &Key, &Value))
        {
            continue;
        }

        Key = Key.TrimStartAndEnd();
        Value = Value.TrimStartAndEnd();
        const FString KeyLower = Key.ToLower();

        if (KeyLower == TEXT("id"))
        {
            if (Value.IsEmpty())
            {
                OutErrorMessage = TEXT("缺少必填字段: id");
                return false;
            }
            OutResult.Id = Value;
            OutResult.bHasId = true;
            continue;
        }

        if (KeyLower == TEXT("type"))
        {
            if (Value.IsEmpty())
            {
                OutErrorMessage = TEXT("缺少必填字段: type");
                return false;
            }
            OutResult.Type = Value;
            bFoundType = true;
            continue;
        }

        if (KeyLower == TEXT("cate") || KeyLower == TEXT("category"))
        {
            const FString ValueLower = Value.ToLower();
            if (ValueLower != TEXT("building")
                && ValueLower != TEXT("trans_facility")
                && ValueLower != TEXT("prop"))
            {
                OutErrorMessage = FString::Printf(
                    TEXT("无效的 cate 值: %s (应为 building, trans_facility 或 prop)"),
                    *Value);
                return false;
            }
            OutResult.bHasCategory = true;
        }
    }

    if (!bFoundType)
    {
        OutErrorMessage = TEXT("缺少必填字段: type");
        return false;
    }

    return true;
}
