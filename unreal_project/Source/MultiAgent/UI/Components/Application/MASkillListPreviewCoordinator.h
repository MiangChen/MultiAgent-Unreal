#pragma once

#include "CoreMinimal.h"
#include "UI/Components/Domain/MASkillListPreviewModels.h"

class FMASkillListPreviewCoordinator
{
public:
    static FMASkillListPreviewModel BuildModel(const FMASkillAllocationData& Data);
    static FMASkillListPreviewModel MakeEmptyModel();
    static bool UpdateSkillStatus(
        FMASkillListPreviewModel& Model,
        int32 TimeStep,
        const FString& RobotId,
        ESkillExecutionStatus NewStatus);
};
