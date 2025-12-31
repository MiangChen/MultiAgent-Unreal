// MASceneGraphManager.cpp
// 场景图管理器实现
// Requirements: 1.1, 1.2, 1.3, 1.4, 2.1, 2.2, 2.3, 2.4, 3.1, 3.3, 4.1, 4.2, 4.3, 4.4, 6.1, 6.2, 6.3, 6.4, 6.5, 7.1, 7.2, 7.3, 7.4, 8.1

#include "MASceneGraphManager.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphManager, Log, All);

// 场景图文件名常量
// Requirements: 6.1
const FString UMASceneGraphManager::SceneGraphFileName = TEXT("datasets/scene_graph_cyberworld.json");

//=============================================================================
// 静态辅助方法 - 几何中心计算
//=============================================================================

FVector UMASceneGraphManager::CalculatePolygonCentroid(const TArray<TSharedPtr<FJsonValue>>& Vertices)
{
    // Requirements: 1.1 - 计算多边形顶点的几何中心 (算术平均值)
    
    if (Vertices.Num() == 0)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("CalculatePolygonCentroid: Empty vertices array"));
        return FVector::ZeroVector;
    }

    FVector Sum = FVector::ZeroVector;
    int32 ValidCount = 0;

    for (const TSharedPtr<FJsonValue>& VertexValue : Vertices)
    {
        if (!VertexValue.IsValid() || VertexValue->Type != EJson::Array)
        {
            continue;
        }

        const TArray<TSharedPtr<FJsonValue>>& Coords = VertexValue->AsArray();
        if (Coords.Num() >= 3)
        {
            Sum.X += Coords[0]->AsNumber();
            Sum.Y += Coords[1]->AsNumber();
            Sum.Z += Coords[2]->AsNumber();
            ValidCount++;
        }
    }

    if (ValidCount == 0)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("CalculatePolygonCentroid: No valid vertices found"));
        return FVector::ZeroVector;
    }

    FVector Centroid = Sum / static_cast<float>(ValidCount);
    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("CalculatePolygonCentroid: %d vertices -> (%f, %f, %f)"), 
        ValidCount, Centroid.X, Centroid.Y, Centroid.Z);
    
    return Centroid;
}

FVector UMASceneGraphManager::CalculateLineStringCentroid(const TArray<TSharedPtr<FJsonValue>>& Points)
{
    // Requirements: 1.2 - 计算线串端点的几何中心 (算术平均值)
    
    if (Points.Num() == 0)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("CalculateLineStringCentroid: Empty points array"));
        return FVector::ZeroVector;
    }

    FVector Sum = FVector::ZeroVector;
    int32 ValidCount = 0;

    for (const TSharedPtr<FJsonValue>& PointValue : Points)
    {
        if (!PointValue.IsValid() || PointValue->Type != EJson::Array)
        {
            continue;
        }

        const TArray<TSharedPtr<FJsonValue>>& Coords = PointValue->AsArray();
        if (Coords.Num() >= 3)
        {
            Sum.X += Coords[0]->AsNumber();
            Sum.Y += Coords[1]->AsNumber();
            Sum.Z += Coords[2]->AsNumber();
            ValidCount++;
        }
    }

    if (ValidCount == 0)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("CalculateLineStringCentroid: No valid points found"));
        return FVector::ZeroVector;
    }

    FVector Centroid = Sum / static_cast<float>(ValidCount);
    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("CalculateLineStringCentroid: %d points -> (%f, %f, %f)"), 
        ValidCount, Centroid.X, Centroid.Y, Centroid.Z);
    
    return Centroid;
}

//=============================================================================
// 生命周期
//=============================================================================

void UMASceneGraphManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initializing"));

    // 加载场景图数据
    // Requirements: 6.1, 6.2
    if (!LoadSceneGraph())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("Failed to load scene graph, creating empty structure"));
        CreateEmptySceneGraph();
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initialized"));
}

void UMASceneGraphManager::Deinitialize()
{
    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager deinitializing"));

    // 清理内存数据
    SceneGraphData.Reset();

    Super::Deinitialize();
}

//=============================================================================
// 核心 API
//=============================================================================

