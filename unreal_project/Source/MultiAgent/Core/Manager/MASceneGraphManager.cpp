// MASceneGraphManager.cpp
// 场景图管理器实现

#include "MASceneGraphManager.h"
#include "scene_graph_tools/MASceneGraphIO.h"
#include "scene_graph_tools/MADynamicNodeManager.h"
#include "scene_graph_services/MASceneGraphQueryService.h"
#include "scene_graph_services/MASceneGraphCommandUseCases.h"
#include "scene_graph_ports/IMASceneGraphCommandPort.h"
#include "scene_graph_adapters/MASceneGraphCommandPortComposer.h"
#include "scene_graph_adapters/MASceneGraphRepositoryAdapter.h"
#include "scene_graph_adapters/MASceneGraphEventPublisherAdapter.h"
#include "ue_tools/MAUESceneApplier.h"
#include "../Comm/MACommTypes.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphManager, Log, All);

namespace
{
FString ResolveActorGuidForSceneNode(AActor* Actor)
{
    if (Actor == nullptr)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("AddSceneNode: Actor is nullptr, GUID will be empty"));
        return TEXT("");
    }

    const FString GuidString = Actor->GetActorGuid().ToString();
    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddSceneNode: Extracted GUID=%s from Actor=%s"), *GuidString, *Actor->GetName());
    return GuidString;
}

bool ExecuteAddPointSceneNode(
    UMASceneGraphManager& Manager,
    const FMASceneGraphCommandUseCases::FAddPointSceneNodeInput& Input,
    FString& OutError)
{
    return FMASceneGraphCommandUseCases::AddPointSceneNode(
        Input,
        [&Manager](const FString& NodeJson, FString& Error)
        {
            return Manager.AddNode(NodeJson, Error);
        },
        [&Manager]()
        {
            return Manager.SaveToSource();
        },
        OutError);
}

FMASceneGraphCommandUseCases::FDynamicGraphContext MakeDynamicGraphContext(
    TArray<FMASceneGraphNode>& DynamicNodes)
{
    return FMASceneGraphCommandUseCases::FDynamicGraphContext{DynamicNodes};
}

void LogWithVerbosity(ELogVerbosity::Type Verbosity, const FString& Message)
{
    switch (Verbosity)
    {
        case ELogVerbosity::Log:
            UE_LOG(LogMASceneGraphManager, Log, TEXT("%s"), *Message);
            break;
        case ELogVerbosity::Warning:
            UE_LOG(LogMASceneGraphManager, Warning, TEXT("%s"), *Message);
            break;
        case ELogVerbosity::Error:
        default:
            UE_LOG(LogMASceneGraphManager, Error, TEXT("%s"), *Message);
            break;
    }
}

bool LogSaveResultReport(
    const FMASceneGraphCommandUseCases::FSaveResultReport& Report)
{
    LogWithVerbosity(Report.Verbosity, Report.Message);
    return Report.bSuccess;
}
}

//=============================================================================
// 生命周期
//=============================================================================

void UMASceneGraphManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initializing"));

    CacheRuntimeConfig();
    InitializePorts();
    EnsureWorkingCopyForStartup();
    RebuildStaticNodesFromWorkingCopy();
    QueryPort = MakeShared<FMASceneGraphQueryService>(&StaticNodes, &DynamicNodes);
    InitializeCommandPort();
    LoadDynamicNodes();

    UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager initialized with %d static nodes and %d dynamic nodes"),
        StaticNodes.Num(), DynamicNodes.Num());
}

void UMASceneGraphManager::CacheRuntimeConfig()
{
    CachedRunMode = EMARunMode::Edit;
    SourceFilePath.Empty();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMAConfigManager* ConfigManager = GameInstance->GetSubsystem<UMAConfigManager>())
        {
            CachedRunMode = ConfigManager->GetRunMode();
            SourceFilePath = ConfigManager->GetSceneGraphFilePath();
            UE_LOG(LogMASceneGraphManager, Log, TEXT("MASceneGraphManager: RunMode=%s, SourceFilePath=%s"),
                CachedRunMode == EMARunMode::Modify ? TEXT("Modify") : TEXT("Edit"),
                *SourceFilePath);
            return;
        }
    }

    UE_LOG(LogMASceneGraphManager, Warning, TEXT("MASceneGraphManager: ConfigManager not available, using defaults"));
}

