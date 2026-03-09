// MAHUDBackendCoordinator.h
// HUD backend interaction coordination.

#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Interaction/Feedback/MAFeedback21.h"
#include "../../Core/MAHUDTypes.h"
#include "../../../Core/Types/MATaskGraphTypes.h"
#include "../../../Core/Comm/MACommTypes.h"

class AMAHUD;
class UWorld;
class UMAUIManager;
class UMACommSubsystem;
struct FMAPlannerResponse;

class MULTIAGENT_API FMAHUDBackendCoordinator
{
public:
    void BindBackendEvents(UWorld* World, UMAUIManager* UIManager, AMAHUD* HUD) const;
    void BindModalDelegates(UMAUIManager* UIManager, AMAHUD* HUD) const;

    void HandleSimpleCommandSubmitted(UWorld* World, UMAUIManager* UIManager, AMAHUD* HUD, const FString& Command) const;
    void HandlePlannerResponse(AMAHUD* HUD, UMAUIManager* UIManager, const FMAPlannerResponse& Response) const;

    void HandleTaskGraphReceived(AMAHUD* HUD, UMAUIManager* UIManager, const FMATaskPlan& TaskPlan) const;
    void HandleSkillAllocationReceived(AMAHUD* HUD, UMAUIManager* UIManager, const FMASkillAllocationData& AllocationData) const;
    void HandleSkillListReceived(AMAHUD* HUD, UMAUIManager* UIManager, const FMASkillListMessage& SkillList, bool bExecutable) const;

    void HandleModalConfirmed(AMAHUD* HUD, UMAUIManager* UIManager, EMAModalType ModalType) const;
    bool HandleModalRejected(AMAHUD* HUD, UMAUIManager* UIManager, EMAModalType ModalType) const;

private:
};
