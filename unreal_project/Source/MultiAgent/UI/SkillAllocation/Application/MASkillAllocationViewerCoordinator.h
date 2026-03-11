#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MASkillAllocationViewerFeedback.h"

struct FMASkillAllocationData;
class UMASkillAllocationModel;

class FMASkillAllocationViewerCoordinator
{
public:
    static FMASkillAllocationViewerActionResult LoadAllocationData(
        UMASkillAllocationModel* AllocationModel,
        const FMASkillAllocationData& Data,
        const FString& SuccessMessage,
        bool bShouldPersist = false);

    static FMASkillAllocationViewerActionResult LoadAllocationJson(
        UMASkillAllocationModel* AllocationModel,
        const FString& JsonText,
        bool bShouldPersist = true);

    static FMASkillAllocationViewerActionResult ResetAllocation(
        UMASkillAllocationModel* AllocationModel,
        bool bShouldPersist = true);
};
