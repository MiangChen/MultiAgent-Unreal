// MATaskGraphModel.cpp
// 任务图数据模型实现

#include "MATaskGraphModel.h"

DEFINE_LOG_CATEGORY(LogMATaskGraphModel);

//=============================================================================
// 构造函数
//=============================================================================

UMATaskGraphModel::UMATaskGraphModel()
{
    NodeIdCounter = 0;
}

//=============================================================================
// 数据加载
//=============================================================================

bool UMATaskGraphModel::LoadFromJson(const FString& JsonString)
{
    FString ErrorMessage;
    return LoadFromJsonWithError(JsonString, ErrorMessage);
}

bool UMATaskGraphModel::LoadFromJsonWithError(const FString& JsonString, FString& OutError)
{
    FMATaskGraphData ParsedData;
    if (!FMATaskGraphData::FromJsonWithError(JsonString, ParsedData, OutError))
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Failed to parse JSON: %s"), *OutError);
        return false;
    }

    LoadFromData(ParsedData);
    return true;
}

void UMATaskGraphModel::LoadFromData(const FMATaskGraphData& Data)
{
    // Store original data (immutable copy)
    OriginalData = Data;
    
    // Create working copy
    WorkingData = Data;

    // Update node ID counter based on existing nodes
    NodeIdCounter = 0;
    for (const FMATaskNodeData& Node : WorkingData.Nodes)
    {
        // Try to extract number from task ID (e.g., "T5" -> 5)
        FString IdStr = Node.TaskId;
        if (IdStr.StartsWith(TEXT("T")))
        {
            IdStr.RemoveFromStart(TEXT("T"));
            int32 IdNum = FCString::Atoi(*IdStr);
            if (IdNum >= NodeIdCounter)
            {
                NodeIdCounter = IdNum + 1;
            }
        }
    }

    UE_LOG(LogMATaskGraphModel, Log, TEXT("Loaded task graph: %d nodes, %d edges, next ID: T%d"),
           WorkingData.Nodes.Num(), WorkingData.Edges.Num(), NodeIdCounter);

    BroadcastDataChanged();
}

void UMATaskGraphModel::ResetToOriginal()
{
    WorkingData = OriginalData;
    BroadcastDataChanged();
    UE_LOG(LogMATaskGraphModel, Log, TEXT("Reset to original data"));
}

void UMATaskGraphModel::Clear()
{
    OriginalData = FMATaskGraphData();
    WorkingData = FMATaskGraphData();
    NodeIdCounter = 0;
    BroadcastDataChanged();
    UE_LOG(LogMATaskGraphModel, Log, TEXT("Cleared all data"));
}

//=============================================================================
// 数据导出
//=============================================================================

FString UMATaskGraphModel::ToJson() const
{
    return WorkingData.ToJson();
}

FMATaskGraphData UMATaskGraphModel::GetWorkingData() const
{
    return WorkingData;
}

FMATaskGraphData UMATaskGraphModel::GetOriginalData() const
{
    return OriginalData;
}

//=============================================================================
// 节点操作
//=============================================================================

void UMATaskGraphModel::AddNode(const FMATaskNodeData& Node)
{
    // Check for duplicate ID
    if (HasNode(Node.TaskId))
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Node with ID '%s' already exists"), *Node.TaskId);
        return;
    }

    WorkingData.Nodes.Add(Node);
    
    UE_LOG(LogMATaskGraphModel, Log, TEXT("Added node: %s"), *Node.TaskId);
    
    OnNodeAdded.Broadcast(Node.TaskId);
    BroadcastDataChanged();
}

FString UMATaskGraphModel::CreateNode(const FString& Description, const FString& Location)
{
    FString NewId = GenerateUniqueNodeId();
    
    FMATaskNodeData NewNode;
    NewNode.TaskId = NewId;
    NewNode.Description = Description.IsEmpty() ? TEXT("New Task") : Description;
    NewNode.Location = Location.IsEmpty() ? TEXT("unspecified") : Location;

    AddNode(NewNode);
    
    return NewId;
}

bool UMATaskGraphModel::RemoveNode(const FString& NodeId)
{
    // Find and remove the node
    int32 RemovedIndex = INDEX_NONE;
    for (int32 i = 0; i < WorkingData.Nodes.Num(); ++i)
    {
        if (WorkingData.Nodes[i].TaskId == NodeId)
        {
            RemovedIndex = i;
            break;
        }
    }

    if (RemovedIndex == INDEX_NONE)
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Node '%s' not found"), *NodeId);
        return false;
    }

    WorkingData.Nodes.RemoveAt(RemovedIndex);

    // Remove all edges connected to this node
    TArray<FMATaskEdgeData> EdgesToRemove;
    for (const FMATaskEdgeData& Edge : WorkingData.Edges)
    {
        if (Edge.FromNodeId == NodeId || Edge.ToNodeId == NodeId)
        {
            EdgesToRemove.Add(Edge);
        }
    }

    for (const FMATaskEdgeData& Edge : EdgesToRemove)
    {
        WorkingData.Edges.RemoveAll([&Edge](const FMATaskEdgeData& E) {
            return E.FromNodeId == Edge.FromNodeId && E.ToNodeId == Edge.ToNodeId;
        });
        OnEdgeRemoved.Broadcast(Edge.FromNodeId, Edge.ToNodeId);
    }

    UE_LOG(LogMATaskGraphModel, Log, TEXT("Removed node: %s (and %d connected edges)"), 
           *NodeId, EdgesToRemove.Num());

    OnNodeRemoved.Broadcast(NodeId);
    BroadcastDataChanged();
    
    return true;
}

