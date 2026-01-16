// MASceneGraphManager.cpp
// 场景图管理器实现

#include "MASceneGraphManager.h"
#include "scene_graph_tools/MASceneGraphIO.h"
#include "scene_graph_tools/MASceneGraphQuery.h"
#include "scene_graph_tools/MADynamicNodeManager.h"
#include "../../Agent/Skill/Utils/MALocationUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphManager, Log, All);

// 场景图文件名常量
const FString UMASceneGraphManager::SceneGraphFileName = TEXT("datasets/scene_graph_cyberworld.json");

//=============================================================================
// 静态辅助方法 - 几何中心计算
//=============================================================================

FVector UMASceneGraphManager::CalculatePolygonCentroid(const TArray<TSharedPtr<FJsonValue>>& Vertices)
{
    
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

    // 加载场景图数据 (原始 JSON)
    if (!LoadSceneGraph())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("Failed to load scene graph, creating empty structure"));
        CreateEmptySceneGraph();
    }

    // 使用 MASceneGraphIO 解析静态节点
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (NodesArray)
    {
        StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);
        UE_LOG(LogMASceneGraphManager, Log, TEXT("Loaded %d static nodes from scene graph"), StaticNodes.Num());
    }

    // 加载动态节点 (机器人、可拾取物品、充电站)
    LoadDynamicNodes();

    // 使缓存失效，下次 GetAllNodes() 时重建
    InvalidateCache();

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initialized with %d static nodes and %d dynamic nodes"), 
        StaticNodes.Num(), DynamicNodes.Num());
}

void UMASceneGraphManager::Deinitialize()
{
    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager deinitializing"));

    // 清理内存数据
    SceneGraphData.Reset();
    StaticNodes.Empty();
    DynamicNodes.Empty();
    CachedAllNodes.Empty();
    bCacheValid = false;

    Super::Deinitialize();
}

//=============================================================================
// 核心 API
//=============================================================================

bool UMASceneGraphManager::ParseLabelInput(const FString& InputText, FString& OutId, FString& OutType, FString& OutErrorMessage)
{
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
            Key = Key.TrimStartAndEnd();
            Value = Value.TrimStartAndEnd();

            // 键名大小写不敏感
            if (Key.ToLower() == TEXT("id"))
            {
                if (Value.IsEmpty())
                {
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
                    OutErrorMessage = TEXT("缺少必填字段: type");
                    return false;
                }
                OutType = Value;
                bFoundType = true;
            }
        }
    }
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
    // 生成格式: "Type-N"
    FString CapitalizedType = CapitalizeFirstLetter(Type);
    int32 Count = GetTypeCount(Type);
    int32 NextNumber = Count + 1;
    FString Label = FString::Printf(TEXT("%s-%d"), *CapitalizedType, NextNumber);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("GenerateLabel: type=%s, count=%d, label=%s"), *Type, Count, *Label);
    return Label;
}

bool UMASceneGraphManager::IsIdExists(const FString& Id) const
{

    if (!SceneGraphData.IsValid())
    {
        return false;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        return false;
    }
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

    OutLabel.Empty();
    OutErrorMessage.Empty();
    if (IsIdExists(Id))
    {
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
    OutLabel = GenerateLabel(Type);
    FString GuidString;
    if (Actor != nullptr)
    {
        GuidString = Actor->GetActorGuid().ToString();
        UE_LOG(LogMASceneGraphManager, Log, TEXT("AddSceneNode: Extracted GUID=%s from Actor=%s"), *GuidString, *Actor->GetName());
    }
    else
    {
        GuidString = TEXT("");
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("AddSceneNode: Actor is nullptr, GUID will be empty"));
    }
    TSharedPtr<FJsonObject> NewNode = MakeShareable(new FJsonObject());
    NewNode->SetStringField(TEXT("id"), Id);
    NewNode->SetStringField(TEXT("guid"), GuidString);
    TSharedPtr<FJsonObject> Properties = MakeShareable(new FJsonObject());
    Properties->SetStringField(TEXT("type"), Type);
    Properties->SetStringField(TEXT("label"), OutLabel);
    NewNode->SetObjectField(TEXT("properties"), Properties);
    TSharedPtr<FJsonObject> Shape = MakeShareable(new FJsonObject());
    Shape->SetStringField(TEXT("type"), TEXT("point"));
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.Z)));
    Shape->SetArrayField(TEXT("center"), CenterArray);

    NewNode->SetObjectField(TEXT("shape"), Shape);
    NodesArray->Add(MakeShareable(new FJsonValueObject(NewNode)));
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
    return FPaths::ProjectDir() / SceneGraphFileName;
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllNodes() const
{
    // 使用缓存机制，合并静态节点和动态节点

    if (!bCacheValid)
    {
        RebuildCache();
    }

    return CachedAllNodes;
}

