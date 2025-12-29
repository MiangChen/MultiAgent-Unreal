// MAWorldQuery.cpp
// 世界状态查询工具实现

#include "MAWorldQuery.h"
#include "../Agent/Character/MACharacter.h"
#include "../Environment/MAPickupItem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

//=============================================================================
// FMAEntityNode 实现
//=============================================================================

TSharedPtr<FJsonObject> FMAEntityNode::ToJsonObject() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("id"), Id);
    JsonObject->SetStringField(TEXT("category"), Category);
    JsonObject->SetStringField(TEXT("type"), Type);
    
    TSharedPtr<FJsonObject> PropsObject = MakeShareable(new FJsonObject());
    for (const auto& Pair : Properties)
    {
        PropsObject->SetStringField(Pair.Key, Pair.Value);
    }
    JsonObject->SetObjectField(TEXT("properties"), PropsObject);
    
    return JsonObject;
}

//=============================================================================
// FMABoundaryFeature 实现
//=============================================================================

TSharedPtr<FJsonObject> FMABoundaryFeature::ToJsonObject() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("label"), Label);
    JsonObject->SetStringField(TEXT("kind"), Kind);
    
    if (Kind == TEXT("circle"))
    {
        TArray<TSharedPtr<FJsonValue>> CenterArray;
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
        JsonObject->SetArrayField(TEXT("center"), CenterArray);
        JsonObject->SetNumberField(TEXT("radius"), Radius);
    }
    else if (Coords.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> CoordsArray;
        for (const FVector2D& Coord : Coords)
        {
            TArray<TSharedPtr<FJsonValue>> PointArray;
            PointArray.Add(MakeShareable(new FJsonValueNumber(Coord.X)));
            PointArray.Add(MakeShareable(new FJsonValueNumber(Coord.Y)));
            CoordsArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
        }
        JsonObject->SetArrayField(TEXT("coords"), CoordsArray);
    }
    
    return JsonObject;
}

//=============================================================================
// FMAWorldQueryResult 实现
//=============================================================================

