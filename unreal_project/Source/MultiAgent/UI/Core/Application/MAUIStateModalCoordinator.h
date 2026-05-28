// MAUIStateModalCoordinator.h
// UIManager 状态机与模态流程协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"
#include "../MAHUDTypes.h"

class UMAUIManager;

class MULTIAGENT_API FMAUIStateModalCoordinator
{
public:
    void InitializeHUDStateManager(UMAUIManager* UIManager) const;
    void CreateModalWidgets(UMAUIManager* UIManager) const;
    void BindStateManagerDelegates(UMAUIManager* UIManager) const;

    void HandleHUDStateChanged(UMAUIManager* UIManager, EMAHUDState OldState, EMAHUDState NewState) const;
    void HandleNotificationReceived(UMAUIManager* UIManager, EMANotificationType Type) const;
    void HandleModalConfirmed(EMAModalType ModalType) const;
    void HandleModalRejected(EMAModalType ModalType) const;
    void HandleModalEditRequested(UMAUIManager* UIManager, EMAModalType ModalType) const;
    void HandleModalHideAnimationComplete(UMAUIManager* UIManager) const;

    void ShowModal(UMAUIManager* UIManager, EMAModalType ModalType, bool bEditMode) const;
    void HideCurrentModal(UMAUIManager* UIManager) const;
    void LoadDataIntoModal(UMAUIManager* UIManager, EMAModalType ModalType) const;
    int32 GetModalZOrder() const;
};
