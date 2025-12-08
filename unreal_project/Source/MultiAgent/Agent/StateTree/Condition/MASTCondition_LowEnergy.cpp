// MASTCondition_LowEnergy.cpp

#include "MASTCondition_LowEnergy.h"
#include "MARobotDogCharacter.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_LowEnergy::TestCondition(FStateTreeExecutionContext& Context) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        return false;
    }

    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        return false;
    }

    float EnergyPercent = Robot->GetEnergyPercent();
    bool bIsLowEnergy = EnergyPercent < EnergyThreshold;
    
    if (bIsLowEnergy)
    {
        UE_LOG(LogTemp, Log, TEXT("[STCondition] %s low energy: %.1f%% < %.1f%%"), 
            *Robot->AgentName, EnergyPercent, EnergyThreshold);
    }
    
    return bIsLowEnergy;
}
