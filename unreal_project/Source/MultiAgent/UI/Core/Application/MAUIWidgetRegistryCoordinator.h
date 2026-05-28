// MAUIWidgetRegistryCoordinator.h
// UIManager Widget 注册与层级策略协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"

class UMAUIManager;
class UUserWidget;
enum class EMAWidgetType : uint8;

class MULTIAGENT_API FMAUIWidgetRegistryCoordinator
{
public:
    int32 GetWidgetZOrder(EMAWidgetType Type) const;
    void RegisterViewportWidget(UMAUIManager* UIManager, EMAWidgetType Type, UUserWidget* Widget, ESlateVisibility Visibility) const;
    void RegisterManagedWidget(UMAUIManager* UIManager, EMAWidgetType Type, UUserWidget* Widget, ESlateVisibility Visibility) const;
};
