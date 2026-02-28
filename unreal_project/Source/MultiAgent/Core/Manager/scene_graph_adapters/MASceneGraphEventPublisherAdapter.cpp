// MASceneGraphEventPublisherAdapter.cpp

#include "scene_graph_adapters/MASceneGraphEventPublisherAdapter.h"
#include "../Comm/MACommSubsystem.h"
#include "Engine/GameInstance.h"

FMASceneGraphEventPublisherAdapter::FMASceneGraphEventPublisherAdapter(UGameInstance* InGameInstance, EMARunMode InRunMode)
    : RunMode(InRunMode)
    , GameInstance(InGameInstance)
{
}

void FMASceneGraphEventPublisherAdapter::UpdateContext(UGameInstance* InGameInstance, EMARunMode InRunMode)
{
    GameInstance = InGameInstance;
    RunMode = InRunMode;
}

void FMASceneGraphEventPublisherAdapter::PublishSceneChange(EMASceneChangeType ChangeType, const FString& Payload)
{
    if (RunMode != EMARunMode::Edit)
    {
        return;
    }

    if (!GameInstance.IsValid())
    {
        return;
    }

    UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
    if (!CommSubsystem)
    {
        return;
    }

    FMASceneChangeMessage Message;
    Message.ChangeType = ChangeType;
    Message.Payload = Payload;
    CommSubsystem->SendSceneChangeMessage(Message);
}
