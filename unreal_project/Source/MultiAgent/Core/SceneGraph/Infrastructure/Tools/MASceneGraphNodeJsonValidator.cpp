// MASceneGraphNodeJsonValidator.cpp

#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphNodeJsonValidator.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphNodeJsonValidator, Log, All);

bool FMASceneGraphNodeJsonValidator::ValidateNodeJsonStructure(
    const TSharedPtr<FJsonObject>& NodeObject,
    FString& OutErrorMessage)
{
    OutErrorMessage.Empty();

    if (!NodeObject.IsValid())
    {
        OutErrorMessage = TEXT("JSON 对象无效");
        return false;
    }

    FString NodeId;
    if (!NodeObject->TryGetStringField(TEXT("id"), NodeId) || NodeId.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON 缺少有效的 id 字段");
        return false;
    }

    const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
    if (!NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject) || !PropertiesObject || !(*PropertiesObject).IsValid())
    {
        OutErrorMessage = TEXT("JSON 缺少 properties 对象");
        return false;
    }

    FString PropertyType;
    if (!(*PropertiesObject)->TryGetStringField(TEXT("type"), PropertyType) || PropertyType.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON properties 缺少有效的 type 字段");
        return false;
    }

    const TSharedPtr<FJsonObject>* ShapeObject = nullptr;
    if (!NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject) || !ShapeObject || !(*ShapeObject).IsValid())
    {
        OutErrorMessage = TEXT("JSON 缺少 shape 对象");
        return false;
    }

    FString ShapeType;
    if (!(*ShapeObject)->TryGetStringField(TEXT("type"), ShapeType) || ShapeType.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON shape 缺少有效的 type 字段");
        return false;
    }

    if (ShapeType.Equals(TEXT("prism"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("prism 类型缺少 vertices 字段");
            return false;
        }
        if (!VerticesArray || VerticesArray->Num() < 3)
        {
            const int32 Count = VerticesArray ? VerticesArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("prism 类型 vertices 数量不足 (需要至少 3 个，实际 %d 个)"), Count);
            return false;
        }

        double Height = 0.0;
        if (!(*ShapeObject)->TryGetNumberField(TEXT("height"), Height))
        {
            OutErrorMessage = TEXT("prism 类型缺少 height 字段");
            return false;
        }
        if (Height <= 0.0)
        {
            OutErrorMessage = FString::Printf(TEXT("prism 类型 height 值无效 (需要 > 0，实际 %f)"), Height);
            return false;
        }

        UE_LOG(LogMASceneGraphNodeJsonValidator, Verbose, TEXT("ValidateNodeJsonStructure: prism validation passed (vertices=%d, height=%f)"),
            VerticesArray->Num(), Height);
    }
    else if (ShapeType.Equals(TEXT("linestring"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* PointsArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("points"), PointsArray))
        {
            OutErrorMessage = TEXT("linestring 类型缺少 points 字段");
            return false;
        }
        if (!PointsArray || PointsArray->Num() < 2)
        {
            const int32 Count = PointsArray ? PointsArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("linestring 类型 points 数量不足 (需要至少 2 个，实际 %d 个)"), Count);
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("linestring 类型缺少 vertices 字段");
            return false;
        }
        if (!VerticesArray || VerticesArray->Num() != 4)
        {
            const int32 Count = VerticesArray ? VerticesArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("linestring 类型 vertices 数量错误 (需要 4 个，实际 %d 个)"), Count);
            return false;
        }

        UE_LOG(LogMASceneGraphNodeJsonValidator, Verbose, TEXT("ValidateNodeJsonStructure: linestring validation passed (points=%d, vertices=%d)"),
            PointsArray->Num(), VerticesArray->Num());
    }
    else if (ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* CenterArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray))
        {
            OutErrorMessage = TEXT("point 类型缺少 center 字段");
            return false;
        }
        if (!CenterArray || CenterArray->Num() != 3)
        {
            const int32 Count = CenterArray ? CenterArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("point 类型 center 坐标数量错误 (需要 3 个，实际 %d 个)"), Count);
            return false;
        }

        UE_LOG(LogMASceneGraphNodeJsonValidator, Verbose, TEXT("ValidateNodeJsonStructure: point validation passed"));
    }
    else if (ShapeType.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("polygon 类型缺少 vertices 字段");
            return false;
        }
        if (!VerticesArray || VerticesArray->Num() < 3)
        {
            const int32 Count = VerticesArray ? VerticesArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("polygon 类型 vertices 数量不足 (需要至少 3 个，实际 %d 个)"), Count);
            return false;
        }

        UE_LOG(LogMASceneGraphNodeJsonValidator, Verbose, TEXT("ValidateNodeJsonStructure: polygon validation passed (vertices=%d)"),
            VerticesArray->Num());
    }
    else
    {
        UE_LOG(LogMASceneGraphNodeJsonValidator, Warning, TEXT("ValidateNodeJsonStructure: Unknown shape type '%s', allowing for backward compatibility"), *ShapeType);
    }

    const TArray<TSharedPtr<FJsonValue>>* GuidArray = nullptr;
    if (NodeObject->TryGetArrayField(TEXT("Guid"), GuidArray))
    {
        int32 Count = 0;
        if (NodeObject->TryGetNumberField(TEXT("count"), Count) && GuidArray && Count != GuidArray->Num())
        {
            UE_LOG(LogMASceneGraphNodeJsonValidator, Warning, TEXT("ValidateNodeJsonStructure: count (%d) does not match Guid array length (%d)"),
                Count, GuidArray->Num());
        }
    }

    UE_LOG(LogMASceneGraphNodeJsonValidator, Verbose, TEXT("ValidateNodeJsonStructure: Validation passed for node id=%s, shape.type=%s"), *NodeId, *ShapeType);
    return true;
}

