// MASTCondition_FullEnergy.cpp

#include "MASTCondition_FullEnergy.h"
#include "../../Character/MACharacter.h"
#include "../../Component/Capability/MACapabilityComponents.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_FullEnergy::TestCondition(FStateTreeExecutionContext& Context) const
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
    bool bIsFull = EnergyPercent >= FullThreshold;
    
    if (bIsFull)
    {
        AMACharacter* Character = Cast<AMACharacter>(Owner);
        FString OwnerName = Character ? Character->AgentName : Owner->GetName();
        UE_LOG(LogTemp, Log, TEXT("[STCondition] %s full energy: %.1f%% >= %.1f%%"), 
            *OwnerName, EnergyPercent, FullThreshold);
    }
    
    return bIsFull;
}
