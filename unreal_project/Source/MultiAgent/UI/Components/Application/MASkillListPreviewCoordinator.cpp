#include "MASkillListPreviewCoordinator.h"

FMASkillListPreviewModel FMASkillListPreviewCoordinator::BuildModel(const FMASkillAllocationData& Data)
{
    FMASkillListPreviewModel Model;
    Model.bHasData = Data.Data.Num() > 0;
    Model.RobotIds = Data.GetAllRobotIds();
    Model.TimeSteps = Data.GetAllTimeSteps();
    Model.StatsText = Model.bHasData
        ? FString::Printf(TEXT("%d robots, %d time steps"), Model.RobotIds.Num(), Model.TimeSteps.Num())
        : TEXT("No data");

    for (int32 RobotIndex = 0; RobotIndex < Model.RobotIds.Num(); ++RobotIndex)
    {
        const FString& RobotId = Model.RobotIds[RobotIndex];
        for (const int32 TimeStep : Model.TimeSteps)
        {
            const FMATimeStepData* TimeStepData = Data.Data.Find(TimeStep);
            if (!TimeStepData)
            {
                continue;
            }

            const FMASkillAssignment* Skill = TimeStepData->RobotSkills.Find(RobotId);
            if (!Skill)
            {
                continue;
            }

            FMASkillListPreviewBarModel Bar;
            Bar.RobotId = RobotId;
            Bar.SkillName = Skill->SkillName;
            Bar.TimeStep = TimeStep;
            Bar.RobotIndex = RobotIndex;
            Bar.Status = Skill->Status;
            Model.Bars.Add(MoveTemp(Bar));
        }
    }

    return Model;
}

FMASkillListPreviewModel FMASkillListPreviewCoordinator::MakeEmptyModel()
{
    return FMASkillListPreviewModel();
}

bool FMASkillListPreviewCoordinator::UpdateSkillStatus(
    FMASkillListPreviewModel& Model,
    int32 TimeStep,
    const FString& RobotId,
    ESkillExecutionStatus NewStatus)
{
    bool bUpdated = false;
    for (FMASkillListPreviewBarModel& Bar : Model.Bars)
    {
        if (Bar.TimeStep == TimeStep && Bar.RobotId == RobotId)
        {
            Bar.Status = NewStatus;
            bUpdated = true;
            break;
        }
    }
    return bUpdated;
}
