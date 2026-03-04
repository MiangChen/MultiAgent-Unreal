// MAUIWidgetRegistryCoordinator.cpp
// UIManager Widget 注册与层级策略协调器实现（L1 Application）

#include "MAUIWidgetRegistryCoordinator.h"
#include "../MAUIManager.h"
#include "Blueprint/UserWidget.h"

int32 FMAUIWidgetRegistryCoordinator::GetWidgetZOrder(EMAWidgetType Type) const
{
    switch (Type)
    {
    case EMAWidgetType::HUD:
        return -1;
    case EMAWidgetType::SemanticMap:
        return 5;
    case EMAWidgetType::Modify:
        return 10;
    case EMAWidgetType::Edit:
        return 10;
    case EMAWidgetType::SceneList:
        return 10;
    case EMAWidgetType::TaskPlanner:
        return 11;
    case EMAWidgetType::SkillAllocation:
        return 11;
    case EMAWidgetType::DirectControl:
        return 15;
    case EMAWidgetType::SystemLogPanel:
        return 5;
    case EMAWidgetType::PreviewPanel:
        return 5;
    case EMAWidgetType::InstructionPanel:
        return 5;
    default:
        return 10;
    }
}

void FMAUIWidgetRegistryCoordinator::RegisterViewportWidget(
    UMAUIManager* UIManager,
    EMAWidgetType Type,
    UUserWidget* Widget,
    ESlateVisibility Visibility) const
{
    if (!UIManager || !Widget)
    {
        return;
    }

    Widget->AddToViewport(GetWidgetZOrder(Type));
    RegisterManagedWidget(UIManager, Type, Widget, Visibility);
}

void FMAUIWidgetRegistryCoordinator::RegisterManagedWidget(
    UMAUIManager* UIManager,
    EMAWidgetType Type,
    UUserWidget* Widget,
    ESlateVisibility Visibility) const
{
    if (!UIManager || !Widget)
    {
        return;
    }

    Widget->SetVisibility(Visibility);
    UIManager->RegisterWidgetMappingInternal(Type, Widget);
}
