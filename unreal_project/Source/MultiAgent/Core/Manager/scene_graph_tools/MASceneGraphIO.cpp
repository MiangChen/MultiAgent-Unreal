// MASceneGraphIO.cpp
// 场景图文件IO模块实现

#include "MASceneGraphIO.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphIO, Log, All);

//=============================================================================
// 文件加载
//=============================================================================

bool FMASceneGraphIO::LoadBaseSceneGraph(const FString& FilePath, TSharedPtr<FJsonObject>& OutData)
{

    OutData.Reset();

    // 检查文件是否存在
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogMASceneGraphIO, Warning, TEXT("LoadBaseSceneGraph: File not found: %s"), *FilePath);
        return false;
    }

    // 读取文件内容
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogMASceneGraphIO, Error, TEXT("LoadBaseSceneGraph: Failed to read file: %s"), *FilePath);
        return false;
    }

    // 解析JSON
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, OutData) || !OutData.IsValid())
    {
        UE_LOG(LogMASceneGraphIO, Error, TEXT("LoadBaseSceneGraph: Failed to parse JSON: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogMASceneGraphIO, Log, TEXT("LoadBaseSceneGraph: Successfully loaded from: %s"), *FilePath);
    return true;
}

bool FMASceneGraphIO::SaveSceneGraph(const FString& FilePath, const TSharedPtr<FJsonObject>& Data)
{
    if (!Data.IsValid())
    {
        UE_LOG(LogMASceneGraphIO, Error, TEXT("SaveSceneGraph: Data is invalid"));
        return false;
    }

    // 序列化为JSON字符串（带格式化缩进）
    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);

    if (!FJsonSerializer::Serialize(Data.ToSharedRef(), Writer))
    {
        UE_LOG(LogMASceneGraphIO, Error, TEXT("SaveSceneGraph: Failed to serialize to JSON"));
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
        UE_LOG(LogMASceneGraphIO, Error, TEXT("SaveSceneGraph: Failed to write file: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogMASceneGraphIO, Log, TEXT("SaveSceneGraph: Successfully saved to: %s"), *FilePath);
    return true;
}


//=============================================================================
// 节点解析
//=============================================================================

TArray<FMASceneGraphNode> FMASceneGraphIO::ParseNodes(const TArray<TSharedPtr<FJsonValue>>& NodesArray)
{

    TArray<FMASceneGraphNode> Result;

    for (const TSharedPtr<FJsonValue>& NodeValue : NodesArray)
    {
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            UE_LOG(LogMASceneGraphIO, Warning, TEXT("ParseNodes: Skipping invalid node value"));
            continue;
        }

        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid())
        {
            continue;
        }

        FMASceneGraphNode Node = ParseSingleNode(NodeObject);
        if (Node.IsValid())
        {
            Result.Add(Node);
        }
    }

    UE_LOG(LogMASceneGraphIO, Log, TEXT("ParseNodes: Parsed %d nodes"), Result.Num());
    return Result;
}

FMASceneGraphNode FMASceneGraphIO::ParseSingleNode(const TSharedPtr<FJsonObject>& NodeObject)
{

    FMASceneGraphNode Node;

    if (!NodeObject.IsValid())
    {
        return Node;
    }

    // 解析id字段
    NodeObject->TryGetStringField(TEXT("id"), Node.Id);

    // 解析guid字段 (小写，用于point类型)
    if (!NodeObject->TryGetStringField(TEXT("guid"), Node.Guid))
    {
        Node.Guid = TEXT("");
    }

    // 解析Guid数组 (大写G，用于polygon/linestring类型)
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

    // 解析properties对象
    const TSharedPtr<FJsonObject>* PropertiesObject;
    if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject))
    {
        // 解析type字段
        (*PropertiesObject)->TryGetStringField(TEXT("type"), Node.Type);

        // 解析label字段
        (*PropertiesObject)->TryGetStringField(TEXT("label"), Node.Label);

        // 解析category字段
        Node.Category = ParseCategory(*PropertiesObject);

        // 解析is_carried字段 (PickupItem专用)
        (*PropertiesObject)->TryGetBoolField(TEXT("is_carried"), Node.bIsCarried);

        // 解析carrier_id字段 (PickupItem专用)
        (*PropertiesObject)->TryGetStringField(TEXT("carrier_id"), Node.CarrierId);

        // 解析Features (color, name等)
        ParseFeatures(*PropertiesObject, Node.Features);
    }

    // 解析shape对象
    const TSharedPtr<FJsonObject>* ShapeObject;
    if (NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject))
    {
        // 解析shape.type字段
        (*ShapeObject)->TryGetStringField(TEXT("type"), Node.ShapeType);

        // 根据shape类型解析中心点
        Node.Center = ParseCenterFromShape(*ShapeObject, Node.ShapeType);
    }
    else
    {
        UE_LOG(LogMASceneGraphIO, Warning, TEXT("ParseSingleNode: Node %s missing shape field"), *Node.Id);
        Node.Center = FVector::ZeroVector;
    }

    // 判断是否为动态节点
    Node.bIsDynamic = Node.IsRobot() || Node.IsPickupItem() || Node.IsChargingStation();

    // 保存原始JSON字符串
    FString NodeJsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJsonString);
    if (FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer))
    {
        Node.RawJson = NodeJsonString;
    }

    return Node;
}


