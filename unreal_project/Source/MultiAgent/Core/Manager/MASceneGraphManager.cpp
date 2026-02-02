// MASceneGraphManager.cpp
// 场景图管理器实现

#include "MASceneGraphManager.h"
#include "scene_graph_tools/MASceneGraphIO.h"
#include "scene_graph_tools/MASceneGraphQuery.h"
#include "scene_graph_tools/MADynamicNodeManager.h"
#include "../../Agent/Skill/Utils/MALocationUtils.h"
#include "../Comm/MACommSubsystem.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphManager, Log, All);

//=============================================================================
// 生命周期
//=============================================================================

void UMASceneGraphManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initializing"));

    // 从 MAConfigManager 获取 RunMode 并缓存
    CachedRunMode = EMARunMode::Edit;  // 默认值
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMAConfigManager* ConfigManager = GameInstance->GetSubsystem<UMAConfigManager>())
        {
            CachedRunMode = ConfigManager->GetRunMode();
            SourceFilePath = ConfigManager->GetSceneGraphFilePath();
            UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager: RunMode=%s, SourceFilePath=%s"),
                CachedRunMode == EMARunMode::Modify ? TEXT("Modify") : TEXT("Edit"),
                *SourceFilePath);
        }
        else
        {
            UE_LOG(LogMASceneGraphManager, Warning, TEXT("MASceneGraphManager: ConfigManager not available, using defaults"));
        }
    }

    // 初始化 Working Copy
    if (!InitializeWorkingCopy())
    {
        if (CachedRunMode == EMARunMode::Modify)
        {
            // Modify 模式: 创建空图
            UE_LOG(LogMASceneGraphManager, Warning, TEXT("MASceneGraphManager: Source file not found in Modify mode, creating empty graph"));
            CreateEmptySceneGraph();
        }
        else
        {
            // Edit 模式: 记录错误，创建空图
            UE_LOG(LogMASceneGraphManager, Error, TEXT("MASceneGraphManager: Source file not found in Edit mode: %s"), *SourceFilePath);
            CreateEmptySceneGraph();
        }
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

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initialized with %d static nodes and %d dynamic nodes"), 
        StaticNodes.Num(), DynamicNodes.Num());
}

void UMASceneGraphManager::Deinitialize()
{
    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager deinitializing"));

    // 清理内存数据
    WorkingCopy.Reset();
    StaticNodes.Empty();
    DynamicNodes.Empty();
    SourceFilePath.Empty();

    Super::Deinitialize();
}

//=============================================================================
// 核心 API
//=============================================================================

