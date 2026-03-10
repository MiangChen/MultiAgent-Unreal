#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASelectionHUDModel.h"

class AMASelectionHUD;

class MULTIAGENT_API FMASelectionHUDRuntimeAdapter
{
public:
    bool IsAnyFullscreenWidgetVisible(const AMASelectionHUD* HUD) const;
    bool HasBlockingVisibleWidget(const AMASelectionHUD* HUD) const;
    void BuildCircleEntries(const AMASelectionHUD* HUD, TArray<FMASelectionHUDCircleEntry>& OutEntries) const;
    void BuildControlGroupEntries(const AMASelectionHUD* HUD, TArray<FMASelectionHUDControlGroupEntry>& OutEntries) const;
    void BuildStatusTextEntries(const AMASelectionHUD* HUD, TArray<FMASelectionHUDStatusTextEntry>& OutEntries) const;

private:
    class UWorld* ResolveWorld(const AMASelectionHUD* HUD) const;
    class APlayerController* ResolvePlayerController(const AMASelectionHUD* HUD) const;
    class AMAHUD* ResolveMAHUD(const AMASelectionHUD* HUD) const;
    class UMAAgentManager* ResolveAgentManager(const AMASelectionHUD* HUD) const;
    class UMASelectionManager* ResolveSelectionManager(const AMASelectionHUD* HUD) const;
    class UMAUIManager* ResolveUIManager(const AMASelectionHUD* HUD) const;
};
