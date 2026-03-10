#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASetupModels.h"

class FMASetupWidgetCoordinator
{
public:
    void InitializeState(FMASetupWidgetState& State) const;
    void SetSelectedScene(FMASetupWidgetState& State, const FString& SelectedScene) const;
    void AddAgent(FMASetupWidgetState& State, const FString& AgentType, int32 Count) const;
    void ClearAgents(FMASetupWidgetState& State) const;
    void RemoveAgentAt(FMASetupWidgetState& State, int32 Index) const;
    void DecreaseAgentCount(FMASetupWidgetState& State, int32 Index) const;
    void IncreaseAgentCount(FMASetupWidgetState& State, int32 Index) const;
    int32 GetTotalAgentCount(const FMASetupWidgetState& State) const;
    FMASetupLaunchRequest BuildLaunchRequest(const FMASetupWidgetState& State) const;
};
