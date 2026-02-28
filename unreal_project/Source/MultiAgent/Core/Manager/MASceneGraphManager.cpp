// MASceneGraphManager.cpp
// 场景图管理器实现

#include "MASceneGraphManager.h"
#include "scene_graph_tools/MASceneGraphIO.h"
#include "scene_graph_tools/MADynamicNodeManager.h"
#include "scene_graph_services/MASceneGraphQueryService.h"
#include "scene_graph_services/MASceneGraphCommandService.h"
#include "scene_graph_services/MASceneGraphCommandUseCases.h"
#include "scene_graph_adapters/MASceneGraphRepositoryAdapter.h"
#include "scene_graph_adapters/MASceneGraphEventPublisherAdapter.h"
#include "ue_tools/MAUESceneApplier.h"
#include "../../Agent/Skill/Utils/MALocationUtils.h"
#include "../Comm/MACommTypes.h"
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

    RepositoryPort = MakeShared<FMASceneGraphRepositoryAdapter>(this, SourceFilePath);
    EventPublisherPort = MakeShared<FMASceneGraphEventPublisherAdapter>(GetGameInstance(), CachedRunMode);

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

    QueryPort = MakeShared<FMASceneGraphQueryService>(&StaticNodes, &DynamicNodes);

    FMASceneGraphCommandService::FHandlers CommandHandlers;
    CommandHandlers.AddNode = [this](const FString& NodeJson, FString& OutError)
    {
        return AddNodeInternal(NodeJson, OutError);
    };
    CommandHandlers.DeleteNode = [this](const FString& NodeId, FString& OutError)
    {
        return DeleteNodeInternal(NodeId, OutError);
    };
    CommandHandlers.EditNode = [this](const FString& NodeId, const FString& NewNodeJson, FString& OutError)
    {
        return EditNodeInternal(NodeId, NewNodeJson, OutError);
    };
    CommandHandlers.SetNodeAsGoal = [this](const FString& NodeId, FString& OutError)
    {
        return SetNodeAsGoalInternal(NodeId, OutError);
    };
    CommandHandlers.UnsetNodeAsGoal = [this](const FString& NodeId, FString& OutError)
    {
        return UnsetNodeAsGoalInternal(NodeId, OutError);
    };
    CommandHandlers.UpdateDynamicNodePosition = [this](const FString& NodeId, const FVector& NewPosition)
    {
        UpdateDynamicNodePositionInternal(NodeId, NewPosition);
    };
    CommandHandlers.UpdateDynamicNodeFeature = [this](const FString& NodeId, const FString& Key, const FString& Value)
    {
        UpdateDynamicNodeFeatureInternal(NodeId, Key, Value);
    };
    CommandHandlers.SaveToSource = [this]()
    {
        return SaveToSourceInternal();
    };
    CommandPort = MakeShared<FMASceneGraphCommandService>(MoveTemp(CommandHandlers));

    // 加载动态节点
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
    QueryPort.Reset();
    CommandPort.Reset();
    RepositoryPort.Reset();
    EventPublisherPort.Reset();
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
    if (CommandPort.IsValid())
    {
        CommandPort->UpdateDynamicNodePosition(NodeId, NewPosition);
        return;
    }
    UpdateDynamicNodePositionInternal(NodeId, NewPosition);
}

void UMASceneGraphManager::UpdateDynamicNodePositionInternal(const FString& NodeId, const FVector& NewPosition)
{
    FString LocationLabel;
    FString Error;
    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext{DynamicNodes};
    if (!FMASceneGraphCommandUseCases::UpdateDynamicNodePosition(
        GraphContext, GetAllNodes(), NodeId, NewPosition, LocationLabel, Error))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateDynamicNodePosition: %s"), *Error);
        return;
    }

    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("UpdateDynamicNodePosition: Updated %s to (%f, %f, %f), location_label='%s'"),
        *NodeId, NewPosition.X, NewPosition.Y, NewPosition.Z, *LocationLabel);
}

