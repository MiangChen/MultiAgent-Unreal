#include "Core/TaskGraph/Bootstrap/MATaskGraphBootstrap.h"

#include "Misc/Paths.h"

TArray<FMANodeTemplate> FTaskGraphBootstrap::BuildDefaultNodeTemplates()
{
    return {
        FMANodeTemplate(TEXT("Navigate"), TEXT("Navigate to target location"), TEXT("target_location")),
        FMANodeTemplate(TEXT("Patrol"), TEXT("Patrol specified area"), TEXT("patrol_area")),
        FMANodeTemplate(TEXT("Perceive"), TEXT("Perceive surroundings"), TEXT("current_location")),
        FMANodeTemplate(TEXT("Broadcast"), TEXT("Broadcast message"), TEXT("broadcast_area")),
        FMANodeTemplate(TEXT("Custom"), TEXT("Custom task"), TEXT("custom_location"))
    };
}

FString FTaskGraphBootstrap::GetMockResponseExamplePath(const FString& ProjectDir)
{
    return FPaths::Combine(ProjectDir, TEXT("datasets/response_example.json"));
}
