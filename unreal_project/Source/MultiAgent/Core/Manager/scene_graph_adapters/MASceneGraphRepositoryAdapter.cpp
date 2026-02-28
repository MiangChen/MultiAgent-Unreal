// MASceneGraphRepositoryAdapter.cpp

#include "scene_graph_adapters/MASceneGraphRepositoryAdapter.h"
#include "scene_graph_tools/MASceneGraphIO.h"

FMASceneGraphRepositoryAdapter::FMASceneGraphRepositoryAdapter(UObject* InWorldContextObject, const FString& InSourcePath)
    : WorldContextObject(InWorldContextObject)
    , SourcePath(InSourcePath)
{
}

void FMASceneGraphRepositoryAdapter::UpdateSourcePath(const FString& InSourcePath)
{
    SourcePath = InSourcePath;
}

FString FMASceneGraphRepositoryAdapter::ResolveSourcePath() const
{
    if (!SourcePath.IsEmpty())
    {
        return SourcePath;
    }

    if (WorldContextObject.IsValid())
    {
        return FMASceneGraphIO::GetSceneGraphFilePath(WorldContextObject.Get());
    }

    return FString();
}

bool FMASceneGraphRepositoryAdapter::Load(TSharedPtr<FJsonObject>& OutData)
{
    const FString FilePath = ResolveSourcePath();
    if (FilePath.IsEmpty())
    {
        return false;
    }
    return FMASceneGraphIO::LoadBaseSceneGraph(FilePath, OutData);
}

bool FMASceneGraphRepositoryAdapter::Save(const TSharedPtr<FJsonObject>& Data)
{
    const FString FilePath = ResolveSourcePath();
    if (FilePath.IsEmpty())
    {
        return false;
    }
    return FMASceneGraphIO::SaveSceneGraph(FilePath, Data);
}

FString FMASceneGraphRepositoryAdapter::GetSourcePath() const
{
    return ResolveSourcePath();
}