bool UMASceneGraphManager::ParseLabelInput(const FString& InputText, FString& OutId, FString& OutType, FString& OutErrorMessage)
{
    // Requirements: 1.1, 1.2, 1.3, 1.4
    // 解析格式: "id:值,type:值"

    OutId.Empty();
    OutType.Empty();
    OutErrorMessage.Empty();

    if (InputText.IsEmpty())
    {
        OutErrorMessage = TEXT("输入格式错误，请使用: id:值,type:值");
        return false;
    }

    // 按逗号分割
    TArray<FString> Parts;
    InputText.ParseIntoArray(Parts, TEXT(","), true);

    bool bFoundId = false;
    bool bFoundType = false;

    for (const FString& Part : Parts)
    {
        // 按冒号分割键值对
        FString Key, Value;
        if (Part.Split(TEXT(":"), &Key, &Value))
        {
            // Requirements: 1.4 - Trim 空格
            Key = Key.TrimStartAndEnd();
            Value = Value.TrimStartAndEnd();

            // 键名大小写不敏感
            if (Key.ToLower() == TEXT("id"))
            {
                if (Value.IsEmpty())
                {
                    // Requirements: 2.2
                    OutErrorMessage = TEXT("缺少必填字段: id");
                    return false;
                }
                OutId = Value;
                bFoundId = true;
            }
            else if (Key.ToLower() == TEXT("type"))
            {
                if (Value.IsEmpty())
                {
                    // Requirements: 2.3
                    OutErrorMessage = TEXT("缺少必填字段: type");
                    return false;
                }
                OutType = Value;
                bFoundType = true;
            }
        }
    }

    // Requirements: 1.3, 2.2, 2.3 - 检查必填字段
    if (!bFoundId)
    {
        OutErrorMessage = TEXT("缺少必填字段: id");
        return false;
    }

    if (!bFoundType)
    {
        OutErrorMessage = TEXT("缺少必填字段: type");
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("ParseLabelInput: id=%s, type=%s"), *OutId, *OutType);
    return true;
}

FString UMASceneGraphManager::GenerateLabel(const FString& Type) const
{
    // Requirements: 4.1, 4.2, 4.3, 4.4
    // 生成格式: "Type-N"

    // Requirements: 4.2 - 首字母大写
    FString CapitalizedType = CapitalizeFirstLetter(Type);

    // Requirements: 4.3 - 计算同类型节点数量
    int32 Count = GetTypeCount(Type);

    // Requirements: 4.4 - 序号从 1 开始
    int32 NextNumber = Count + 1;

    // Requirements: 4.1 - 生成标签
    FString Label = FString::Printf(TEXT("%s-%d"), *CapitalizedType, NextNumber);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("GenerateLabel: type=%s, count=%d, label=%s"), *Type, Count, *Label);
    return Label;
}

bool UMASceneGraphManager::IsIdExists(const FString& Id) const
{
    // Requirements: 3.1 - 检查 ID 是否已存在

    if (!SceneGraphData.IsValid())
    {
        return false;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        return false;
    }

    // Requirements: 3.4 - 大小写敏感比较
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            if (NodeObject.IsValid())
            {
                FString NodeId;
                if (NodeObject->TryGetStringField(TEXT("id"), NodeId))
                {
                    if (NodeId == Id)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

int32 UMASceneGraphManager::GetTypeCount(const FString& Type) const
{
    // Requirements: 4.3 - 计算同类型节点数量

    if (!SceneGraphData.IsValid())
    {
        return 0;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        return 0;
    }

    int32 Count = 0;
    FString LowerType = Type.ToLower();

    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            if (NodeObject.IsValid())
            {
                const TSharedPtr<FJsonObject>* PropertiesObject;
                if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject))
                {
                    FString NodeType;
                    if ((*PropertiesObject)->TryGetStringField(TEXT("type"), NodeType))
                    {
                        // 类型比较大小写不敏感
                        if (NodeType.ToLower() == LowerType)
                        {
                            Count++;
                        }
                    }
                }
            }
        }
    }

    return Count;
}

