#include "Core/SkillAllocation/Bootstrap/MASkillAllocationBootstrap.h"

FMASkillAllocationData FMASkillAllocationBootstrap::BuildEmptyData(
    const FString& Name,
    const FString& Description)
{
    FMASkillAllocationData Data;
    Data.Name = Name;
    Data.Description = Description;
    return Data;
}