FString FMAWorldQueryResult::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    // Entities
    TArray<TSharedPtr<FJsonValue>> EntitiesArray;
    for (const FMAEntityNode& Entity : Entities)
    {
        EntitiesArray.Add(MakeShareable(new FJsonValueObject(Entity.ToJsonObject())));
    }
    JsonObject->SetArrayField(TEXT("entities"), EntitiesArray);
    
    // Boundaries
    TArray<TSharedPtr<FJsonValue>> BoundariesArray;
    for (const FMABoundaryFeature& Boundary : Boundaries)
    {
        BoundariesArray.Add(MakeShareable(new FJsonValueObject(Boundary.ToJsonObject())));
    }
    JsonObject->SetArrayField(TEXT("boundaries"), BoundariesArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

//=============================================================================
// FMAWorldQuery 实现
//=============================================================================

TArray<FMAEntityNode> FMAWorldQuery::GetAllEntities(UWorld* World)
{
    TArray<FMAEntityNode> Result;
    if (!World) return Result;
    
    // 获取机器人
    Result.Append(GetAllRobots(World));
    
    // 获取道具
    Result.Append(GetAllProps(World));
    
    // 获取场景中的静态网格体 Actor（建筑、街道等）
    Result.Append(GetAllSceneObjects(World));
    
    return Result;
}

TArray<FMAEntityNode> FMAWorldQuery::GetAllRobots(UWorld* World)
{
    TArray<FMAEntityNode> Result;
    if (!World) return Result;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMACharacter::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (!Character) continue;
        
        FMAEntityNode Node;
        Node.Id = Character->AgentID;
        Node.Category = TEXT("robot");
        
        // 类型映射
        switch (Character->AgentType)
        {
            case EMAAgentType::UAV: Node.Type = TEXT("UAV"); break;
            case EMAAgentType::FixedWingUAV: Node.Type = TEXT("FixedWingUAV"); break;
            case EMAAgentType::UGV: Node.Type = TEXT("UGV"); break;
            case EMAAgentType::Quadruped: Node.Type = TEXT("Quadruped"); break;
            case EMAAgentType::Humanoid: Node.Type = TEXT("Humanoid"); break;
            default: Node.Type = TEXT("Unknown"); break;
        }
        
        // 属性
        FVector Loc = Character->GetActorLocation();
        FRotator Rot = Character->GetActorRotation();
        
        Node.Properties.Add(TEXT("name"), Character->AgentName);
        Node.Properties.Add(TEXT("x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Node.Properties.Add(TEXT("y"), FString::Printf(TEXT("%.1f"), Loc.Y));
        Node.Properties.Add(TEXT("z"), FString::Printf(TEXT("%.1f"), Loc.Z));
        Node.Properties.Add(TEXT("yaw"), FString::Printf(TEXT("%.1f"), Rot.Yaw));
        Node.Properties.Add(TEXT("is_moving"), Character->bIsMoving ? TEXT("true") : TEXT("false"));
        
        Result.Add(Node);
    }
    
    return Result;
}

TArray<FMAEntityNode> FMAWorldQuery::GetAllProps(UWorld* World)
{
    TArray<FMAEntityNode> Result;
    if (!World) return Result;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (!Item) continue;
        
        FMAEntityNode Node;
        Node.Id = Item->GetName();
        Node.Category = TEXT("prop");
        Node.Type = TEXT("item");
        
        FVector Loc = Item->GetActorLocation();
        Node.Properties.Add(TEXT("name"), Item->ItemName);
        Node.Properties.Add(TEXT("x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Node.Properties.Add(TEXT("y"), FString::Printf(TEXT("%.1f"), Loc.Y));
        Node.Properties.Add(TEXT("z"), FString::Printf(TEXT("%.1f"), Loc.Z));
        
        Result.Add(Node);
    }
    
    return Result;
}

TArray<FMAEntityNode> FMAWorldQuery::GetAllSceneObjects(UWorld* World)
{
    TArray<FMAEntityNode> Result;
    if (!World) return Result;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
        if (!MeshActor) continue;
        
        // 跳过太小的物体（可能是装饰物）
        FVector Origin, BoxExtent;
        MeshActor->GetActorBounds(false, Origin, BoxExtent);
        float Size = BoxExtent.Size();
        if (Size < 50.f) continue;  // 跳过小于 50cm 的物体
        
        FMAEntityNode Node;
        Node.Id = MeshActor->GetName();
        
        // 根据 Actor 标签或名称推断类别
        FString ActorName = MeshActor->GetActorLabel();
        if (ActorName.IsEmpty())
        {
            ActorName = MeshActor->GetName();
        }
        
        // 根据名称或标签推断类别
        FString NameLower = ActorName.ToLower();
        if (NameLower.Contains(TEXT("building")) || NameLower.Contains(TEXT("house")) || 
            NameLower.Contains(TEXT("tower")) || NameLower.Contains(TEXT("skyscraper")))
        {
            Node.Category = TEXT("building");
            Node.Type = TEXT("building");
        }
        else if (NameLower.Contains(TEXT("road")) || NameLower.Contains(TEXT("street")) || 
                 NameLower.Contains(TEXT("path")) || NameLower.Contains(TEXT("sidewalk")))
        {
            Node.Category = TEXT("trans_facility");
            Node.Type = TEXT("road");
        }
        else if (NameLower.Contains(TEXT("tree")) || NameLower.Contains(TEXT("plant")) || 
                 NameLower.Contains(TEXT("grass")) || NameLower.Contains(TEXT("bush")))
        {
            Node.Category = TEXT("vegetation");
            Node.Type = TEXT("plant");
        }
        else if (NameLower.Contains(TEXT("car")) || NameLower.Contains(TEXT("vehicle")) || 
                 NameLower.Contains(TEXT("truck")))
        {
            Node.Category = TEXT("vehicle");
            Node.Type = TEXT("car");
        }
        else
        {
            // 根据大小推断
            if (Size > 5000.f)
            {
                Node.Category = TEXT("building");
                Node.Type = TEXT("structure");
            }
            else if (Size > 1000.f)
            {
                Node.Category = TEXT("object");
                Node.Type = TEXT("large_object");
            }
            else
            {
                Node.Category = TEXT("object");
                Node.Type = TEXT("object");
            }
        }
        
        // 检查 Actor 标签
        for (const FName& Tag : MeshActor->Tags)
        {
            FString TagStr = Tag.ToString();
            if (TagStr.StartsWith(TEXT("category:")))
            {
                Node.Category = TagStr.Mid(9);
            }
            else if (TagStr.StartsWith(TEXT("type:")))
            {
                Node.Type = TagStr.Mid(5);
            }
        }
        
        FVector Loc = MeshActor->GetActorLocation();
        Node.Properties.Add(TEXT("name"), ActorName);
        Node.Properties.Add(TEXT("x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Node.Properties.Add(TEXT("y"), FString::Printf(TEXT("%.1f"), Loc.Y));
        Node.Properties.Add(TEXT("z"), FString::Printf(TEXT("%.1f"), Loc.Z));
        Node.Properties.Add(TEXT("size_x"), FString::Printf(TEXT("%.1f"), BoxExtent.X * 2));
        Node.Properties.Add(TEXT("size_y"), FString::Printf(TEXT("%.1f"), BoxExtent.Y * 2));
        Node.Properties.Add(TEXT("size_z"), FString::Printf(TEXT("%.1f"), BoxExtent.Z * 2));
        
        Result.Add(Node);
    }
    
    return Result;
}

TArray<FMABoundaryFeature> FMAWorldQuery::GetBoundaryFeatures(UWorld* World, const TArray<FString>& Categories)
{
    TArray<FMABoundaryFeature> Result;
    if (!World) return Result;
    
    // 查找所有静态网格体 Actor
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
        if (!MeshActor) continue;
        
        // 获取边界盒
        FVector Origin, BoxExtent;
        MeshActor->GetActorBounds(false, Origin, BoxExtent);
        float Size = BoxExtent.Size();
        
        // 只处理较大的物体（大于 100cm）
        if (Size < 100.f) continue;
        
        // 推断类别
        FString ActorName = MeshActor->GetActorLabel();
        if (ActorName.IsEmpty())
        {
            ActorName = MeshActor->GetName();
        }
        
        FString Category;
        FString NameLower = ActorName.ToLower();
        
        if (NameLower.Contains(TEXT("building")) || NameLower.Contains(TEXT("house")) || 
            NameLower.Contains(TEXT("tower")) || Size > 5000.f)
        {
            Category = TEXT("building");
        }
        else if (NameLower.Contains(TEXT("road")) || NameLower.Contains(TEXT("street")))
        {
            Category = TEXT("trans_facility");
        }
        else if (Size > 1000.f)
        {
            Category = TEXT("area");
        }
        else
        {
            continue;  // 跳过小物体
        }
        
        // 检查类别过滤
        if (Categories.Num() > 0 && !Categories.Contains(Category))
        {
            continue;
        }
        
        FMABoundaryFeature Feature;
        Feature.Label = ActorName;
        Feature.Kind = TEXT("area");
        Feature.Coords = GetActorFootprint(MeshActor);
        
        if (Feature.Coords.Num() > 0)
        {
            Result.Add(Feature);
        }
    }
    
    // 也查找带有 "Boundary" 标签的 Actor
    TArray<AActor*> TaggedActors;
    UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("Boundary")), TaggedActors);
    
    for (AActor* Actor : TaggedActors)
    {
        FMABoundaryFeature Feature = ExtractBoundaryFeature(Actor);
        if (!Feature.Label.IsEmpty() && Feature.Coords.Num() > 0)
        {
            Result.Add(Feature);
        }
    }
    
    return Result;
}

FMAEntityNode FMAWorldQuery::GetEntityById(UWorld* World, const FString& EntityId)
{
    FMAEntityNode Result;
    if (!World || EntityId.IsEmpty()) return Result;
    
    // 先查找机器人
    TArray<AActor*> Characters;
    UGameplayStatics::GetAllActorsOfClass(World, AMACharacter::StaticClass(), Characters);
    
    for (AActor* Actor : Characters)
    {
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (Character && Character->AgentID == EntityId)
        {
            TArray<FMAEntityNode> Robots = GetAllRobots(World);
            for (const FMAEntityNode& Node : Robots)
            {
                if (Node.Id == EntityId) return Node;
            }
        }
    }
    
    // 再查找道具
    TArray<AActor*> Props;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), Props);
    
    for (AActor* Actor : Props)
    {
        if (Actor->GetName() == EntityId)
        {
            TArray<FMAEntityNode> AllProps = GetAllProps(World);
            for (const FMAEntityNode& Node : AllProps)
            {
                if (Node.Id == EntityId) return Node;
            }
        }
    }
    
    return Result;
}

