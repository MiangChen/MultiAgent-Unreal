// MASceneGraphQuery.cpp
// 场景图查询模块实现

#include "MASceneGraphQuery.h"
#include "../../../Utils/MAGeometryUtils.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphQuery, Log, All);

//=============================================================================
// 分类查询
//=============================================================================

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetNodesByCategory(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& Category)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Category.Equals(Category, ESearchCase::IgnoreCase))
        {
            Result.Add(Node);
        }
    }

    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllBuildings(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsBuilding())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllBuildings: Found %d buildings"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllRoads(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsRoad())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllRoads: Found %d roads"), Result.Num());
    return Result;
}


TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllIntersections(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsIntersection())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllIntersections: Found %d intersections"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllProps(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsProp())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllProps: Found %d props"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllRobots(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsRobot())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllRobots: Found %d robots"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllPickupItems(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsPickupItem())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllPickupItems: Found %d pickup items"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllChargingStations(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.IsChargingStation())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllChargingStations: Found %d charging stations"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllGoals(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Type == TEXT("goal"))
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllGoals: Found %d goals"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllZones(const TArray<FMASceneGraphNode>& AllNodes)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Type == TEXT("zone"))
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllZones: Found %d zones"), Result.Num());
    return Result;
}

FMASceneGraphNode FMASceneGraphQuery::GetNodeById(const TArray<FMASceneGraphNode>& AllNodes, const FString& NodeId)
{
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Id == NodeId)
        {
            return Node;
        }
    }

    // 未找到，返回无效节点
    return FMASceneGraphNode();
}


//=============================================================================
// 语义查询
//=============================================================================

bool FMASceneGraphQuery::MatchesLabel(const FMASceneGraphNode& Node, const FMASemanticLabel& Label)
{
    // 如果标签为空，匹配所有节点
    if (Label.IsEmpty())
    {
        return true;
    }

    // 调试日志
    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("MatchesLabel: Checking Node[Id=%s, Type=%s, Label=%s] against Label[Type=%s, Features=%d]"),
        *Node.Id, *Node.Type, *Node.Label, *Label.Type, Label.Features.Num());

    // 检查 Type 匹配
    if (!Label.Type.IsEmpty())
    {
        bool bTypeMatches = Label.Type.Equals(Node.Type, ESearchCase::IgnoreCase);
        if (!bTypeMatches)
        {
            return false;
        }
    }

    // 检查 Features 匹配
    for (const auto& Pair : Label.Features)
    {
        const FString& Key = Pair.Key;
        const FString& Value = Pair.Value;

        // 特殊处理 "label" 特征 - 可以匹配 Id, Label, 或 Features["label"]
        if (Key.Equals(TEXT("label"), ESearchCase::IgnoreCase))
        {
            bool bLabelMatches = 
                Value.Equals(Node.Id, ESearchCase::IgnoreCase) ||
                Value.Equals(Node.Label, ESearchCase::IgnoreCase);

            const FString* NodeLabel = Node.Features.Find(TEXT("label"));
            if (NodeLabel && Value.Equals(*NodeLabel, ESearchCase::IgnoreCase))
            {
                bLabelMatches = true;
            }

            if (!bLabelMatches)
            {
                return false;
            }
        }
        // 特殊处理 "name" 特征 - 可以匹配 Id, Label, 或 Features["name"/"label"]
        else if (Key.Equals(TEXT("name"), ESearchCase::IgnoreCase))
        {
            bool bNameMatches = 
                Value.Equals(Node.Id, ESearchCase::IgnoreCase) ||
                Value.Equals(Node.Label, ESearchCase::IgnoreCase);

            const FString* NodeLabel = Node.Features.Find(TEXT("label"));
            if (NodeLabel && Value.Equals(*NodeLabel, ESearchCase::IgnoreCase))
            {
                bNameMatches = true;
            }

            const FString* NodeName = Node.Features.Find(TEXT("name"));
            if (NodeName && Value.Equals(*NodeName, ESearchCase::IgnoreCase))
            {
                bNameMatches = true;
            }

            if (!bNameMatches)
            {
                return false;
            }
        }
        // 其他特征从 Features 映射中查找
        else
        {
            const FString* NodeValue = Node.Features.Find(Key);
            if (!NodeValue || !Value.Equals(*NodeValue, ESearchCase::IgnoreCase))
            {
                return false;
            }
        }
    }

    return true;
}