bool UMASceneGraphManager::AddSceneNode(const FString& Id, const FString& Type, const FVector& WorldLocation,
                                         AActor* Actor, FString& OutLabel, FString& OutErrorMessage)
{
    // Requirements: 3.1, 3.3, 6.3, 7.1, 7.2, 7.3, 7.4

    OutLabel.Empty();
    OutErrorMessage.Empty();

    // Requirements: 3.1 - 检查 ID 是否重复
    if (IsIdExists(Id))
    {
        // Requirements: 3.2
        OutErrorMessage = FString::Printf(TEXT("ID 已存在: %s，已忽略本次提交"), *Id);
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("AddSceneNode: %s"), *OutErrorMessage);
        return false;
    }

    // 确保场景图数据有效
    if (!SceneGraphData.IsValid())
    {
        CreateEmptySceneGraph();
    }

    // 获取或创建 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutErrorMessage = TEXT("保存失败: 无法获取节点数组");
        return false;
    }

    // Requirements: 4.1, 4.2, 4.3, 4.4 - 生成标签
    OutLabel = GenerateLabel(Type);

    // Requirements: 1.3, 7.2, 7.3 - 提取 GUID
    FString GuidString;
    if (Actor != nullptr)
    {
        // Requirements: 7.2 - 使用 Actor->GetActorGuid().ToString() 获取 GUID
        GuidString = Actor->GetActorGuid().ToString();
        UE_LOG(LogMASceneGraphManager, Log, TEXT("AddSceneNode: Extracted GUID=%s from Actor=%s"), *GuidString, *Actor->GetName());
    }
    else
    {
        // Requirements: 7.3 - Actor 为 nullptr 时生成空字符串并记录警告
        GuidString = TEXT("");
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("AddSceneNode: Actor is nullptr, GUID will be empty"));
    }

    // Requirements: 7.1, 7.2, 7.3, 7.4 - 构建节点 JSON 对象
    TSharedPtr<FJsonObject> NewNode = MakeShareable(new FJsonObject());

    // Requirements: 7.1 - id 字段
    NewNode->SetStringField(TEXT("id"), Id);

    // Requirements: 1.1, 2.1 - guid 字段 (紧跟在 id 字段后)
    NewNode->SetStringField(TEXT("guid"), GuidString);

    // Requirements: 7.2 - properties 对象
    TSharedPtr<FJsonObject> Properties = MakeShareable(new FJsonObject());
    Properties->SetStringField(TEXT("type"), Type);
    Properties->SetStringField(TEXT("label"), OutLabel);
    NewNode->SetObjectField(TEXT("properties"), Properties);

    // Requirements: 7.3, 7.4 - shape 对象
    TSharedPtr<FJsonObject> Shape = MakeShareable(new FJsonObject());
    Shape->SetStringField(TEXT("type"), TEXT("point"));

    // Requirements: 7.4 - center 数组 [x, y, z]
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.Z)));
    Shape->SetArrayField(TEXT("center"), CenterArray);

    NewNode->SetObjectField(TEXT("shape"), Shape);

    // Requirements: 6.3 - 追加到 nodes 数组
    NodesArray->Add(MakeShareable(new FJsonValueObject(NewNode)));

    // Requirements: 6.4 - 保存到文件
    if (!SaveSceneGraph())
    {
        OutErrorMessage = TEXT("保存失败: 无法写入文件");
        // 回滚：移除刚添加的节点
        NodesArray->RemoveAt(NodesArray->Num() - 1);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddSceneNode: id=%s, guid=%s, type=%s, label=%s, location=(%f, %f, %f)"),
        *Id, *GuidString, *Type, *OutLabel, WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

    return true;
}

