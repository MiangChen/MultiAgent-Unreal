// MAHUDWidgetCoordinator.cpp
// HUD widget coordination.

#include "MAHUDWidgetCoordinator.h"
#include "../../Core/MAUIManager.h"
#include "../../TaskGraph/Presentation/MATaskPlannerWidget.h"
#include "../../Components/Presentation/MADirectControlIndicator.h"

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

FMAHUDWidgetEditModeViewModel FMAHUDWidgetCoordinator::BuildEditModeViewModel(
    bool bVisible,
    const TArray<FString>& POIInfos,
    const TArray<FString>& GoalInfos,
    const TArray<FString>& ZoneInfos) const
{
    FMAHUDWidgetEditModeViewModel Model;
    Model.bVisible = bVisible;
    Model.POIText = BuildPrefixedListText(TEXT("POI:"), POIInfos);
    Model.GoalText = BuildPrefixedListText(TEXT("Goal:"), GoalInfos);
    Model.ZoneText = BuildPrefixedListText(TEXT("Zone:"), ZoneInfos);
    return Model;
}

FMAHUDWidgetEditModeViewModel FMAHUDWidgetCoordinator::BuildEditModeViewModel(const FMAHUDEditModeFeedback& Feedback) const
{
    return BuildEditModeViewModel(Feedback.bVisible, Feedback.POIInfos, Feedback.GoalInfos, Feedback.ZoneInfos);
}

FMAHUDWidgetNotificationModel FMAHUDWidgetCoordinator::BuildNotificationModel(
    const FString& Message,
    bool bIsError,
    bool bIsWarning) const
{
    FMAHUDWidgetNotificationModel Model;
    Model.Message = Message;
    Model.Severity = bIsError
        ? EMAHUDWidgetNotificationSeverity::Error
        : (bIsWarning ? EMAHUDWidgetNotificationSeverity::Warning : EMAHUDWidgetNotificationSeverity::Info);
    return Model;
}

FMAHUDWidgetNotificationModel FMAHUDWidgetCoordinator::BuildNotificationModel(const FMAHUDNotificationFeedback& Feedback) const
{
    return BuildNotificationModel(Feedback.Message, Feedback.bIsError, Feedback.bIsWarning);
}

FString FMAHUDWidgetCoordinator::BuildPrefixedListText(const TCHAR* Prefix, const TArray<FString>& Infos) const
{
    if (Infos.Num() == 0)
    {
        return FString();
    }

    FString Text = Prefix;
    Text += TEXT(" ");
    for (const FString& Info : Infos)
    {
        Text += Info;
        Text += TEXT(" ");
    }

    return Text;
}
