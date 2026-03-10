// MASubsystem.cpp

#include "MASubsystem.h"
#include "AgentRuntime/Runtime/MAAgentManager.h"
#include "Command/Runtime/MACommandManager.h"
#include "Selection/Runtime/MASelectionManager.h"
#include "Squad/Runtime/MASquadManager.h"
#include "Camera/Runtime/MAViewportManager.h"
#include "Editing/Runtime/MAEditModeManager.h"
#include "Engine/World.h"

FMASubsystem FMASubsystem::Get(UWorld* World)
{
    FMASubsystem Subs;
    
    if (World)
    {
        Subs.AgentManager = World->GetSubsystem<UMAAgentManager>();
        Subs.CommandManager = World->GetSubsystem<UMACommandManager>();
        Subs.SelectionManager = World->GetSubsystem<UMASelectionManager>();
        Subs.SquadManager = World->GetSubsystem<UMASquadManager>();
        Subs.ViewportManager = World->GetSubsystem<UMAViewportManager>();
        Subs.EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    }
    
    return Subs;
}

bool FMASubsystem::IsValid() const
{
    return AgentManager && CommandManager && SelectionManager && SquadManager && ViewportManager && EditModeManager;
}
