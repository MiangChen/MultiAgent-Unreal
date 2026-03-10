#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

DEFINE_LOG_CATEGORY(LogMASkillAllocation);

bool FMASkillAssignment::operator==(const FMASkillAssignment& Other) const
{
    return SkillName == Other.SkillName &&
           ParamsJson == Other.ParamsJson &&
           Status == Other.Status;
}

bool FMATimeStepData::operator==(const FMATimeStepData& Other) const
{
    if (RobotSkills.Num() != Other.RobotSkills.Num())
    {
        return false;
    }

    for (const auto& Pair : RobotSkills)
    {
        const FString& RobotId = Pair.Key;
        const FMASkillAssignment* OtherSkill = Other.RobotSkills.Find(RobotId);

        if (!OtherSkill || !(Pair.Value == *OtherSkill))
        {
            return false;
        }
    }

    return true;
}

TArray<FString> FMASkillAllocationData::GetAllRobotIds() const
{
    TSet<FString> RobotIdSet;

    for (const auto& TimeStepPair : Data)
    {
        const FMATimeStepData& TimeStepData = TimeStepPair.Value;
        for (const auto& RobotPair : TimeStepData.RobotSkills)
        {
            RobotIdSet.Add(RobotPair.Key);
        }
    }

    return RobotIdSet.Array();
}

TArray<int32> FMASkillAllocationData::GetAllTimeSteps() const
{
    TArray<int32> TimeSteps;
    Data.GetKeys(TimeSteps);
    TimeSteps.Sort();
    return TimeSteps;
}

FMASkillAssignment* FMASkillAllocationData::FindSkill(int32 TimeStep, const FString& RobotId)
{
    FMATimeStepData* TimeStepData = Data.Find(TimeStep);
    if (TimeStepData)
    {
        return TimeStepData->RobotSkills.Find(RobotId);
    }
    return nullptr;
}

const FMASkillAssignment* FMASkillAllocationData::FindSkill(int32 TimeStep, const FString& RobotId) const
{
    const FMATimeStepData* TimeStepData = Data.Find(TimeStep);
    if (TimeStepData)
    {
        return TimeStepData->RobotSkills.Find(RobotId);
    }
    return nullptr;
}

bool FMASkillAllocationData::ValidateRobotIds(TArray<FString>& OutUndefinedRobots) const
{
    OutUndefinedRobots.Empty();
    return true;
}

bool FMASkillAllocationData::operator==(const FMASkillAllocationData& Other) const
{
    if (Name != Other.Name || Description != Other.Description)
    {
        return false;
    }

    if (Data.Num() != Other.Data.Num())
    {
        return false;
    }

    for (const auto& Pair : Data)
    {
        const int32 TimeStep = Pair.Key;
        const FMATimeStepData* OtherTimeStepData = Other.Data.Find(TimeStep);

        if (!OtherTimeStepData || !(Pair.Value == *OtherTimeStepData))
        {
            return false;
        }
    }

    return true;
}
