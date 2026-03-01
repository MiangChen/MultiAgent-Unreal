// MADynamicNodeManager.cpp
// 动态节点管理模块实现

#include "MADynamicNodeManager.h"
#include "../../Config/MAConfigManager.h"
#include "../../../Agent/Skill/Utils/MALocationUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMADynamicNodeManager, Log, All);

//=============================================================================
// 节点创建
//=============================================================================

FMASceneGraphNode FMADynamicNodeManager::CreateAgentNode(const FMAAgentConfigData& Config)
{
    FMASceneGraphNode Node;

    Node.Id = Config.ID;
    Node.Type = Config.Type;
    Node.Category = TEXT("robot");
    Node.Label = Config.Label.IsEmpty() ? Config.ID : Config.Label;
    Node.Center = Config.Position;
    Node.Rotation = Config.Rotation;
    Node.ShapeType = TEXT("point");
    Node.bIsDynamic = true;
    Node.bIsCarried = false;
    Node.Features.Add(TEXT("status"), TEXT("idle"));
    Node.Features.Add(TEXT("battery_level"), FString::Printf(TEXT("%.0f"), Config.BatteryLevel));
    
    if (!Config.Label.IsEmpty())
    {
        Node.Features.Add(TEXT("label"), Config.Label);
    }

    // 生成 RawJson
    Node.RawJson = GenerateRawJson(Node);

    UE_LOG(LogMADynamicNodeManager, Verbose, TEXT("CreateAgentNode: Id=%s, Type=%s, Label=%s"),
        *Node.Id, *Node.Type, *Node.Label);

    return Node;
}

FMASceneGraphNode FMADynamicNodeManager::CreateEnvironmentObjectNode(const FMAEnvironmentObjectConfig& Config)
{
    FMASceneGraphNode Node;

    Node.Id = Config.ID;
    Node.Type = Config.Type;
    Node.Label = Config.Label.IsEmpty() ? Config.ID : Config.Label;
    Node.Center = Config.Position;
    Node.Rotation = Config.Rotation;
    Node.ShapeType = TEXT("point");
    Node.bIsDynamic = true;
    Node.Features = Config.Features;

    // 根据类型设置 Category
    if (Config.Type == TEXT("cargo") || Config.Type == TEXT("assembly_component"))
    {
        Node.Category = TEXT("prop");
        Node.bIsCarried = false;
        Node.CarrierId = TEXT("");
    }
    else if (Config.Type == TEXT("charging_station"))
    {
        Node.Category = TEXT("prop");
        if (!Node.Features.Contains(TEXT("status")))
        {
            Node.Features.Add(TEXT("status"), TEXT("available"));
        }
    }
    else if (Config.Type == TEXT("person"))
    {
        Node.Category = TEXT("prop");
    }
    else if (Config.Type == TEXT("vehicle") || Config.Type == TEXT("boat"))
    {
        Node.Category = TEXT("prop");
    }
    else if (Config.Type == TEXT("fire") || Config.Type == TEXT("smoke") || Config.Type == TEXT("wind"))
    {
        Node.Category = TEXT("effect");

        // Set effect_radius for smoke/wind nodes from config "radius" feature, or use defaults
        if (Config.Type == TEXT("smoke") || Config.Type == TEXT("wind"))
        {
            if (!Node.Features.Contains(TEXT("effect_radius")))
            {
                float DefaultRadius = (Config.Type == TEXT("smoke")) ? 3000.f : 5000.f;
                float Radius = DefaultRadius;

                if (const FString* RadiusStr = Node.Features.Find(TEXT("radius")))
                {
                    float Parsed = FCString::Atof(**RadiusStr);
                    if (Parsed > 0.f)
                    {
                        Radius = Parsed;
                    }
                }

                Node.Features.Add(TEXT("effect_radius"), FString::SanitizeFloat(Radius));
            }
        }
    }
    else
    {
        Node.Category = TEXT("prop");
    }

    // 确保 label 在 Features 中
    if (!Config.Label.IsEmpty() && !Node.Features.Contains(TEXT("label")))
    {
        Node.Features.Add(TEXT("label"), Config.Label);
    }

    // 生成 RawJson
    Node.RawJson = GenerateRawJson(Node);

    UE_LOG(LogMADynamicNodeManager, Verbose, TEXT("CreateEnvironmentObjectNode: Id=%s, Type=%s, Category=%s, Label=%s"),
        *Node.Id, *Node.Type, *Node.Category, *Node.Label);

    return Node;
}

