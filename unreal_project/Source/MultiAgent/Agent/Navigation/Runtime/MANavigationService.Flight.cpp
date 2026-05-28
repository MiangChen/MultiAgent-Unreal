// MANavigationService.Flight.cpp
// 飞行导航公共辅助与飞行导航入口

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "../Infrastructure/MAFlightController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UMANavigationService::EnsureCanRunFlightCommand(const TCHAR* CommandName)
{
    if (!OwnerCharacter || !bIsFlying)
    {
        const FMANavigationCommandFeedback Feedback =
            FMANavigationUseCases::BuildFlightCommandPrecheck(OwnerCharacter, bIsFlying, false, CommandName);
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s"), *Feedback.Message);
        return false;
    }

    if (CurrentState != EMANavigationState::Idle && CurrentState != EMANavigationState::Arrived)
    {
        CleanupNavigation();
    }

    EnsureFlightControllerInitialized();
    const FMANavigationCommandFeedback Feedback =
        FMANavigationUseCases::BuildFlightCommandPrecheck(
            OwnerCharacter,
            bIsFlying,
            FlightController.IsValid(),
            CommandName);
    if (!Feedback.bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: %s"),
            *OwnerCharacter->GetName(), *Feedback.Message);
        return false;
    }

    return true;
}

void UMANavigationService::StartFlightTimer(void (UMANavigationService::*UpdateFunction)(float))
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    if (FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
    }

    World->GetTimerManager().SetTimer(
        FlightCheckTimerHandle,
        FTimerDelegate::CreateUObject(this, UpdateFunction, FlightCheckInterval),
        FlightCheckInterval,
        true);
}

FVector UMANavigationService::AdjustFlightAltitude(const FVector& Location) const
{
    FVector AdjustedLocation = Location;
    if (AdjustedLocation.Z < MinFlightAltitude)
    {
        AdjustedLocation.Z = MinFlightAltitude;
    }
    return AdjustedLocation;
}

bool UMANavigationService::StartFlightNavigation()
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character not found"));
        return false;
    }

    TargetLocation = AdjustFlightAltitude(TargetLocation);
    bIsReturnHomeActive = false;
    EnsureFlightControllerInitialized();

    if (FlightController.IsValid())
    {
        FlightController->SetAcceptanceRadius(AcceptanceRadius);
        FlightController->FlyTo(TargetLocation);
        StartFlightTimer(&UMANavigationService::UpdateFlightNavigation);
        return true;
    }

    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->SetMovementMode(MOVE_Flying);
    }

    StartFlightTimer(&UMANavigationService::UpdateFlightNavigation);
    return true;
}
