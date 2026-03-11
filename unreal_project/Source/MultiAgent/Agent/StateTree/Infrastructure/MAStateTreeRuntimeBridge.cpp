#include "Agent/StateTree/Infrastructure/MAStateTreeRuntimeBridge.h"

#include "Agent/StateTree/Runtime/MAStateTreeComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

void FMAStateTreeRuntimeBridge::RestartLogicNextTick(UMAStateTreeComponent& StateTreeComponent)
{
    if (UWorld* World = StateTreeComponent.GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick([&StateTreeComponent]()
        {
            StateTreeComponent.StopLogic(TEXT("Resetting for new StateTree"));
            StateTreeComponent.StartLogic();
        });
    }
}
