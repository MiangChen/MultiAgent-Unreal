#include "Core/Config/Infrastructure/MAConfigPathResolver.h"
#include "Misc/Paths.h"

FString FMAConfigPathResolver::GetConfigRootPath()
{
    return FPaths::ProjectDir() / TEXT("..");
}

FString FMAConfigPathResolver::GetConfigFilePath(const FString& RelativePath)
{
    return GetConfigRootPath() / TEXT("config") / RelativePath;
}

FString FMAConfigPathResolver::GetDatasetFilePath(const FString& RelativePath)
{
    return GetConfigRootPath() / TEXT("datasets") / RelativePath;
}

FString FMAConfigPathResolver::ResolveSceneGraphFilePath(const FString& SceneGraphPath)
{
    if (SceneGraphPath.IsEmpty())
    {
        return FPaths::ProjectDir() / TEXT("datasets/scene_graph_cyberworld.json");
    }

    FString FullPath = FPaths::ProjectDir() / SceneGraphPath;
    if (!SceneGraphPath.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
    {
        FullPath = FullPath / TEXT("scene_graph.json");
    }
    return FullPath;
}
