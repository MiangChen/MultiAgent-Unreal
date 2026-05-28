// MAUINotificationCoordinator.h
// UIManager 通知协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"
#include "../MAHUDTypes.h"

class UMAUIManager;

class MULTIAGENT_API FMAUINotificationCoordinator
{
public:
    void ShowNotification(UMAUIManager* UIManager, EMANotificationType Type) const;
    void DismissNotification(UMAUIManager* UIManager) const;
    void DismissRequestUserCommandNotification(UMAUIManager* UIManager) const;
    EMAModalType GetModalTypeForNotification(EMANotificationType NotificationType) const;
};
