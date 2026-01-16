// MASkillAllocationModel.h
// 技能分配数据模型 - 管理技能分配数据的 CRUD 操作

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MASkillAllocationModel.generated.h"

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMASkillAllocationModel, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/** 数据变更委托 - 当技能分配数据发生变化时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillAllocationDataChanged);

/** 技能状态变更委托 - 当技能状态更新时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillStatusChanged, int32, TimeStep, const FString&, RobotId, ESkillExecutionStatus, NewStatus);

//=============================================================================
// UMASkillAllocationModel - 技能分配数据模型
//=============================================================================

/**
 * 技能分配数据模型
 * 管理技能分配数据，提供 CRUD 操作接口和变更通知
 * 维护 OriginalData (不可变) 和 WorkingData (可编辑) 双副本
 */
UCLASS(BlueprintType)
class MULTIAGENT_API UMASkillAllocationModel : public UObject
{
    GENERATED_BODY()

public:
    UMASkillAllocationModel();

    //=========================================================================
    // 数据加载
    //=========================================================================

    /** 从 JSON 字符串加载数据 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    bool LoadFromJson(const FString& JsonString);

    /** 从 JSON 字符串加载数据，带错误信息 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    bool LoadFromJsonWithError(const FString& JsonString, FString& OutError);

    /** 从数据结构加载 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    void LoadFromData(const FMASkillAllocationData& Data);

    /** 重置为原始数据 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    void ResetToOriginal();

    /** 清空所有数据 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    void Clear();

    //=========================================================================
    // 数据导出
    //=========================================================================

    /** 导出为格式化的 JSON 字符串 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    FString ToJson() const;

    /** 获取工作数据副本 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    FMASkillAllocationData GetWorkingData() const;

    /** 获取原始数据副本 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    FMASkillAllocationData GetOriginalData() const;

    //=========================================================================
    // 查询方法
    //=========================================================================

    /** 获取所有机器人 ID */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    TArray<FString> GetAllRobotIds() const;

    /** 获取所有时间步 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    TArray<int32> GetAllTimeSteps() const;

    /** 查找特定技能分配 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    bool FindSkill(int32 TimeStep, const FString& RobotId, FMASkillAssignment& OutSkill) const;

    /** 获取指定时间步的所有技能 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    TArray<FMASkillAssignment> GetSkillsAtTimeStep(int32 TimeStep) const;

    /** 获取指定机器人的所有技能 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    TArray<FMASkillAssignment> GetSkillsForRobot(const FString& RobotId) const;

    //=========================================================================
    // 状态更新
    //=========================================================================

    /** 更新技能执行状态 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    bool UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    //=========================================================================
    // 验证
    //=========================================================================

    /** 检查是否有有效数据 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    bool IsValidData() const;

    /** 获取时间步数量 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    int32 GetTimeStepCount() const;

    /** 获取机器人数量 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation|Model")
    int32 GetRobotCount() const;

    //=========================================================================
    // 委托
    //=========================================================================

    /** 数据变更委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillAllocation|Model")
    FOnSkillAllocationDataChanged OnDataChanged;

    /** 技能状态变更委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillAllocation|Model")
    FOnSkillStatusChanged OnSkillStatusChanged;

protected:
    /** 广播数据变更 */
    void BroadcastDataChanged();

private:
    /** 原始数据 (不可变) */
    UPROPERTY()
    FMASkillAllocationData OriginalData;

    /** 工作数据 (可编辑) */
    UPROPERTY()
    FMASkillAllocationData WorkingData;
};