bool UMATaskGraphModel::UpdateNode(const FString& NodeId, const FMATaskNodeData& NewData)
{
    FMATaskNodeData* Node = WorkingData.FindNode(NodeId);
    if (!Node)
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Node '%s' not found for update"), *NodeId);
        return false;
    }

    // Preserve canvas position if not set in new data
    FVector2D OldPosition = Node->CanvasPosition;
    *Node = NewData;
    if (NewData.CanvasPosition == FVector2D::ZeroVector)
    {
        Node->CanvasPosition = OldPosition;
    }

    UE_LOG(LogMATaskGraphModel, Log, TEXT("Updated node: %s"), *NodeId);

    OnNodeUpdated.Broadcast(NodeId);
    BroadcastDataChanged();
    
    return true;
}

bool UMATaskGraphModel::UpdateNodePosition(const FString& NodeId, FVector2D NewPosition)
{
    FMATaskNodeData* Node = WorkingData.FindNode(NodeId);
    if (!Node)
    {
        return false;
    }

    Node->CanvasPosition = NewPosition;
    // Don't broadcast for position-only updates to avoid excessive refreshes
    return true;
}

//=============================================================================
// 边操作
//=============================================================================

bool UMATaskGraphModel::AddEdge(const FString& FromId, const FString& ToId, const FString& EdgeType)
{
    // Validate nodes exist
    if (!HasNode(FromId))
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Source node '%s' not found"), *FromId);
        return false;
    }
    if (!HasNode(ToId))
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Target node '%s' not found"), *ToId);
        return false;
    }

    // Check for duplicate edge
    if (HasEdge(FromId, ToId))
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Edge from '%s' to '%s' already exists"), *FromId, *ToId);
        return false;
    }

    // Check for cycle
    if (WouldCreateCycle(FromId, ToId))
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Adding edge from '%s' to '%s' would create a cycle"), *FromId, *ToId);
        return false;
    }

    FMATaskEdgeData NewEdge(FromId, ToId, EdgeType);
    WorkingData.Edges.Add(NewEdge);

    UE_LOG(LogMATaskGraphModel, Log, TEXT("Added edge: %s -> %s (%s)"), *FromId, *ToId, *EdgeType);

    OnEdgeAdded.Broadcast(FromId, ToId);
    BroadcastDataChanged();
    
    return true;
}

bool UMATaskGraphModel::RemoveEdge(const FString& FromId, const FString& ToId)
{
    int32 RemovedCount = WorkingData.Edges.RemoveAll([&FromId, &ToId](const FMATaskEdgeData& Edge) {
        return Edge.FromNodeId == FromId && Edge.ToNodeId == ToId;
    });

    if (RemovedCount == 0)
    {
        UE_LOG(LogMATaskGraphModel, Warning, TEXT("Edge from '%s' to '%s' not found"), *FromId, *ToId);
        return false;
    }

    UE_LOG(LogMATaskGraphModel, Log, TEXT("Removed edge: %s -> %s"), *FromId, *ToId);

    OnEdgeRemoved.Broadcast(FromId, ToId);
    BroadcastDataChanged();
    
    return true;
}

//=============================================================================
// 查询
//=============================================================================

TArray<FMATaskNodeData> UMATaskGraphModel::GetAllNodes() const
{
    return WorkingData.Nodes;
}

TArray<FMATaskEdgeData> UMATaskGraphModel::GetAllEdges() const
{
    return WorkingData.Edges;
}

bool UMATaskGraphModel::FindNode(const FString& NodeId, FMATaskNodeData& OutNode) const
{
    const FMATaskNodeData* Node = WorkingData.FindNode(NodeId);
    if (Node)
    {
        OutNode = *Node;
        return true;
    }
    return false;
}

bool UMATaskGraphModel::HasNode(const FString& NodeId) const
{
    return WorkingData.HasNode(NodeId);
}

bool UMATaskGraphModel::HasEdge(const FString& FromId, const FString& ToId) const
{
    return WorkingData.HasEdge(FromId, ToId);
}

int32 UMATaskGraphModel::GetNodeCount() const
{
    return WorkingData.Nodes.Num();
}

int32 UMATaskGraphModel::GetEdgeCount() const
{
    return WorkingData.Edges.Num();
}

TArray<FMATaskEdgeData> UMATaskGraphModel::GetEdgesForNode(const FString& NodeId) const
{
    TArray<FMATaskEdgeData> Result;
    for (const FMATaskEdgeData& Edge : WorkingData.Edges)
    {
        if (Edge.FromNodeId == NodeId || Edge.ToNodeId == NodeId)
        {
            Result.Add(Edge);
        }
    }
    return Result;
}

//=============================================================================
// 验证
//=============================================================================

bool UMATaskGraphModel::IsValidDAG() const
{
    return WorkingData.IsValidDAG();
}

bool UMATaskGraphModel::WouldCreateCycle(const FString& FromId, const FString& ToId) const
{
    // Create a temporary copy with the new edge
    FMATaskGraphData TempData = WorkingData;
    TempData.Edges.Add(FMATaskEdgeData(FromId, ToId));
    
    // Check if it's still a valid DAG
    return !TempData.IsValidDAG();
}

//=============================================================================
// 辅助方法
//=============================================================================

FString UMATaskGraphModel::GenerateUniqueNodeId() const
{
    // Generate ID in format "T{number}"
    FString NewId;
    int32 Counter = NodeIdCounter;
    
    do
    {
        NewId = FString::Printf(TEXT("T%d"), Counter++);
    }
    while (HasNode(NewId));

    // Update counter (const_cast needed since this is logically const but modifies mutable state)
    const_cast<UMATaskGraphModel*>(this)->NodeIdCounter = Counter;
    
    return NewId;
}

void UMATaskGraphModel::BroadcastDataChanged()
{
    OnDataChanged.Broadcast();
}
