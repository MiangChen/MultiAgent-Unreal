#include "Core/Squad/Bootstrap/MASquadBootstrap.h"

#include "Core/Squad/Runtime/MASquadManager.h"
#include "Engine/World.h"

UMASquadManager* FMASquadBootstrap::Resolve(const UObject* WorldContext)
{
    if (const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
    {
        return World->GetSubsystem<UMASquadManager>();
    }

    return nullptr;
}
