// MAHUDDelegateCoordinator.h
// HUD 委托绑定协调器
// 封装 Widget 与 Controller 的委托绑定流程

#pragma once

#include "CoreMinimal.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDDelegateCoordinator
{
public:
    void BindWidgetDelegates(AMAHUD* HUD) const;
    void BindControllerEvents(AMAHUD* HUD) const;
};
