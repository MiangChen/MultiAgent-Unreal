#pragma once

#include "CoreMinimal.h"

class FJsonObject;

struct FMAConfigJsonLoader
{
    static bool LoadJsonObjectFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutRootObject);
};
