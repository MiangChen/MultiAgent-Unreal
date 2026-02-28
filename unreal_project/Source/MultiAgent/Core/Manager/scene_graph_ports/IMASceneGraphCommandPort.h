// IMASceneGraphCommandPort.h
// SceneGraph 写端口定义 (Hexagonal Port)

#pragma once

#include "CoreMinimal.h"

class IMASceneGraphCommandPort
{
public:
    virtual ~IMASceneGraphCommandPort() = default;

    virtual bool AddNode(const FString& NodeJson, FString& OutError) = 0;
    virtual bool DeleteNode(const FString& NodeId, FString& OutError) = 0;
    virtual bool EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError) = 0;

    virtual bool SetNodeAsGoal(const FString& NodeId, FString& OutError) = 0;
    virtual bool UnsetNodeAsGoal(const FString& NodeId, FString& OutError) = 0;

    virtual void UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition) = 0;
    virtual void UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value) = 0;

    virtual bool SaveToSource() = 0;
};
