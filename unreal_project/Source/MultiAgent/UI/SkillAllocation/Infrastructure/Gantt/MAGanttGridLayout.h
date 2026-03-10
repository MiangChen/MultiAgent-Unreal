// MAGanttGridLayout.h
// 甘特图网格布局计算 - 时间步/列索引映射与坐标转换

#pragma once

#include "CoreMinimal.h"

/**
 * 甘特图网格布局帮助类
 *
 * 负责：
 * - TimeStep <-> ColumnIndex 映射
 * - 网格坐标转换（TimeStep/RobotIndex 与屏幕坐标互转）
 * - 槽位中心坐标计算
 */
struct MULTIAGENT_API FMAGanttGridLayout
{
public:
    /** 更新时间步顺序并重建索引映射 */
    void SetTimeStepOrder(const TArray<int32>& InTimeStepOrder);

    /** 清空映射数据 */
    void Reset();

    /** 将时间步转换为屏幕 X 坐标 */
    float TimeStepToScreen(int32 TimeStep, float LabelWidth, float TimeStepWidth, float ScrollX) const;

    /** 将屏幕 X 坐标转换为时间步（返回实际 TimeStep 值，不是列索引） */
    int32 ScreenToTimeStep(float ScreenX, float LabelWidth, float TimeStepWidth, float ScrollX) const;

    /** 机器人索引转换为屏幕 Y 坐标 */
    float RobotIndexToScreen(int32 RobotIndex, float HeaderHeight, float RobotRowHeight, float ScrollY) const;

    /** 屏幕 Y 坐标转换为机器人索引 */
    int32 ScreenToRobotIndex(float ScreenY, float HeaderHeight, float RobotRowHeight, float ScrollY) const;

    /** 获取槽位中心位置 */
    FVector2D GetSlotCenterPosition(int32 TimeStep, int32 RobotIndex,
                                    float LabelWidth, float HeaderHeight,
                                    float TimeStepWidth, float RobotRowHeight,
                                    const FVector2D& ScrollOffset) const;

private:
    /** 时间步顺序（列顺序） */
    TArray<int32> TimeStepOrder;

    /** 时间步到列索引的映射 */
    TMap<int32, int32> TimeStepToColumnIndex;
};
