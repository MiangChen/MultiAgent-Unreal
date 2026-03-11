// MANavigationService.FlightCommand.cpp
// 起飞/降落/返航命令入口

#include "MANavigationService.h"
#include "../Infrastructure/MAFlightController.h"
#include "GameFramework/Character.h"

bool UMANavigationService::TakeOff(float TargetAltitude)
{
    if (!EnsureCanRunFlightCommand(TEXT("TakeOff")))
    {
        return false;
    }

    TargetLocation = OwnerCharacter->GetActorLocation();
    TargetLocation.Z = TargetAltitude;
    bIsReturnHomeActive = false;

    SetNavigationState(EMANavigationState::TakingOff);

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: TakeOff to altitude %.0f"),
        *OwnerCharacter->GetName(), TargetAltitude);

    if (!FlightController->TakeOff(TargetAltitude))
    {
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, TEXT("TakeOff failed: FlightController rejected request"));
        return false;
    }

    StartFlightTimer(&UMANavigationService::UpdateFlightOperation);
    return true;
}

bool UMANavigationService::Land(float TargetAltitude)
{
    if (!EnsureCanRunFlightCommand(TEXT("Land")))
    {
        return false;
    }

    TargetLocation = OwnerCharacter->GetActorLocation();
    TargetLocation.Z = TargetAltitude;
    bIsReturnHomeActive = false;

    SetNavigationState(EMANavigationState::Landing);

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Land to altitude %.0f"),
        *OwnerCharacter->GetName(), TargetAltitude);

    if (!FlightController->Land())
    {
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, TEXT("Land failed: FlightController rejected request"));
        return false;
    }

    StartFlightTimer(&UMANavigationService::UpdateFlightOperation);
    return true;
}

bool UMANavigationService::ReturnHome(FVector HomePosition, float LandAltitude)
{
    if (!EnsureCanRunFlightCommand(TEXT("ReturnHome")))
    {
        return false;
    }

    StartLocation = OwnerCharacter->GetActorLocation();
    TargetLocation = HomePosition;
    TargetLocation.Z = StartLocation.Z;
    ReturnHomeLandAltitude = LandAltitude;
    bReturnHomeIsLanding = false;
    bIsReturnHomeActive = true;
    AcceptanceRadius = 100.f;

    SetNavigationState(EMANavigationState::Navigating);

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: ReturnHome to (%.0f, %.0f), then land at %.0f"),
        *OwnerCharacter->GetName(), HomePosition.X, HomePosition.Y, LandAltitude);

    FlightController->SetAcceptanceRadius(AcceptanceRadius);
    FlightController->FlyTo(TargetLocation);

    StartFlightTimer(&UMANavigationService::UpdateReturnHome);
    return true;
}
