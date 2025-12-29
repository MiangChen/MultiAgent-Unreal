// MASubsystem.cpp

#include "MASubsystem.h"
#include "MAAgentManager.h"
#include "MACommandManager.h"
#include "MASelectionManager.h"
#include "MASquadManager.h"
#include "MAViewportManager.h"
#include "MAEmergencyManager.h"
#include "MAEditModeManager.h"
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
        Subs.EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
        Subs.EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    }
    
    return Subs;
}

bool FMASubsystem::IsValid() const
{
    return AgentManager && CommandManager && SelectionManager && SquadManager && ViewportManager && EmergencyManager && EditModeManager;
}
