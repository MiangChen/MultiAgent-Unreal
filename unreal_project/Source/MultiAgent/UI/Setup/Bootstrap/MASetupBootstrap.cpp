#include "MASetupBootstrap.h"

const TMap<FString, FString>& FMASetupBootstrap::GetAvailableAgentTypes()
{
    static const TMap<FString, FString> AgentTypes = {
        {TEXT("UAV"), TEXT("UAV (Multi-rotor)")},
        {TEXT("FixedWingUAV"), TEXT("Fixed Wing UAV")},
        {TEXT("UGV"), TEXT("UGV (Ground Vehicle)")},
        {TEXT("Quadruped"), TEXT("Quadruped Robot")},
        {TEXT("Humanoid"), TEXT("Humanoid")},
    };
    return AgentTypes;
}

const TArray<FString>& FMASetupBootstrap::GetAvailableScenes()
{
    static const TArray<FString> Scenes = {
        TEXT("CyberCity"),
        TEXT("DesertLab"),
        TEXT("SpruceForest"),
        TEXT("Warehouse"),
    };
    return Scenes;
}

const TMap<FString, FString>& FMASetupBootstrap::GetSceneToMapPaths()
{
    static const TMap<FString, FString> ScenePaths = {
        {TEXT("CyberCity"), TEXT("/Game/Map/LS_Scifi_ModernCity/Maps/Scifi_ModernCity")},
        {TEXT("DesertLab"), TEXT("/Game/Map/DesertLab/Maps/DesertLab")},
        {TEXT("SpruceForest"), TEXT("/Game/Map/Spruce_Forest/Demo/Maps/SpruceForest")},
        {TEXT("Town"), TEXT("/Game/Map/Town/DreamscapeFarmlands/Maps/Demo_Village_New")},
        {TEXT("Warehouse"), TEXT("/Game/Map/Warehouse/Maps/Demonstration_Map")},
    };
    return ScenePaths;
}

TArray<FMAAgentSetupConfig> FMASetupBootstrap::BuildDefaultAgentConfigs()
{
    return {
        FMAAgentSetupConfig(TEXT("UAV"), TEXT("UAV"), 1),
        FMAAgentSetupConfig(TEXT("Quadruped"), TEXT("Quadruped Robot"), 1),
        FMAAgentSetupConfig(TEXT("Humanoid"), TEXT("Humanoid"), 1),
    };
}
