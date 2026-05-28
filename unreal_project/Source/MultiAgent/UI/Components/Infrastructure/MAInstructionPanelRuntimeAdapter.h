#pragma once

#include "CoreMinimal.h"

class UUserWidget;

class FMAInstructionPanelRuntimeAdapter
{
public:
    void DismissRequestNotification(const UUserWidget* Context) const;
};
