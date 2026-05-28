// MASceneGraphCommandService.h
// SceneGraph 写服务 (P1c-b2: Manager 内部写路径转发)

#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Infrastructure/Ports/IMASceneGraphCommandPort.h"

class FMASceneGraphCommandService final : public IMASceneGraphCommandPort
{
public:
    struct FHandlers
    {
        TFunction<bool(const FString&, FString&)> AddNode;
        TFunction<bool(const FString&, FString&)> DeleteNode;
        TFunction<bool(const FString&, const FString&, FString&)> EditNode;
        TFunction<bool(const FString&, FString&)> SetNodeAsGoal;
        TFunction<bool(const FString&, FString&)> UnsetNodeAsGoal;
        TFunction<void(const FString&, const FVector&)> UpdateDynamicNodePosition;
        TFunction<void(const FString&, const FString&, const FString&)> UpdateDynamicNodeFeature;
        TFunction<bool()> SaveToSource;
    };

    explicit FMASceneGraphCommandService(FHandlers InHandlers);

    virtual bool AddNode(const FString& NodeJson, FString& OutError) override;
    virtual bool DeleteNode(const FString& NodeId, FString& OutError) override;
    virtual bool EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError) override;
    virtual bool SetNodeAsGoal(const FString& NodeId, FString& OutError) override;
    virtual bool UnsetNodeAsGoal(const FString& NodeId, FString& OutError) override;
    virtual void UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition) override;
    virtual void UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value) override;
    virtual bool SaveToSource() override;

private:
    FHandlers Handlers;
};