FMAEntityNode FMAWorldQuery::GetEntityByName(UWorld* World, const FString& EntityName)
{
    FMAEntityNode Result;
    if (!World || EntityName.IsEmpty()) return Result;
    
    // 遍历所有实体查找匹配名称的
    TArray<FMAEntityNode> AllEntities = GetAllEntities(World);
    for (const FMAEntityNode& Node : AllEntities)
    {
        const FString* Name = Node.Properties.Find(TEXT("name"));
        if (Name && *Name == EntityName)
        {
            return Node;
        }
    }
    
    return Result;
}

FMAWorldQueryResult FMAWorldQuery::GetWorldState(UWorld* World)
{
    FMAWorldQueryResult Result;
    if (!World) return Result;
    
    Result.Entities = GetAllEntities(World);
    Result.Boundaries = GetBoundaryFeatures(World);
    
    return Result;
}

FMAEntityNode FMAWorldQuery::ExtractEntityNode(AActor* Actor)
{
    FMAEntityNode Node;
    if (!Actor) return Node;
    
    Node.Id = Actor->GetName();
    
    FVector Loc = Actor->GetActorLocation();
    Node.Properties.Add(TEXT("x"), FString::Printf(TEXT("%.1f"), Loc.X));
    Node.Properties.Add(TEXT("y"), FString::Printf(TEXT("%.1f"), Loc.Y));
    Node.Properties.Add(TEXT("z"), FString::Printf(TEXT("%.1f"), Loc.Z));
    
    return Node;
}

