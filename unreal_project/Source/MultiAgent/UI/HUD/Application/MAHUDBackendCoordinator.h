// MAHUDBackendCoordinator.h
// HUD backend interaction coordination.

#pragma once

#include "CoreMinimal.h"
#include "../../Core/MAHUDTypes.h"
#include "../../../Core/Types/MATaskGraphTypes.h"
#include "../../../Core/Comm/MACommTypes.h"

class AMAHUD;
class UWorld;
class UMAUIManager;
struct FMAPlannerResponse;

class MULTIAGENT_API FMAHUDBackendCoordinator
{
public:
    void BindBackendEvents(UWorld* World, UMAUIManager* UIManager, AMAHUD* HUD) const;
    void BindModalDelegates(UMAUIManager* UIManager, AMAHUD* HUD) const;

    void HandleSimpleCommandSubmitted(UWorld* World, UMAUIManager* UIManager, AMAHUD* HUD, const FString& Command) const;
    void HandlePlannerResponse(UMAUIManager* UIManager, const FMAPlannerResponse& Response) const;

    void HandleTaskGraphReceived(UMAUIManager* UIManager, const FMATaskPlan& TaskPlan) const;
    void HandleSkillAllocationReceived(UMAUIManager* UIManager, const FMASkillAllocationData& AllocationData) const;
    void HandleSkillListReceived(UMAUIManager* UIManager, const FMASkillListMessage& SkillList, bool bExecutable) const;

    void HandleModalConfirmed(UWorld* World, UMAUIManager* UIManager, EMAModalType ModalType) const;
    bool HandleModalRejected(UWorld* World, UMAUIManager* UIManager, EMAModalType ModalType) const;

private:
    class UMACommSubsystem* GetCommSubsystem(UWorld* World) const;
};
