#pragma once

#include "CoreMinimal.h"

class UMACommandManager;
class UMASelectionManager;

enum class EMACommand : uint8;

class MULTIAGENT_API FMACommandInputCoordinator
{
public:
    void SendCommandToSelection(
        UMASelectionManager* SelectionManager,
        UMACommandManager* CommandManager,
        EMACommand Command) const;

    void TogglePauseExecution(UMACommandManager* CommandManager) const;
};