bool UMASceneGraphManager::AddMultiSelectNode(const FString& JsonString, FString& OutErrorMessage)
{
    // Requirements: 4.1, 4.2, 5.1, 5.2, 7.4, 8.1, 8.3, 8.4
    // 添加多选节点 (Polygon/LineString/Prism/Point)

    OutErrorMessage.Empty();

    if (JsonString.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON 字符串为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddMultiSelectNode: JSON string is empty"));
        return false;
    }

    // 解析 JSON 字符串
    TSharedPtr<FJsonObject> NodeObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, NodeObject) || !NodeObject.IsValid())
    {
        OutErrorMessage = TEXT("JSON 解析失败");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddMultiSelectNode: Failed to parse JSON: %s"), *JsonString);
        return false;
    }

    // 提取 ID 用于重复检查
    FString NodeId;
    if (!NodeObject->TryGetStringField(TEXT("id"), NodeId))
    {
        OutErrorMessage = TEXT("JSON 缺少 id 字段");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddMultiSelectNode: JSON missing 'id' field"));
        return false;
    }

    // 检查 ID 是否已存在
    if (IsIdExists(NodeId))
    {
        OutErrorMessage = FString::Printf(TEXT("ID 已存在: %s"), *NodeId);
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("AddMultiSelectNode: %s"), *OutErrorMessage);
        return false;
    }

    // Requirements: 7.4, 8.3, 8.4 - 验证 JSON 结构
    if (!ValidateNodeJsonStructure(NodeObject, OutErrorMessage))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddMultiSelectNode: JSON validation failed for node id=%s: %s"), *NodeId, *OutErrorMessage);
        // 验证失败，不修改文件，直接返回
        return false;
    }

    // 确保场景图数据有效
    if (!SceneGraphData.IsValid())
    {
        CreateEmptySceneGraph();
    }

    // 获取 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutErrorMessage = TEXT("保存失败: 无法获取节点数组");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddMultiSelectNode: Failed to get nodes array"));
        return false;
    }

    // 追加到 nodes 数组
    NodesArray->Add(MakeShareable(new FJsonValueObject(NodeObject)));

    // 保存到文件
    if (!SaveSceneGraph())
    {
        OutErrorMessage = TEXT("保存失败: 无法写入文件");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddMultiSelectNode: Failed to save scene graph"));
        // 回滚：移除刚添加的节点
        NodesArray->RemoveAt(NodesArray->Num() - 1);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddMultiSelectNode: Successfully added node with id=%s"), *NodeId);
    return true;
}