//=============================================================================
// 节点序列化
//=============================================================================

TSharedPtr<FJsonObject> FMASceneGraphIO::SerializeNode(const FMASceneGraphNode& Node)
{

    if (!Node.IsValid())
    {
        UE_LOG(LogMASceneGraphIO, Warning, TEXT("SerializeNode: Invalid node"));
        return nullptr;
    }

    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());

    // 设置id字段
    NodeObject->SetStringField(TEXT("id"), Node.Id);

    // 设置count字段 (用于兼容现有格式)
    if (Node.GuidArray.Num() > 0)
    {
        NodeObject->SetNumberField(TEXT("count"), Node.GuidArray.Num());

        // 设置Guid数组
        TArray<TSharedPtr<FJsonValue>> GuidArray;
        for (const FString& Guid : Node.GuidArray)
        {
            GuidArray.Add(MakeShareable(new FJsonValueString(Guid)));
        }
        NodeObject->SetArrayField(TEXT("Guid"), GuidArray);
    }
    else if (!Node.Guid.IsEmpty())
    {
        // 单个guid (point类型)
        NodeObject->SetStringField(TEXT("guid"), Node.Guid);
    }

    // 构建properties对象
    TSharedPtr<FJsonObject> Properties = MakeShareable(new FJsonObject());
    Properties->SetStringField(TEXT("type"), Node.Type);

    if (!Node.Category.IsEmpty())
    {
        Properties->SetStringField(TEXT("category"), Node.Category);
    }

    if (!Node.Label.IsEmpty())
    {
        Properties->SetStringField(TEXT("label"), Node.Label);
    }

    // 机器人专用字段
    if (Node.IsRobot())
    {
        Properties->SetStringField(TEXT("status"), TEXT("idle"));
    }

    // PickupItem专用字段 (Type == "cargo")
    if (Node.IsPickupItem())
    {
        Properties->SetBoolField(TEXT("is_carried"), Node.bIsCarried);
        if (!Node.CarrierId.IsEmpty())
        {
            Properties->SetStringField(TEXT("carrier_id"), Node.CarrierId);
        }

        // 添加Features到properties
        for (const auto& Pair : Node.Features)
        {
            Properties->SetStringField(Pair.Key, Pair.Value);
        }
    }

    // ChargingStation专用字段
    if (Node.IsChargingStation())
    {
        Properties->SetStringField(TEXT("status"), TEXT("available"));
    }

    NodeObject->SetObjectField(TEXT("properties"), Properties);

    // 构建shape对象
    TSharedPtr<FJsonObject> Shape = MakeShareable(new FJsonObject());

    // 动态节点使用point类型
    if (Node.bIsDynamic || Node.ShapeType.IsEmpty())
    {
        Shape->SetStringField(TEXT("type"), TEXT("point"));
        Shape->SetArrayField(TEXT("center"), CreateCenterArray(Node.Center));
    }
    else
    {
        Shape->SetStringField(TEXT("type"), Node.ShapeType);

        // 对于point类型，设置center
        if (Node.ShapeType == TEXT("point"))
        {
            Shape->SetArrayField(TEXT("center"), CreateCenterArray(Node.Center));
        }
        // 其他类型保持原有shape数据（从RawJson解析）
    }

    NodeObject->SetObjectField(TEXT("shape"), Shape);

    return NodeObject;
}

FString FMASceneGraphIO::SerializeNodeToString(const FMASceneGraphNode& Node, bool bPrettyPrint)
{
    TSharedPtr<FJsonObject> NodeObject = SerializeNode(Node);
    if (!NodeObject.IsValid())
    {
        return FString();
    }

    FString JsonString;
    if (bPrettyPrint)
    {
        TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
            TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);
        FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
    }
    else
    {
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
            TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
        FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
    }

    return JsonString;
}


//=============================================================================
// 内部辅助方法
//=============================================================================

