// MASubsystem.cpp

#include "MASubsystem.h"
#include "Manager/MAAgentManager.h"
#include "Manager/MACommandManager.h"
#include "Manager/MASelectionManager.h"
#include "Manager/MASquadManager.h"
#include "Manager/MAViewportManager.h"
#include "Manager/MAEditModeManager.h"
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