FString UMASceneGraphManager::GetSceneGraphFilePath() const
{
    // Requirements: 6.1
    return FPaths::ProjectDir() / SceneGraphFileName;
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllNodes() const
{
    // Requirements: 3.4, 6.1, 6.2, 1.1, 1.2, 1.3, 1.4, 1.5
    // 遍历 JSON nodes 数组，解析每个节点的字段
    // 支持 point, polygon, linestring 三种形状类型

    TArray<FMASceneGraphNode> Result;

    if (!SceneGraphData.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: SceneGraphData is invalid"));
        return Result;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: nodes array not found"));
        return Result;
    }

    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid())
        {
            continue;
        }

        FMASceneGraphNode Node;

        // 解析 id 字段
        NodeObject->TryGetStringField(TEXT("id"), Node.Id);

        // 解析 guid 字段 (小写，用于 point 类型)
        // Requirements: 6.1, 6.2 - 处理缺少 guid 字段的旧数据（设置为空字符串）
        if (!NodeObject->TryGetStringField(TEXT("guid"), Node.Guid))
        {
            Node.Guid = TEXT("");  // 旧数据没有 guid 字段，设置为空字符串
        }

        // 解析 Guid 数组 (大写 G，用于 polygon/linestring 类型)
        // Requirements: 1.3, 1.4
        const TArray<TSharedPtr<FJsonValue>>* GuidArrayJson;
        if (NodeObject->TryGetArrayField(TEXT("Guid"), GuidArrayJson))
        {
            for (const TSharedPtr<FJsonValue>& GuidValue : *GuidArrayJson)
            {
                if (GuidValue.IsValid() && GuidValue->Type == EJson::String)
                {
                    Node.GuidArray.Add(GuidValue->AsString());
                }
            }
        }

        // 解析 properties 对象中的 type 和 label 字段
        const TSharedPtr<FJsonObject>* PropertiesObject;
        if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject))
        {
            (*PropertiesObject)->TryGetStringField(TEXT("type"), Node.Type);
            (*PropertiesObject)->TryGetStringField(TEXT("label"), Node.Label);
        }

        // 解析 shape 对象
        const TSharedPtr<FJsonObject>* ShapeObject;
        if (NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject))
        {
            // 解析 shape.type 字段
            // Requirements: 1.1, 1.2, 1.5
            (*ShapeObject)->TryGetStringField(TEXT("type"), Node.ShapeType);

            if (Node.ShapeType == TEXT("point"))
            {
                // Requirements: 1.5 - 对于 point 类型，使用 center 字段 (保持现有行为)
                const TArray<TSharedPtr<FJsonValue>>* CenterArray;
                if ((*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray))
                {
                    if (CenterArray->Num() >= 3)
                    {
                        Node.Center.X = (*CenterArray)[0]->AsNumber();
                        Node.Center.Y = (*CenterArray)[1]->AsNumber();
                        Node.Center.Z = (*CenterArray)[2]->AsNumber();
                    }
                }
            }
            else if (Node.ShapeType == TEXT("polygon"))
            {
                // Requirements: 1.1 - 对于 polygon 类型，从 vertices 计算中心点
                const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
                if ((*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
                {
                    Node.Center = CalculatePolygonCentroid(*VerticesArray);
                }
                else
                {
                    UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: polygon node %s missing vertices"), *Node.Id);
                    Node.Center = FVector::ZeroVector;
                }
            }
            else if (Node.ShapeType == TEXT("linestring"))
            {
                // Requirements: 1.2 - 对于 linestring 类型，从 points 计算中心点
                const TArray<TSharedPtr<FJsonValue>>* PointsArray;
                if ((*ShapeObject)->TryGetArrayField(TEXT("points"), PointsArray))
                {
                    Node.Center = CalculateLineStringCentroid(*PointsArray);
                }
                else
                {
                    UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: linestring node %s missing points"), *Node.Id);
                    Node.Center = FVector::ZeroVector;
                }
            }
            else if (Node.ShapeType == TEXT("prism"))
            {
                // 对于 prism 类型，从 vertices (底面顶点) 计算中心点
                const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
                if ((*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
                {
                    // 使用 CalculatePolygonCentroid 计算底面多边形的中心
                    Node.Center = CalculatePolygonCentroid(*VerticesArray);
                    
                    // 可选：将 Z 坐标调整为棱柱高度的一半
                    double Height = 0.0;
                    if ((*ShapeObject)->TryGetNumberField(TEXT("height"), Height))
                    {
                        Node.Center.Z += Height / 2.0;
                    }
                }
                else
                {
                    UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: prism node %s missing vertices"), *Node.Id);
                    Node.Center = FVector::ZeroVector;
                }
            }
            else
            {
                // 未知类型，尝试使用 center 字段
                const TArray<TSharedPtr<FJsonValue>>* CenterArray;
                if ((*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray))
                {
                    if (CenterArray->Num() >= 3)
                    {
                        Node.Center.X = (*CenterArray)[0]->AsNumber();
                        Node.Center.Y = (*CenterArray)[1]->AsNumber();
                        Node.Center.Z = (*CenterArray)[2]->AsNumber();
                    }
                }
            }
        }
        else
        {
            // 节点缺少 shape 字段
            UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: node %s missing shape field"), *Node.Id);
            Node.Center = FVector::ZeroVector;
        }

        // 保存原始 JSON 字符串到 RawJson 字段
        // Requirements: 2.2, 3.2
        FString NodeJsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJsonString);
        if (FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer))
        {
            Node.RawJson = NodeJsonString;
        }

        Result.Add(Node);
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("GetAllNodes: returned %d nodes"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> UMASceneGraphManager::FindNodesByGuid(const FString& ActorGuid) const
{
    // Requirements: 2.1, 2.2, 2.4
    // 根据 Actor GUID 查找包含该 GUID 的所有节点
    // 遍历所有节点，检查 GuidArray 和 Guid 字段

    TArray<FMASceneGraphNode> Result;

    if (ActorGuid.IsEmpty())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("FindNodesByGuid: ActorGuid is empty"));
        return Result;
    }

    // 规范化输入的 GUID（移除连字符，转为大写）
    FString NormalizedInputGuid = ActorGuid.Replace(TEXT("-"), TEXT("")).ToUpper();

    // 获取所有节点
    TArray<FMASceneGraphNode> AllNodes = GetAllNodes();

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
            UE_LOG(LogMASceneGraphManager, Verbose, TEXT("FindNodesByGuid: Found node %s (type=%s) containing GUID %s"), 
                *Node.Id, *Node.ShapeType, *ActorGuid);
        }
    }

    // Requirements: 2.4 - 返回所有匹配的节点
    UE_LOG(LogMASceneGraphManager, Log, TEXT("FindNodesByGuid: Found %d nodes containing GUID %s"), 
        Result.Num(), *ActorGuid);

    return Result;
}

//=============================================================================
// JSON 文件 I/O
//=============================================================================

bool UMASceneGraphManager::LoadSceneGraph()
{
    // Requirements: 6.1, 6.2

    FString FilePath = GetSceneGraphFilePath();

    // 检查文件是否存在
    if (!FPaths::FileExists(FilePath))
    {
        // Requirements: 6.2 - 文件不存在时创建空结构
        UE_LOG(LogMASceneGraphManager, Log, TEXT("Scene graph file not found: %s"), *FilePath);
        return false;
    }

    // 读取文件内容
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Failed to read scene graph file: %s"), *FilePath);
        return false;
    }

    // 解析 JSON
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, SceneGraphData) || !SceneGraphData.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Failed to parse scene graph JSON: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("Scene graph loaded from: %s"), *FilePath);
    return true;
}

