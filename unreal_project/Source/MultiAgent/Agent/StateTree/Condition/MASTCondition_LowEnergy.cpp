// MASTCondition_LowEnergy.cpp

#include "MASTCondition_LowEnergy.h"
#include "../../Character/MACharacter.h"
#include "../../Component/Capability/MACapabilityComponents.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_LowEnergy::TestCondition(FStateTreeExecutionContext& Context) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        return false;
    }

    // 使用 Component 获取能量信息
    UMAEnergyComponent* EnergyComp = Owner->FindComponentByClass<UMAEnergyComponent>();
    if (!EnergyComp)
    {
        return false;
    }

    float EnergyPercent = EnergyComp->GetEnergyPercent();
    bool bIsLowEnergy = EnergyPercent < EnergyThreshold;
    
    if (bIsLowEnergy)
    {
        AMACharacter* Character = Cast<AMACharacter>(Owner);
        FString OwnerName = Character ? Character->AgentName : Owner->GetName();
        UE_LOG(LogTemp, Log, TEXT("[STCondition] %s low energy: %.1f%% < %.1f%%"), 
            *OwnerName, EnergyPercent, EnergyThreshold);
    }
    
    return bIsLowEnergy;
}