TArray<FMASceneGraphNode> FMADynamicNodeManager::BuildDynamicNodesFromConfig(
    const TArray<FMAAgentConfigData>& AgentConfigs,
    const TArray<FMAEnvironmentObjectConfig>& EnvironmentObjects)
{
    TArray<FMASceneGraphNode> Nodes;
    Nodes.Reserve(AgentConfigs.Num() + EnvironmentObjects.Num());

    for (const FMAAgentConfigData& AgentConfig : AgentConfigs)
    {
        FMASceneGraphNode Node = CreateAgentNode(AgentConfig);
        UE_LOG(LogMADynamicNodeManager, Log, TEXT("BuildDynamicNodesFromConfig: Created agent node - Id: %s, Type: %s, Label: %s"),
            *Node.Id, *Node.Type, *Node.Label);
        Nodes.Add(MoveTemp(Node));
    }

    for (const FMAEnvironmentObjectConfig& EnvConfig : EnvironmentObjects)
    {
        FMASceneGraphNode Node = CreateEnvironmentObjectNode(EnvConfig);
        UE_LOG(LogMADynamicNodeManager, Log, TEXT("BuildDynamicNodesFromConfig: Created %s node - Id: %s, Label: %s"),
            *EnvConfig.Type, *Node.Id, *Node.Label);
        Nodes.Add(MoveTemp(Node));
    }

    UE_LOG(LogMADynamicNodeManager, Log, TEXT("BuildDynamicNodesFromConfig: Total %d dynamic nodes built"), Nodes.Num());
    return Nodes;
}

void FMADynamicNodeManager::RefreshLocationLabels(
    TArray<FMASceneGraphNode>& DynamicNodes,
    const TArray<FMASceneGraphNode>& AllNodesForLookup,
    float MaxSearchDistanceCm)
{
    for (FMASceneGraphNode& Node : DynamicNodes)
    {
        if (FMALocationUtils::IsLocationNode(Node))
        {
            continue;
        }

        Node.LocationLabel = FMALocationUtils::InferNearestLocationLabel(
            AllNodesForLookup,
            Node.Center,
            MaxSearchDistanceCm,
            Node.Id);

        UE_LOG(LogMADynamicNodeManager, Verbose, TEXT("RefreshLocationLabels: Node %s (%s) location_label='%s'"),
            *Node.Id, *Node.Label, *Node.LocationLabel);
    }
}

//=============================================================================
// 节点更新
//=============================================================================

bool FMADynamicNodeManager::UpdateNodePosition(FMASceneGraphNode& Node, const FVector& NewPosition)
{
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodePosition: Invalid node"));
        return false;
    }

    if (!IsValidPosition(NewPosition))
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodePosition: Invalid position for node %s"), *Node.Id);
        return false;
    }

    if (!Node.bIsDynamic)
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodePosition: Cannot update position of static node %s"), *Node.Id);
        return false;
    }

    Node.Center = NewPosition;
    Node.RawJson = GenerateRawJson(Node);
    return true;
}

bool FMADynamicNodeManager::UpdateNodeRotation(FMASceneGraphNode& Node, const FRotator& NewRotation)
{
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeRotation: Invalid node"));
        return false;
    }

    if (!Node.bIsDynamic)
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeRotation: Cannot update rotation of static node %s"), *Node.Id);
        return false;
    }

    Node.Rotation = NewRotation;
    return true;
}