bool UMASceneGraphManager::ParseLabelInput(const FString& InputText, FString& OutId, FString& OutType, FString& OutErrorMessage)
{
    // 支持两种格式:
    // 1. 旧格式: "id:值,type:值" - id 必填
    // 2. 新格式: "cate:值,type:值" - id 自动分配

    OutId.Empty();
    OutType.Empty();
    OutErrorMessage.Empty();

    if (InputText.IsEmpty())
    {
        OutErrorMessage = TEXT("输入格式错误，请使用: cate:building/trans_facility/prop,type:值");
        return false;
    }

    // 按逗号分割
    TArray<FString> Parts;
    InputText.ParseIntoArray(Parts, TEXT(","), true);

    bool bFoundId = false;
    bool bFoundType = false;
    bool bFoundCate = false;  // 新格式标志

    for (const FString& Part : Parts)
    {
        // 按冒号分割键值对
        FString Key, Value;
        if (Part.Split(TEXT(":"), &Key, &Value))
        {
            Key = Key.TrimStartAndEnd();
            Value = Value.TrimStartAndEnd();

            // 键名大小写不敏感
            FString KeyLower = Key.ToLower();
            
            if (KeyLower == TEXT("id"))
            {
                if (Value.IsEmpty())
                {
                    OutErrorMessage = TEXT("缺少必填字段: id");
                    return false;
                }
                OutId = Value;
                bFoundId = true;
            }
            else if (KeyLower == TEXT("type"))
            {
                if (Value.IsEmpty())
                {
                    OutErrorMessage = TEXT("缺少必填字段: type");
                    return false;
                }
                OutType = Value;
                bFoundType = true;
            }
            else if (KeyLower == TEXT("cate") || KeyLower == TEXT("category"))
            {
                // 新格式: cate:building/trans_facility/prop
                FString ValueLower = Value.ToLower();
                if (ValueLower != TEXT("building") && ValueLower != TEXT("trans_facility") && ValueLower != TEXT("prop"))
                {
                    OutErrorMessage = FString::Printf(TEXT("无效的 cate 值: %s (应为 building, trans_facility 或 prop)"), *Value);
                    return false;
                }
                bFoundCate = true;
            }
        }
    }

    // 验证必填字段
    if (!bFoundType)
    {
        OutErrorMessage = TEXT("缺少必填字段: type");
        return false;
    }

    // 如果使用新格式 (cate:xxx) 且没有指定 id，自动分配
    if (bFoundCate && !bFoundId)
    {
        OutId = GetNextAvailableId();
        UE_LOG(LogMASceneGraphManager, Log, TEXT("ParseLabelInput: Auto-assigned ID = %s"), *OutId);
    }
    else if (!bFoundId)
    {
        // 旧格式必须有 id
        OutErrorMessage = TEXT("缺少必填字段: id (或使用 cate:xxx 格式自动分配)");
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("ParseLabelInput: id=%s, type=%s, cate=%s"), 
        *OutId, *OutType, bFoundCate ? TEXT("true") : TEXT("false"));
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

    if (!WorkingCopy.IsValid())
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

    if (!WorkingCopy.IsValid())
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

FString UMASceneGraphManager::GetNextAvailableId() const
{
    if (!WorkingCopy.IsValid())
    {
        return TEXT("1");
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        return TEXT("1");
    }

    // 遍历节点找到最大数字 ID
    int32 MaxId = 0;
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            if (NodeObject.IsValid())
            {
                FString IdStr;
                if (NodeObject->TryGetStringField(TEXT("id"), IdStr))
                {
                    // 尝试将 ID 转换为数字
                    if (IdStr.IsNumeric())
                    {
                        int32 IdNum = FCString::Atoi(*IdStr);
                        if (IdNum > MaxId)
                        {
                            MaxId = IdNum;
                        }
                    }
                }
            }
        }
    }

    // 返回 (最大 ID + 1) 的字符串
    int32 NextId = MaxId + 1;
    UE_LOG(LogMASceneGraphManager, Log, TEXT("GetNextAvailableId: Max ID = %d, Next ID = %d"), MaxId, NextId);
    return FString::FromInt(NextId);
}

bool UMASceneGraphManager::AddSceneNode(const FString& Id, const FString& Type, const FVector& WorldLocation,
                                         AActor* Actor, FString& OutLabel, FString& OutErrorMessage)
{
    // 重构: 委托给统一的 AddNode API + SaveToSource
    OutLabel.Empty();
    OutErrorMessage.Empty();

    // 生成标签
    OutLabel = GenerateLabel(Type);

    // 获取 Actor GUID
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

    // 构建节点 JSON
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

    // 序列化为 JSON 字符串
    FString NodeJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJson);
    FJsonSerializer::Serialize(NewNode.ToSharedRef(), Writer);

    // 调用统一的 AddNode API
    if (!AddNode(NodeJson, OutErrorMessage))
    {
        return false;
    }

    // 保存到源文件 (仅 Modify 模式会实际保存)
    SaveToSource();

    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddSceneNode: id=%s, guid=%s, type=%s, label=%s, location=(%f, %f, %f)"),
        *Id, *GuidString, *Type, *OutLabel, WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

    return true;
}



//=============================================================================
// 动态节点加载
//=============================================================================

