// MADynamicNodeManager.cpp
// 动态节点管理模块实现
// Requirements: 1.1, 1.2, 1.3, 1.4, 10.3, 10.4

#include "MADynamicNodeManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMADynamicNodeManager, Log, All);

//=============================================================================
// 节点创建 - 从配置文件
//=============================================================================

TArray<FMASceneGraphNode> FMADynamicNodeManager::CreateRobotNodes(const FString& AgentsJsonPath)
{
    // Requirements: 1.1, 1.2 - 从 agents.json 创建机器人节点

    TArray<FMASceneGraphNode> Result;

    // 加载 JSON 文件
    TSharedPtr<FJsonObject> JsonData;
    if (!LoadJsonFile(AgentsJsonPath, JsonData))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreateRobotNodes: Failed to load agents.json from: %s"), *AgentsJsonPath);
        return Result;
    }

    // 获取 agents 数组
    const TArray<TSharedPtr<FJsonValue>>* AgentsArray;
    if (!JsonData->TryGetArrayField(TEXT("agents"), AgentsArray))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreateRobotNodes: No 'agents' array found in JSON"));
        return Result;
    }

    // 用于跟踪每种类型的机器人数量，以生成标签
    TMap<FString, int32> TypeCountMap;

    // 遍历每种机器人类型
    for (const TSharedPtr<FJsonValue>& AgentTypeValue : *AgentsArray)
    {
        if (!AgentTypeValue.IsValid() || AgentTypeValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> AgentTypeObject = AgentTypeValue->AsObject();
        if (!AgentTypeObject.IsValid())
        {
            continue;
        }

        // 获取机器人类型
        FString AgentType;
        if (!AgentTypeObject->TryGetStringField(TEXT("type"), AgentType))
        {
            UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreateRobotNodes: Agent entry missing 'type' field"));
            continue;
        }

        // 获取 instances 数组
        const TArray<TSharedPtr<FJsonValue>>* InstancesArray;
        if (!AgentTypeObject->TryGetArrayField(TEXT("instances"), InstancesArray))
        {
            UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreateRobotNodes: Agent type %s missing 'instances' array"), *AgentType);
            continue;
        }

        // 遍历每个实例
        for (const TSharedPtr<FJsonValue>& InstanceValue : *InstancesArray)
        {
            if (!InstanceValue.IsValid() || InstanceValue->Type != EJson::Object)
            {
                continue;
            }

            TSharedPtr<FJsonObject> InstanceObject = InstanceValue->AsObject();
            if (!InstanceObject.IsValid())
            {
                continue;
            }

            // 获取实例标签 (label 字段，用于显示名称)
            FString InstanceLabel;
            if (!InstanceObject->TryGetStringField(TEXT("label"), InstanceLabel))
            {
                UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreateRobotNodes: Instance missing 'label' field"));
                continue;
            }

            // 解析位置
            FVector Position = FVector::ZeroVector;
            const TArray<TSharedPtr<FJsonValue>>* PositionArray;
            if (InstanceObject->TryGetArrayField(TEXT("position"), PositionArray))
            {
                Position = ParsePositionFromArray(*PositionArray);
            }

            // 解析旋转
            FRotator Rotation = FRotator::ZeroRotator;
            const TArray<TSharedPtr<FJsonValue>>* RotationArray;
            if (InstanceObject->TryGetArrayField(TEXT("rotation"), RotationArray))
            {
                Rotation = ParseRotationFromArray(*RotationArray);
            }

            // 生成数字 ID (格式: 机器人从 101 开始)
            int32& TypeCount = TypeCountMap.FindOrAdd(AgentType);
            TypeCount++;
            int32 GlobalRobotIndex = Result.Num() + 1;
            FString NumericId = FString::Printf(TEXT("%d"), 100 + GlobalRobotIndex);

            // 创建机器人节点 (使用数字 ID 和 label)
            FMASceneGraphNode Node = CreateRobotNode(NumericId, AgentType, Position, Rotation);
            Node.Label = InstanceLabel;  // 使用配置文件中的 label
            Node.Features.Add(TEXT("label"), InstanceLabel);  // 同时存入 Features

            Result.Add(Node);

            UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreateRobotNodes: Created robot node - Id: %s, Type: %s, Label: %s, Position: %s"),
                *Node.Id, *Node.AgentType, *Node.Label, *Position.ToString());
        }
    }

    UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreateRobotNodes: Created %d robot nodes"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMADynamicNodeManager::CreatePickupItemNodes(const FString& EnvironmentJsonPath)
{
    // Requirements: 1.1, 1.3 - 从 environment.json 创建可拾取物品节点

    TArray<FMASceneGraphNode> Result;

    // 加载 JSON 文件
    TSharedPtr<FJsonObject> JsonData;
    if (!LoadJsonFile(EnvironmentJsonPath, JsonData))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreatePickupItemNodes: Failed to load environment.json from: %s"), *EnvironmentJsonPath);
        return Result;
    }

    // 获取 objects 数组
    const TArray<TSharedPtr<FJsonValue>>* ObjectsArray;
    if (!JsonData->TryGetArrayField(TEXT("objects"), ObjectsArray))
    {
        UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreatePickupItemNodes: No 'objects' array found in JSON"));
        return Result;
    }

    // 遍历每个物品
    for (const TSharedPtr<FJsonValue>& ObjectValue : *ObjectsArray)
    {
        if (!ObjectValue.IsValid() || ObjectValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> ObjectData = ObjectValue->AsObject();
        if (!ObjectData.IsValid())
        {
            continue;
        }

        // 获取物品 ID
        FString ItemId;
        if (!ObjectData->TryGetStringField(TEXT("id"), ItemId))
        {
            UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreatePickupItemNodes: Object missing 'id' field"));
            continue;
        }

        // 获取物品标签 (label 字段)
        FString ItemLabel;
        ObjectData->TryGetStringField(TEXT("label"), ItemLabel);

        // 获取物品类型
        FString ItemType;
        ObjectData->TryGetStringField(TEXT("type"), ItemType);

        // 解析位置
        FVector Position = FVector::ZeroVector;
        const TArray<TSharedPtr<FJsonValue>>* PositionArray;
        if (ObjectData->TryGetArrayField(TEXT("position"), PositionArray))
        {
            Position = ParsePositionFromArray(*PositionArray);
        }

        // 解析 Features
        TMap<FString, FString> Features;
        const TSharedPtr<FJsonObject>* FeaturesObject;
        if (ObjectData->TryGetObjectField(TEXT("features"), FeaturesObject))
        {
            Features = ParseFeaturesFromObject(*FeaturesObject);
        }

        // 如果有标签，添加到 Features
        if (!ItemLabel.IsEmpty())
        {
            Features.Add(TEXT("label"), ItemLabel);
        }

        // 如果有类型，添加到 Features
        if (!ItemType.IsEmpty())
        {
            Features.Add(TEXT("item_type"), ItemType);
        }

        // 创建可拾取物品节点
        FMASceneGraphNode Node = CreatePickupItemNode(ItemId, ItemLabel, ItemType, Position, Features);
        Result.Add(Node);

        UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreatePickupItemNodes: Created pickup item node - Id: %s, Label: %s, Position: %s"),
            *Node.Id, *Node.Label, *Position.ToString());
    }

    UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreatePickupItemNodes: Created %d pickup item nodes"), Result.Num());
    return Result;
}

