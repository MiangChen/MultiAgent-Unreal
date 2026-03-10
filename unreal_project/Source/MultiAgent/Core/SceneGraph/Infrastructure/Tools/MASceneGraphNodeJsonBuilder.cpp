// MASceneGraphNodeJsonBuilder.cpp

#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphNodeJsonBuilder.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString FMASceneGraphNodeJsonBuilder::BuildPointNodeJson(
    const FString& Id,
    const FString& Type,
    const FString& Label,
    const FVector& WorldLocation,
    const FString& Guid)
{
    TSharedPtr<FJsonObject> NewNode = MakeShareable(new FJsonObject());
    NewNode->SetStringField(TEXT("id"), Id);
    NewNode->SetStringField(TEXT("guid"), Guid);

    TSharedPtr<FJsonObject> Properties = MakeShareable(new FJsonObject());
    Properties->SetStringField(TEXT("type"), Type);
    Properties->SetStringField(TEXT("label"), Label);
    NewNode->SetObjectField(TEXT("properties"), Properties);

    TSharedPtr<FJsonObject> Shape = MakeShareable(new FJsonObject());
    Shape->SetStringField(TEXT("type"), TEXT("point"));
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(WorldLocation.Z)));
    Shape->SetArrayField(TEXT("center"), CenterArray);
    NewNode->SetObjectField(TEXT("shape"), Shape);

    FString NodeJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJson);
    FJsonSerializer::Serialize(NewNode.ToSharedRef(), Writer);
    return NodeJson;
}