void UMASceneGraphManager::InitializePorts()
{
    RepositoryPort = MakeShared<FMASceneGraphRepositoryAdapter>(this, SourceFilePath);
    EventPublisherPort = MakeShared<FMASceneGraphEventPublisherAdapter>(GetGameInstance(), CachedRunMode);
}

void UMASceneGraphManager::EnsureWorkingCopyForStartup()
{
    if (InitializeWorkingCopy())
    {
        return;
    }

    if (CachedRunMode == EMARunMode::Modify)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("MASceneGraphManager: Source file not found in Modify mode, creating empty graph"));
    }
    else
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("MASceneGraphManager: Source file not found in Edit mode: %s"), *SourceFilePath);
    }

    CreateEmptySceneGraph();
}

void UMASceneGraphManager::RebuildStaticNodesFromWorkingCopy()
{
    StaticNodes.Empty();

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArray();
    if (!NodesArray)
    {
        return;
    }

    StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);
    UE_LOG(LogMASceneGraphManager, Log, TEXT("Loaded %d static nodes from scene graph"), StaticNodes.Num());
}

void UMASceneGraphManager::InitializeCommandPort()
{
    CommandPort = FMASceneGraphCommandPortComposer::Create(*this);
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
    OutId.Empty();
    OutType.Empty();
    OutErrorMessage.Empty();

    FMASceneGraphCommandUseCases::FResolvedLabelInput Resolved;
    if (!FMASceneGraphCommandUseCases::ResolveLabelInput(
            InputText,
            [this]()
            {
                return GetNextAvailableId();
            },
            Resolved,
            OutErrorMessage))
    {
        return false;
    }

    OutId = Resolved.Id;
    OutType = Resolved.Type;
    if (Resolved.bUsedAutoId)
    {
        UE_LOG(LogMASceneGraphManager, Log, TEXT("ParseLabelInput: Auto-assigned ID = %s"), *OutId);
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("ParseLabelInput: id=%s, type=%s, cate=%s"), 
        *OutId, *OutType, Resolved.bHasCategory ? TEXT("true") : TEXT("false"));
    return true;
}

FString UMASceneGraphManager::GenerateLabel(const FString& Type) const
{
    const FString Label = FMASceneGraphCommandUseCases::BuildGeneratedLabel(StaticNodes, Type);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("GenerateLabel: type=%s, label=%s"), *Type, *Label);
    return Label;
}

bool UMASceneGraphManager::IsIdExists(const FString& Id) const
{
    return FMASceneGraphCommandUseCases::ContainsNodeId(StaticNodes, Id);
}

int32 UMASceneGraphManager::GetTypeCount(const FString& Type) const
{
    return FMASceneGraphCommandUseCases::CountNodeType(StaticNodes, Type);
}

FString UMASceneGraphManager::GetNextAvailableId() const
{
    const FString NextId = FMASceneGraphCommandUseCases::CalculateNextNumericId(StaticNodes);
    UE_LOG(LogMASceneGraphManager, Log, TEXT("GetNextAvailableId: Next ID = %s"), *NextId);
    return NextId;
}