void UMASceneGraphManager::UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value)
{
    if (CommandPort.IsValid())
    {
        CommandPort->UpdateDynamicNodeFeature(NodeId, Key, Value);
        return;
    }
    UpdateDynamicNodeFeatureInternal(NodeId, Key, Value);
}

void UMASceneGraphManager::UpdateDynamicNodeFeatureInternal(const FString& NodeId, const FString& Key, const FString& Value)
{
    FString Error;
    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext{DynamicNodes};
    if (!FMASceneGraphCommandUseCases::UpdateDynamicNodeFeature(GraphContext, NodeId, Key, Value, Error))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateDynamicNodeFeature: %s"), *Error);
        return;
    }

    UE_LOG(LogMASceneGraphManager, Verbose, TEXT("UpdateDynamicNodeFeature: Updated %s - %s=%s"),
        *NodeId, *Key, *Value);
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

    // 用 Actor 的实际位置校准节点 (Spawn 时的地面投影可能修改了 Z)
    if (UWorld* World = GetWorld())
    {
        FMAUESceneApplier::CalibrateNodeFromActor(World, *Node);
    }

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
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("BuildWorldStateJson: QueryPort not initialized"));
        return TEXT("{\"nodes\":[],\"edges\":[]}");
    }
    return QueryPort->BuildWorldStateJson(CategoryFilter, TypeFilter, LabelFilter);
}


//=============================================================================
// JSON 文件 I/O，委托给 MASceneGraphIO
//=============================================================================

FString UMASceneGraphManager::GetSceneGraphFilePath() const
{
    if (RepositoryPort.IsValid())
    {
        const FString PortPath = RepositoryPort->GetSourcePath();
        if (!PortPath.IsEmpty())
        {
            return PortPath;
        }
    }

    // 优先使用缓存的 SourceFilePath
    if (!SourceFilePath.IsEmpty())
    {
        return SourceFilePath;
    }
    return FMASceneGraphIO::GetSceneGraphFilePath(this);
}

bool UMASceneGraphManager::LoadSceneGraph()
{
    if (RepositoryPort.IsValid())
    {
        return RepositoryPort->Load(WorkingCopy);
    }

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

    if (RepositoryPort.IsValid())
    {
        return RepositoryPort->Save(WorkingCopy);
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
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllBuildings: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllBuildings();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllRoads() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllRoads: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllRoads();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllIntersections() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllIntersections: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllIntersections();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllProps() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllProps: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllProps();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllRobots() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllRobots: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllRobots();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllPickupItems() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllPickupItems: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllPickupItems();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllChargingStations() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllChargingStations: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllChargingStations();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllGoals() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllGoals: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllGoals();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllZones() const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllZones: QueryPort not initialized"));
        return {};
    }
    return QueryPort->GetAllZones();
}

FMASceneGraphNode UMASceneGraphManager::GetNodeById(const FString& NodeId) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetNodeById: QueryPort not initialized"));
        return FMASceneGraphNode();
    }
    return QueryPort->GetNodeById(NodeId);
}

TArray<FMASceneGraphNode> UMASceneGraphManager::GetAllNodes() const
{
    if (!QueryPort.IsValid())
    {
        TArray<FMASceneGraphNode> FallbackNodes;
        FallbackNodes.Reserve(StaticNodes.Num() + DynamicNodes.Num());
        FallbackNodes.Append(StaticNodes);
        FallbackNodes.Append(DynamicNodes);
        return FallbackNodes;
    }
    return QueryPort->GetAllNodes();
}

TArray<FMASceneGraphNode> UMASceneGraphManager::FindNodesByGuid(const FString& ActorGuid) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("FindNodesByGuid: QueryPort not initialized"));
        return {};
    }
    return QueryPort->FindNodesByGuid(ActorGuid);
}

//=============================================================================
// 语义查询接口 (委托给 MASceneGraphQuery)
//=============================================================================