void UMASceneGraphManager::LoadDynamicNodes()
{
    // 从 MAConfigManager 获取已解析的配置数据
    // 配置数据来自 config/maps/{MapType}.json 文件

    DynamicNodes.Empty();

    // 获取 ConfigManager
    UMAConfigManager* ConfigManager = nullptr;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        ConfigManager = GameInstance->GetSubsystem<UMAConfigManager>();
    }

    if (!ConfigManager)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("LoadDynamicNodes: ConfigManager not available, cannot load dynamic nodes"));
        return;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loading from ConfigManager (MapType=%s)"), *ConfigManager->GetMapType());

    // 加载机器人节点
    for (const FMAAgentConfigData& AgentConfig : ConfigManager->AgentConfigs)
    {
        FMASceneGraphNode Node = FMADynamicNodeManager::CreateAgentNode(AgentConfig);
        DynamicNodes.Add(Node);
        
        UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Created agent node - Id: %s, Type: %s, Label: %s"),
            *Node.Id, *Node.Type, *Node.Label);
    }
    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loaded %d agent nodes"), ConfigManager->AgentConfigs.Num());

    // 加载环境对象节点
    for (const FMAEnvironmentObjectConfig& EnvConfig : ConfigManager->EnvironmentObjects)
    {
        FMASceneGraphNode Node = FMADynamicNodeManager::CreateEnvironmentObjectNode(EnvConfig);
        DynamicNodes.Add(Node);
        
        UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Created %s node - Id: %s, Label: %s"),
            *EnvConfig.Type, *Node.Id, *Node.Label);
    }
    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loaded %d environment object nodes"), ConfigManager->EnvironmentObjects.Num());

    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Total %d dynamic nodes loaded"), DynamicNodes.Num());
    
    // 需要先合并静态节点和动态节点，以便计算时能访问所有地点节点
    TArray<FMASceneGraphNode> AllNodesForLocationCalc = GetAllNodes();
    
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
// 动态节点管理接口
//=============================================================================

FMASceneGraphNode* UMASceneGraphManager::FindDynamicNodeByIdMutable(const FString& NodeId)
{
    for (FMASceneGraphNode& DynNode : DynamicNodes)
    {
        if (DynNode.Id == NodeId || DynNode.Label == NodeId)
        {
            return &DynNode;
        }
    }
    return nullptr;
}

void UMASceneGraphManager::UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition)
{
    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(NodeId);
    if (!Node)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateDynamicNodePosition: Node not found: %s"), *NodeId);
        return;
    }

    if (FMADynamicNodeManager::UpdateNodePosition(*Node, NewPosition))
    {
        // 重新计算 LocationLabel
        TArray<FMASceneGraphNode> AllNodes = GetAllNodes();
        Node->LocationLabel = FMALocationUtils::InferNearestLocationLabel(
            AllNodes,
            NewPosition,
            5000.f,
            Node->Id
        );
        
        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("UpdateDynamicNodePosition: Updated %s to (%f, %f, %f), location_label='%s'"), 
            *NodeId, NewPosition.X, NewPosition.Y, NewPosition.Z, *Node->LocationLabel);
    }
}

void UMASceneGraphManager::UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value)
{
    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(NodeId);
    if (!Node)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateDynamicNodeFeature: Node not found: %s"), *NodeId);
        return;
    }

    if (FMADynamicNodeManager::UpdateNodeFeature(*Node, Key, Value))
    {
        UE_LOG(LogMASceneGraphManager, Verbose, TEXT("UpdateDynamicNodeFeature: Updated %s - %s=%s"), 
            *NodeId, *Key, *Value);
    }
}

