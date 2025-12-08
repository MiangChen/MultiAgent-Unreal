// MASTCondition_FullEnergy.cpp

#include "MASTCondition_FullEnergy.h"
#include "MARobotDogCharacter.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_FullEnergy::TestCondition(FStateTreeExecutionContext& Context) const
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
    bool bIsFull = EnergyPercent >= FullThreshold;
    
    if (bIsFull)
    {
        UE_LOG(LogTemp, Log, TEXT("[STCondition] %s full energy: %.1f%% >= %.1f%%"), 
            *Robot->AgentName, EnergyPercent, FullThreshold);
    }
    
    return bIsFull;
}
