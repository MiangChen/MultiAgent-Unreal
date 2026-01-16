// MASkillAllocationModel.cpp
// 技能分配数据模型实现

#include "MASkillAllocationModel.h"

DEFINE_LOG_CATEGORY(LogMASkillAllocationModel);

//=============================================================================
// 构造函数
//=============================================================================

UMASkillAllocationModel::UMASkillAllocationModel()
{
}

//=============================================================================
// 数据加载
//=============================================================================

bool UMASkillAllocationModel::LoadFromJson(const FString& JsonString)
{
    FString ErrorMessage;
    return LoadFromJsonWithError(JsonString, ErrorMessage);
}

bool UMASkillAllocationModel::LoadFromJsonWithError(const FString& JsonString, FString& OutError)
{
    // Check for empty input first
    if (JsonString.IsEmpty())
    {
        OutError = TEXT("JSON editor is empty");
        UE_LOG(LogMASkillAllocationModel, Warning, TEXT("LoadFromJsonWithError: Empty JSON string"));
        return false;
    }

    FMASkillAllocationData ParsedData;
    if (!FMASkillAllocationData::FromJson(JsonString, ParsedData, OutError))
    {
        UE_LOG(LogMASkillAllocationModel, Warning, TEXT("Failed to parse JSON: %s"), *OutError);
        return false;
    }

    // Check if there were warnings (OutError starts with "[Warning]")
    bool bHasWarnings = OutError.StartsWith(TEXT("[Warning]"));
    
    LoadFromData(ParsedData);
    
    // If there were warnings, keep them in OutError but return success
    if (bHasWarnings)
    {
        UE_LOG(LogMASkillAllocationModel, Warning, TEXT("JSON parsed with warnings: %s"), *OutError);
    }
    
    return true;
}

void UMASkillAllocationModel::LoadFromData(const FMASkillAllocationData& Data)
{
    // Store original data (immutable copy)
    OriginalData = Data;
    
    // Create working copy
    WorkingData = Data;

    UE_LOG(LogMASkillAllocationModel, Log, TEXT("Loaded skill allocation: %s (%d time steps, %d robots)"),
           *WorkingData.Name, WorkingData.Data.Num(), GetRobotCount());

    BroadcastDataChanged();
}

void UMASkillAllocationModel::ResetToOriginal()
{
    WorkingData = OriginalData;
    BroadcastDataChanged();
    UE_LOG(LogMASkillAllocationModel, Log, TEXT("Reset to original data"));
}

void UMASkillAllocationModel::Clear()
{
    OriginalData = FMASkillAllocationData();
    WorkingData = FMASkillAllocationData();
    BroadcastDataChanged();
    UE_LOG(LogMASkillAllocationModel, Log, TEXT("Cleared all data"));
}

//=============================================================================
// 数据导出
//=============================================================================

FString UMASkillAllocationModel::ToJson() const
{
    return WorkingData.ToJson();
}

FMASkillAllocationData UMASkillAllocationModel::GetWorkingData() const
{
    return WorkingData;
}

FMASkillAllocationData UMASkillAllocationModel::GetOriginalData() const
{
    return OriginalData;
}

//=============================================================================
// 查询方法
//=============================================================================

TArray<FString> UMASkillAllocationModel::GetAllRobotIds() const
{
    return WorkingData.GetAllRobotIds();
}

TArray<int32> UMASkillAllocationModel::GetAllTimeSteps() const
{
    return WorkingData.GetAllTimeSteps();
}

bool UMASkillAllocationModel::FindSkill(int32 TimeStep, const FString& RobotId, FMASkillAssignment& OutSkill) const
{
    const FMASkillAssignment* Skill = WorkingData.FindSkill(TimeStep, RobotId);
    if (Skill)
    {
        OutSkill = *Skill;
        return true;
    }
    return false;
}

TArray<FMASkillAssignment> UMASkillAllocationModel::GetSkillsAtTimeStep(int32 TimeStep) const
{
    TArray<FMASkillAssignment> Result;
    
    const FMATimeStepData* TimeStepData = WorkingData.Data.Find(TimeStep);
    if (TimeStepData)
    {
        for (const auto& Pair : TimeStepData->RobotSkills)
        {
            Result.Add(Pair.Value);
        }
    }
    
    return Result;
}

TArray<FMASkillAssignment> UMASkillAllocationModel::GetSkillsForRobot(const FString& RobotId) const
{
    TArray<FMASkillAssignment> Result;
    
    for (const auto& TimeStepPair : WorkingData.Data)
    {
        const FMATimeStepData& TimeStepData = TimeStepPair.Value;
        const FMASkillAssignment* Skill = TimeStepData.RobotSkills.Find(RobotId);
        if (Skill)
        {
            Result.Add(*Skill);
        }
    }
    
    return Result;
}

//=============================================================================
// 状态更新
//=============================================================================

bool UMASkillAllocationModel::UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    // Validate inputs
    if (TimeStep < 0)
    {
        UE_LOG(LogMASkillAllocationModel, Warning, 
               TEXT("UpdateSkillStatus: Invalid TimeStep %d"), TimeStep);
        return false;
    }
    
    if (RobotId.IsEmpty())
    {
        UE_LOG(LogMASkillAllocationModel, Warning, 
               TEXT("UpdateSkillStatus: Empty RobotId"));
        return false;
    }

    FMASkillAssignment* Skill = WorkingData.FindSkill(TimeStep, RobotId);
    if (!Skill)
    {
        UE_LOG(LogMASkillAllocationModel, Warning, 
               TEXT("Skill not found for update: TimeStep=%d, RobotId=%s"), TimeStep, *RobotId);
        return false;
    }

    ESkillExecutionStatus OldStatus = Skill->Status;
    
    // Validate status transition (optional: can be made stricter)
    // For now, we allow any transition but log unusual ones
    if (OldStatus == ESkillExecutionStatus::Completed && NewStatus == ESkillExecutionStatus::Pending)
    {
        UE_LOG(LogMASkillAllocationModel, Warning, 
               TEXT("Unusual status transition: Completed -> Pending for TimeStep=%d, RobotId=%s"), 
               TimeStep, *RobotId);
    }
    
    Skill->Status = NewStatus;

    UE_LOG(LogMASkillAllocationModel, Log, 
           TEXT("Updated skill status: TimeStep=%d, RobotId=%s, %d -> %d"), 
           TimeStep, *RobotId, (int32)OldStatus, (int32)NewStatus);

    // Broadcast status change event
    OnSkillStatusChanged.Broadcast(TimeStep, RobotId, NewStatus);
    
    return true;
}

//=============================================================================
// 验证
//=============================================================================

bool UMASkillAllocationModel::IsValidData() const
{
    return !WorkingData.Name.IsEmpty() && WorkingData.Data.Num() > 0;
}

int32 UMASkillAllocationModel::GetTimeStepCount() const
{
    return WorkingData.Data.Num();
}

int32 UMASkillAllocationModel::GetRobotCount() const
{
    return GetAllRobotIds().Num();
}

//=============================================================================
// 辅助方法
//=============================================================================

void UMASkillAllocationModel::BroadcastDataChanged()
{
    OnDataChanged.Broadcast();
}