bool UMASceneGraphManager::BindDynamicNodeGuid(const FString& NodeIdOrLabel, const FString& ActorGuid)
{
    if (NodeIdOrLabel.IsEmpty())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("BindDynamicNodeGuid: NodeIdOrLabel 为空"));
        return false;
    }

    if (ActorGuid.IsEmpty())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("BindDynamicNodeGuid: ActorGuid 为空"));
        return false;
    }

    // 在 DynamicNodes 中查找匹配的节点 (按 Id 或 Label)
    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(NodeIdOrLabel);
    if (!Node)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("BindDynamicNodeGuid: 未找到节点: %s"), *NodeIdOrLabel);
        return false;
    }

    // 调用 MADynamicNodeManager::UpdateNodeGuid() 更新节点
    if (!FMADynamicNodeManager::UpdateNodeGuid(*Node, ActorGuid))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("BindDynamicNodeGuid: 更新节点 GUID 失败: %s"), *NodeIdOrLabel);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("BindDynamicNodeGuid: 成功绑定节点 %s (Label: %s) 到 GUID %s"), 
        *Node->Id, *Node->Label, *ActorGuid);
    return true;
}

bool UMASceneGraphManager::EditDynamicNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditDynamicNode: NodeId 为空"));
        return false;
    }

    if (NewNodeJson.IsEmpty())
    {
        OutError = TEXT("新节点 JSON 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditDynamicNode: NewNodeJson 为空"));
        return false;
    }

    // 解析 JSON 字符串
    TSharedPtr<FJsonObject> NewNodeObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NewNodeJson);
    if (!FJsonSerializer::Deserialize(Reader, NewNodeObject) || !NewNodeObject.IsValid())
    {
        OutError = TEXT("JSON 解析失败");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditDynamicNode: JSON 解析失败: %s"), *NewNodeJson);
        return false;
    }

    // 验证 JSON 结构
    if (!ValidateNodeJsonStructure(NewNodeObject, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditDynamicNode: JSON 验证失败: %s"), *OutError);
        return false;
    }

    // 查找动态节点
    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(NodeId);
    if (!Node)
    {
        OutError = FString::Printf(TEXT("动态节点不存在: %s"), *NodeId);
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("EditDynamicNode: 未找到动态节点: %s"), *NodeId);
        return false;
    }

    // 从 JSON 更新节点内存数据
    // 更新 id (如果提供)
    FString NewId;
    if (NewNodeObject->TryGetStringField(TEXT("id"), NewId) && !NewId.IsEmpty())
    {
        Node->Id = NewId;
    }

    // 更新 guid (如果提供)
    FString NewGuid;
    if (NewNodeObject->TryGetStringField(TEXT("guid"), NewGuid))
    {
        Node->Guid = NewGuid;
    }

    // 更新 properties
    const TSharedPtr<FJsonObject>* PropertiesObject;
    if (NewNodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject) && PropertiesObject->IsValid())
    {
        // 更新 type
        FString NewType;
        if ((*PropertiesObject)->TryGetStringField(TEXT("type"), NewType) && !NewType.IsEmpty())
        {
            Node->Type = NewType;
        }

        // 更新 label
        FString NewLabel;
        if ((*PropertiesObject)->TryGetStringField(TEXT("label"), NewLabel) && !NewLabel.IsEmpty())
        {
            Node->Label = NewLabel;
        }

        // 更新 category
        FString NewCategory;
        if ((*PropertiesObject)->TryGetStringField(TEXT("category"), NewCategory) && !NewCategory.IsEmpty())
        {
            Node->Category = NewCategory;
        }

        // 更新 is_dynamic
        bool bNewIsDynamic;
        if ((*PropertiesObject)->TryGetBoolField(TEXT("is_dynamic"), bNewIsDynamic))
        {
            Node->bIsDynamic = bNewIsDynamic;
        }

        // 更新 Features (遍历 properties 中的其他字段)
        for (const auto& Pair : (*PropertiesObject)->Values)
        {
            const FString& Key = Pair.Key;
            // 跳过已处理的标准字段
            if (Key == TEXT("type") || Key == TEXT("label") || Key == TEXT("category") || Key == TEXT("is_dynamic"))
            {
                continue;
            }

            // 将其他字段作为 Feature 添加
            FString Value;
            if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
            {
                Node->Features.Add(Key, Value);
            }
        }
    }

    // 更新 shape
    const TSharedPtr<FJsonObject>* ShapeObject;
    if (NewNodeObject->TryGetObjectField(TEXT("shape"), ShapeObject) && ShapeObject->IsValid())
    {
        // 更新 shape type
        FString NewShapeType;
        if ((*ShapeObject)->TryGetStringField(TEXT("type"), NewShapeType) && !NewShapeType.IsEmpty())
        {
            Node->ShapeType = NewShapeType;
        }

        // 更新 center
        const TArray<TSharedPtr<FJsonValue>>* CenterArray;
        if ((*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray) && CenterArray->Num() == 3)
        {
            Node->Center.X = (*CenterArray)[0]->AsNumber();
            Node->Center.Y = (*CenterArray)[1]->AsNumber();
            Node->Center.Z = (*CenterArray)[2]->AsNumber();
        }
    }

    // 重新生成 RawJson
    Node->RawJson = FMADynamicNodeManager::GenerateRawJson(*Node);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("EditDynamicNode: 成功编辑动态节点 %s"), *NodeId);
    return true;
}