FMASceneGraphNode FMASceneGraphQuery::FindNodeByLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label)
{
    if (Label.IsEmpty())
    {
        UE_LOG(LogMASceneGraphQuery, Warning, TEXT("FindNodeByLabel: Empty label provided"));
        return FMASceneGraphNode();
    }

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (MatchesLabel(Node, Label))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByLabel: Found node %s matching label [%s]"),
                *Node.Id, *Label.ToString());
            return Node;
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByLabel: No node found matching label [%s]"),
        *Label.ToString());
    return FMASceneGraphNode();
}

FMASceneGraphNode FMASceneGraphQuery::FindNearestNode(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label,
    const FVector& FromLocation)
{
    FMASceneGraphNode NearestNode;
    float MinDistanceSq = TNumericLimits<float>::Max();

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (!MatchesLabel(Node, Label))
        {
            continue;
        }

        float DistanceSq = FVector::DistSquared(FromLocation, Node.Center);
        if (DistanceSq < MinDistanceSq)
        {
            MinDistanceSq = DistanceSq;
            NearestNode = Node;
        }
    }

    if (NearestNode.IsValid())
    {
        UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNearestNode: Found node %s at distance %.2f"),
            *NearestNode.Id, FMath::Sqrt(MinDistanceSq));
    }
    else
    {
        UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNearestNode: No matching node found"));
    }

    return NearestNode;
}

TArray<FMASceneGraphNode> FMASceneGraphQuery::FindAllNodesByLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label)
{
    TArray<FMASceneGraphNode> Result;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (MatchesLabel(Node, Label))
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindAllNodesByLabel: Found %d nodes matching label [%s]"),
        Result.Num(), *Label.ToString());
    return Result;
}


//=============================================================================
// 边界查询
//=============================================================================

TArray<FMASceneGraphNode> FMASceneGraphQuery::FindNodesInBoundary(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label,
    const TArray<FVector>& BoundaryVertices)
{
    TArray<FMASceneGraphNode> Result;

    // 边界至少需要3个顶点
    if (BoundaryVertices.Num() < 3)
    {
        UE_LOG(LogMASceneGraphQuery, Warning, TEXT("FindNodesInBoundary: Boundary needs at least 3 vertices, got %d"),
            BoundaryVertices.Num());
        return Result;
    }

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        // 首先检查语义标签匹配
        if (!MatchesLabel(Node, Label))
        {
            continue;
        }

        // 然后检查节点中心是否在边界内
        if (FMAGeometryUtils::IsPointInPolygon2D(Node.Center, BoundaryVertices))
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodesInBoundary: Found %d nodes in boundary"), Result.Num());
    return Result;
}

bool FMASceneGraphQuery::IsPointInsideBuilding(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FVector& Point)
{
    // 获取所有建筑物节点
    TArray<FMASceneGraphNode> Buildings = GetAllBuildings(AllNodes);

    for (const FMASceneGraphNode& Building : Buildings)
    {
        // 从建筑物节点提取多边形顶点
        TArray<FVector> Vertices;
        if (!ExtractPolygonVertices(Building, Vertices))
        {
            continue;
        }

        // 检查点是否在建筑物多边形内
        if (FMAGeometryUtils::IsPointInPolygon2D(Point, Vertices))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("IsPointInsideBuilding: Point (%.2f, %.2f, %.2f) is inside building %s"),
                Point.X, Point.Y, Point.Z, *Building.Id);
            return true;
        }
    }

    return false;
}

FMASceneGraphNode FMASceneGraphQuery::FindNearestLandmark(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FVector& Location,
    float MaxDistance)
{
    FMASceneGraphNode NearestLandmark;
    float MinDistanceSq = MaxDistance * MaxDistance;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        // 只考虑地标节点
        if (!IsLandmark(Node))
        {
            continue;
        }

        float DistanceSq = FVector::DistSquared(Location, Node.Center);
        if (DistanceSq < MinDistanceSq)
        {
            MinDistanceSq = DistanceSq;
            NearestLandmark = Node;
        }
    }

    if (NearestLandmark.IsValid())
    {
        UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNearestLandmark: Found landmark %s (%s) at distance %.2f"),
            *NearestLandmark.Label, *NearestLandmark.Type, FMath::Sqrt(MinDistanceSq));
    }
    else
    {
        UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNearestLandmark: No landmark found within %.2f distance"),
            MaxDistance);
    }

    return NearestLandmark;
}

