// MASceneGraphQuery.cpp
// 场景图查询模块实现

#include "MASceneGraphQuery.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("MatchesLabel: Checking Node[Id=%s, Category=%s, Type=%s, Label=%s] against Label[Class=%s, Type=%s]"),
        *Node.Id, *Node.Category, *Node.Type, *Node.Label, *Label.Class, *Label.Type);

    // 检查 Class 匹配
    if (!Label.Class.IsEmpty())
    {
        // Class 可以匹配 Category 或 Type
        bool bClassMatches = 
            Label.Class.Equals(Node.Category, ESearchCase::IgnoreCase) ||
            Label.Class.Equals(Node.Type, ESearchCase::IgnoreCase);

        // 特殊处理: "robot" 类别
        if (Label.IsRobot() && Node.IsRobot())
        {
            bClassMatches = true;
        }

        // 特殊处理: "pickup_item" 或 "cargo" 类别
        if (Label.IsPickupItem() && Node.IsPickupItem())
        {
            bClassMatches = true;
        }

        // 特殊处理: "object" 类别 - 匹配 cargo 节点
        // 这是因为外部系统可能使用 "object" 来表示可拾取物品
        if (Label.Class.Equals(TEXT("object"), ESearchCase::IgnoreCase) && Node.IsPickupItem())
        {
            bClassMatches = true;
        }

        // 特殊处理: "charging_station" 类别
        if (Label.IsChargingStation() && Node.IsChargingStation())
        {
            bClassMatches = true;
        }

        // 特殊处理: "building" 类别
        if (Label.IsBuilding() && Node.IsBuilding())
        {
            bClassMatches = true;
        }

        if (!bClassMatches)
        {
            return false;
        }
    }

    // 检查 Type 匹配
    if (!Label.Type.IsEmpty())
    {
        // Type 可以匹配 Node.Type 或 Features["subtype"]
        bool bTypeMatches = Label.Type.Equals(Node.Type, ESearchCase::IgnoreCase);
        
        // 对于 cargo 类型节点，Type 也可以匹配 Features["subtype"]
        // 例如: Label.Type="box" 可以匹配 Node.Features["subtype"]="box"
        if (!bTypeMatches && Node.IsPickupItem())
        {
            const FString* Subtype = Node.Features.Find(TEXT("subtype"));
            if (Subtype && Label.Type.Equals(*Subtype, ESearchCase::IgnoreCase))
            {
                bTypeMatches = true;
            }
        }

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

            // 也检查 Features 中的 label
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
        // 特殊处理 "name" 特征 (向后兼容) - 可以匹配 Id, Label, 或 Features["name"/"label"]
        else if (Key.Equals(TEXT("name"), ESearchCase::IgnoreCase))
        {
            bool bNameMatches = 
                Value.Equals(Node.Id, ESearchCase::IgnoreCase) ||
                Value.Equals(Node.Label, ESearchCase::IgnoreCase);

            // 检查 Features 中的 label (优先)
            const FString* NodeLabel = Node.Features.Find(TEXT("label"));
            if (NodeLabel && Value.Equals(*NodeLabel, ESearchCase::IgnoreCase))
            {
                bNameMatches = true;
            }

            // 也检查 Features 中的 name (向后兼容)
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
        if (IsPointInPolygon2D(Node.Center, BoundaryVertices))
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
        if (IsPointInPolygon2D(Point, Vertices))
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

FMASceneGraphNode FMASceneGraphQuery::FindNodeById(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeId)
{
    // 统一查询逻辑：按 Id, Label, Features["label"], Features["name"] 顺序查找
    // 这样可以用 "UAV_01", "UAV-1", "RedBox", "101" 等任意标识符查找节点
    
    if (NodeId.IsEmpty())
    {
        return FMASceneGraphNode();
    }
    
    // 1. 首先精确匹配 Id
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Id.Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeById: Found node by Id match: %s"), *NodeId);
            return Node;
        }
    }
    
    // 2. 然后匹配 Label
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Label.Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeById: Found node by Label match: %s -> Id=%s"), *NodeId, *Node.Id);
            return Node;
        }
    }
    
    // 3. 匹配 Features["label"]
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        const FString* NodeLabel = Node.Features.Find(TEXT("label"));
        if (NodeLabel && NodeLabel->Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeById: Found node by Features[label] match: %s -> Id=%s"), *NodeId, *Node.Id);
            return Node;
        }
    }
    
    // 4. 最后匹配 Features["name"] (向后兼容)
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        const FString* NodeName = Node.Features.Find(TEXT("name"));
        if (NodeName && NodeName->Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeById: Found node by Features[name] match: %s -> Id=%s"), *NodeId, *Node.Id);
            return Node;
        }
    }

    UE_LOG(LogMASceneGraphQuery, Verbose, TEXT("FindNodeById: No node found for: %s"), *NodeId);
    return FMASceneGraphNode();
}

FMASceneGraphNode FMASceneGraphQuery::FindNodeByLabelString(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeLabel)
{
    // 注意: 此函数现在与 FindNodeById 行为相同，保留是为了向后兼容
    // 推荐使用 FindNodeById 进行统一查询
    return FindNodeById(AllNodes, NodeLabel);
}


//=============================================================================
// 内部辅助方法
//=============================================================================

bool FMASceneGraphQuery::IsPointInPolygon2D(const FVector& Point, const TArray<FVector>& PolygonVertices)
{
    // 使用射线法 (Ray Casting Algorithm) 判断点是否在多边形内
    // 从点向右发射一条射线，计算与多边形边的交点数
    // 奇数个交点表示在多边形内，偶数个交点表示在多边形外

    if (PolygonVertices.Num() < 3)
    {
        return false;
    }

    int32 NumVertices = PolygonVertices.Num();
    bool bInside = false;

    for (int32 i = 0, j = NumVertices - 1; i < NumVertices; j = i++)
    {
        const FVector& Vi = PolygonVertices[i];
        const FVector& Vj = PolygonVertices[j];

        // 检查射线是否与边相交 (只考虑X和Y坐标)
        if (((Vi.Y > Point.Y) != (Vj.Y > Point.Y)) &&
            (Point.X < (Vj.X - Vi.X) * (Point.Y - Vi.Y) / (Vj.Y - Vi.Y) + Vi.X))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}

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