//=============================================================================
// 世界状态查询
//=============================================================================

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


//=============================================================================
// JSON 文件 I/O，委托给 MASceneGraphIO
//=============================================================================

FString UMASceneGraphManager::GetSceneGraphFilePath() const
{
    // 优先使用缓存的 SourceFilePath
    if (!SourceFilePath.IsEmpty())
    {
        return SourceFilePath;
    }
    return FMASceneGraphIO::GetSceneGraphFilePath(this);
}

bool UMASceneGraphManager::LoadSceneGraph()
{
    FString FilePath = GetSceneGraphFilePath();
    return FMASceneGraphIO::LoadBaseSceneGraph(FilePath, WorkingCopy);
}

bool UMASceneGraphManager::SaveSceneGraph()
{
    if (!WorkingCopy.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Cannot save: WorkingCopy is invalid"));
        return false;
    }

    FString FilePath = GetSceneGraphFilePath();
    return FMASceneGraphIO::SaveSceneGraph(FilePath, WorkingCopy);
}

void UMASceneGraphManager::CreateEmptySceneGraph()
{

    WorkingCopy = MakeShareable(new FJsonObject());

    // 创建空的 nodes 数组
    TArray<TSharedPtr<FJsonValue>> EmptyNodes;
    WorkingCopy->SetArrayField(TEXT("nodes"), EmptyNodes);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("Created empty scene graph structure"));
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

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllGoals() const
{
    return FMASceneGraphQuery::GetAllGoals(GetAllNodes());
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllZones() const
{
    return FMASceneGraphQuery::GetAllZones(GetAllNodes());
}

FMASceneGraphNode UMASceneGraphManager::GetNodeById(const FString& NodeId) const
{
    return FMASceneGraphQuery::GetNodeById(GetAllNodes(), NodeId);
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllNodes() const
{
    return FMASceneGraphQuery::GetAllNodes(StaticNodes, DynamicNodes);
}

TArray<FMASceneGraphNode> UMASceneGraphManager::FindNodesByGuid(const FString& ActorGuid) const
{
    TArray<FMASceneGraphNode> AllNodes = GetAllNodes();
    return FMASceneGraphQuery::FindNodesByGuid(AllNodes, ActorGuid);
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

TSharedPtr<FJsonObject> UMASceneGraphManager::NodeToJsonObject(const FMASceneGraphNode& Node) const
{
    return FMASceneGraphQuery::NodeToJsonObject(Node);
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
    if (!WorkingCopy.IsValid())
    {
        return nullptr;
    }

    // 获取 nodes 数组
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!WorkingCopy->TryGetArrayField(TEXT("nodes"), NodesArray))
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
// 统一图操作 API 实现
//=============================================================================

bool UMASceneGraphManager::InitializeWorkingCopy()
{
    // 从源文件加载数据到 WorkingCopy
    if (SourceFilePath.IsEmpty())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("InitializeWorkingCopy: SourceFilePath is empty"));
        return false;
    }

    return FMASceneGraphIO::LoadBaseSceneGraph(SourceFilePath, WorkingCopy);
}

bool UMASceneGraphManager::AddNode(const FString& NodeJson, FString& OutError)
{
    OutError.Empty();

    if (NodeJson.IsEmpty())
    {
        OutError = TEXT("JSON 字符串为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: JSON string is empty"));
        return false;
    }

    // 解析 JSON 字符串
    TSharedPtr<FJsonObject> NodeObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodeJson);
    if (!FJsonSerializer::Deserialize(Reader, NodeObject) || !NodeObject.IsValid())
    {
        OutError = TEXT("JSON 解析失败");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: Failed to parse JSON: %s"), *NodeJson);
        return false;
    }

    // 提取 ID 用于重复检查
    FString NodeId;
    if (!NodeObject->TryGetStringField(TEXT("id"), NodeId))
    {
        OutError = TEXT("JSON 缺少 id 字段");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: JSON missing 'id' field"));
        return false;
    }

    // 检查 ID 是否已存在
    if (IsIdExists(NodeId))
    {
        OutError = FString::Printf(TEXT("ID 已存在: %s"), *NodeId);
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("AddNode: %s"), *OutError);
        return false;
    }

    // 验证 JSON 结构
    if (!ValidateNodeJsonStructure(NodeObject, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: JSON validation failed for node id=%s: %s"), *NodeId, *OutError);
        return false;
    }

    // 确保 WorkingCopy 有效
    if (!WorkingCopy.IsValid())
    {
        CreateEmptySceneGraph();
    }

    // 获取 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: Failed to get nodes array"));
        return false;
    }

    // 添加到 Working Copy 的 nodes 数组
    NodesArray->Add(MakeShareable(new FJsonValueObject(NodeObject)));

    // 重新解析 StaticNodes 数组
    StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddNode: Successfully added node with id=%s"), *NodeId);

    // 发送场景变更通知 (仅 Edit 模式)
    NotifySceneChange(TEXT("add_node"), NodeJson);

    return true;
}