TArray<FMASceneGraphNode> FMADynamicNodeManager::CreateChargingStationNodes(const FString& EnvironmentJsonPath)
{
    // Requirements: 1.1, 10.6 - 从 environment.json 创建充电站节点

    TArray<FMASceneGraphNode> Result;

    // 加载 JSON 文件
    TSharedPtr<FJsonObject> JsonData;
    if (!LoadJsonFile(EnvironmentJsonPath, JsonData))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("CreateChargingStationNodes: Failed to load environment.json from: %s"), *EnvironmentJsonPath);
        return Result;
    }

    // 获取 charging_stations 数组
    const TArray<TSharedPtr<FJsonValue>>* StationsArray;
    if (!JsonData->TryGetArrayField(TEXT("charging_stations"), StationsArray))
    {
        UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreateChargingStationNodes: No 'charging_stations' array found in JSON"));
        return Result;
    }

    int32 StationIndex = 1;

    // 遍历每个充电站
    for (const TSharedPtr<FJsonValue>& StationValue : *StationsArray)
    {
        if (!StationValue.IsValid() || StationValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> StationData = StationValue->AsObject();
        if (!StationData.IsValid())
        {
            continue;
        }

        // 获取充电站 ID
        FString StationId;
        if (!StationData->TryGetStringField(TEXT("id"), StationId))
        {
            // 如果没有 ID，自动生成数字 ID
            StationId = FString::Printf(TEXT("%d"), 2000 + StationIndex);
        }

        // 获取充电站标签
        FString StationLabel;
        StationData->TryGetStringField(TEXT("label"), StationLabel);

        // 解析位置
        FVector Position = FVector::ZeroVector;
        const TArray<TSharedPtr<FJsonValue>>* PositionArray;
        if (StationData->TryGetArrayField(TEXT("position"), PositionArray))
        {
            Position = ParsePositionFromArray(*PositionArray);
        }

        // 创建充电站节点
        FMASceneGraphNode Node = CreateChargingStationNode(StationId, StationLabel, Position);
        Result.Add(Node);

        UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreateChargingStationNodes: Created charging station node - Id: %s, Position: %s"),
            *Node.Id, *Position.ToString());

        StationIndex++;
    }

    UE_LOG(LogMADynamicNodeManager, Log, TEXT("CreateChargingStationNodes: Created %d charging station nodes"), Result.Num());
    return Result;
}