bool UMASceneGraphManager::AddSceneNode(const FString& Id, const FString& Type, const FVector& WorldLocation,
                                         AActor* Actor, FString& OutLabel, FString& OutErrorMessage)
{
    OutLabel.Empty();
    OutErrorMessage.Empty();

    OutLabel = GenerateLabel(Type);
    const FString GuidString = ResolveActorGuidForSceneNode(Actor);

    FMASceneGraphCommandUseCases::FAddPointSceneNodeInput Input;
    Input.Id = Id;
    Input.Type = Type;
    Input.Label = OutLabel;
    Input.WorldLocation = WorldLocation;
    Input.Guid = GuidString;
    if (!ExecuteAddPointSceneNode(*this, Input, OutErrorMessage))
    {
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("AddSceneNode: id=%s, guid=%s, type=%s, label=%s, location=(%f, %f, %f)"),
        *Id, *GuidString, *Type, *OutLabel, WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

    return true;
}



//=============================================================================
// 动态节点加载
//=============================================================================

void UMASceneGraphManager::LoadDynamicNodes()
{
    DynamicNodes.Empty();
    UMAConfigManager* ConfigManager = ResolveConfigManager();

    if (!ConfigManager)
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("LoadDynamicNodes: ConfigManager not available, cannot load dynamic nodes"));
        return;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Loading from ConfigManager (MapType=%s)"), *ConfigManager->GetMapType());
    BuildDynamicNodesFromRuntimeConfig(*ConfigManager);
    RefreshDynamicNodeLocationLabels();
}

UMAConfigManager* UMASceneGraphManager::ResolveConfigManager() const
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        return GameInstance->GetSubsystem<UMAConfigManager>();
    }
    return nullptr;
}

void UMASceneGraphManager::BuildDynamicNodesFromRuntimeConfig(const UMAConfigManager& ConfigManager)
{
    DynamicNodes = FMADynamicNodeManager::BuildDynamicNodesFromConfig(
        ConfigManager.AgentConfigs,
        ConfigManager.EnvironmentObjects);

    UE_LOG(LogMASceneGraphManager, Log, TEXT("LoadDynamicNodes: Total %d dynamic nodes loaded"), DynamicNodes.Num());
}

void UMASceneGraphManager::RefreshDynamicNodeLocationLabels()
{
    TArray<FMASceneGraphNode> AllNodesForLocationCalc = GetAllNodes();
    FMADynamicNodeManager::RefreshLocationLabels(DynamicNodes, AllNodesForLocationCalc, 5000.f);
}

void UMASceneGraphManager::CalibrateDynamicNodeFromWorldActor(const FString& NodeIdOrLabel)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext = MakeDynamicGraphContext(DynamicNodes);
    if (FMASceneGraphNode* Node =
        FMASceneGraphCommandUseCases::FindDynamicNodeByIdOrLabelMutable(GraphContext, NodeIdOrLabel))
    {
        FMAUESceneApplier::CalibrateNodeFromActor(World, *Node);
    }
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
    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext = MakeDynamicGraphContext(DynamicNodes);
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
    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext = MakeDynamicGraphContext(DynamicNodes);
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
    FString BoundNodeId;
    FString BoundNodeLabel;
    FString Error;
    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext = MakeDynamicGraphContext(DynamicNodes);
    if (!FMASceneGraphCommandUseCases::BindDynamicNodeGuid(
        GraphContext,
        NodeIdOrLabel,
        ActorGuid,
        BoundNodeId,
        BoundNodeLabel,
        Error))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("BindDynamicNodeGuid: %s"), *Error);
        return false;
    }

    UE_LOG(LogMASceneGraphManager, Log, TEXT("BindDynamicNodeGuid: 成功绑定节点 %s (Label: %s) 到 GUID %s"),
        *BoundNodeId, *BoundNodeLabel, *ActorGuid);

    CalibrateDynamicNodeFromWorldActor(BoundNodeId);

    return true;
}

bool UMASceneGraphManager::EditDynamicNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    OutError.Empty();
    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext = MakeDynamicGraphContext(DynamicNodes);
    if (!FMASceneGraphCommandUseCases::EditDynamicNodeFromJsonString(
        GraphContext, NodeId, NewNodeJson, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("EditDynamicNode: %s"), *OutError);
        return false;
    }

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
    if (!RepositoryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("GetSceneGraphFilePath: RepositoryPort not initialized"));
        return FString();
    }
    return RepositoryPort->GetSourcePath();
}