bool UMASceneGraphManager::SaveSceneGraph()
{
    // Requirements: 6.4

    if (!SceneGraphData.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Cannot save: SceneGraphData is invalid"));
        return false;
    }

    FString FilePath = GetSceneGraphFilePath();

    // 序列化为 JSON 字符串（带格式化缩进）
    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);

    if (!FJsonSerializer::Serialize(SceneGraphData.ToSharedRef(), Writer))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Failed to serialize scene graph to JSON"));
        return false;
    }

    // 确保目录存在
    FString Directory = FPaths::GetPath(FilePath);
    if (!FPaths::DirectoryExists(Directory))
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
    }

    // 写入文件
    if (!FFileHelper::SaveStringToFile(JsonString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Failed to write scene graph file: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("Scene graph saved to: %s"), *FilePath);
    return true;
}

void UMASceneGraphManager::CreateEmptySceneGraph()
{
    // Requirements: 6.2

    SceneGraphData = MakeShareable(new FJsonObject());

    // 创建空的 nodes 数组
    TArray<TSharedPtr<FJsonValue>> EmptyNodes;
    SceneGraphData->SetArrayField(TEXT("nodes"), EmptyNodes);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("Created empty scene graph structure"));
}

//=============================================================================
// 内部辅助方法
//=============================================================================

FString UMASceneGraphManager::CapitalizeFirstLetter(const FString& Type) const
{
    // Requirements: 4.2

    if (Type.IsEmpty())
    {
        return Type;
    }

    // 首字母大写，其余保持原样
    FString Result = Type;
    Result[0] = FChar::ToUpper(Result[0]);
    return Result;
}

TArray<TSharedPtr<FJsonValue>>* UMASceneGraphManager::GetNodesArray() const
{
    if (!SceneGraphData.IsValid())
    {
        return nullptr;
    }

    // 获取 nodes 数组
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!SceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return nullptr;
    }

    // 返回非 const 指针以便修改
    // 注意：这里使用 const_cast 是因为我们需要修改数组内容
    return const_cast<TArray<TSharedPtr<FJsonValue>>*>(NodesArray);
}