//=============================================================================
// 节点创建 - 工厂方法
//=============================================================================

FMASceneGraphNode FMADynamicNodeManager::CreateRobotNode(
    const FString& Id,
    const FString& AgentType,
    const FVector& Position,
    const FRotator& Rotation)
{
    // Requirements: 1.2, 10.1 - 创建机器人节点

    FMASceneGraphNode Node;

    Node.Id = Id;
    Node.Type = TEXT("robot");
    Node.Category = TEXT("robot");
    Node.AgentType = AgentType;
    Node.Label = Id;  // 默认使用 ID 作为标签，可以后续更新
    Node.Center = Position;
    Node.Rotation = Rotation;
    Node.ShapeType = TEXT("point");
    Node.bIsDynamic = true;
    Node.bIsCarried = false;

    // 添加 Features
    Node.Features.Add(TEXT("agent_type"), AgentType);
    Node.Features.Add(TEXT("status"), TEXT("idle"));

    return Node;
}

FMASceneGraphNode FMADynamicNodeManager::CreatePickupItemNode(
    const FString& Id,
    const FString& ItemLabel,
    const FString& ItemType,
    const FVector& Position,
    const TMap<FString, FString>& Features)
{
    // Requirements: 1.3, 10.2 - 创建可拾取物品节点

    FMASceneGraphNode Node;

    Node.Id = Id;
    Node.Type = TEXT("pickup_item");
    Node.Category = TEXT("pickup_item");
    Node.Label = ItemLabel.IsEmpty() ? Id : ItemLabel;
    Node.Center = Position;
    Node.ShapeType = TEXT("point");
    Node.bIsDynamic = true;
    Node.bIsCarried = false;
    Node.CarrierId = TEXT("");

    // 复制 Features
    Node.Features = Features;

    // 确保 label 和 item_type 在 Features 中
    if (!ItemLabel.IsEmpty() && !Node.Features.Contains(TEXT("label")))
    {
        Node.Features.Add(TEXT("label"), ItemLabel);
    }
    if (!ItemType.IsEmpty() && !Node.Features.Contains(TEXT("item_type")))
    {
        Node.Features.Add(TEXT("item_type"), ItemType);
    }

    return Node;
}

FMASceneGraphNode FMADynamicNodeManager::CreateChargingStationNode(
    const FString& Id,
    const FString& StationLabel,
    const FVector& Position)
{
    // Requirements: 10.6 - 创建充电站节点

    FMASceneGraphNode Node;

    Node.Id = Id;
    Node.Type = TEXT("charging_station");
    Node.Category = TEXT("charging_station");
    Node.Label = StationLabel.IsEmpty() ? Id : StationLabel;
    Node.Center = Position;
    Node.ShapeType = TEXT("point");
    Node.bIsDynamic = true;  // 充电站位置可能需要更新
    Node.bIsCarried = false;

    // 添加 Features
    if (!StationLabel.IsEmpty())
    {
        Node.Features.Add(TEXT("label"), StationLabel);
    }
    Node.Features.Add(TEXT("status"), TEXT("available"));

    return Node;
}


//=============================================================================
// 节点更新
//=============================================================================

bool FMADynamicNodeManager::UpdateNodePosition(FMASceneGraphNode& Node, const FVector& NewPosition)
{
    // Requirements: 1.4 - 更新节点位置

    // 验证节点有效性
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodePosition: Invalid node"));
        return false;
    }

    // 验证位置有效性
    if (!IsValidPosition(NewPosition))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodePosition: Invalid position for node %s"), *Node.Id);
        return false;
    }

    // 检查是否为静态节点
    if (!Node.bIsDynamic)
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodePosition: Cannot update position of static node %s"), *Node.Id);
        return false;
    }

    // 更新位置
    Node.Center = NewPosition;

    UE_LOG(LogMADynamicNodeManager, Verbose, TEXT("UpdateNodePosition: Updated node %s position to %s"),
        *Node.Id, *NewPosition.ToString());

    return true;
}

bool FMADynamicNodeManager::UpdateNodeRotation(FMASceneGraphNode& Node, const FRotator& NewRotation)
{
    // 验证节点有效性
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeRotation: Invalid node"));
        return false;
    }

    // 检查是否为静态节点
    if (!Node.bIsDynamic)
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeRotation: Cannot update rotation of static node %s"), *Node.Id);
        return false;
    }

    // 更新旋转
    Node.Rotation = NewRotation;

    UE_LOG(LogMADynamicNodeManager, Verbose, TEXT("UpdateNodeRotation: Updated node %s rotation to %s"),
        *Node.Id, *NewRotation.ToString());

    return true;
}