//=============================================================================
// 辅助查询
//=============================================================================

FMASceneGraphNode FMASceneGraphQuery::FindNodeByIdOrLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeIdOrLabel)
{
    // 统一查询逻辑：按 Id, Label, Features["label"], Features["name"] 顺序查找
    // 这样可以用 "UAV-1", "UAV-1", "RedBox", "101" 等任意标识符查找节点
    
    if (NodeIdOrLabel.IsEmpty())
    {
        return FMASceneGraphNode();
    }
    
    // 1. 首先精确匹配 Id
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Id.Equals(NodeIdOrLabel, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByIdOrLabel: Found node by Id match: %s"), *NodeIdOrLabel);
            return Node;
        }
    }
    
    // 2. 然后匹配 Label
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Label.Equals(NodeIdOrLabel, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByIdOrLabel: Found node by Label match: %s -> Id=%s"), *NodeIdOrLabel, *Node.Id);
            return Node;
        }
    }
    
    // 3. 匹配 Features["label"]
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        const FString* NodeLabel = Node.Features.Find(TEXT("label"));
        if (NodeLabel && NodeLabel->Equals(NodeIdOrLabel, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByIdOrLabel: Found node by Features[label] match: %s -> Id=%s"), *NodeIdOrLabel, *Node.Id);
            return Node;
        }
    }
    
    // 4. 最后匹配 Features["name"] (向后兼容)
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        const FString* NodeName = Node.Features.Find(TEXT("name"));
        if (NodeName && NodeName->Equals(NodeIdOrLabel, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByIdOrLabel: Found node by Features[name] match: %s -> Id=%s"), *NodeIdOrLabel, *Node.Id);
            return Node;
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeByIdOrLabel: No node found for: %s"), *NodeIdOrLabel);
    return FMASceneGraphNode();
}

FString FMASceneGraphQuery::GetIdByLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& Label)
{
    if (Label.IsEmpty())
    {
        return FString();
    }
    
    FMASceneGraphNode Node = FindNodeByIdOrLabel(AllNodes, Label);
    return Node.IsValid() ? Node.Id : FString();
}

FString FMASceneGraphQuery::GetLabelById(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& Id)
{
    if (Id.IsEmpty())
    {
        return FString();
    }
    
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Id.Equals(Id, ESearchCase::IgnoreCase))
        {
            // 优先返回 Label，如果为空则返回 Id
            return Node.Label.IsEmpty() ? Node.Id : Node.Label;
        }
    }
    
    return Id;  // 未找到时返回原 Id
}


//=============================================================================
// 内部辅助方法
//=============================================================================

bool FMASceneGraphQuery::ExtractPolygonVertices(const FMASceneGraphNode& Node, TArray<FVector>& OutVertices)
{
    OutVertices.Empty();

    // 只有 prism 和 polygon 类型的节点有顶点数据
    if (Node.ShapeType != TEXT("prism") && Node.ShapeType != TEXT("polygon"))
    {
        return false;
    }

    // 从 RawJson 解析顶点
    if (Node.RawJson.IsEmpty())
    {
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Node.RawJson);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMASceneGraphQuery, Warning, TEXT("ExtractPolygonVertices: Failed to parse RawJson for node %s"),
            *Node.Id);
        return false;
    }

    // 获取 shape 对象
    const TSharedPtr<FJsonObject>* ShapeObject;
    if (!JsonObject->TryGetObjectField(TEXT("shape"), ShapeObject))
    {
        return false;
    }

    // 获取 vertices 数组
    const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
    if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
    {
        return false;
    }

    // 解析每个顶点
    for (const TSharedPtr<FJsonValue>& VertexValue : *VerticesArray)
    {
        if (!VertexValue.IsValid() || VertexValue->Type != EJson::Array)
        {
            continue;
        }

        const TArray<TSharedPtr<FJsonValue>>& Coords = VertexValue->AsArray();
        if (Coords.Num() >= 2)
        {
            FVector Vertex;
            Vertex.X = Coords[0]->AsNumber();
            Vertex.Y = Coords[1]->AsNumber();
            Vertex.Z = Coords.Num() >= 3 ? Coords[2]->AsNumber() : 0.0f;
            OutVertices.Add(Vertex);
        }
    }

    return OutVertices.Num() >= 3;
}