bool UMASceneGraphManager::DeleteNode(const FString& NodeId, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("DeleteNode: NodeId is empty"));
        return false;
    }

    // 确保 WorkingCopy 有效
    if (!WorkingCopy.IsValid())
    {
        OutError = TEXT("Working Copy 无效");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("DeleteNode: WorkingCopy is invalid"));
        return false;
    }

    // 获取 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("DeleteNode: Failed to get nodes array"));
        return false;
    }

    // 查找节点索引
    int32 NodeIndex = INDEX_NONE;
    FString DeletedNodeJson;
    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& NodeValue = (*NodesArray)[i];
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            if (NodeObject.IsValid())
            {
                FString CurrentId;
                if (NodeObject->TryGetStringField(TEXT("id"), CurrentId) && CurrentId == NodeId)
                {
                    NodeIndex = i;
                    // 保存被删除节点的 JSON 用于通知
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DeletedNodeJson);
                    FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
                    break;
                }
            }
        }
    }

    if (NodeIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("节点不存在: %s"), *NodeId);
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("DeleteNode: Node not found: %s"), *NodeId);
        return false;
    }

    // 从 nodes 数组中移除
    NodesArray->RemoveAt(NodeIndex);

    // TODO: 删除相关的 edges (如果有 edges 数组)
    // 当前实现暂不处理 edges

    // 重新解析 StaticNodes 数组
    StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("DeleteNode: Successfully deleted node with id=%s"), *NodeId);

    // 发送场景变更通知 (仅 Edit 模式)
    NotifySceneChange(TEXT("delete_node"), DeletedNodeJson);

    return true;
}

bool UMASceneGraphManager::EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditNode: NodeId is empty"));
        return false;
    }

    if (NewNodeJson.IsEmpty())
    {
        OutError = TEXT("新节点 JSON 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditNode: NewNodeJson is empty"));
        return false;
    }

    // 根据节点类型分发到对应的编辑函数
    if (FindDynamicNodeByIdMutable(NodeId) != nullptr)
    {
        return EditDynamicNode(NodeId, NewNodeJson, OutError);
    }
    return EditStaticNode(NodeId, NewNodeJson, OutError);
}

