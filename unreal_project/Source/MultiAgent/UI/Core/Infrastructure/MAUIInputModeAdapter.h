// MAUIInputModeAdapter.h
// UI 输入模式适配器（L3 Infrastructure）

#pragma once

#include "CoreMinimal.h"

class UMAUIManager;
class UUserWidget;

class MULTIAGENT_API FMAUIInputModeAdapter
{
public:
    void SetInputModeGameAndUI(UMAUIManager* UIManager, UUserWidget* Widget) const;
    void SetInputModeGameOnly(UMAUIManager* UIManager) const;
};