TArray<FMASceneGraphNode> UMASceneGraphManager::FindNodesByGuid(const FString& ActorGuid) const
{
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
    UE_LOG(LogMASceneGraphManager, Log, TEXT("FindNodesByGuid: Found %d nodes containing GUID %s"), 
        Result.Num(), *ActorGuid);

    return Result;
}

//=============================================================================
// JSON 文件 I/O
//=============================================================================

bool UMASceneGraphManager::LoadSceneGraph()
{

    FString FilePath = GetSceneGraphFilePath();

    // 检查文件是否存在
    if (!FPaths::FileExists(FilePath))
    {
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

//=============================================================================
// 动态节点加载
//=============================================================================

void UMASceneGraphManager::LoadDynamicNodes()
{
    // 从 agents.json 和 environment.json 加载动态节点

    DynamicNodes.Empty();

    // 获取配置文件路径
    // 注意: 配置文件在 unreal_project 的上一级目录的 config 文件夹中
    // 与 MAConfigManager::GetConfigRootPath() 保持一致
    FString ConfigDir = FPaths::ProjectDir() / TEXT("..") / TEXT("config");
    FString AgentsJsonPath = ConfigDir / TEXT("agents.json");
    FString EnvironmentJsonPath = ConfigDir / TEXT("environment.json");
    
    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: ConfigDir=%s"), *FPaths::ConvertRelativePathToFull(ConfigDir));

    // 加载机器人节点
    TArray<FMASceneGraphNode> RobotNodes = FMADynamicNodeManager::CreateRobotNodes(AgentsJsonPath);
    DynamicNodes.Append(RobotNodes);
    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loaded %d robot nodes from agents.json"), RobotNodes.Num());

    // 加载可拾取物品节点
    TArray<FMASceneGraphNode> PickupItemNodes = FMADynamicNodeManager::CreatePickupItemNodes(EnvironmentJsonPath);
    DynamicNodes.Append(PickupItemNodes);
    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loaded %d pickup item nodes from environment.json"), PickupItemNodes.Num());

    // 加载充电站节点
    TArray<FMASceneGraphNode> ChargingStationNodes = FMADynamicNodeManager::CreateChargingStationNodes(EnvironmentJsonPath);
    DynamicNodes.Append(ChargingStationNodes);
    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loaded %d charging station nodes from environment.json"), ChargingStationNodes.Num());

    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Total %d dynamic nodes loaded"), DynamicNodes.Num());
    // 需要先合并静态节点和动态节点，以便计算时能访问所有地点节点
    TArray<FMASceneGraphNode> AllNodesForLocationCalc;
    AllNodesForLocationCalc.Append(StaticNodes);
    AllNodesForLocationCalc.Append(DynamicNodes);
    
    for (FMASceneGraphNode& Node : DynamicNodes)
    {
        // 只为非地点类型的动态节点计算 LocationLabel
        if (!FMALocationUtils::IsLocationNode(Node))
        {
            Node.LocationLabel = FMALocationUtils::InferNearestLocationLabel(
                AllNodesForLocationCalc,
                Node.Center,
                5000.f,  // 最大搜索距离 5000cm = 50m
                Node.Id  // 排除自身
            );
            
            UE_LOG(LogMASceneGraphManager, Verbose, TEXT("LoadDynamicNodes: Node %s (%s) location_label = '%s'"),
                *Node.Id, *Node.Label, *Node.LocationLabel);
        }
    }
}

//=============================================================================
// 缓存管理
//=============================================================================

void UMASceneGraphManager::InvalidateCache()
{
    bCacheValid = false;
    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("InvalidateCache: Cache invalidated"));
}

void UMASceneGraphManager::RebuildCache() const
{
    // 合并静态节点和动态节点到缓存

    CachedAllNodes.Empty();
    CachedAllNodes.Reserve(StaticNodes.Num() + DynamicNodes.Num());

    // 添加静态节点
    CachedAllNodes.Append(StaticNodes);

    // 添加动态节点
    CachedAllNodes.Append(DynamicNodes);

    bCacheValid = true;

    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("RebuildCache: Cache rebuilt with %d static + %d dynamic = %d total nodes"), 
        StaticNodes.Num(), DynamicNodes.Num(), CachedAllNodes.Num());
}

