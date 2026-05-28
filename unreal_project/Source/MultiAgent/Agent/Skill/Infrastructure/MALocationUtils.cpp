// MALocationUtils.cpp
// 位置计算辅助类实现

#include "MALocationUtils.h"
#include "Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMALocationUtils, Log, All);

// 地点类型的 Category 列表
const TArray<FString> FMALocationUtils::LocationCategories = {
    TEXT("building"),
    TEXT("trans_facility"),
    TEXT("area")
};

// 地点类型的 Type 列表
const TArray<FString> FMALocationUtils::LocationTypes = {
    TEXT("building"),
    TEXT("intersection"),
    TEXT("street_segment"),
    TEXT("road_segment"),
    TEXT("area")
};

bool FMALocationUtils::IsLocationNode(const FMASceneGraphNode& Node)
{
    
    // 检查 Category
    for (const FString& Cat : LocationCategories)
    {
        if (Node.Category.Equals(Cat, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }
    
    // 检查 Type
    for (const FString& Type : LocationTypes)
    {
        if (Node.Type.Equals(Type, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }
    
    return false;
}

float FMALocationUtils::Distance2DSquared(const FVector& A, const FVector& B)
{
    float DX = A.X - B.X;
    float DY = A.Y - B.Y;
    return DX * DX + DY * DY;
}

FVector FMALocationUtils::GetNodeCenterPosition(const FMASceneGraphNode& Node)
{
    // 对于所有节点类型，Center 字段已经在解析时计算好了
    // 包括 polygon/prism 的顶点几何中心和 linestring 的端点几何中心
    return Node.Center;
}

FString FMALocationUtils::InferNearestLocationLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FVector& Position,
    float MaxDistance,
    const FString& ExcludeNodeId)
{
    // 基于几何距离计算最近地点的标签
    // 参考 GSI 的 location_utils.py 中的 infer_nearest_location 逻辑
    
    const FMASceneGraphNode* BestNode = nullptr;
    float BestDistanceSquared = FLT_MAX;
    float MaxDistanceSquared = MaxDistance * MaxDistance;
    
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        // 跳过无效节点
        if (!Node.IsValid())
        {
            continue;
        }
        
        // 跳过要排除的节点
        if (!ExcludeNodeId.IsEmpty() && Node.Id == ExcludeNodeId)
        {
            continue;
        }
        
        // 只考虑地点类型节点
        if (!IsLocationNode(Node))
        {
            continue;
        }
        
        // 获取节点中心位置
        FVector NodeCenter = GetNodeCenterPosition(Node);
        
        // 计算2D距离平方 (忽略Z轴)
        float DistSquared = Distance2DSquared(Position, NodeCenter);
        
        // 检查是否在最大距离范围内且是最近的
        if (DistSquared < BestDistanceSquared && DistSquared <= MaxDistanceSquared)
        {
            BestDistanceSquared = DistSquared;
            BestNode = &Node;
        }
    }
    if (!BestNode)
    {
        UE_LOG(LogMALocationUtils, Verbose, TEXT("InferNearestLocationLabel: No location node found within %.1f cm of position (%.1f, %.1f, %.1f)"),
            MaxDistance, Position.X, Position.Y, Position.Z);
        return TEXT("");
    }
    
    // 返回最近地点的标签
    FString ResultLabel = BestNode->Label;
    float Distance = FMath::Sqrt(BestDistanceSquared);
    
    UE_LOG(LogMALocationUtils, Verbose, TEXT("InferNearestLocationLabel: Found nearest location '%s' (type=%s) at distance %.1f cm"),
        *ResultLabel, *BestNode->Type, Distance);
    
    return ResultLabel;
}