FMABoundaryFeature FMAWorldQuery::ExtractBoundaryFeature(AActor* Actor)
{
    FMABoundaryFeature Feature;
    if (!Actor) return Feature;
    
    // 获取标签
    Feature.Label = Actor->GetActorLabel();
    if (Feature.Label.IsEmpty())
    {
        Feature.Label = Actor->GetName();
    }
    
    // 获取俯视轮廓
    TArray<FVector2D> Footprint = GetActorFootprint(Actor);
    
    if (Footprint.Num() >= 3)
    {
        Feature.Kind = TEXT("area");
        Feature.Coords = Footprint;
    }
    else if (Footprint.Num() == 2)
    {
        Feature.Kind = TEXT("line");
        Feature.Coords = Footprint;
    }
    else if (Footprint.Num() == 1)
    {
        Feature.Kind = TEXT("point");
        Feature.Coords = Footprint;
    }
    
    return Feature;
}

TArray<FVector2D> FMAWorldQuery::GetActorFootprint(AActor* Actor)
{
    TArray<FVector2D> Result;
    if (!Actor) return Result;
    
    // 获取 Actor 的边界盒
    FVector Origin, BoxExtent;
    Actor->GetActorBounds(false, Origin, BoxExtent);
    
    // 生成矩形轮廓 (俯视图)
    float MinX = Origin.X - BoxExtent.X;
    float MaxX = Origin.X + BoxExtent.X;
    float MinY = Origin.Y - BoxExtent.Y;
    float MaxY = Origin.Y + BoxExtent.Y;
    
    Result.Add(FVector2D(MinX, MinY));
    Result.Add(FVector2D(MaxX, MinY));
    Result.Add(FVector2D(MaxX, MaxY));
    Result.Add(FVector2D(MinX, MaxY));
    
    return Result;
}
