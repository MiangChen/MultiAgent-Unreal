// IMASceneGraphEventPublisherPort.h
// SceneGraph 事件发布端口定义 (Hexagonal Port)

#pragma once

#include "CoreMinimal.h"
#include "Core/Comm/MACommTypes.h"

class IMASceneGraphEventPublisherPort
{
public:
    virtual ~IMASceneGraphEventPublisherPort() = default;

    virtual void PublishSceneChange(EMASceneChangeType ChangeType, const FString& Payload) = 0;
};
