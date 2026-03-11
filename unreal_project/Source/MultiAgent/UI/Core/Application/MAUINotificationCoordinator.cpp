// MAUINotificationCoordinator.cpp
// UIManager 通知协调器实现（L1 Application）

#include "MAUINotificationCoordinator.h"
#include "../MAUIManager.h"
#include "../MAHUDStateManager.h"
#include "../../HUD/Presentation/MAMainHUDWidget.h"
#include "../../Components/Presentation/MANotificationWidget.h"

void FMAUINotificationCoordinator::ShowNotification(UMAUIManager* UIManager, EMANotificationType Type) const
{
    if (!UIManager)
    {
        return;
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->ShowNotification(Type);
    }
}

void FMAUINotificationCoordinator::DismissNotification(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->DismissNotification();
    }

    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget())
    {
        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
        if (NotificationWidget)
        {
            NotificationWidget->HideNotification();
        }
    }
}

void FMAUINotificationCoordinator::DismissRequestUserCommandNotification(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
    if (!MainHUDWidget)
    {
        return;
    }

    UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
    if (!NotificationWidget)
    {
        return;
    }

    if (NotificationWidget->GetCurrentNotificationType() == EMANotificationType::RequestUserCommand)
    {
        NotificationWidget->HideNotification();
        UE_LOG(LogMAUIManager, Log, TEXT("DismissRequestUserCommandNotification: RequestUserCommand notification dismissed"));
    }
}

EMAModalType FMAUINotificationCoordinator::GetModalTypeForNotification(EMANotificationType NotificationType) const
{
    switch (NotificationType)
    {
    case EMANotificationType::TaskGraphUpdate:
        return EMAModalType::TaskGraph;

    case EMANotificationType::SkillListUpdate:
        return EMAModalType::SkillAllocation;

    case EMANotificationType::DecisionUpdate:
        return EMAModalType::Decision;

    default:
        return EMAModalType::None;
    }
}
