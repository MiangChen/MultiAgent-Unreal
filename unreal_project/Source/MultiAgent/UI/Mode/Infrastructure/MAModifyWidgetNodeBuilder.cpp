#include "MAModifyWidgetNodeBuilder.h"

#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Utils/MAGeometryUtils.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    UMASceneGraphManager* ResolveSceneGraphManager(UWorld* World)
    {
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        return GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    }
}

FString FMAModifyWidgetNodeBuilder::GetNextAvailableId(UWorld* World) const
{
    if (UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(World))
    {
        return SceneGraphManager->GetNextAvailableId();
    }

    return TEXT("1");
}

TMap<FString, FString> FMAModifyWidgetNodeBuilder::GetDefaultPropertiesForCategory(EMANodeCategory Category) const
{
    TMap<FString, FString> DefaultProperties;

    switch (Category)
    {
    case EMANodeCategory::Building:
        DefaultProperties.Add(TEXT("category"), TEXT("building"));
        DefaultProperties.Add(TEXT("status"), TEXT("undiscovered"));
        DefaultProperties.Add(TEXT("visibility"), TEXT("high"));
        DefaultProperties.Add(TEXT("wind_condition"), TEXT("weak"));
        DefaultProperties.Add(TEXT("congestion"), TEXT("none"));
        DefaultProperties.Add(TEXT("is_fire"), TEXT("false"));
        DefaultProperties.Add(TEXT("is_spill"), TEXT("false"));
        break;
    case EMANodeCategory::TransFacility:
        DefaultProperties.Add(TEXT("category"), TEXT("trans_facility"));
        DefaultProperties.Add(TEXT("status"), TEXT("undiscovered"));
        DefaultProperties.Add(TEXT("visibility"), TEXT("high"));
        DefaultProperties.Add(TEXT("wind_condition"), TEXT("weak"));
        DefaultProperties.Add(TEXT("congestion"), TEXT("none"));
        DefaultProperties.Add(TEXT("is_fire"), TEXT("false"));
        DefaultProperties.Add(TEXT("is_spill"), TEXT("false"));
        break;
    case EMANodeCategory::Prop:
        DefaultProperties.Add(TEXT("category"), TEXT("prop"));
        DefaultProperties.Add(TEXT("is_abnormal"), TEXT("false"));
        break;
    case EMANodeCategory::None:
    default:
        break;
    }

    return DefaultProperties;
}

FString FMAModifyWidgetNodeBuilder::ResolveGeneratedLabel(UWorld* World, const FString& Type) const
{
    if (UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(World))
    {
        return SceneGraphManager->GenerateLabel(Type);
    }

    FString CapitalizedType = Type;
    if (!CapitalizedType.IsEmpty())
    {
        CapitalizedType[0] = FChar::ToUpper(CapitalizedType[0]);
    }
    return FString::Printf(TEXT("%s-1"), *CapitalizedType);
}

TArray<FString> FMAModifyWidgetNodeBuilder::CollectActorGuids(const TArray<AActor*>& Actors) const
{
    TArray<FString> Guids;
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            Guids.Add(Actor->GetActorGuid().ToString());
        }
    }
    return Guids;
}

TArray<FVector2D> FMAModifyWidgetNodeBuilder::ComputeConvexHull(const TArray<AActor*>& Actors) const
{
    return FMAGeometryUtils::ComputeConvexHull2D(FMAGeometryUtils::CollectBoundingBoxCorners(Actors));
}

TArray<FVector2D> FMAModifyWidgetNodeBuilder::ComputeLineString(const TArray<AActor*>& Actors) const
{
    TArray<FVector2D> Result;
    TArray<FVector2D> CenterPoints;

    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            const FVector Location = Actor->GetActorLocation();
            CenterPoints.Add(FVector2D(Location.X, Location.Y));
        }
    }

    if (CenterPoints.Num() == 0)
    {
        return Result;
    }
    if (CenterPoints.Num() == 1)
    {
        Result.Add(CenterPoints[0]);
        Result.Add(CenterPoints[0]);
        return Result;
    }
    if (CenterPoints.Num() == 2)
    {
        Result = CenterPoints;
        return Result;
    }

    FVector2D Centroid(0.0f, 0.0f);
    for (const FVector2D& Point : CenterPoints)
    {
        Centroid += Point;
    }
    Centroid /= CenterPoints.Num();

    float Cxx = 0.0f;
    float Cyy = 0.0f;
    float Cxy = 0.0f;
    for (const FVector2D& Point : CenterPoints)
    {
        const float Dx = Point.X - Centroid.X;
        const float Dy = Point.Y - Centroid.Y;
        Cxx += Dx * Dx;
        Cyy += Dy * Dy;
        Cxy += Dx * Dy;
    }

    const float Theta = 0.5f * FMath::Atan2(2.0f * Cxy, Cxx - Cyy);
    const FVector2D PrincipalDirection(FMath::Cos(Theta), FMath::Sin(Theta));

    float MinProjection = TNumericLimits<float>::Max();
    float MaxProjection = TNumericLimits<float>::Lowest();
    int32 MinIndex = 0;
    int32 MaxIndex = 0;
    for (int32 Index = 0; Index < CenterPoints.Num(); ++Index)
    {
        const float Projection = FVector2D::DotProduct(CenterPoints[Index] - Centroid, PrincipalDirection);
        if (Projection < MinProjection)
        {
            MinProjection = Projection;
            MinIndex = Index;
        }
        if (Projection > MaxProjection)
        {
            MaxProjection = Projection;
            MaxIndex = Index;
        }
    }

    Result.Add(CenterPoints[MinIndex]);
    Result.Add(CenterPoints[MaxIndex]);
    return Result;
}

