#include "MASetupWidgetCoordinator.h"

#include "../Bootstrap/MASetupBootstrap.h"

void FMASetupWidgetCoordinator::InitializeState(FMASetupWidgetState& State) const
{
    if (!State.AvailableScenes.IsEmpty())
    {
        return;
    }

    State.AvailableAgentTypes = FMASetupBootstrap::GetAvailableAgentTypes();
    State.AvailableScenes = FMASetupBootstrap::GetAvailableScenes();
    State.AgentConfigs = FMASetupBootstrap::BuildDefaultAgentConfigs();

    if (!State.AvailableScenes.Contains(State.SelectedScene) && !State.AvailableScenes.IsEmpty())
    {
        State.SelectedScene = State.AvailableScenes[0];
    }
}

void FMASetupWidgetCoordinator::SetSelectedScene(FMASetupWidgetState& State, const FString& SelectedScene) const
{
    if (State.AvailableScenes.Contains(SelectedScene))
    {
        State.SelectedScene = SelectedScene;
    }
}

void FMASetupWidgetCoordinator::AddAgent(FMASetupWidgetState& State, const FString& AgentType, int32 Count) const
{
    const int32 SafeCount = FMath::Max(Count, 1);
    const FString DisplayName = State.AvailableAgentTypes.FindRef(AgentType).IsEmpty() ? AgentType : State.AvailableAgentTypes.FindRef(AgentType);

    for (FMAAgentSetupConfig& Config : State.AgentConfigs)
    {
        if (Config.AgentType == AgentType)
        {
            Config.Count += SafeCount;
            return;
        }
    }

    State.AgentConfigs.Add(FMAAgentSetupConfig(AgentType, DisplayName, SafeCount));
}

void FMASetupWidgetCoordinator::ClearAgents(FMASetupWidgetState& State) const
{
    State.AgentConfigs.Reset();
}

void FMASetupWidgetCoordinator::RemoveAgentAt(FMASetupWidgetState& State, int32 Index) const
{
    if (State.AgentConfigs.IsValidIndex(Index))
    {
        State.AgentConfigs.RemoveAt(Index);
    }
}

void FMASetupWidgetCoordinator::DecreaseAgentCount(FMASetupWidgetState& State, int32 Index) const
{
    if (!State.AgentConfigs.IsValidIndex(Index))
    {
        return;
    }

    FMAAgentSetupConfig& Config = State.AgentConfigs[Index];
    Config.Count--;
    if (Config.Count <= 0)
    {
        State.AgentConfigs.RemoveAt(Index);
    }
}

void FMASetupWidgetCoordinator::IncreaseAgentCount(FMASetupWidgetState& State, int32 Index) const
{
    if (State.AgentConfigs.IsValidIndex(Index))
    {
        State.AgentConfigs[Index].Count++;
    }
}

int32 FMASetupWidgetCoordinator::GetTotalAgentCount(const FMASetupWidgetState& State) const
{
    int32 Total = 0;
    for (const FMAAgentSetupConfig& Config : State.AgentConfigs)
    {
        Total += Config.Count;
    }
    return Total;
}

FMASetupLaunchRequest FMASetupWidgetCoordinator::BuildLaunchRequest(const FMASetupWidgetState& State) const
{
    FMASetupLaunchRequest Request;
    Request.SelectedScene = State.SelectedScene;
    Request.TotalAgentCount = GetTotalAgentCount(State);

    for (const FMAAgentSetupConfig& Config : State.AgentConfigs)
    {
        Request.AgentConfigs.FindOrAdd(Config.AgentType) += Config.Count;
    }

    return Request;
}