FMASceneGraphNode UMASceneGraphManager::FindNodeByLabel(const FMASemanticLabel& Label) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("FindNodeByLabel: QueryPort not initialized"));
        return FMASceneGraphNode();
    }
    return QueryPort->FindNodeByLabel(Label);
}

FMASceneGraphNode UMASceneGraphManager::FindNearestNode(const FMASemanticLabel& Label, const FVector& FromLocation) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("FindNearestNode: QueryPort not initialized"));
        return FMASceneGraphNode();
    }
    return QueryPort->FindNearestNode(Label, FromLocation);
}

TArray<FMASceneGraphNode> UMASceneGraphManager::FindNodesInBoundary(const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("FindNodesInBoundary: QueryPort not initialized"));
        return {};
    }
    return QueryPort->FindNodesInBoundary(Label, BoundaryVertices);
}

bool UMASceneGraphManager::IsPointInsideBuilding(const FVector& Point) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("IsPointInsideBuilding: QueryPort not initialized"));
        return false;
    }
    return QueryPort->IsPointInsideBuilding(Point);
}

FMASceneGraphNode UMASceneGraphManager::FindNearestLandmark(const FVector& Location, float MaxDistance) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("FindNearestLandmark: QueryPort not initialized"));
        return FMASceneGraphNode();
    }
    return QueryPort->FindNearestLandmark(Location, MaxDistance);
}

TSharedPtr<FJsonObject> UMASceneGraphManager::NodeToJsonObject(const FMASceneGraphNode& Node) const
{
    if (!QueryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("NodeToJsonObject: QueryPort not initialized"));
        return MakeShareable(new FJsonObject());
    }
    return QueryPort->NodeToJsonObject(Node);
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
    if (RepositoryPort.IsValid())
    {
        return RepositoryPort->Load(WorkingCopy);
    }

    // Fallback: 直接通过 IO 加载（兼容旧路径）
    if (SourceFilePath.IsEmpty())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("InitializeWorkingCopy: SourceFilePath is empty"));
        return false;
    }
    return FMASceneGraphIO::LoadBaseSceneGraph(SourceFilePath, WorkingCopy);
}

bool UMASceneGraphManager::AddNode(const FString& NodeJson, FString& OutError)
{
    if (CommandPort.IsValid())
    {
        return CommandPort->AddNode(NodeJson, OutError);
    }
    return AddNodeInternal(NodeJson, OutError);
}

bool UMASceneGraphManager::AddNodeInternal(const FString& NodeJson, FString& OutError)
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

    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::AddStaticNode(GraphContext, NodeObject, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: %s"), *OutError);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddNode: Successfully added node with id=%s"), *NodeId);

    // 发送场景变更通知 (仅 Edit 模式)
    NotifySceneChange(EMASceneChangeType::AddNode, NodeJson);

    return true;
}

bool UMASceneGraphManager::DeleteNode(const FString& NodeId, FString& OutError)
{
    if (CommandPort.IsValid())
    {
        return CommandPort->DeleteNode(NodeId, OutError);
    }
    return DeleteNodeInternal(NodeId, OutError);
}

bool UMASceneGraphManager::DeleteNodeInternal(const FString& NodeId, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("DeleteNode: NodeId is empty"));
        return false;
    }

    FString DeletedNodeJson;
    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::DeleteStaticNodeById(GraphContext, NodeId, DeletedNodeJson, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("DeleteNode: %s"), *OutError);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("DeleteNode: Successfully deleted node with id=%s"), *NodeId);

    // 发送场景变更通知 (仅 Edit 模式)
    NotifySceneChange(EMASceneChangeType::DeleteNode, DeletedNodeJson);

    return true;
}

bool UMASceneGraphManager::EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    if (CommandPort.IsValid())
    {
        return CommandPort->EditNode(NodeId, NewNodeJson, OutError);
    }
    return EditNodeInternal(NodeId, NewNodeJson, OutError);
}