FVector FMASceneGraphIO::ParseCenterFromShape(const TSharedPtr<FJsonObject>& ShapeObject, const FString& ShapeType)
{
    if (!ShapeObject.IsValid())
    {
        return FVector::ZeroVector;
    }

    if (ShapeType == TEXT("point"))
    {
        // point类型: 使用center字段
        const TArray<TSharedPtr<FJsonValue>>* CenterArray;
        if (ShapeObject->TryGetArrayField(TEXT("center"), CenterArray))
        {
            if (CenterArray->Num() >= 3)
            {
                return FVector(
                    (*CenterArray)[0]->AsNumber(),
                    (*CenterArray)[1]->AsNumber(),
                    (*CenterArray)[2]->AsNumber()
                );
            }
        }
    }
    else if (ShapeType == TEXT("polygon"))
    {
        // polygon类型: 从vertices计算中心点
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
        if (ShapeObject->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            return CalculatePolygonCentroid(*VerticesArray);
        }
    }
    else if (ShapeType == TEXT("linestring"))
    {
        // linestring类型: 从points计算中心点
        const TArray<TSharedPtr<FJsonValue>>* PointsArray;
        if (ShapeObject->TryGetArrayField(TEXT("points"), PointsArray))
        {
            return CalculateLineStringCentroid(*PointsArray);
        }
    }
    else if (ShapeType == TEXT("prism"))
    {
        // prism类型: 从vertices (底面顶点) 计算中心点
        const TArray<TSharedPtr<FJsonValue>>* VerticesArray;
        if (ShapeObject->TryGetArrayField(TEXT("vertices"), VerticesArray))
        {
            FVector Center = CalculatePolygonCentroid(*VerticesArray);

            // 将Z坐标调整为棱柱高度的一半
            double Height = 0.0;
            if (ShapeObject->TryGetNumberField(TEXT("height"), Height))
            {
                Center.Z += Height / 2.0;
            }

            return Center;
        }
    }
    else
    {
        // 未知类型，尝试使用center字段
        const TArray<TSharedPtr<FJsonValue>>* CenterArray;
        if (ShapeObject->TryGetArrayField(TEXT("center"), CenterArray))
        {
            if (CenterArray->Num() >= 3)
            {
                return FVector(
                    (*CenterArray)[0]->AsNumber(),
                    (*CenterArray)[1]->AsNumber(),
                    (*CenterArray)[2]->AsNumber()
                );
            }
        }
    }

    return FVector::ZeroVector;
}

FVector FMASceneGraphIO::CalculatePolygonCentroid(const TArray<TSharedPtr<FJsonValue>>& Vertices)
{
    if (Vertices.Num() == 0)
    {
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
        return FVector::ZeroVector;
    }

    return Sum / static_cast<float>(ValidCount);
}

FVector FMASceneGraphIO::CalculateLineStringCentroid(const TArray<TSharedPtr<FJsonValue>>& Points)
{
    if (Points.Num() == 0)
    {
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
        return FVector::ZeroVector;
    }

    return Sum / static_cast<float>(ValidCount);
}

FString FMASceneGraphIO::ParseCategory(const TSharedPtr<FJsonObject>& PropertiesObject)
{
    if (!PropertiesObject.IsValid())
    {
        return FString();
    }

    // 首先尝试直接获取category字段
    FString Category;
    if (PropertiesObject->TryGetStringField(TEXT("category"), Category))
    {
        return Category;
    }

    // 如果没有category字段，根据type推断
    FString Type;
    if (PropertiesObject->TryGetStringField(TEXT("type"), Type))
    {
        Type = Type.ToLower();

        if (Type == TEXT("building"))
        {
            return TEXT("building");
        }
        else if (Type == TEXT("road_segment") || Type == TEXT("street_segment") || Type == TEXT("intersection"))
        {
            return TEXT("trans_facility");
        }
        else if (Type == TEXT("robot") || Type == TEXT("uav") || Type == TEXT("ugv") || 
                 Type == TEXT("quadruped") || Type == TEXT("humanoid") || Type == TEXT("fixedwinguav"))
        {
            return TEXT("robot");
        }
        else if (Type == TEXT("cargo") || Type == TEXT("charging_station"))
        {
            return TEXT("prop");  // cargo 和 charging_station 归类为 prop
        }
        else
        {
            // 默认为prop
            return TEXT("prop");
        }
    }

    return FString();
}

void FMASceneGraphIO::ParseFeatures(const TSharedPtr<FJsonObject>& PropertiesObject, TMap<FString, FString>& OutFeatures)
{
    if (!PropertiesObject.IsValid())
    {
        return;
    }

    // 解析常见的feature字段
    static const TArray<FString> FeatureKeys = {
        TEXT("color"),
        TEXT("name"),
        TEXT("item_type"),
        TEXT("status"),
        TEXT("visibility"),
        TEXT("wind_condition"),
        TEXT("congestion")
    };

    for (const FString& Key : FeatureKeys)
    {
        FString Value;
        if (PropertiesObject->TryGetStringField(Key, Value))
        {
            OutFeatures.Add(Key, Value);
        }
    }

    // 也检查嵌套的features对象
    const TSharedPtr<FJsonObject>* FeaturesObject;
    if (PropertiesObject->TryGetObjectField(TEXT("features"), FeaturesObject))
    {
        for (const auto& Pair : (*FeaturesObject)->Values)
        {
            if (Pair.Value.IsValid() && Pair.Value->Type == EJson::String)
            {
                OutFeatures.Add(Pair.Key, Pair.Value->AsString());
            }
        }
    }
}

TArray<TSharedPtr<FJsonValue>> FMASceneGraphIO::CreateCenterArray(const FVector& Center)
{
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Center.Z)));
    return CenterArray;
}
