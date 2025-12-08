// MATypes.h
// 公共类型定义 - 避免循环依赖

#pragma once

#include "CoreMinimal.h"
#include "MATypes.generated.h"

// Agent 类型枚举
UENUM(BlueprintType)
enum class EMAAgentType : uint8
{
    Human,
    RobotDog,
    Drone,           // 通用无人机 (基类，不直接使用)
    DronePhantom4,   // DJI Phantom 4
    DroneInspire2,   // DJI Inspire 2
    Camera
};

// 编队类型
UENUM(BlueprintType)
enum class EMAFormationType : uint8
{
    None,       // 无编队
    Line,       // 横向一字排开
    Column,     // 纵队
    Wedge,      // 楔形 (V形)
    Diamond,    // 菱形 (X)
    Circle      // 圆形
};