bool UMASceneGraphManager::EditNodeInternal(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
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

    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::ReplaceStaticNodeById(GraphContext, NodeId, NewNodeObject, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("EditStaticNode: %s"), *OutError);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("EditStaticNode: Successfully edited node %s"), *NodeId);

    // 发送场景变更通知 (仅 Edit 模式)
    NotifySceneChange(EMASceneChangeType::EditNode, NewNodeJson);

    return true;
}

bool UMASceneGraphManager::SaveToSource()
{
    if (CommandPort.IsValid())
    {
        return CommandPort->SaveToSource();
    }
    return SaveToSourceInternal();
}

bool UMASceneGraphManager::SaveToSourceInternal()
{
    const FString FilePath = GetSceneGraphFilePath();
    const FMASceneGraphCommandUseCases::ESaveResult SaveResult =
        FMASceneGraphCommandUseCases::SaveWorkingCopy(
            CachedRunMode == EMARunMode::Modify,
            WorkingCopy,
            FilePath,
            [this, &FilePath](const TSharedPtr<FJsonObject>& Data)
            {
                return RepositoryPort.IsValid()
                    ? RepositoryPort->Save(Data)
                    : FMASceneGraphIO::SaveSceneGraph(FilePath, Data);
            });

    if (SaveResult == FMASceneGraphCommandUseCases::ESaveResult::SkippedByRunMode)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("SaveToSource: Cannot save in Edit mode. Changes are only kept in memory."));
        return false;
    }

    if (SaveResult == FMASceneGraphCommandUseCases::ESaveResult::InvalidWorkingCopy)
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: WorkingCopy is invalid"));
        return false;
    }

    if (SaveResult == FMASceneGraphCommandUseCases::ESaveResult::EmptySourcePath)
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: SourceFilePath is empty"));
        return false;
    }

    if (SaveResult == FMASceneGraphCommandUseCases::ESaveResult::Saved)
    {
        UE_LOG(LogMASceneGraphManager, Log, TEXT("SaveToSource: Successfully saved to %s"), *FilePath);
        return true;
    }

    UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: Failed to save to %s"), *FilePath);
    return false;
}

bool UMASceneGraphManager::SetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    if (CommandPort.IsValid())
    {
        return CommandPort->SetNodeAsGoal(NodeId, OutError);
    }
    return SetNodeAsGoalInternal(NodeId, OutError);
}

bool UMASceneGraphManager::SetNodeAsGoalInternal(const FString& NodeId, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SetNodeAsGoal: NodeId is empty"));
        return false;
    }

    FString ModifiedNodeJson;
    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::BuildNodeJsonWithGoalFlag(
        GraphContext, NodeId, true, ModifiedNodeJson, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("SetNodeAsGoal: %s"), *OutError);
        return false;
    }

    // 调用 EditNode 完成更新（包括重新解析和通知）
    return EditNode(NodeId, ModifiedNodeJson, OutError);
}

bool UMASceneGraphManager::UnsetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    if (CommandPort.IsValid())
    {
        return CommandPort->UnsetNodeAsGoal(NodeId, OutError);
    }
    return UnsetNodeAsGoalInternal(NodeId, OutError);
}

bool UMASceneGraphManager::UnsetNodeAsGoalInternal(const FString& NodeId, FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("UnsetNodeAsGoal: NodeId is empty"));
        return false;
    }

    FString ModifiedNodeJson;
    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::BuildNodeJsonWithGoalFlag(
        GraphContext, NodeId, false, ModifiedNodeJson, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UnsetNodeAsGoal: %s"), *OutError);
        return false;
    }

    // 调用 EditNode 完成更新（包括重新解析和通知）
    return EditNode(NodeId, ModifiedNodeJson, OutError);
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

void UMASceneGraphManager::NotifySceneChange(EMASceneChangeType ChangeType, const FString& NodeJson)
{
    if (!EventPublisherPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("NotifySceneChange: EventPublisherPort not available"));
        return;
    }

    EventPublisherPort->PublishSceneChange(ChangeType, NodeJson);
    UE_LOG(LogMASceneGraphManager, Log, TEXT("NotifySceneChange: Delegated type=%d"), static_cast<int32>(ChangeType));
}