bool UMASceneGraphManager::EditStaticNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    // 解析新的 JSON 字符串
    TSharedPtr<FJsonObject> NewNodeObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NewNodeJson);
    if (!FJsonSerializer::Deserialize(Reader, NewNodeObject) || !NewNodeObject.IsValid())
    {
        OutError = TEXT("新节点 JSON 解析失败");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditStaticNode: Failed to parse JSON: %s"), *NewNodeJson);
        return false;
    }

    // 验证新 JSON 结构
    if (!ValidateNodeJsonStructure(NewNodeObject, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditStaticNode: JSON validation failed: %s"), *OutError);
        return false;
    }

    // 确保 WorkingCopy 有效
    if (!WorkingCopy.IsValid())
    {
        OutError = TEXT("Working Copy 无效");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditStaticNode: WorkingCopy is invalid"));
        return false;
    }

    // 获取 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditStaticNode: Failed to get nodes array"));
        return false;
    }

    // 查找节点索引
    int32 NodeIndex = INDEX_NONE;
    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& NodeValue = (*NodesArray)[i];
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            if (NodeObject.IsValid())
            {
                FString CurrentId;
                if (NodeObject->TryGetStringField(TEXT("id"), CurrentId) && CurrentId == NodeId)
                {
                    NodeIndex = i;
                    break;
                }
            }
        }
    }

    if (NodeIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("静态节点不存在: %s"), *NodeId);
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("EditStaticNode: Node not found: %s"), *NodeId);
        return false;
    }

    // 替换节点数据
    (*NodesArray)[NodeIndex] = MakeShareable(new FJsonValueObject(NewNodeObject));

    // 重新解析 StaticNodes 数组
    StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("EditStaticNode: Successfully edited node %s"), *NodeId);

    // 发送场景变更通知 (仅 Edit 模式)
    NotifySceneChange(TEXT("edit_node"), NewNodeJson);

    return true;
}

bool UMASceneGraphManager::SaveToSource()
{
    // 检查当前运行模式
    if (CachedRunMode == EMARunMode::Edit)
    {
        // Edit 模式: 记录警告日志，返回 false
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("SaveToSource: Cannot save in Edit mode. Changes are only kept in memory."));
        return false;
    }

    // Modify 模式: 调用 MASceneGraphIO::SaveSceneGraph() 保存到源文件
    if (!WorkingCopy.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: WorkingCopy is invalid"));
        return false;
    }

    FString FilePath = GetSceneGraphFilePath();
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: SourceFilePath is empty"));
        return false;
    }

    bool bSuccess = FMASceneGraphIO::SaveSceneGraph(FilePath, WorkingCopy);
    if (bSuccess)
    {
        UE_LOG(LogMASceneGraphManager, Log, TEXT("SaveToSource: Successfully saved to %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: Failed to save to %s"), *FilePath);
    }

    return bSuccess;
}

bool UMASceneGraphManager::SetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SetNodeAsGoal: NodeId is empty"));
        return false;
    }

    // 获取 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SetNodeAsGoal: Failed to get nodes array"));
        return false;
    }

    // 查找节点并获取其 JSON 对象
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

        FString CurrentId;
        if (!NodeObject->TryGetStringField(TEXT("id"), CurrentId) || CurrentId != NodeId)
        {
            continue;
        }

        // 找到节点，获取或创建 properties 对象
        TSharedPtr<FJsonObject> Properties;
        const TSharedPtr<FJsonObject>* ExistingProps = nullptr;
        if (NodeObject->TryGetObjectField(TEXT("properties"), ExistingProps) && ExistingProps && (*ExistingProps).IsValid())
        {
            Properties = *ExistingProps;
        }
        else
        {
            Properties = MakeShareable(new FJsonObject());
            NodeObject->SetObjectField(TEXT("properties"), Properties);
        }

        // 设置 is_goal 为 true
        Properties->SetBoolField(TEXT("is_goal"), true);

        // 序列化修改后的节点 JSON
        FString ModifiedNodeJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ModifiedNodeJson);
        FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);

        // 调用 EditNode 完成更新（包括重新解析和通知）
        return EditNode(NodeId, ModifiedNodeJson, OutError);
    }

    OutError = FString::Printf(TEXT("节点不存在: %s"), *NodeId);
    UE_LOG(LogMASceneGraphManager, Warning, TEXT("SetNodeAsGoal: Node not found: %s"), *NodeId);
    return false;
}

