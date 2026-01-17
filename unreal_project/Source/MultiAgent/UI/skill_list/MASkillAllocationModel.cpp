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
// 技能移动操作
//=============================================================================

bool UMASkillAllocationModel::IsSlotEmpty(int32 TimeStep, const FString& RobotId) const
{
    // Validate inputs
    if (TimeStep < 0)
    {
        UE_LOG(LogMASkillAllocationModel, Warning, 
               TEXT("IsSlotEmpty: Invalid TimeStep %d"), TimeStep);
        return true; // Invalid slot is considered empty
    }
    
    if (RobotId.IsEmpty())
    {
        UE_LOG(LogMASkillAllocationModel, Warning, 
               TEXT("IsSlotEmpty: Empty RobotId"));
        return true; // Invalid slot is considered empty
    }

    // Check if the time step exists
    const FMATimeStepData* TimeStepData = WorkingData.Data.Find(TimeStep);
    if (!TimeStepData)
    {
        // Time step doesn't exist, so slot is empty
        return true;
    }

    // Check if the robot has a skill at this time step
    const FMASkillAssignment* Skill = TimeStepData->RobotSkills.Find(RobotId);
    return Skill == nullptr;
}

bool UMASkillAllocationModel::ValidateMoveSkill(int32 SourceTimeStep, const FString& SourceRobotId,
                                                 int32 TargetTimeStep, const FString& TargetRobotId,
                                                 FString& OutErrorMessage) const
{
    // Validate source time step
    if (SourceTimeStep < 0)
    {
        OutErrorMessage = FString::Printf(TEXT("无效的源时间步: %d"), SourceTimeStep);
        return false;
    }

    // Validate source robot ID
    if (SourceRobotId.IsEmpty())
    {
        OutErrorMessage = TEXT("源机器人ID为空");
        return false;
    }

    // Validate target time step
    if (TargetTimeStep < 0)
    {
        OutErrorMessage = FString::Printf(TEXT("无效的目标时间步: %d"), TargetTimeStep);
        return false;
    }

    // Validate target robot ID
    if (TargetRobotId.IsEmpty())
    {
        OutErrorMessage = TEXT("目标机器人ID为空");
        return false;
    }

    // Check if source and target are the same
    if (SourceTimeStep == TargetTimeStep && SourceRobotId == TargetRobotId)
    {
        OutErrorMessage = TEXT("源位置和目标位置相同");
        return false;
    }

    // Check if source has a skill
    FMASkillAssignment SourceSkill;
    if (!FindSkill(SourceTimeStep, SourceRobotId, SourceSkill))
    {
        OutErrorMessage = FString::Printf(TEXT("源位置没有技能: T%d, %s"), SourceTimeStep, *SourceRobotId);
        return false;
    }

    // Check if target slot is empty
    if (!IsSlotEmpty(TargetTimeStep, TargetRobotId))
    {
        OutErrorMessage = FString::Printf(TEXT("目标槽位已被占用: T%d, %s"), TargetTimeStep, *TargetRobotId);
        return false;
    }

    // All validations passed
    OutErrorMessage.Empty();
    return true;
}

bool UMASkillAllocationModel::MoveSkill(int32 SourceTimeStep, const FString& SourceRobotId,
                                         int32 TargetTimeStep, const FString& TargetRobotId)
{
    // Validate the move first
    FString ErrorMessage;
    if (!ValidateMoveSkill(SourceTimeStep, SourceRobotId, TargetTimeStep, TargetRobotId, ErrorMessage))
    {
        UE_LOG(LogMASkillAllocationModel, Warning, TEXT("MoveSkill failed: %s"), *ErrorMessage);
        return false;
    }

    // Get the source skill data
    FMATimeStepData* SourceTimeStepData = WorkingData.Data.Find(SourceTimeStep);
    if (!SourceTimeStepData)
    {
        UE_LOG(LogMASkillAllocationModel, Error, TEXT("MoveSkill: Source time step %d not found"), SourceTimeStep);
        return false;
    }

    FMASkillAssignment* SourceSkill = SourceTimeStepData->RobotSkills.Find(SourceRobotId);
    if (!SourceSkill)
    {
        UE_LOG(LogMASkillAllocationModel, Error, TEXT("MoveSkill: Source skill not found at T%d, %s"), 
               SourceTimeStep, *SourceRobotId);
        return false;
    }

    // Copy the skill data before removing
    FMASkillAssignment SkillToMove = *SourceSkill;

    // Remove from source position
    SourceTimeStepData->RobotSkills.Remove(SourceRobotId);
    UE_LOG(LogMASkillAllocationModel, Log, TEXT("MoveSkill: Removed skill '%s' from T%d, %s"), 
           *SkillToMove.SkillName, SourceTimeStep, *SourceRobotId);

    // Clean up empty time step data if needed
    if (SourceTimeStepData->RobotSkills.Num() == 0)
    {
        WorkingData.Data.Remove(SourceTimeStep);
        UE_LOG(LogMASkillAllocationModel, Log, TEXT("MoveSkill: Removed empty time step %d"), SourceTimeStep);
    }

    // Add to target position
    FMATimeStepData* TargetTimeStepData = WorkingData.Data.Find(TargetTimeStep);
    if (!TargetTimeStepData)
    {
        // Create new time step data if it doesn't exist
        FMATimeStepData NewTimeStepData;
        WorkingData.Data.Add(TargetTimeStep, NewTimeStepData);
        TargetTimeStepData = WorkingData.Data.Find(TargetTimeStep);
        UE_LOG(LogMASkillAllocationModel, Log, TEXT("MoveSkill: Created new time step %d"), TargetTimeStep);
    }

    // Add the skill to target position
    TargetTimeStepData->RobotSkills.Add(TargetRobotId, SkillToMove);
    UE_LOG(LogMASkillAllocationModel, Log, TEXT("MoveSkill: Added skill '%s' to T%d, %s"), 
           *SkillToMove.SkillName, TargetTimeStep, *TargetRobotId);

    // Broadcast data changed event
    BroadcastDataChanged();

    UE_LOG(LogMASkillAllocationModel, Log, 
           TEXT("MoveSkill: Successfully moved skill '%s' from T%d,%s to T%d,%s"), 
           *SkillToMove.SkillName, SourceTimeStep, *SourceRobotId, TargetTimeStep, *TargetRobotId);

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