bool FMASceneGraphQuery::IsLandmark(const FMASceneGraphNode& Node)
{
    // 地标包括: 建筑物、路口、道具
    return Node.IsBuilding() || Node.IsIntersection() || Node.IsProp();
}


//=============================================================================
// GUID 查询
//=============================================================================

TArray<FMASceneGraphNode> FMASceneGraphQuery::FindNodesByGuid(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& ActorGuid)
{
    // 根据 Actor GUID 查找包含该 GUID 的所有节点
    // 遍历所有节点，检查 GuidArray 和 Guid 字段

    TArray<FMASceneGraphNode> Result;

    if (ActorGuid.IsEmpty())
    {
        UE_LOG(LogMASceneGraphQuery, Warning, TEXT("FindNodesByGuid: ActorGuid is empty"));
        return Result;
    }

    // 调试: 输出搜索的节点总数
    UE_LOG(LogMASceneGraphQuery, Log, TEXT("FindNodesByGuid: Searching in %d nodes for GUID %s"), AllNodes.Num(), *ActorGuid);

    // 规范化输入的 GUID（移除连字符，转为大写）
    FString NormalizedInputGuid = ActorGuid.Replace(TEXT("-"), TEXT("")).ToUpper();
    UE_LOG(LogMASceneGraphQuery, Log, TEXT("FindNodesByGuid: Normalized input GUID: %s"), *NormalizedInputGuid);

    // 遍历所有节点，检查 GuidArray 和 Guid 字段
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        bool bFound = false;
        
        // 检查单个 Guid 字段 (point 类型)
        if (!Node.Guid.IsEmpty())
        {
            FString NormalizedNodeGuid = Node.Guid.Replace(TEXT("-"), TEXT("")).ToUpper();
            if (NormalizedNodeGuid == NormalizedInputGuid)
            {
                bFound = true;
            }
        }
        
        // 检查 GuidArray (polygon/linestring 类型)
        if (!bFound)
        {
            for (const FString& Guid : Node.GuidArray)
            {
                FString NormalizedGuid = Guid.Replace(TEXT("-"), TEXT("")).ToUpper();
                if (NormalizedGuid == NormalizedInputGuid)
                {
                    bFound = true;
                    break;
                }
            }
        }
        
        if (bFound)
        {
            Result.Add(Node);
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodesByGuid: Found node %s (type=%s) containing GUID %s"), 
                *Node.Id, *Node.ShapeType, *ActorGuid);
        }
    }
    
    UE_LOG(LogMASceneGraphQuery, Log, TEXT("FindNodesByGuid: Found %d nodes containing GUID %s"), 
        Result.Num(), *ActorGuid);

    return Result;
}


//=============================================================================
// 节点合并
//=============================================================================

TArray<FMASceneGraphNode> FMASceneGraphQuery::GetAllNodes(
    const TArray<FMASceneGraphNode>& StaticNodes,
    const TArray<FMASceneGraphNode>& DynamicNodes)
{
    TArray<FMASceneGraphNode> AllNodes;
    AllNodes.Reserve(StaticNodes.Num() + DynamicNodes.Num());
    AllNodes.Append(StaticNodes);
    AllNodes.Append(DynamicNodes);
    
    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("GetAllNodes: Merged %d static + %d dynamic = %d total nodes"), 
        StaticNodes.Num(), DynamicNodes.Num(), AllNodes.Num());
    
    return AllNodes;
}


//=============================================================================
// JSON 序列化
//=============================================================================