bool FMADynamicNodeManager::UpdateNodeFeature(FMASceneGraphNode& Node, const FString& Key, const FString& Value)
{
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeFeature: Invalid node"));
        return false;
    }

    if (Key.IsEmpty())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeFeature: Key is empty"));
        return false;
    }

    Node.Features.Add(Key, Value);
    
    // 特殊处理: 同步 bIsCarried 字段
    if (Key == TEXT("is_carried"))
    {
        Node.bIsCarried = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
    }
    else if (Key == TEXT("carrier_id"))
    {
        Node.CarrierId = Value;
    }

    return true;
}

bool FMADynamicNodeManager::RemoveNodeFeature(FMASceneGraphNode& Node, const FString& Key)
{
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("RemoveNodeFeature: Invalid node"));
        return false;
    }

    if (Key.IsEmpty())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("RemoveNodeFeature: Key is empty"));
        return false;
    }

    Node.Features.Remove(Key);
    
    // 特殊处理: 同步 bIsCarried 字段
    if (Key == TEXT("is_carried"))
    {
        Node.bIsCarried = false;
    }
    else if (Key == TEXT("carrier_id"))
    {
        Node.CarrierId = TEXT("");
    }

    return true;
}

//=============================================================================
// GUID 绑定相关
//=============================================================================

FString FMADynamicNodeManager::GenerateRawJson(const FMASceneGraphNode& Node)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());

    // 基础字段
    RootObject->SetStringField(TEXT("id"), Node.Id);
    
    // GUID (如果有)
    if (!Node.Guid.IsEmpty())
    {
        RootObject->SetStringField(TEXT("guid"), Node.Guid);
    }

    // properties 对象
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    PropertiesObject->SetStringField(TEXT("type"), Node.Type);
    PropertiesObject->SetStringField(TEXT("label"), Node.Label);
    PropertiesObject->SetStringField(TEXT("category"), Node.Category);
    PropertiesObject->SetBoolField(TEXT("is_dynamic"), Node.bIsDynamic);

    // 添加 Features 到 properties (数值类型自动输出为 JSON number)
    for (const auto& Pair : Node.Features)
    {
        // 尝试解析为数字
        if (Pair.Value.IsNumeric())
        {
            PropertiesObject->SetNumberField(Pair.Key, FCString::Atod(*Pair.Value));
        }
        else if (Pair.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Pair.Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
        {
            PropertiesObject->SetBoolField(Pair.Key, Pair.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
        }
        else
        {
            PropertiesObject->SetStringField(Pair.Key, Pair.Value);
        }
    }

    RootObject->SetObjectField(TEXT("properties"), PropertiesObject);

    // shape 对象
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), Node.ShapeType.IsEmpty() ? TEXT("point") : Node.ShapeType);

    // center 数组 [x, y, z]
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);

    RootObject->SetObjectField(TEXT("shape"), ShapeObject);

    // 序列化为字符串
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    return OutputString;
}

bool FMADynamicNodeManager::UpdateNodeGuid(FMASceneGraphNode& Node, const FString& NewGuid)
{
    if (!Node.IsValid())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeGuid: Invalid node"));
        return false;
    }

    if (NewGuid.IsEmpty())
    {
        UE_LOG(LogMADynamicNodeManager, Warning, TEXT("UpdateNodeGuid: NewGuid is empty for node %s"), *Node.Id);
        return false;
    }

    // 更新 Guid 字段
    Node.Guid = NewGuid;

    // 重新生成 RawJson 以包含新 GUID
    Node.RawJson = GenerateRawJson(Node);

    UE_LOG(LogMADynamicNodeManager, Verbose, TEXT("UpdateNodeGuid: Node %s bound to GUID %s"), *Node.Id, *NewGuid);

    return true;
}

//=============================================================================
// 辅助方法
//=============================================================================

bool FMADynamicNodeManager::IsValidPosition(const FVector& Position)
{
    return !Position.ContainsNaN() &&
           FMath::IsFinite(Position.X) &&
           FMath::IsFinite(Position.Y) &&
           FMath::IsFinite(Position.Z);
}
