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
        OutErrorMessage = TEXT("Invalid JSON object");
        return false;
    }

    FString NodeId;
    if (!NodeObject->TryGetStringField(TEXT("id"), NodeId) || NodeId.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON missing valid 'id' field");
        return false;
    }

    const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
    if (!NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject) || !PropertiesObject || !(*PropertiesObject).IsValid())
    {
        OutErrorMessage = TEXT("JSON missing 'properties' object");
        return false;
    }

    FString PropertyType;
    if (!(*PropertiesObject)->TryGetStringField(TEXT("type"), PropertyType) || PropertyType.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON properties missing valid 'type' field");
        return false;
    }

    const TSharedPtr<FJsonObject>* ShapeObject = nullptr;
    if (!NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject) || !ShapeObject || !(*ShapeObject).IsValid())
    {
        OutErrorMessage = TEXT("JSON missing 'shape' object");
        return false;
    }

    FString ShapeType;
    if (!(*ShapeObject)->TryGetStringField(TEXT("type"), ShapeType) || ShapeType.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON shape missing valid 'type' field");
        return false;
    }

    if (ShapeType.Equals(TEXT("prism"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("prism type missing 'vertices' field");
            return false;
        }
        if (!VerticesArray || VerticesArray->Num() < 3)
        {
            const int32 Count = VerticesArray ? VerticesArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("prism type insufficient vertices (need at least 3, got %d)"), Count);
            return false;
        }

        double Height = 0.0;
        if (!(*ShapeObject)->TryGetNumberField(TEXT("height"), Height))
        {
            OutErrorMessage = TEXT("prism type missing 'height' field");
            return false;
        }
        if (Height <= 0.0)
        {
            OutErrorMessage = FString::Printf(TEXT("prism type invalid height (need > 0, got %f)"), Height);
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
            OutErrorMessage = TEXT("linestring type missing 'points' field");
            return false;
        }
        if (!PointsArray || PointsArray->Num() < 2)
        {
            const int32 Count = PointsArray ? PointsArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("linestring type insufficient points (need at least 2, got %d)"), Count);
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("linestring type missing 'vertices' field");
            return false;
        }
        if (!VerticesArray || VerticesArray->Num() != 4)
        {
            const int32 Count = VerticesArray ? VerticesArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("linestring type incorrect vertices count (need 4, got %d)"), Count);
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
            OutErrorMessage = TEXT("point type missing 'center' field");
            return false;
        }
        if (!CenterArray || CenterArray->Num() != 3)
        {
            const int32 Count = CenterArray ? CenterArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("point type incorrect center coordinate count (need 3, got %d)"), Count);
            return false;
        }

        UE_LOG(LogMASceneGraphNodeJsonValidator, Verbose, TEXT("ValidateNodeJsonStructure: point validation passed"));
    }
    else if (ShapeType.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
    {
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray = nullptr;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("polygon type missing 'vertices' field");
            return false;
        }
        if (!VerticesArray || VerticesArray->Num() < 3)
        {
            const int32 Count = VerticesArray ? VerticesArray->Num() : 0;
            OutErrorMessage = FString::Printf(TEXT("polygon type insufficient vertices (need at least 3, got %d)"), Count);
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