TSharedPtr<FJsonObject> FMASceneGraphQuery::NodeToJsonObject(const FMASceneGraphNode& Node)
{
    // 根据节点类型正确构建shape对象
    
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    
    // 设置 id
    NodeObject->SetStringField(TEXT("id"), Node.Id);
    
    // 构建 properties 对象
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    PropertiesObject->SetStringField(TEXT("category"), Node.Category);
    PropertiesObject->SetStringField(TEXT("type"), Node.Type);
    PropertiesObject->SetStringField(TEXT("label"), Node.Label);
    
    // 添加可拾取物品特有属性
    if (Node.IsPickupItem())
    {
        PropertiesObject->SetBoolField(TEXT("is_carried"), Node.bIsCarried);
        if (!Node.CarrierId.IsEmpty())
        {
            PropertiesObject->SetStringField(TEXT("carrier_id"), Node.CarrierId);
        }
    }
    
    // 添加动态节点标记
    if (Node.bIsDynamic)
    {
        PropertiesObject->SetBoolField(TEXT("is_dynamic"), true);
    }
    
    // 对于非地点类型节点，输出其所在地点的标签
    if (!Node.LocationLabel.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("location_label"), Node.LocationLabel);
    }
    
    // 添加 Features
    for (const auto& Feature : Node.Features)
    {
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    }
    
    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);
    
    // 构建 shape 对象
    // 尝试从 RawJson 中提取原始 shape 数据
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    bool bShapeFromRawJson = false;
    
    if (!Node.RawJson.IsEmpty())
    {
        // 解析原始JSON以获取完整的shape数据
        TSharedPtr<FJsonObject> RawJsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Node.RawJson);
        if (FJsonSerializer::Deserialize(Reader, RawJsonObject) && RawJsonObject.IsValid())
        {
            const TSharedPtr<FJsonObject>* OriginalShapeObject;
            if (RawJsonObject->TryGetObjectField(TEXT("shape"), OriginalShapeObject) && OriginalShapeObject->IsValid())
            {
                // 复制原始shape对象
                ShapeObject = MakeShareable(new FJsonObject());
                
                // 复制 type
                FString ShapeType;
                if ((*OriginalShapeObject)->TryGetStringField(TEXT("type"), ShapeType))
                {
                    ShapeObject->SetStringField(TEXT("type"), ShapeType);
                }
                
                // 根据形状类型复制相应字段
                if (ShapeType == TEXT("prism") || ShapeType == TEXT("polygon"))
                {
                    // prism: type, vertices, height
                    const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
                    if ((*OriginalShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
                    {
                        ShapeObject->SetArrayField(TEXT("vertices"), *VerticesArray);
                    }
                    
                    double Height;
                    if ((*OriginalShapeObject)->TryGetNumberField(TEXT("height"), Height))
                    {
                        ShapeObject->SetNumberField(TEXT("height"), Height);
                    }
                    
                    bShapeFromRawJson = true;
                }
                else if (ShapeType == TEXT("linestring"))
                {
                    // linestring: type, points, vertices
                    const TArray<TSharedPtr<FJsonValue>>* PointsArray;
                    if ((*OriginalShapeObject)->TryGetArrayField(TEXT("points"), PointsArray))
                    {
                        ShapeObject->SetArrayField(TEXT("points"), *PointsArray);
                    }
                    
                    const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
                    if ((*OriginalShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
                    {
                        ShapeObject->SetArrayField(TEXT("vertices"), *VerticesArray);
                    }
                    
                    bShapeFromRawJson = true;
                }
                else if (ShapeType == TEXT("point"))
                {
                    // point: type, center, vertices (可选)
                    const TArray<TSharedPtr<FJsonValue>>* CenterArray;
                    if ((*OriginalShapeObject)->TryGetArrayField(TEXT("center"), CenterArray))
                    {
                        ShapeObject->SetArrayField(TEXT("center"), *CenterArray);
                    }
                    
                    // 路口等节点可能有 vertices
                    const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
                    if ((*OriginalShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
                    {
                        ShapeObject->SetArrayField(TEXT("vertices"), *VerticesArray);
                    }
                    
                    bShapeFromRawJson = true;
                }
                else if (ShapeType == TEXT("polygon"))
                {
                    // polygon: type, vertices
                    const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
                    if ((*OriginalShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
                    {
                        ShapeObject->SetArrayField(TEXT("vertices"), *VerticesArray);
                    }
                    
                    bShapeFromRawJson = true;
                }
            }
        }
    }
    
    // 如果无法从RawJson获取shape，使用默认的point类型
    if (!bShapeFromRawJson)
    {
        ShapeObject->SetStringField(TEXT("type"), Node.ShapeType.IsEmpty() ? TEXT("point") : Node.ShapeType);
        
        // 设置center
        TArray<TSharedPtr<FJsonValue>> CenterArray;
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.X)));
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Y)));
        CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Z)));
        ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    }
    
    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    
    return NodeObject;
}
