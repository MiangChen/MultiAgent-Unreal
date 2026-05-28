#include "Agent/CharacterRuntime/Bootstrap/MACharacterRuntimeBootstrap.h"

#include "Agent/CharacterRuntime/Infrastructure/MACharacterRuntimeBridge.h"
#include "Agent/CharacterRuntime/Runtime/MAUAVCharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Config/Domain/MAConfigNavigationTypes.h"

void FMACharacterRuntimeBootstrap::ApplyUAVFlightConfig(AMAUAVCharacter& UAVCharacter)
{
    FMAFlightConfig FlightConfig;
    if (!FMACharacterRuntimeBridge::ResolveFlightConfig(&UAVCharacter, FlightConfig))
    {
        return;
    }

    UAVCharacter.MinFlightAltitude = FlightConfig.MinAltitude;
    UAVCharacter.DefaultFlightAltitude = FlightConfig.DefaultAltitude;
    UAVCharacter.MaxFlightSpeed = FlightConfig.MaxSpeed;
    UAVCharacter.ObstacleDetectionRange = FlightConfig.ObstacleDetectionRange;
    UAVCharacter.ObstacleAvoidanceRadius = FlightConfig.ObstacleAvoidanceRadius;
    UAVCharacter.AcceptanceRadius = FlightConfig.AcceptanceRadius;

    if (UMANavigationService* NavigationService = UAVCharacter.GetNavigationService())
    {
        NavigationService->bIsFlying = true;
        NavigationService->bIsFixedWing = false;
        NavigationService->MinFlightAltitude = FlightConfig.MinAltitude;
    }
}