void FMAModifyWidgetNodeBuilder::ApplyNodeProperties(
    TSharedPtr<FJsonObject> PropertiesObject,
    const FMAAnnotationInput& Input,
    const TMap<FString, FString>& DefaultProps,
    bool bSkipCategoryInDefaults) const
{
    if (!PropertiesObject.IsValid())
    {
        return;
    }

    PropertiesObject->SetStringField(TEXT("type"), Input.Type);

    if (Input.HasCategory())
    {
        PropertiesObject->SetStringField(TEXT("category"), Input.GetCategoryString());
    }

    for (const TPair<FString, FString>& DefaultProp : DefaultProps)
    {
        if (bSkipCategoryInDefaults && DefaultProp.Key == TEXT("category"))
        {
            continue;
        }

        if (!Input.Properties.Contains(DefaultProp.Key))
        {
            if (DefaultProp.Value == TEXT("true") || DefaultProp.Value == TEXT("false"))
            {
                PropertiesObject->SetBoolField(DefaultProp.Key, DefaultProp.Value == TEXT("true"));
            }
            else
            {
                PropertiesObject->SetStringField(DefaultProp.Key, DefaultProp.Value);
            }
        }
    }

    for (const TPair<FString, FString>& Prop : Input.Properties)
    {
        if (Prop.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
            Prop.Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
        {
            PropertiesObject->SetBoolField(Prop.Key, Prop.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
        }
        else
        {
            PropertiesObject->SetStringField(Prop.Key, Prop.Value);
        }
    }
}

FString FMAModifyWidgetNodeBuilder::GenerateSceneGraphNode(
    UWorld* World,
    const FMAAnnotationInput& Input,
    const TArray<AActor*>& Actors) const
{
    if (Actors.Num() == 0)
    {
        return TEXT("{}");
    }

    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
    RootObject->SetStringField(TEXT("id"), Input.Id);
    RootObject->SetNumberField(TEXT("count"), Actors.Num());

    TArray<TSharedPtr<FJsonValue>> GuidArray;
    for (const FString& Guid : CollectActorGuids(Actors))
    {
        GuidArray.Add(MakeShared<FJsonValueString>(Guid));
    }
    RootObject->SetArrayField(TEXT("Guid"), GuidArray);

    TSharedPtr<FJsonObject> PropertiesObject = MakeShared<FJsonObject>();
    PropertiesObject->SetStringField(TEXT("label"), ResolveGeneratedLabel(World, Input.Type));
    ApplyNodeProperties(PropertiesObject, Input, GetDefaultPropertiesForCategory(Input.Category), false);
    RootObject->SetObjectField(TEXT("properties"), PropertiesObject);

    TSharedPtr<FJsonObject> ShapeObject = MakeShared<FJsonObject>();
    if (Input.IsPolygon())
    {
        ShapeObject->SetStringField(TEXT("type"), TEXT("polygon"));
        TArray<TSharedPtr<FJsonValue>> VerticesArray;
        for (const FVector2D& Vertex : ComputeConvexHull(Actors))
        {
            TArray<TSharedPtr<FJsonValue>> PointArray;
            PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.X));
            PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.Y));
            PointArray.Add(MakeShared<FJsonValueNumber>(0.0));
            VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
        }
        ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
    }
    else if (Input.IsLineString())
    {
        ShapeObject->SetStringField(TEXT("type"), TEXT("linestring"));
        TArray<TSharedPtr<FJsonValue>> PointsArray;
        for (const FVector2D& Point : ComputeLineString(Actors))
        {
            TArray<TSharedPtr<FJsonValue>> PointArray;
            PointArray.Add(MakeShared<FJsonValueNumber>(Point.X));
            PointArray.Add(MakeShared<FJsonValueNumber>(Point.Y));
            PointArray.Add(MakeShared<FJsonValueNumber>(0.0));
            PointsArray.Add(MakeShared<FJsonValueArray>(PointArray));
        }
        ShapeObject->SetArrayField(TEXT("points"), PointsArray);
    }
    else
    {
        ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
        const FVector Center = Actors[0]->GetActorLocation();
        TArray<TSharedPtr<FJsonValue>> CenterArray;
        CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
        CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
        CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
        ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    }
    RootObject->SetObjectField(TEXT("shape"), ShapeObject);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}

