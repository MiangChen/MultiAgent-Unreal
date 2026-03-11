#include "Core/Squad/Infrastructure/MASquadRuntimeBridge.h"

#include "Core/Squad/Application/MASquadUseCases.h"
#include "Core/Squad/Bootstrap/MASquadBootstrap.h"
#include "Core/Squad/Domain/MASquad.h"
#include "Core/Squad/Runtime/MASquadManager.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"

FMASquadOperationFeedback FMASquadRuntimeBridge::CycleFormation(const UObject* WorldContext)
{
    UMASquadManager* SquadManager = FMASquadBootstrap::Resolve(WorldContext);
    if (!SquadManager)
    {
        return FMASquadUseCases::BuildRuntimeUnavailable(EMASquadOperationKind::CycleFormation);
    }

    UMASquad* Squad = SquadManager->GetOrCreateDefaultSquad();
    if (!Squad)
    {
        return FMASquadUseCases::BuildMissingDefaultSquad();
    }

    SquadManager->CycleFormation(Squad);
    return FMASquadUseCases::BuildCycleResult(*Squad);
}

FMASquadOperationFeedback FMASquadRuntimeBridge::CreateSquadFromSelection(
    const UObject* WorldContext,
    const TArray<AMACharacter*>& SelectedAgents)
{
    UMASquadManager* SquadManager = FMASquadBootstrap::Resolve(WorldContext);
    if (!SquadManager)
    {
        return FMASquadUseCases::BuildRuntimeUnavailable(EMASquadOperationKind::CreateSquad);
    }

    if (SelectedAgents.Num() < 2)
    {
        return FMASquadUseCases::BuildSelectionTooSmall(SelectedAgents.Num());
    }

    if (UMASquad* Squad = SquadManager->CreateSquad(SelectedAgents, SelectedAgents[0]))
    {
        return FMASquadUseCases::BuildCreated(*Squad);
    }

    return FMASquadUseCases::BuildCreateFailed();
}

FMASquadOperationFeedback FMASquadRuntimeBridge::DisbandSelectedSquads(
    const UObject* WorldContext,
    const TArray<AMACharacter*>& SelectedAgents)
{
    UMASquadManager* SquadManager = FMASquadBootstrap::Resolve(WorldContext);
    if (!SquadManager)
    {
        return FMASquadUseCases::BuildRuntimeUnavailable(EMASquadOperationKind::DisbandSquad);
    }

    TSet<UMASquad*> SquadsToDisband;
    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent && Agent->CurrentSquad)
        {
            SquadsToDisband.Add(Agent->CurrentSquad);
        }
    }

    int32 DisbandedCount = 0;
    for (UMASquad* Squad : SquadsToDisband)
    {
        if (SquadManager->DisbandSquad(Squad))
        {
            ++DisbandedCount;
        }
    }

    if (DisbandedCount <= 0)
    {
        return FMASquadUseCases::BuildDisbandEmpty();
    }

    return FMASquadUseCases::BuildDisbanded(DisbandedCount);
}
