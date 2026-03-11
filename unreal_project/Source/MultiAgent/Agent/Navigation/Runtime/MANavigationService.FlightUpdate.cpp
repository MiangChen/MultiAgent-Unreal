// MANavigationService.FlightUpdate.cpp
// 飞行导航与返航更新

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "../Infrastructure/MAFlightController.h"
#include "GameFramework/Character.h"

void UMANavigationService::UpdateFlightNavigation(float DeltaTime)
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character lost during flight"));
        return;
    }

    if (FlightController.IsValid())
    {
        FlightController->UpdateFlight(DeltaTime);

        if (FlightController->HasArrived())
        {
            CompleteNavigateSuccess();
        }
        return;
    }

    const FVector CurrentLoc = OwnerCharacter->GetActorLocation();
    const FVector ToTarget = TargetLocation - CurrentLoc;
    const float Distance3D = ToTarget.Size();

    if (Distance3D < AcceptanceRadius)
    {
        CompleteNavigateSuccess();
        return;
    }

    const FVector Direction = ToTarget.GetSafeNormal();
    OwnerCharacter->AddMovementInput(Direction, 1.0f);

    if (!Direction.IsNearlyZero())
    {
        const FRotator TargetRotation = Direction.Rotation();
        const FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);
        OwnerCharacter->SetActorRotation(NewRotation);
    }
}

void UMANavigationService::UpdateFlightOperation(float DeltaTime)
{
    if (FlightController.IsValid())
    {
        FlightController->UpdateFlight(DeltaTime);
    }

    const bool bReachedTerminalState = FlightController.IsValid() &&
        (FlightController->GetState() == EMAFlightControlState::Idle || FlightController->GetState() == EMAFlightControlState::Arrived);
    const FMANavigationFlightOperationFeedback Feedback =
        FMANavigationUseCases::BuildFlightOperationUpdate(
            CurrentState,
            OwnerCharacter != nullptr,
            FlightController.IsValid(),
            bReachedTerminalState,
            OwnerCharacter ? OwnerCharacter->GetActorLocation().Z / 100.f : 0.f);

    if (Feedback.Action == EMANavigationFlightOperationAction::CompleteFailure)
    {
        CompleteNavigation(false, Feedback.CompletionMessage);
        return;
    }

    if (Feedback.Action == EMANavigationFlightOperationAction::CompleteSuccess)
    {
        SetNavigationState(Feedback.NextState);
        CompleteNavigation(true, Feedback.CompletionMessage);
    }
}

void UMANavigationService::UpdateReturnHome(float DeltaTime)
{
    if (FlightController.IsValid())
    {
        FlightController->UpdateFlight(DeltaTime);
    }

    const bool bHasArrivedAtHome = FlightController.IsValid() && !bReturnHomeIsLanding && FlightController->HasArrived();
    const bool bHasFinishedLanding = FlightController.IsValid() && bReturnHomeIsLanding && FlightController->GetState() == EMAFlightControlState::Idle;
    const FMANavigationReturnHomeFeedback Feedback =
        FMANavigationUseCases::BuildReturnHomeUpdate(
            OwnerCharacter != nullptr,
            FlightController.IsValid(),
            bReturnHomeIsLanding,
            bHasArrivedAtHome,
            bHasFinishedLanding,
            OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector);

    if (Feedback.Action == EMANavigationReturnHomeAction::CompleteFailure)
    {
        CompleteNavigation(false, Feedback.CompletionMessage);
        return;
    }

    if (Feedback.Action == EMANavigationReturnHomeAction::StartLanding)
    {
        bReturnHomeIsLanding = true;
        SetNavigationState(Feedback.NextState);
        if (FlightController.IsValid())
        {
            FlightController->Land();
        }
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: %s"),
            OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
            *Feedback.LogMessage);
        return;
    }

    if (Feedback.Action == EMANavigationReturnHomeAction::CompleteSuccess)
    {
        SetNavigationState(Feedback.NextState);
        CompleteNavigation(true, Feedback.CompletionMessage);
    }
}
