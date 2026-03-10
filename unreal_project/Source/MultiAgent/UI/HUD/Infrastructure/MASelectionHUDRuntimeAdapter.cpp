// L4 runtime bridge for selection HUD queries.

#include "MASelectionHUDRuntimeAdapter.h"

#include "../MAHUD.h"
#include "../MASelectionHUD.h"
#include "../Core/MAUIManager.h"
#include "../../../Agent/Character/MACharacter.h"
#include "../../../Core/AgentRuntime/Runtime/MAAgentManager.h"
#include "../../../Core/Selection/Runtime/MASelectionManager.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"

UWorld* FMASelectionHUDRuntimeAdapter::ResolveWorld(const AMASelectionHUD* HUD) const
{
    return HUD ? HUD->GetWorld() : nullptr;
}

APlayerController* FMASelectionHUDRuntimeAdapter::ResolvePlayerController(const AMASelectionHUD* HUD) const
{
    return HUD ? HUD->GetOwningPlayerController() : nullptr;
}

AMAHUD* FMASelectionHUDRuntimeAdapter::ResolveMAHUD(const AMASelectionHUD* HUD) const
{
    return HUD ? Cast<AMAHUD>(HUD->GetOwningPlayerController() ? HUD->GetOwningPlayerController()->GetHUD() : nullptr) : nullptr;
}

UMAAgentManager* FMASelectionHUDRuntimeAdapter::ResolveAgentManager(const AMASelectionHUD* HUD) const
{
    if (UWorld* World = ResolveWorld(HUD))
    {
        return World->GetSubsystem<UMAAgentManager>();
    }

    return nullptr;
}

UMASelectionManager* FMASelectionHUDRuntimeAdapter::ResolveSelectionManager(const AMASelectionHUD* HUD) const
{
    if (UWorld* World = ResolveWorld(HUD))
    {
        return World->GetSubsystem<UMASelectionManager>();
    }

    return nullptr;
}

UMAUIManager* FMASelectionHUDRuntimeAdapter::ResolveUIManager(const AMASelectionHUD* HUD) const
{
    if (AMAHUD* MAHUD = ResolveMAHUD(HUD))
    {
        return MAHUD->GetUIManager();
    }

    return nullptr;
}

bool FMASelectionHUDRuntimeAdapter::IsAnyFullscreenWidgetVisible(const AMASelectionHUD* HUD) const
{
    if (const UMAUIManager* UIManager = ResolveUIManager(HUD))
    {
        return UIManager->IsAnyFullscreenWidgetVisible();
    }

    return false;
}

bool FMASelectionHUDRuntimeAdapter::HasBlockingVisibleWidget(const AMASelectionHUD* HUD) const
{
    UWorld* World = ResolveWorld(HUD);
    if (!World)
    {
        return false;
    }

    TArray<UUserWidget*> FoundWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, UUserWidget::StaticClass(), false);
    for (UUserWidget* Widget : FoundWidgets)
    {
        if (!Widget || !Widget->IsVisible() || Widget->GetVisibility() != ESlateVisibility::Visible)
        {
            continue;
        }

        const FString WidgetName = Widget->GetClass()->GetName();
        if (!WidgetName.Contains(TEXT("HUDWidget")))
        {
            return true;
        }
    }

    return false;
}

void FMASelectionHUDRuntimeAdapter::BuildCircleEntries(const AMASelectionHUD* HUD, TArray<FMASelectionHUDCircleEntry>& OutEntries) const
{
    OutEntries.Reset();

    UMAAgentManager* AgentManager = ResolveAgentManager(HUD);
    if (!AgentManager)
    {
        return;
    }

    UMASelectionManager* SelectionManager = ResolveSelectionManager(HUD);
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (!Agent)
        {
            continue;
        }

        FMASelectionHUDCircleEntry& Entry = OutEntries.AddDefaulted_GetRef();
        Entry.WorldLocation = Agent->GetActorLocation();
        Entry.bSelected = SelectionManager && SelectionManager->IsSelected(Agent);
    }
}

void FMASelectionHUDRuntimeAdapter::BuildControlGroupEntries(const AMASelectionHUD* HUD, TArray<FMASelectionHUDControlGroupEntry>& OutEntries) const
{
    OutEntries.Reset();

    UMASelectionManager* SelectionManager = ResolveSelectionManager(HUD);
    if (!SelectionManager)
    {
        return;
    }

    for (int32 GroupIndex = 1; GroupIndex <= 5; ++GroupIndex)
    {
        const int32 UnitCount = SelectionManager->GetControlGroup(GroupIndex).Num();
        if (UnitCount <= 0)
        {
            continue;
        }

        FMASelectionHUDControlGroupEntry& Entry = OutEntries.AddDefaulted_GetRef();
        Entry.GroupIndex = GroupIndex;
        Entry.UnitCount = UnitCount;
    }
}

void FMASelectionHUDRuntimeAdapter::BuildStatusTextEntries(const AMASelectionHUD* HUD, TArray<FMASelectionHUDStatusTextEntry>& OutEntries) const
{
    OutEntries.Reset();

    APlayerController* PlayerController = ResolvePlayerController(HUD);
    UMAAgentManager* AgentManager = ResolveAgentManager(HUD);
    if (!PlayerController || !AgentManager || !HUD)
    {
        return;
    }

    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (!Agent)
        {
            continue;
        }

        const FString StatusText = Agent->GetCurrentStatusText();
        if (StatusText.IsEmpty())
        {
            continue;
        }

        const FVector WorldLocation = Agent->GetActorLocation() + FVector(0.0f, 0.0f, 180.0f);
        FVector2D ScreenPosition;
        if (!PlayerController->ProjectWorldLocationToScreen(WorldLocation, ScreenPosition, true))
        {
            continue;
        }

        if (ScreenPosition.X < 0.0f || ScreenPosition.X > ViewportWidth ||
            ScreenPosition.Y < 0.0f || ScreenPosition.Y > ViewportHeight)
        {
            continue;
        }

        FMASelectionHUDStatusTextEntry& Entry = OutEntries.AddDefaulted_GetRef();
        Entry.ScreenPosition = ScreenPosition;
        Entry.Text = StatusText;
    }
}
