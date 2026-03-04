// MAHUDLifecycleCoordinator.h
// HUD 生命周期协调器
// 封装 BeginPlay 中的 UI 初始化与事件绑定流程

#pragma once

#include "CoreMinimal.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDLifecycleCoordinator
{
public:
    void InitializeUI(AMAHUD* HUD) const;
    void BindRuntimeDelegates(AMAHUD* HUD) const;
};
