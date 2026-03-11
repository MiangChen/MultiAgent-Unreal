#include "MAStateTreeBootstrap.h"
#include "Agent/StateTree/Infrastructure/MAStateTreeRuntimeBridge.h"
#include "Agent/StateTree/Runtime/MAStateTreeComponent.h"
#include "Core/Config/MAConfigManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "StateTree.h"

bool FMAStateTreeBootstrap::ShouldUseStateTree(UWorld* World)
{
    if (!World)
    {
        return true;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return true;
    }

    if (UMAConfigManager* ConfigMgr = GameInstance->GetSubsystem<UMAConfigManager>())
    {
        return ConfigMgr->bUseStateTree;
    }

    return true;
}

void FMAStateTreeBootstrap::RestartWithAsset(UMAStateTreeComponent& StateTreeComponent, UStateTree* StateTree)
{
    if (!StateTree)
    {
        return;
    }

    if (!StateTreeComponent.SetResolvedStateTree(StateTree))
    {
        return;
    }

    FMAStateTreeRuntimeBridge::RestartLogicNextTick(StateTreeComponent);
}
