// IMASceneGraphRepositoryPort.h
// SceneGraph 仓储端口定义 (Hexagonal Port)

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class IMASceneGraphRepositoryPort
{
public:
    virtual ~IMASceneGraphRepositoryPort() = default;

    virtual bool Load(TSharedPtr<FJsonObject>& OutData) = 0;
    virtual bool Save(const TSharedPtr<FJsonObject>& Data) = 0;
    virtual FString GetSourcePath() const = 0;
};
