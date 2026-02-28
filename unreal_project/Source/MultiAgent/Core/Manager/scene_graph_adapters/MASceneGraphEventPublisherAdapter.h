// MASceneGraphEventPublisherAdapter.h
// SceneGraph 事件发布适配器

#pragma once

#include "CoreMinimal.h"
#include "../Config/MAConfigManager.h"
#include "scene_graph_ports/IMASceneGraphEventPublisherPort.h"

class UGameInstance;

class FMASceneGraphEventPublisherAdapter final : public IMASceneGraphEventPublisherPort
{
public:
    FMASceneGraphEventPublisherAdapter(UGameInstance* InGameInstance, EMARunMode InRunMode);

    void UpdateContext(UGameInstance* InGameInstance, EMARunMode InRunMode);

    virtual void PublishSceneChange(EMASceneChangeType ChangeType, const FString& Payload) override;

private:
    EMARunMode RunMode;
    TWeakObjectPtr<UGameInstance> GameInstance;
};
