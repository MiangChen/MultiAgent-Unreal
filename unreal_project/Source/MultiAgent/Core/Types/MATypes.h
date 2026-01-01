// MATypes.h
// 公共类型定义

#pragma once

#include "CoreMinimal.h"
#include "MATypes.generated.h"

// Agent 类型枚举
UENUM(BlueprintType)
enum class EMAAgentType : uint8
{
    UAV,            // 多旋翼无人机
    FixedWingUAV,   // 固定翼无人机
    UGV,            // 无人车
    Quadruped,      // 四足机器人
    Humanoid        // 人形机器人
};

// 技能类型枚举
UENUM(BlueprintType)
enum class EMASkillType : uint8
{
    Navigate,   // 导航
    Search,     // 搜索
    Follow,     // 跟随
    Place,      // 放置/搬运
    Carry,      // 装载运送
    Charge      // 充电
};

// 编队类型
UENUM(BlueprintType)
enum class EMAFormationType : uint8
{
    None,
    Line,
    Column,
    Wedge,
    Diamond,
    Circle
};
