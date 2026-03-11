#pragma once

#include "CoreMinimal.h"
#include "../MAHUDTypes.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

struct FMAUIPreviewFeedback
{
    bool bUpdateTaskGraph = false;
    FMATaskGraphData TaskGraphData;
    bool bUpdateSkillAllocation = false;
    FMASkillAllocationData SkillAllocationData;
    bool bUpdateSkillStatus = false;
    int32 TimeStep = INDEX_NONE;
    FString RobotId;
    ESkillExecutionStatus SkillStatus = ESkillExecutionStatus::Pending;
};

struct FMAUINotificationFeedback
{
    EMANotificationType Type = EMANotificationType::RequestUserCommand;
};

struct FMAUIDecisionModalFeedback
{
    FString Description;
    FString ContextJson;
};
