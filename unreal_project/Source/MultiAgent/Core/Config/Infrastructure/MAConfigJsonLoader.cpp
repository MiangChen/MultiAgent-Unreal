#include "Core/Config/Infrastructure/MAConfigJsonLoader.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FMAConfigJsonLoader::LoadJsonObjectFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutRootObject)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        return false;
    }

    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    return FJsonSerializer::Deserialize(Reader, OutRootObject) && OutRootObject.IsValid();
}
