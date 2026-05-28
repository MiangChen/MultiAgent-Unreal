#pragma once

#include "CoreMinimal.h"

struct FMAConfigPathResolver
{
    static FString GetConfigRootPath();
    static FString GetConfigFilePath(const FString& RelativePath);
    static FString GetDatasetFilePath(const FString& RelativePath);
    static FString ResolveSceneGraphFilePath(const FString& SceneGraphPath);
};
