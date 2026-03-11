#include "MAMiniMapRuntimeAdapter.h"

#include "../Presentation/MAMiniMapWidget.h"
#include "../../HUD/Runtime/MAHUD.h"
#include "../../HUD/Presentation/MAMainHUDWidget.h"
#include "../../../Core/AgentRuntime/Runtime/MAAgentManager.h"
#include "../../../Core/Selection/Runtime/MASelectionManager.h"
#include "../../../Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
FLinearColor ResolveAgentColor(const AMACharacter* Agent, const bool bIsSelected)
{
    if (bIsSelected)
    {
        return FLinearColor(0.95f, 0.8f, 0.2f, 1.0f);
    }

    if (!Agent)
    {
        return FLinearColor::White;
    }

    switch (Agent->AgentType)
    {
    case EMAAgentType::Humanoid:
        return FLinearColor(0.35f, 0.8f, 0.55f, 1.0f);
    case EMAAgentType::UAV:
    case EMAAgentType::FixedWingUAV:
        return FLinearColor(0.2f, 0.65f, 0.95f, 1.0f);
    case EMAAgentType::UGV:
        return FLinearColor(0.85f, 0.35f, 0.3f, 1.0f);
    case EMAAgentType::Quadruped:
        return FLinearColor(0.75f, 0.55f, 0.95f, 1.0f);
    default:
        return FLinearColor::White;
    }
}

APlayerController* ResolvePlayerController(const UUserWidget* Context)
{
    if (!Context)
    {
        return nullptr;
    }

    if (APlayerController* OwningPlayer = Context->GetOwningPlayer())
    {
        return OwningPlayer;
    }

    if (const UWorld* World = Context->GetWorld())
    {
        return World->GetFirstPlayerController();
    }

    return nullptr;
}
}

FMAMiniMapRuntimeSnapshot FMAMiniMapRuntimeAdapter::Capture(const UUserWidget* Context) const
{
    FMAMiniMapRuntimeSnapshot Snapshot;
    if (!Context)
    {
        return Snapshot;
    }

    UWorld* World = Context->GetWorld();
    if (!World)
    {
        return Snapshot;
    }

    const UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
    const UMASelectionManager* SelectionManager = World->GetSubsystem<UMASelectionManager>();
    if (AgentManager)
    {
        TSet<const AMACharacter*> SelectedAgents;
        if (SelectionManager)
        {
            const TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();
            for (AMACharacter* Agent : Selected)
            {
                SelectedAgents.Add(Agent);
            }
        }

        for (AMACharacter* Agent : AgentManager->GetAllAgents())
        {
            if (!Agent)
            {
                continue;
            }

            FMAMiniMapAgentSample Sample;
            Sample.WorldLocation = Agent->GetActorLocation();
            Sample.Color = ResolveAgentColor(Agent, SelectedAgents.Contains(Agent));
            Snapshot.Agents.Add(Sample);
        }
    }

    if (APlayerController* PC = ResolvePlayerController(Context))
    {
        if (const APawn* Pawn = PC->GetPawn())
        {
            Snapshot.Camera.bValid = true;
            Snapshot.Camera.WorldLocation = Pawn->GetActorLocation();
            Snapshot.Camera.Rotation = Pawn->GetActorRotation();
        }
    }

    return Snapshot;
}

bool FMAMiniMapRuntimeAdapter::MoveViewToWorldLocation(const UUserWidget* Context, const FVector& WorldLocation) const
{
    if (APlayerController* PC = ResolvePlayerController(Context))
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            FVector NewLocation = Pawn->GetActorLocation();
            NewLocation.X = WorldLocation.X;
            NewLocation.Y = WorldLocation.Y;
            Pawn->SetActorLocation(NewLocation);
            return true;
        }
    }

    return false;
}

UMAMiniMapWidget* FMAMiniMapManagerHUDBridge::ResolveMiniMapWidget(const AActor* Context) const
{
    if (!Context)
    {
        return nullptr;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(Context->GetWorld(), 0);
    if (!PC)
    {
        return nullptr;
    }

    AMAHUD* MAHUD = Cast<AMAHUD>(PC->GetHUD());
    if (!MAHUD)
    {
        return nullptr;
    }

    UMAMainHUDWidget* MainHUDWidget = MAHUD->GetMainHUDWidget();
    return MainHUDWidget ? MainHUDWidget->GetMiniMap() : nullptr;
}
