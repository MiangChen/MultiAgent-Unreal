// IMAEnvironmentObject.cpp
// 环境对象接口默认实现

#include "IMAEnvironmentObject.h"
#include "../Core/Types/MASceneGraphTypes.h"

FString IMAEnvironmentObject::GetFeature(const FString& Key, const FString& DefaultValue) const
{
    const TMap<FString, FString>& Features = GetObjectFeatures();
    const FString* ValuePtr = Features.Find(Key);
    return ValuePtr ? *ValuePtr : DefaultValue;
}

bool IMAEnvironmentObject::HasFeature(const FString& Key) const
{
    const TMap<FString, FString>& Features = GetObjectFeatures();
    return Features.Contains(Key);
}

bool IMAEnvironmentObject::MatchesSemanticLabel(const FMASemanticLabel& Label) const
{
    // 空标签不匹配任何对象
    if (Label.IsEmpty())
    {
        return false;
    }

    // 检查 Type - 与对象类型匹配
    if (!Label.Type.IsEmpty())
    {
        if (!Label.Type.Equals(GetObjectType(), ESearchCase::IgnoreCase))
        {
            return false;
        }
    }

    // 检查所有 Features
    const TMap<FString, FString>& ObjectFeatures = GetObjectFeatures();
    for (const auto& Pair : Label.Features)
    {
        const FString& Key = Pair.Key;
        const FString& QueryValue = Pair.Value;

        // 特殊处理 "label" 特征 - 与 GetObjectLabel() 匹配
        if (Key.Equals(TEXT("label"), ESearchCase::IgnoreCase))
        {
            FString ObjectLabel = GetObjectLabel();
            if (!QueryValue.Equals(ObjectLabel, ESearchCase::IgnoreCase))
            {
                // 也尝试部分匹配 (标签包含查询值)
                if (!ObjectLabel.Contains(QueryValue, ESearchCase::IgnoreCase))
                {
                    return false;
                }
            }
            continue;
        }

        // 检查对象的 Features 中是否有匹配的值
        const FString* ObjectValue = ObjectFeatures.Find(Key);
        if (!ObjectValue || !QueryValue.Equals(*ObjectValue, ESearchCase::IgnoreCase))
        {
            return false;
        }
    }

    return true;
}