bool UMASceneGraphManager::ValidateNodeJsonStructure(const TSharedPtr<FJsonObject>& NodeObject, FString& OutErrorMessage) const
{
    // Requirements: 7.4, 8.3, 8.4
    // 验证场景图节点 JSON 结构
    // 根据 shape.type 验证必需字段

    OutErrorMessage.Empty();

    if (!NodeObject.IsValid())
    {
        OutErrorMessage = TEXT("JSON 对象无效");
        return false;
    }

    // 验证必需的顶级字段: id
    FString NodeId;
    if (!NodeObject->TryGetStringField(TEXT("id"), NodeId) || NodeId.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON 缺少有效的 id 字段");
        return false;
    }

    // 验证 properties 对象
    const TSharedPtr<FJsonObject>* PropertiesObject;
    if (!NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject) || !PropertiesObject->IsValid())
    {
        OutErrorMessage = TEXT("JSON 缺少 properties 对象");
        return false;
    }

    // 验证 properties.type 字段
    FString PropertyType;
    if (!(*PropertiesObject)->TryGetStringField(TEXT("type"), PropertyType) || PropertyType.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON properties 缺少有效的 type 字段");
        return false;
    }

    // 验证 shape 对象
    const TSharedPtr<FJsonObject>* ShapeObject;
    if (!NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject) || !ShapeObject->IsValid())
    {
        OutErrorMessage = TEXT("JSON 缺少 shape 对象");
        return false;
    }

    // 验证 shape.type 字段
    FString ShapeType;
    if (!(*ShapeObject)->TryGetStringField(TEXT("type"), ShapeType) || ShapeType.IsEmpty())
    {
        OutErrorMessage = TEXT("JSON shape 缺少有效的 type 字段");
        return false;
    }

    // 根据 shape.type 验证特定字段
    if (ShapeType.Equals(TEXT("prism"), ESearchCase::IgnoreCase))
    {
        // Prism 类型: 需要 vertices 和 height
        // Requirements: 2.5, 2.6, 2.7
        
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("prism 类型缺少 vertices 字段");
            return false;
        }
        
        if (VerticesArray->Num() < 3)
        {
            OutErrorMessage = FString::Printf(TEXT("prism 类型 vertices 数量不足 (需要至少 3 个，实际 %d 个)"), VerticesArray->Num());
            return false;
        }

        double Height;
        if (!(*ShapeObject)->TryGetNumberField(TEXT("height"), Height))
        {
            OutErrorMessage = TEXT("prism 类型缺少 height 字段");
            return false;
        }

        if (Height <= 0)
        {
            OutErrorMessage = FString::Printf(TEXT("prism 类型 height 值无效 (需要 > 0，实际 %f)"), Height);
            return false;
        }

        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("ValidateNodeJsonStructure: prism validation passed (vertices=%d, height=%f)"), 
            VerticesArray->Num(), Height);
    }
    else if (ShapeType.Equals(TEXT("linestring"), ESearchCase::IgnoreCase))
    {
        // LineString 类型: 需要 points 和 vertices
        // Requirements: 3.4, 3.5, 3.6
        
        const TArray<TSharedPtr<FJsonValue>>* PointsArray;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("points"), PointsArray))
        {
            OutErrorMessage = TEXT("linestring 类型缺少 points 字段");
            return false;
        }
        
        if (PointsArray->Num() < 2)
        {
            OutErrorMessage = FString::Printf(TEXT("linestring 类型 points 数量不足 (需要至少 2 个，实际 %d 个)"), PointsArray->Num());
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("linestring 类型缺少 vertices 字段");
            return false;
        }
        
        if (VerticesArray->Num() != 4)
        {
            OutErrorMessage = FString::Printf(TEXT("linestring 类型 vertices 数量错误 (需要 4 个，实际 %d 个)"), VerticesArray->Num());
            return false;
        }

        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("ValidateNodeJsonStructure: linestring validation passed (points=%d, vertices=%d)"), 
            PointsArray->Num(), VerticesArray->Num());
    }
    else if (ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase))
    {
        // Point 类型: 需要 center
        // Requirements: 4.3, 4.4
        
        const TArray<TSharedPtr<FJsonValue>>* CenterArray;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray))
        {
            OutErrorMessage = TEXT("point 类型缺少 center 字段");
            return false;
        }
        
        if (CenterArray->Num() != 3)
        {
            OutErrorMessage = FString::Printf(TEXT("point 类型 center 坐标数量错误 (需要 3 个，实际 %d 个)"), CenterArray->Num());
            return false;
        }

        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("ValidateNodeJsonStructure: point validation passed"));
    }
    else if (ShapeType.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
    {
        // Polygon 类型: 需要 vertices (向后兼容)
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
        if (!(*ShapeObject)->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            OutErrorMessage = TEXT("polygon 类型缺少 vertices 字段");
            return false;
        }
        
        if (VerticesArray->Num() < 3)
        {
            OutErrorMessage = FString::Printf(TEXT("polygon 类型 vertices 数量不足 (需要至少 3 个，实际 %d 个)"), VerticesArray->Num());
            return false;
        }

        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("ValidateNodeJsonStructure: polygon validation passed (vertices=%d)"), 
            VerticesArray->Num());
    }
    else
    {
        // 未知的 shape.type，记录警告但允许通过（向后兼容）
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("ValidateNodeJsonStructure: Unknown shape type '%s', allowing for backward compatibility"), *ShapeType);
    }

    // 验证 Guid 数组（如果存在）
    const TArray<TSharedPtr<FJsonValue>>* GuidArray;
    if (NodeObject->TryGetArrayField(TEXT("Guid"), GuidArray))
    {
        // 验证 count 字段与 Guid 数组长度一致
        int32 Count;
        if (NodeObject->TryGetNumberField(TEXT("count"), Count))
        {
            if (Count != GuidArray->Num())
            {
                UE_LOG(LogMASceneGraphManager, Warning, TEXT("ValidateNodeJsonStructure: count (%d) does not match Guid array length (%d)"), 
                    Count, GuidArray->Num());
                // 这是警告，不是错误，允许继续
            }
        }
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("ValidateNodeJsonStructure: Validation passed for node id=%s, shape.type=%s"), *NodeId, *ShapeType);
    return true;
}
