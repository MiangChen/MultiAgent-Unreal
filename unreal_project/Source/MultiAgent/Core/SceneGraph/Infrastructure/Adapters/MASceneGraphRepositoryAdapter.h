// MASceneGraphRepositoryAdapter.h
// SceneGraph 仓储适配器

#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Infrastructure/Ports/IMASceneGraphRepositoryPort.h"

class FMASceneGraphRepositoryAdapter final : public IMASceneGraphRepositoryPort
{
public:
    FMASceneGraphRepositoryAdapter(UObject* InWorldContextObject, const FString& InSourcePath);

    void UpdateSourcePath(const FString& InSourcePath);

    virtual bool Load(TSharedPtr<FJsonObject>& OutData) override;
    virtual bool Save(const TSharedPtr<FJsonObject>& Data) override;
    virtual FString GetSourcePath() const override;

private:
    FString ResolveSourcePath() const;

    TWeakObjectPtr<UObject> WorldContextObject;
    FString SourcePath;
};
