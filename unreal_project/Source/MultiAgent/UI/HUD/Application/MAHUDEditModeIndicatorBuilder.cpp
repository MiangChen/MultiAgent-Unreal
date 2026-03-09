// Builder for edit-mode indicator list content.

#include "MAHUDEditModeIndicatorBuilder.h"

#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"
#include "Engine/GameInstance.h"

bool FMAHUDEditModeIndicatorBuilder::Build(UWorld* World, FMAHUDEditModeIndicatorModel& OutModel) const
{
    OutModel = FMAHUDEditModeIndicatorModel();

    if (!World)
    {
        return false;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return false;
    }

    const TArray<AMAPointOfInterest*> AllPOIs = EditModeManager->GetAllPOIs();
    for (int32 Index = 0; Index < AllPOIs.Num(); ++Index)
    {
        if (AMAPointOfInterest* POI = AllPOIs[Index])
        {
            const FVector Location = POI->GetActorLocation();
            OutModel.POIInfos.Add(FString::Printf(TEXT("[%d](%.0f, %.0f, %.0f)"), Index + 1, Location.X, Location.Y, Location.Z));
        }
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        return false;
    }

    for (const FMASceneGraphNode& GoalNode : SceneGraphManager->GetAllGoals())
    {
        FString Label = GoalNode.Label.IsEmpty() ? GoalNode.Id : GoalNode.Label;
        if (AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalNode.Id))
        {
            const FVector Location = GoalActor->GetActorLocation();
            OutModel.GoalInfos.Add(FString::Printf(TEXT("[%s](%.0f, %.0f, %.0f)"), *Label, Location.X, Location.Y, Location.Z));
        }
        else
        {
            OutModel.GoalInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
        }
    }

    for (const FMASceneGraphNode& ZoneNode : SceneGraphManager->GetAllZones())
    {
        const FString Label = ZoneNode.Label.IsEmpty() ? ZoneNode.Id : ZoneNode.Label;
        OutModel.ZoneInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
    }

    return true;
}
