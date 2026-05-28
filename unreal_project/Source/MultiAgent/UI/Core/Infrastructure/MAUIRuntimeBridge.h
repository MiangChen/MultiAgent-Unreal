#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

class UMAUIManager;

class MULTIAGENT_API FMAUIRuntimeBridge
{
public:
    bool BindTempDataEvents(UMAUIManager* UIManager, FString& OutError) const;
    bool BindCommEvents(UMAUIManager* UIManager, FString& OutError) const;
    bool BindCommandEvents(UMAUIManager* UIManager, FString& OutError) const;

    bool TryLoadTaskGraphData(const UMAUIManager* UIManager, FMATaskGraphData& OutData, FString& OutError) const;
    bool TryLoadSkillAllocationData(const UMAUIManager* UIManager, FMASkillAllocationData& OutData, FString& OutError) const;
    bool TrySendDecisionResponse(const UMAUIManager* UIManager, const FString& SelectedOption, const FString& UserFeedback, FString& OutError) const;
    bool TryClearResumeNotificationTimer(const UMAUIManager* UIManager, FTimerHandle& TimerHandle, FString& OutError) const;
    bool TryScheduleResumeNotificationAutoHide(const UMAUIManager* UIManager, FTimerHandle& TimerHandle, float DelaySeconds, FString& OutError) const;
};
