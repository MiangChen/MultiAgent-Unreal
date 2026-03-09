// Command dispatch coordination for player input.

#include "MACommandInputCoordinator.h"

#include "../../Core/Manager/MACommandManager.h"
#include "../../Core/Manager/MASelectionManager.h"
#include "../../Agent/Character/MACharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommandInputCoordinator, Log, All);

void FMACommandInputCoordinator::SendCommandToSelection(
    UMASelectionManager* SelectionManager,
    UMACommandManager* CommandManager,
    EMACommand Command) const
{
    if (!SelectionManager || !CommandManager || Command == EMACommand::None)
    {
        return;
    }

    const TArray<AMACharacter*> SelectedAgents = SelectionManager->GetSelectedAgents();
    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent)
        {
            CommandManager->SendCommand(Agent, Command);
        }
    }

    UE_LOG(LogMACommandInputCoordinator, Log, TEXT("Sent %s to %d selected agents"),
        *UMACommandManager::CommandToString(Command),
        SelectedAgents.Num());
}

void FMACommandInputCoordinator::TogglePauseExecution(UMACommandManager* CommandManager) const
{
    if (!CommandManager)
    {
        UE_LOG(LogMACommandInputCoordinator, Warning, TEXT("TogglePauseExecution: CommandManager is null"));
        return;
    }

    CommandManager->TogglePauseExecution();
}
