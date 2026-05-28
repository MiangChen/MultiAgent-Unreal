#include "MASkillAllocationViewerCoordinator.h"

#include "UI/SkillAllocation/MASkillAllocationModel.h"
#include "Core/SkillAllocation/Application/MASkillAllocationUseCases.h"

namespace
{
FMASkillAllocationViewerActionResult MakeSkillAllocationFailure(const FString& Message)
{
    FMASkillAllocationViewerActionResult Result;
    Result.Message = Message;
    return Result;
}

FMASkillAllocationViewerActionResult MakeSkillAllocationSuccess(
    const FString& Message,
    const FMASkillAllocationData& Data,
    bool bShouldPersist)
{
    FMASkillAllocationViewerActionResult Result;
    Result.bSuccess = true;
    Result.bDataChanged = true;
    Result.bShouldPersist = bShouldPersist;
    Result.bHasData = true;
    Result.Message = Message;
    Result.Data = Data;
    return Result;
}
}

FMASkillAllocationViewerActionResult FMASkillAllocationViewerCoordinator::LoadAllocationData(
    UMASkillAllocationModel* AllocationModel,
    const FMASkillAllocationData& Data,
    const FString& SuccessMessage,
    bool bShouldPersist)
{
    if (!AllocationModel)
    {
        return MakeSkillAllocationFailure(TEXT("AllocationModel is null"));
    }

    AllocationModel->LoadFromData(Data);
    return MakeSkillAllocationSuccess(SuccessMessage, AllocationModel->GetWorkingData(), bShouldPersist);
}

FMASkillAllocationViewerActionResult FMASkillAllocationViewerCoordinator::LoadAllocationJson(
    UMASkillAllocationModel* AllocationModel,
    const FString& JsonText,
    bool bShouldPersist)
{
    if (!AllocationModel)
    {
        return MakeSkillAllocationFailure(TEXT("AllocationModel is null"));
    }

    const FString TrimmedJson = JsonText.TrimStartAndEnd();
    if (TrimmedJson.IsEmpty())
    {
        return MakeSkillAllocationFailure(TEXT("[Warning] JSON editor is empty"));
    }

    const FMASkillAllocationParseFeedback ParseFeedback = FMASkillAllocationUseCases::ParseJson(JsonText);
    if (!ParseFeedback.bSuccess)
    {
        return MakeSkillAllocationFailure(FString::Printf(TEXT("[Error] %s"), *ParseFeedback.Message));
    }

    AllocationModel->LoadFromData(ParseFeedback.Data);

    FMASkillAllocationViewerActionResult Result = MakeSkillAllocationSuccess(
        FString::Printf(
            TEXT("[Success] Skill allocation updated: %d time steps, %d robots"),
            AllocationModel->GetTimeStepCount(),
            AllocationModel->GetRobotCount()),
        AllocationModel->GetWorkingData(),
        bShouldPersist);

    if (ParseFeedback.bWarning && !ParseFeedback.Message.IsEmpty())
    {
        Result.DetailLines.Add(ParseFeedback.Message);
    }

    return Result;
}

FMASkillAllocationViewerActionResult FMASkillAllocationViewerCoordinator::ResetAllocation(
    UMASkillAllocationModel* AllocationModel,
    bool bShouldPersist)
{
    if (!AllocationModel)
    {
        return MakeSkillAllocationFailure(TEXT("AllocationModel is null"));
    }

    AllocationModel->ResetToOriginal();
    return MakeSkillAllocationSuccess(TEXT("[重置] 所有技能状态已重置为待执行"), AllocationModel->GetWorkingData(), bShouldPersist);
}