bool UMASceneGraphManager::UnsetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("UnsetNodeAsGoal: NodeId is empty"));
        return false;
    }

    // 获取 nodes 数组
    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("UnsetNodeAsGoal: Failed to get nodes array"));
        return false;
    }

    // 查找节点并获取其 JSON 对象
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

        FString CurrentId;
        if (!NodeObject->TryGetStringField(TEXT("id"), CurrentId) || CurrentId != NodeId)
        {
            continue;
        }

        // 找到节点，获取 properties 对象并移除 is_goal 字段
        const TSharedPtr<FJsonObject>* ExistingProps = nullptr;
        if (NodeObject->TryGetObjectField(TEXT("properties"), ExistingProps) && ExistingProps && (*ExistingProps).IsValid())
        {
            (*ExistingProps)->RemoveField(TEXT("is_goal"));
        }

        // 序列化修改后的节点 JSON
        FString ModifiedNodeJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ModifiedNodeJson);
        FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);

        // 调用 EditNode 完成更新（包括重新解析和通知）
        return EditNode(NodeId, ModifiedNodeJson, OutError);
    }

    OutError = FString::Printf(TEXT("节点不存在: %s"), *NodeId);
    UE_LOG(LogMASceneGraphManager, Warning, TEXT("UnsetNodeAsGoal: Node not found: %s"), *NodeId);
    return false;
}

bool UMASceneGraphManager::IsNodeGoal(const FString& NodeId) const
{
    if (NodeId.IsEmpty())
    {
        return false;
    }

    // 使用 GetNodeById 查找节点
    FMASceneGraphNode Node = GetNodeById(NodeId);
    if (!Node.IsValid())
    {
        return false;
    }

    // 通过 RawJson 检查 is_goal 字段
    return Node.RawJson.Contains(TEXT("\"is_goal\"")) &&
           (Node.RawJson.Contains(TEXT("\"is_goal\": true")) || 
            Node.RawJson.Contains(TEXT("\"is_goal\":true")));
}

void UMASceneGraphManager::NotifySceneChange(const FString& ChangeType, const FString& NodeJson)
{
    // 检查当前运行模式
    if (CachedRunMode != EMARunMode::Edit)
    {
        // Modify 模式: 不发送消息
        return;
    }

    // Edit 模式: 通过 MACommSubsystem 发送场景变更消息
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
        {
            FMASceneChangeMessage Message;
            Message.ChangeType = EMASceneChangeType::AddNode;  // 默认值
            
            if (ChangeType == TEXT("add_node"))
            {
                Message.ChangeType = EMASceneChangeType::AddNode;
            }
            else if (ChangeType == TEXT("delete_node"))
            {
                Message.ChangeType = EMASceneChangeType::DeleteNode;
            }
            else if (ChangeType == TEXT("edit_node"))
            {
                Message.ChangeType = EMASceneChangeType::EditNode;
            }
            
            Message.Payload = NodeJson;
            
            CommSubsystem->SendSceneChangeMessage(Message);
            
            UE_LOG(LogMASceneGraphManager, Log, TEXT("NotifySceneChange: Sent %s message"), *ChangeType);
        }
        else
        {
            UE_LOG(LogMASceneGraphManager, Warning, TEXT("NotifySceneChange: MACommSubsystem not available"));
        }
    }
}