bool UMASceneGraphManager::LoadSceneGraph()
{
    if (!RepositoryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("LoadSceneGraph: RepositoryPort not initialized"));
        return false;
    }
    return RepositoryPort->Load(WorkingCopy);
}

bool UMASceneGraphManager::SaveSceneGraph()
{
    if (!WorkingCopy.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("Cannot save: WorkingCopy is invalid"));
        return false;
    }

    if (!RepositoryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveSceneGraph: RepositoryPort not initialized"));
        return false;
    }
    return RepositoryPort->Save(WorkingCopy);
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
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("GetAllNodes: QueryPort not initialized"));
        return {};
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

//=============================================================================
// 统一图操作 API 实现
//=============================================================================

bool UMASceneGraphManager::InitializeWorkingCopy()
{
    if (!RepositoryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("InitializeWorkingCopy: RepositoryPort not initialized"));
        return false;
    }
    return RepositoryPort->Load(WorkingCopy);
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

    FString NodeId;
    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::AddStaticNodeFromJson(
            GraphContext,
            NodeJson,
            [this](const FString& CandidateId)
            {
                return IsIdExists(CandidateId);
            },
            NodeId,
            OutError))
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("AddNode: validation/add failed for node id=%s: %s"), *NodeId, *OutError);
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

    FMASceneGraphCommandUseCases::FDynamicGraphContext GraphContext = MakeDynamicGraphContext(DynamicNodes);
    if (FMASceneGraphCommandUseCases::HasDynamicNode(GraphContext, NodeId))
    {
        return EditDynamicNode(NodeId, NewNodeJson, OutError);
    }
    return EditStaticNode(NodeId, NewNodeJson, OutError);
}

bool UMASceneGraphManager::EditStaticNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::ReplaceStaticNodeByIdFromJson(
        GraphContext, NodeId, NewNodeJson, OutError))
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
    if (!RepositoryPort.IsValid())
    {
        UE_LOG(LogMASceneGraphManager, Error, TEXT("SaveToSource: RepositoryPort not initialized"));
        return false;
    }

    const FString FilePath = GetSceneGraphFilePath();
    const FMASceneGraphCommandUseCases::ESaveResult SaveResult =
        FMASceneGraphCommandUseCases::SaveWorkingCopy(
            CachedRunMode == EMARunMode::Modify,
            WorkingCopy,
            FilePath,
            [this](const TSharedPtr<FJsonObject>& Data)
            {
                return RepositoryPort->Save(Data);
            });
    const FMASceneGraphCommandUseCases::FSaveResultReport Report =
        FMASceneGraphCommandUseCases::BuildSaveResultReport(SaveResult, FilePath);
    return LogSaveResultReport(Report);
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
    return UpdateNodeGoalFlagInternal(NodeId, true, OutError);
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
    return UpdateNodeGoalFlagInternal(NodeId, false, OutError);
}

bool UMASceneGraphManager::UpdateNodeGoalFlagInternal(
    const FString& NodeId,
    bool bIsGoal,
    FString& OutError)
{
    OutError.Empty();

    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        UE_LOG(LogMASceneGraphManager, Error, TEXT("UpdateNodeGoalFlag: NodeId is empty"));
        return false;
    }

    FString ModifiedNodeJson;
    FMASceneGraphCommandUseCases::FStaticGraphContext GraphContext{WorkingCopy, StaticNodes};
    if (!FMASceneGraphCommandUseCases::BuildNodeJsonWithGoalFlag(
        GraphContext, NodeId, bIsGoal, ModifiedNodeJson, OutError))
    {
        UE_LOG(LogMASceneGraphManager, Warning, TEXT("UpdateNodeGoalFlag: %s"), *OutError);
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
    const FMASceneGraphCommandUseCases::FSceneChangePublishReport Report =
        FMASceneGraphCommandUseCases::PublishSceneChange(
            EventPublisherPort.Get(),
            ChangeType,
            NodeJson);
    LogWithVerbosity(Report.Verbosity, Report.Message);
}