bool FMADynamicNodeManager::UpdatePickupItemCarrierStatus(
    FMASceneGraphNode& Node,
    bool bIsCarried,
    const FString& CarrierId)
{
    // Requirements: 10.3, 10.4 - 更新可拾取物品的携带状态

    // 验证节点有效性
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdatePickupItemCarrierStatus: Invalid node"));
        return false;
    }

    // 验证节点类型
    if (!Node.IsPickupItem())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdatePickupItemCarrierStatus: Node %s is not a pickup item"), *Node.Id);
        return false;
    }

    // 更新携带状态
    Node.bIsCarried = bIsCarried;

    if (bIsCarried)
    {
        // 被拾取
        Node.CarrierId = CarrierId;
        Node.Features.Add(TEXT("is_carried"), TEXT("true"));
        Node.Features.Add(TEXT("carrier_id"), CarrierId);

        UE_LOG(LogMADynamicNodeManager, Log, TEXT("UpdatePickupItemCarrierStatus: Item %s picked up by %s"),
            *Node.Id, *CarrierId);
    }
    else
    {
        // 被放下
        Node.CarrierId = TEXT("");
        Node.Features.Add(TEXT("is_carried"), TEXT("false"));
        Node.Features.Remove(TEXT("carrier_id"));

        UE_LOG(LogMADynamicNodeManager, Log, TEXT("UpdatePickupItemCarrierStatus: Item %s dropped"),
            *Node.Id);
    }

    return true;
}


//=============================================================================
// 辅助方法
//=============================================================================

bool FMADynamicNodeManager::IsValidPosition(const FVector& Position)
{
    // 检查是否为 NaN 或 Inf
    return !Position.ContainsNaN() &&
           FMath::IsFinite(Position.X) &&
           FMath::IsFinite(Position.Y) &&
           FMath::IsFinite(Position.Z);
}

FString FMADynamicNodeManager::GenerateRobotLabel(const FString& AgentType, int32 Index)
{
    // 格式: "AgentType-N" (如 "UAV-1", "UGV-1")
    return FString::Printf(TEXT("%s-%d"), *AgentType, Index);
}


//=============================================================================
// 内部辅助方法
//=============================================================================

bool FMADynamicNodeManager::LoadJsonFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutData)
{
    OutData.Reset();

    // 检查文件是否存在
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("LoadJsonFile: File not found: %s"), *FilePath);
        return false;
    }

    // 读取文件内容
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogMADynamicNodeManager, Error, TEXT("LoadJsonFile: Failed to read file: %s"), *FilePath);
        return false;
    }

    // 解析 JSON
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, OutData) || !OutData.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Error, TEXT("LoadJsonFile: Failed to parse JSON: %s"), *FilePath);
        return false;
    }

    return true;
}

FVector FMADynamicNodeManager::ParsePositionFromArray(const TArray<TSharedPtr<FJsonValue>>& PositionArray)
{
    if (PositionArray.Num() < 3)
    {
        return FVector::ZeroVector;
    }

    return FVector(
        PositionArray[0]->AsNumber(),
        PositionArray[1]->AsNumber(),
        PositionArray[2]->AsNumber()
    );
}

FRotator FMADynamicNodeManager::ParseRotationFromArray(const TArray<TSharedPtr<FJsonValue>>& RotationArray)
{
    if (RotationArray.Num() < 3)
    {
        return FRotator::ZeroRotator;
    }

    // 数组格式: [pitch, yaw, roll]
    return FRotator(
        RotationArray[0]->AsNumber(),  // Pitch
        RotationArray[1]->AsNumber(),  // Yaw
        RotationArray[2]->AsNumber()   // Roll
    );
}

TMap<FString, FString> FMADynamicNodeManager::ParseFeaturesFromObject(const TSharedPtr<FJsonObject>& FeaturesObject)
{
    TMap<FString, FString> Result;

    if (!FeaturesObject.IsValid())
    {
        return Result;
    }

    // 遍历所有字段
    for (const auto& Pair : FeaturesObject->Values)
    {
        if (Pair.Value.IsValid())
        {
            if (Pair.Value->Type == EJson::String)
            {
                Result.Add(Pair.Key, Pair.Value->AsString());
            }
            else if (Pair.Value->Type == EJson::Number)
            {
                Result.Add(Pair.Key, FString::SanitizeFloat(Pair.Value->AsNumber()));
            }
            else if (Pair.Value->Type == EJson::Boolean)
            {
                Result.Add(Pair.Key, Pair.Value->AsBool() ? TEXT("true") : TEXT("false"));
            }
        }
    }

    return Result;
}