FString FMAModifyWidgetNodeBuilder::GenerateSceneGraphNodeV2(
    UWorld* World,
    const FMAAnnotationInput& Input,
    const TArray<AActor*>& Actors) const
{
    if (Actors.Num() == 0)
    {
        return TEXT("{}");
    }

    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
    RootObject->SetStringField(TEXT("id"), Input.Id);
    RootObject->SetNumberField(TEXT("count"), Actors.Num());

    TArray<TSharedPtr<FJsonValue>> GuidArray;
    for (const FString& Guid : CollectActorGuids(Actors))
    {
        GuidArray.Add(MakeShared<FJsonValueString>(Guid));
    }
    RootObject->SetArrayField(TEXT("Guid"), GuidArray);

    TSharedPtr<FJsonObject> PropertiesObject = MakeShared<FJsonObject>();
    PropertiesObject->SetStringField(TEXT("label"), ResolveGeneratedLabel(World, Input.Type));
    ApplyNodeProperties(PropertiesObject, Input, GetDefaultPropertiesForCategory(Input.Category), true);
    RootObject->SetObjectField(TEXT("properties"), PropertiesObject);

    TSharedPtr<FJsonObject> ShapeObject = MakeShared<FJsonObject>();
    switch (Input.Category)
    {
    case EMANodeCategory::Building:
        {
            ShapeObject->SetStringField(TEXT("type"), TEXT("prism"));
            const FMAPrismGeometry PrismGeometry = FMAGeometryUtils::ComputePrismFromActors(Actors);
            if (PrismGeometry.bIsValid)
            {
                TArray<TSharedPtr<FJsonValue>> VerticesArray;
                for (const FVector& Vertex : PrismGeometry.BottomVertices)
                {
                    TArray<TSharedPtr<FJsonValue>> PointArray;
                    PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.X));
                    PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.Y));
                    PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.Z));
                    VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
                }
                ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                ShapeObject->SetNumberField(TEXT("height"), PrismGeometry.Height);
            }
            else if (Actors[0])
            {
                FVector Origin;
                FVector BoxExtent;
                Actors[0]->GetActorBounds(false, Origin, BoxExtent);
                const float MinZ = Origin.Z - BoxExtent.Z;
                TArray<TSharedPtr<FJsonValue>> VerticesArray;
                const TArray<FVector2D> Corners = {
                    FVector2D(Origin.X - BoxExtent.X, Origin.Y - BoxExtent.Y),
                    FVector2D(Origin.X + BoxExtent.X, Origin.Y - BoxExtent.Y),
                    FVector2D(Origin.X + BoxExtent.X, Origin.Y + BoxExtent.Y),
                    FVector2D(Origin.X - BoxExtent.X, Origin.Y + BoxExtent.Y)
                };
                for (const FVector2D& Corner : Corners)
                {
                    TArray<TSharedPtr<FJsonValue>> PointArray;
                    PointArray.Add(MakeShared<FJsonValueNumber>(Corner.X));
                    PointArray.Add(MakeShared<FJsonValueNumber>(Corner.Y));
                    PointArray.Add(MakeShared<FJsonValueNumber>(MinZ));
                    VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
                }
                ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                ShapeObject->SetNumberField(TEXT("height"), BoxExtent.Z * 2.0f);
            }
        }
        break;
    case EMANodeCategory::TransFacility:
        {
            const FMAOBBGeometry OBBGeometry = FMAGeometryUtils::ComputeOBBFromActors(Actors);
            const bool bIsIntersection = Input.Type.Equals(TEXT("intersection"), ESearchCase::IgnoreCase);
            if (bIsIntersection)
            {
                ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
                if (OBBGeometry.bIsValid && OBBGeometry.CornerPoints.Num() == 4)
                {
                    FVector Center = FVector::ZeroVector;
                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    for (const FVector& Corner : OBBGeometry.CornerPoints)
                    {
                        Center += Corner;
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShared<FJsonValueNumber>(Corner.X));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Corner.Y));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Corner.Z));
                        VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
                    }
                    Center /= 4.0f;
                    TArray<TSharedPtr<FJsonValue>> CenterArray;
                    CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
                    CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
                    CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
                    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                }
                else
                {
                    const FVector Center = FMAGeometryUtils::ComputeCenterFromActors(Actors);
                    TArray<TSharedPtr<FJsonValue>> CenterArray;
                    CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
                    CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
                    CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
                    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
                }
            }
            else
            {
                ShapeObject->SetStringField(TEXT("type"), TEXT("linestring"));
                if (OBBGeometry.bIsValid && OBBGeometry.CornerPoints.Num() == 4)
                {
                    TArray<TSharedPtr<FJsonValue>> PointsArray;
                    for (const FVector& Midpoint : FMAGeometryUtils::ComputeShortEdgeMidpoints(OBBGeometry.CornerPoints))
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShared<FJsonValueNumber>(Midpoint.X));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Midpoint.Y));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Midpoint.Z));
                        PointsArray.Add(MakeShared<FJsonValueArray>(PointArray));
                    }
                    ShapeObject->SetArrayField(TEXT("points"), PointsArray);

                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    for (const FVector& Corner : OBBGeometry.CornerPoints)
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShared<FJsonValueNumber>(Corner.X));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Corner.Y));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Corner.Z));
                        VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
                    }
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                }
                else
                {
                    TArray<TSharedPtr<FJsonValue>> PointsArray;
                    for (const FVector2D& Point : ComputeLineString(Actors))
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShared<FJsonValueNumber>(Point.X));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Point.Y));
                        PointArray.Add(MakeShared<FJsonValueNumber>(0.0));
                        PointsArray.Add(MakeShared<FJsonValueArray>(PointArray));
                    }
                    ShapeObject->SetArrayField(TEXT("points"), PointsArray);

                    TArray<TSharedPtr<FJsonValue>> VerticesArray;
                    for (const FVector2D& Vertex : ComputeConvexHull(Actors))
                    {
                        TArray<TSharedPtr<FJsonValue>> PointArray;
                        PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.X));
                        PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.Y));
                        PointArray.Add(MakeShared<FJsonValueNumber>(0.0));
                        VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
                    }
                    ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
                }
            }
        }
        break;
    case EMANodeCategory::Prop:
        {
            ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
            const FVector Center = FMAGeometryUtils::ComputeCenterFromActors(Actors);
            TArray<TSharedPtr<FJsonValue>> CenterArray;
            CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
            CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
            CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
            ShapeObject->SetArrayField(TEXT("center"), CenterArray);
        }
        break;
    case EMANodeCategory::None:
    default:
        {
            if (Input.IsPolygon())
            {
                ShapeObject->SetStringField(TEXT("type"), TEXT("polygon"));
                TArray<TSharedPtr<FJsonValue>> VerticesArray;
                for (const FVector2D& Vertex : ComputeConvexHull(Actors))
                {
                    TArray<TSharedPtr<FJsonValue>> PointArray;
                    PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.X));
                    PointArray.Add(MakeShared<FJsonValueNumber>(Vertex.Y));
                    PointArray.Add(MakeShared<FJsonValueNumber>(0.0));
                    VerticesArray.Add(MakeShared<FJsonValueArray>(PointArray));
                }
                ShapeObject->SetArrayField(TEXT("vertices"), VerticesArray);
            }
            else if (Input.IsLineString())
            {
                ShapeObject->SetStringField(TEXT("type"), TEXT("linestring"));
                TArray<TSharedPtr<FJsonValue>> PointsArray;
                for (const FVector2D& Point : ComputeLineString(Actors))
                {
                    TArray<TSharedPtr<FJsonValue>> PointArray;
                    PointArray.Add(MakeShared<FJsonValueNumber>(Point.X));
                    PointArray.Add(MakeShared<FJsonValueNumber>(Point.Y));
                    PointArray.Add(MakeShared<FJsonValueNumber>(0.0));
                    PointsArray.Add(MakeShared<FJsonValueArray>(PointArray));
                }
                ShapeObject->SetArrayField(TEXT("points"), PointsArray);
            }
            else
            {
                ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
                const FVector Center = Actors[0]->GetActorLocation();
                TArray<TSharedPtr<FJsonValue>> CenterArray;
                CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
                CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
                CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
                ShapeObject->SetArrayField(TEXT("center"), CenterArray);
            }
        }
        break;
    }

    RootObject->SetObjectField(TEXT("shape"), ShapeObject);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}
