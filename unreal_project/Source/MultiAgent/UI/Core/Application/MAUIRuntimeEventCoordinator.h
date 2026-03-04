// MAUIRuntimeEventCoordinator.h
// UIManager 运行时事件桥接协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"
#include "../MAHUDTypes.h"

class UMAUIManager;
struct FMATaskGraphData;
struct FMASkillAllocationData;
enum class ESkillExecutionStatus : uint8;

class MULTIAGENT_API FMAUIRuntimeEventCoordinator
{
public:
    void BindTempDataManagerEvents(UMAUIManager* UIManager) const;
    void HandleTempTaskGraphChanged(UMAUIManager* UIManager, const FMATaskGraphData& Data) const;
    void HandleTempSkillAllocationChanged(UMAUIManager* UIManager, const FMASkillAllocationData& Data) const;
    void HandleSkillStatusUpdated(UMAUIManager* UIManager, int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus) const;

    void BindCommSubsystemEvents(UMAUIManager* UIManager) const;
    void HandleRequestUserCommandReceived(UMAUIManager* UIManager) const;

    void BindCommandManagerEvents(UMAUIManager* UIManager) const;
    void HandleExecutionPauseStateChanged(UMAUIManager* UIManager, bool bPaused) const;

    void HandleDecisionDataReceived(UMAUIManager* UIManager, const FString& Description, const FString& ContextJson) const;
    void HandleDecisionModalConfirmed(UMAUIManager* UIManager, const FString& SelectedOption, const FString& UserFeedback) const;
    void HandleDecisionModalRejected(UMAUIManager* UIManager, const FString& SelectedOption, const FString& UserFeedback) const;
};
