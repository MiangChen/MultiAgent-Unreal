#pragma once

#include "CoreMinimal.h"
#include "MAConfigNavigationTypes.generated.h"

USTRUCT(BlueprintType)
struct FMAFlightConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float MinAltitude = 800.f;

    UPROPERTY(BlueprintReadOnly)
    float DefaultAltitude = 1000.f;

    UPROPERTY(BlueprintReadOnly)
    float MaxSpeed = 600.f;

    UPROPERTY(BlueprintReadOnly)
    float ObstacleDetectionRange = 800.f;

    UPROPERTY(BlueprintReadOnly)
    float ObstacleAvoidanceRadius = 150.f;

    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 200.f;
};

USTRUCT(BlueprintType)
struct FMAGroundNavigationConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 200.f;

    UPROPERTY(BlueprintReadOnly)
    float StuckTimeout = 10.f;
};

USTRUCT(BlueprintType)
struct FMAFollowConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float Distance = 300.f;

    UPROPERTY(BlueprintReadOnly)
    float PositionTolerance = 200.f;

    UPROPERTY(BlueprintReadOnly)
    float ContinuousTimeThreshold = 30.f;
};

USTRUCT(BlueprintType)
struct FMAGuideConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString TargetMoveMode = TEXT("navmesh");

    UPROPERTY(BlueprintReadOnly)
    float WaitDistanceThreshold = 500.f;
};

USTRUCT(BlueprintType)
struct FMAHandleHazardConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float SafeDistance = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float Duration = 15.0f;

    UPROPERTY(BlueprintReadOnly)
    float SpraySpeed = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float SprayWidth = 30.f;
};

USTRUCT(BlueprintType)
struct FMATakePhotoConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float PhotoDistance = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float PhotoDuration = 3.0f;

    UPROPERTY(BlueprintReadOnly)
    float CameraFOV = 60.f;

    UPROPERTY(BlueprintReadOnly)
    float CameraForwardOffset = 50.f;
};

USTRUCT(BlueprintType)
struct FMABroadcastConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float BroadcastDistance = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float BroadcastDuration = 5.0f;

    UPROPERTY(BlueprintReadOnly)
    float EffectSpeed = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float EffectWidth = 30.f;

    UPROPERTY(BlueprintReadOnly)
    float ShockRate = 5.0f;
};

USTRUCT(BlueprintType)
struct FMAClearConfig
{
    GENERATED_BODY()

    /** 清洗作业平面与目标平整面的间距 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float StandoffDistance = 1000.f;

    /** 直线喷水初速度 (cm/s) */
    UPROPERTY(BlueprintReadOnly)
    float SpraySpeed = 800.f;

    /** 直线喷水水柱宽度 */
    UPROPERTY(BlueprintReadOnly)
    float SprayWidth = 1.f;

    /** 每个航点之间的移动速度 (cm/s)；0 表示使用机器人默认速度 */
    UPROPERTY(BlueprintReadOnly)
    float MoveSpeed = 0.f;

    /** 到达每个航点的判定半径 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 150.f;
};

USTRUCT(BlueprintType)
struct FMATransportConfig
{
    GENERATED_BODY()

    /** 抓取目标对象时，机器人相对抓取点上方的悬停高度 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float GraspHeightOffset = 15.f;

    /**
     * 抬起后的过渡爬升高度 (cm，世界 Z)。
     *
     * 全员就位、物体已与 leader 绑定后，每个参与者会先垂直爬升到该高度再奔赴目的地。
     * 若当前 Z 已经 >= LiftAltitude 则跳过这一段，直接进入运输阶段。
     * 仅对飞行器生效；地面机器人忽略。
     */
    UPROPERTY(BlueprintReadOnly)
    float LiftAltitude = 1000.f;

    /** 运输巡航高度 (cm)：编队抬起物体后飞行的高度 */
    UPROPERTY(BlueprintReadOnly)
    float CarryAltitude = 1500.f;

    /** 到达抓取点 / 目的地的判定半径 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 100.f;

    /** 所有参与者就位的等待超时 (秒)，超时则判定协作失败 */
    UPROPERTY(BlueprintReadOnly)
    float ReadyTimeout = 30.f;
};
