// MAHUDWidgetCoordinator.cpp
// HUD widget coordination.

#include "MAHUDWidgetCoordinator.h"
#include "../../Core/MAUIManager.h"
#include "../../TaskGraph/MATaskPlannerWidget.h"
#include "../../Components/MADirectControlIndicator.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDWidgetCoordinator, Log, All);

void FMAHUDWidgetCoordinator::LoadTaskGraph(UMAUIManager* UIManager, const FMATaskGraphData& Data) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDWidgetCoordinator, Warning, TEXT("LoadTaskGraph: UIManager is null"));
        return;
    }

    UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget();
    if (!TaskPlannerWidget)
    {
        UE_LOG(LogMAHUDWidgetCoordinator, Warning, TEXT("LoadTaskGraph: TaskPlannerWidget is null"));
        return;
    }

    TaskPlannerWidget->LoadTaskGraph(Data);
}

void FMAHUDWidgetCoordinator::ShowDirectControlIndicator(UMAUIManager* UIManager, const FString& AgentLabel) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDWidgetCoordinator, Warning, TEXT("ShowDirectControlIndicator: UIManager is null"));
        return;
    }

    if (AgentLabel.IsEmpty())
    {
        UE_LOG(LogMAHUDWidgetCoordinator, Warning, TEXT("ShowDirectControlIndicator: Agent label is empty"));
        return;
    }

    UMADirectControlIndicator* DirectControlIndicator = UIManager->GetDirectControlIndicator();
    if (!DirectControlIndicator)
    {
        UE_LOG(LogMAHUDWidgetCoordinator, Warning, TEXT("ShowDirectControlIndicator: DirectControlIndicator is null"));
        return;
    }

    DirectControlIndicator->SetAgentLabel(AgentLabel);
    UIManager->ShowWidget(EMAWidgetType::DirectControl, false);
}

void FMAHUDWidgetCoordinator::HideDirectControlIndicator(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDWidgetCoordinator, Warning, TEXT("HideDirectControlIndicator: UIManager is null"));
        return;
    }

    UIManager->HideWidget(EMAWidgetType::DirectControl);
}

bool FMAHUDWidgetCoordinator::IsDirectControlIndicatorVisible(const UMAUIManager* UIManager) const
{
    return UIManager && UIManager->IsWidgetVisible(EMAWidgetType::DirectControl);
}
