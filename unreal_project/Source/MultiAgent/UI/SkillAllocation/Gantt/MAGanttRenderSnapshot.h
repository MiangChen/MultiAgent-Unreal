// MAGanttRenderSnapshot.h
// 甘特图渲染快照 - 统一管理绘制数据与槽位索引

#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Types/MATaskGraphTypes.h"

/** 甘特图渲染快照 */
struct MULTIAGENT_API FMAGanttRenderSnapshot
{
public:
    /** 绘制用技能条数据 */
    TArray<FMASkillBarRenderData> Bars;

    /** 机器人顺序 */
    TArray<FString> RobotOrder;

    /** 时间步顺序 */
    TArray<int32> TimeStepOrder;

    /** 机器人 ID -> 行索引 */
    TMap<FString, int32> RobotIdToIndexMap;

    /** 时间步 -> 列索引 */
    TMap<int32, int32> TimeStepToIndexMap;

    /** 槽位索引映射（TimeStep + RobotId -> Bars 下标） */
    TMap<FString, int32> SlotIndexMap;

    /** 清空快照 */
    void Reset();

    /** 重建槽位索引映射 */
    void RebuildSlotIndex();

    /** 重建顺序索引映射（Robot/TimeStep） */
    void RebuildOrderIndices();

    /** 查询机器人索引（未找到返回 INDEX_NONE） */
    int32 FindRobotIndex(const FString& RobotId) const;

    /** 查询时间步索引（未找到返回 INDEX_NONE） */
    int32 FindTimeStepIndex(int32 TimeStep) const;

    /** 查询时间步是否存在 */
    bool HasTimeStep(int32 TimeStep) const;

    /** 查询机器人索引是否合法 */
    bool IsValidRobotIndex(int32 RobotIndex) const;

    /** 通过行索引获取机器人 ID（越界返回 nullptr） */
    const FString* GetRobotIdByIndex(int32 RobotIndex) const;

    /** 查询槽位是否有技能 */
    bool HasSkillAtSlot(int32 TimeStep, const FString& RobotId) const;

    /** 查询槽位对应的技能条下标，未找到返回 INDEX_NONE */
    int32 FindBarIndexAtSlot(int32 TimeStep, const FString& RobotId) const;

    /** 查询槽位对应的技能条（只读） */
    const FMASkillBarRenderData* FindBarAtSlot(int32 TimeStep, const FString& RobotId) const;

    /** 查询槽位对应的技能条（可写） */
    FMASkillBarRenderData* FindMutableBarAtSlot(int32 TimeStep, const FString& RobotId);

private:
    static FString MakeSlotKey(int32 TimeStep, const FString& RobotId);
};
