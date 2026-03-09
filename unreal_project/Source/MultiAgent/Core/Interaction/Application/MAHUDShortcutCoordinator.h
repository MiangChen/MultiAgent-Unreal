#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"

class APlayerController;

class MULTIAGENT_API FMAHUDShortcutCoordinator
{
public:
    FMAFeedback21Batch HandleCheckTask() const;
    FMAFeedback21Batch HandleCheckSkill() const;
    FMAFeedback21Batch HandleCheckDecision() const;

    FMAFeedback21Batch ToggleSystemLogPanel() const;
    FMAFeedback21Batch TogglePreviewPanel() const;
    FMAFeedback21Batch ToggleInstructionPanel() const;
    FMAFeedback21Batch ToggleAgentHighlight() const;
};