//=============================================================================
// 分类查询接口 (委托给 MASceneGraphQuery)
//=============================================================================

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllBuildings() const
{
    return FMASceneGraphQuery::GetAllBuildings(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllRoads() const
{
    return FMASceneGraphQuery::GetAllRoads(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllIntersections() const
{
    return FMASceneGraphQuery::GetAllIntersections(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllProps() const
{
    return FMASceneGraphQuery::GetAllProps(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllRobots() const
{
    return FMASceneGraphQuery::GetAllRobots(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllPickupItems() const
{
    return FMASceneGraphQuery::GetAllPickupItems(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllChargingStations() const
{
    return FMASceneGraphQuery::GetAllChargingStations(GetAllNodes());
}

//=============================================================================
// 语义查询接口 (委托给 MASceneGraphQuery)
//=============================================================================

FMASceneGraphNode UMASceneGraphManager::FindNodeByLabel(const FMASemanticLabel& Label) const
{
    return FMASceneGraphQuery::FindNodeByLabel(GetAllNodes(), Label);
}

FMASceneGraphNode UMASceneGraphManager::FindNearestNode(const FMASemanticLabel& Label, const FVector& FromLocation) const
{
    return FMASceneGraphQuery::FindNearestNode(GetAllNodes(), Label, FromLocation);
}

TArray<FMASceneGraphNode> UMASceneGraphManager::FindNodesInBoundary(const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices) const
{
    return FMASceneGraphQuery::FindNodesInBoundary(GetAllNodes(), Label, BoundaryVertices);
}

bool UMASceneGraphManager::IsPointInsideBuilding(const FVector& Point) const
{
    return FMASceneGraphQuery::IsPointInsideBuilding(GetAllNodes(), Point);
}

FMASceneGraphNode UMASceneGraphManager::FindNearestLandmark(const FVector& Location, float MaxDistance) const
{
    return FMASceneGraphQuery::FindNearestLandmark(GetAllNodes(), Location, MaxDistance);
}

//=============================================================================
// 动态节点管理接口
//=============================================================================

FMASceneGraphNode* UMASceneGraphManager::FindDynamicNodeById(const FString& NodeId)
{
    // 首先尝试精确匹配 Id
    for (FMASceneGraphNode& Node : DynamicNodes)
    {
        if (Node.Id == NodeId)
        {
            return &Node;
        }
    }
    
    // 如果没找到，尝试匹配 Label 或 Features["label"]
    // 这是因为外部系统可能使用物品标签（如 "RedBox"）而不是 ID（如 "1001"）
    for (FMASceneGraphNode& Node : DynamicNodes)
    {
        if (Node.Label.Equals(NodeId, ESearchCase::IgnoreCase))
        {
            return &Node;
        }
        
        const FString* NodeLabel = Node.Features.Find(TEXT("label"));
        if (NodeLabel && NodeLabel->Equals(NodeId, ESearchCase::IgnoreCase))
        {
            return &Node;
        }
    }
    
    return nullptr;
}

void UMASceneGraphManager::UpdateRobotPosition(const FString& RobotId, const FVector& NewPosition)
{
    // 更新机器人位置并重新计算 LocationLabel

    FMASceneGraphNode* Node = FindDynamicNodeById(RobotId);
    if (!Node)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateRobotPosition: Robot node not found: %s"), *RobotId);
        return;
    }

    if (!Node->IsRobot())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateRobotPosition: Node %s is not a robot"), *RobotId);
        return;
    }

    if (FMADynamicNodeManager::UpdateNodePosition(*Node, NewPosition))
    {
        TArray<FMASceneGraphNode> AllNodes = GetAllNodes();
        Node->LocationLabel = FMALocationUtils::InferNearestLocationLabel(
            AllNodes,
            NewPosition,
            5000.f,
            Node->Id
        );
        
        InvalidateCache();
        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("UpdateRobotPosition: Updated %s to (%f, %f, %f), location_label='%s'"), 
            *RobotId, NewPosition.X, NewPosition.Y, NewPosition.Z, *Node->LocationLabel);
    }
}

void UMASceneGraphManager::UpdatePickupItemPosition(const FString& ItemId, const FVector& NewPosition)
{
    // 更新可拾取物品位置并重新计算 LocationLabel

    FMASceneGraphNode* Node = FindDynamicNodeById(ItemId);
    if (!Node)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdatePickupItemPosition: PickupItem node not found: %s"), *ItemId);
        return;
    }

    if (!Node->IsPickupItem())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdatePickupItemPosition: Node %s is not a pickup item"), *ItemId);
        return;
    }

    if (FMADynamicNodeManager::UpdateNodePosition(*Node, NewPosition))
    {
        TArray<FMASceneGraphNode> AllNodes = GetAllNodes();
        Node->LocationLabel = FMALocationUtils::InferNearestLocationLabel(
            AllNodes,
            NewPosition,
            5000.f,
            Node->Id
        );
        
        InvalidateCache();
        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("UpdatePickupItemPosition: Updated %s to (%f, %f, %f), location_label='%s'"), 
            *ItemId, NewPosition.X, NewPosition.Y, NewPosition.Z, *Node->LocationLabel);
    }
}

void UMASceneGraphManager::UpdatePickupItemCarrierStatus(const FString& ItemId, bool bIsCarried, const FString& CarrierId)
{
    // 更新可拾取物品携带状态

    FMASceneGraphNode* Node = FindDynamicNodeById(ItemId);
    if (!Node)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdatePickupItemCarrierStatus: PickupItem node not found: %s"), *ItemId);
        return;
    }

    if (!Node->IsPickupItem())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdatePickupItemCarrierStatus: Node %s is not a pickup item"), *ItemId);
        return;
    }

    if (FMADynamicNodeManager::UpdatePickupItemCarrierStatus(*Node, bIsCarried, CarrierId))
    {
        InvalidateCache();
        UE_LOG(LogMASceneGraphManager, Log, TEXT("UpdatePickupItemCarrierStatus: Updated %s - bIsCarried=%s, CarrierId=%s"), 
            *ItemId, bIsCarried ? TEXT("true") : TEXT("false"), *CarrierId);
    }
}


//=============================================================================
// HTTP API 支持 - 世界状态查询
//=============================================================================

TSharedPtr<FJsonObject> UMASceneGraphManager::NodeToJsonObject(const FMASceneGraphNode& Node) const
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

FString UMASceneGraphManager::BuildWorldStateJson(const FString& CategoryFilter, const FString& TypeFilter, const FString& LabelFilter) const
{
    
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    
    // 获取所有节点
    TArray<FMASceneGraphNode> AllNodes = GetAllNodes();
    
    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("BuildWorldStateJson: Processing %d nodes"), AllNodes.Num());
    
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        // 应用过滤条件 (多条件AND)
        bool bPassFilter = true;
        
        if (!CategoryFilter.IsEmpty() && !Node.Category.Equals(CategoryFilter, ESearchCase::IgnoreCase))
        {
            bPassFilter = false;
        }
        
        if (bPassFilter && !TypeFilter.IsEmpty() && !Node.Type.Equals(TypeFilter, ESearchCase::IgnoreCase))
        {
            bPassFilter = false;
        }
        
        if (bPassFilter && !LabelFilter.IsEmpty() && !Node.Label.Contains(LabelFilter, ESearchCase::IgnoreCase))
        {
            bPassFilter = false;
        }
        
        if (bPassFilter)
        {
            TSharedPtr<FJsonObject> NodeJson = NodeToJsonObject(Node);
            NodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
        }
    }
    
    UE_LOG(LogMASceneGraphManager, Log, TEXT("BuildWorldStateJson: Returning %d nodes after filtering"), NodesArray.Num());
    
    // 设置 nodes 和 edges 数组
    RootObject->SetArrayField(TEXT("nodes"), NodesArray);
    RootObject->SetArrayField(TEXT("edges"), EdgesArray);
    
    // 序列化为JSON字符串
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    
    return OutputString;
}
