// MAFlightController.h
// 飞行控制器接口 - 为不同类型飞行器提供统一的飞行控制抽象

#pragma once

#include "CoreMinimal.h"

class ACharacter;

//=============================================================================
// 飞行状态枚举
//=============================================================================

enum class EMAFlightControlState : uint8
{
    Idle,           // 空闲（地面/悬停）
    TakingOff,      // 起飞中
    Flying,         // 飞行中
    Landing,        // 降落中
    Arrived         // 已到达
};

//=============================================================================
// 飞行控制器接口
//=============================================================================

class IMAFlightController
{
public:
    virtual ~IMAFlightController() = default;

    /** 初始化控制器 */
    virtual void Initialize(ACharacter* InOwner) = 0;

    /** 起飞到指定高度 */
    virtual bool TakeOff(float TargetAltitude = 0.f) = 0;

    /** 降落到指定目标点 */
    virtual bool Land(const FVector& LandingTarget) = 0;

    /** 飞向目标位置 */
    virtual bool FlyTo(const FVector& Destination) = 0;

    /** 跟随目标 Actor（每帧更新目标位置） */
    virtual void UpdateFollowTarget(const FVector& TargetLocation) = 0;

    /** 更新飞行（每帧调用） */
    virtual void UpdateFlight(float DeltaTime) = 0;

    /** 取消飞行（进入悬停/盘旋） */
    virtual void CancelFlight() = 0;

    /** 立即停止移动（清零速度，与 AIController::StopMovement 对应） */
    virtual void StopMovement() = 0;

    /** 设置是否平滑到达（接近目标时减速） */
    virtual void SetSmoothArrival(bool bSmooth) = 0;

    /** 获取当前状态 */
    virtual EMAFlightControlState GetState() const = 0;

    /** 是否正在飞行 */
    virtual bool IsFlying() const = 0;

    /** 是否已到达目标 */
    virtual bool HasArrived() const = 0;

    /** 获取到达判定半径 */
    virtual float GetAcceptanceRadius() const = 0;

    /** 设置到达判定半径 */
    virtual void SetAcceptanceRadius(float Radius) = 0;
};

//=============================================================================
// 多旋翼飞行控制器 (UAV)
//=============================================================================

class FMAMultiRotorFlightController : public IMAFlightController
{
public:
    virtual void Initialize(ACharacter* InOwner) override;
    virtual bool TakeOff(float TargetAltitude = 0.f) override;
    virtual bool Land(const FVector& LandingTarget) override;
    virtual bool FlyTo(const FVector& Destination) override;
    virtual void UpdateFollowTarget(const FVector& TargetLocation) override;
    virtual void UpdateFlight(float DeltaTime) override;
    virtual void CancelFlight() override;
    virtual void StopMovement() override;
    virtual void SetSmoothArrival(bool bSmooth) override { bSmoothArrival = bSmooth; }
    virtual EMAFlightControlState GetState() const override { return State; }
    virtual bool IsFlying() const override;
    virtual bool HasArrived() const override { return State == EMAFlightControlState::Arrived; }
    virtual float GetAcceptanceRadius() const override { return AcceptanceRadius; }
    virtual void SetAcceptanceRadius(float Radius) override { AcceptanceRadius = Radius; }

    /** 悬停 */
    void Hover();

private:
    /** 计算避障方向 */
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

    /** 获取地面高度 */
    float GetGroundHeight() const;

    /** 设置状态 */
    void SetState(EMAFlightControlState NewState);

    ACharacter* Owner = nullptr;
    EMAFlightControlState State = EMAFlightControlState::Idle;
    FVector TargetLocation = FVector::ZeroVector;
    FVector SmoothedAvoidanceDirection = FVector::ZeroVector;
    float CurrentSpeed = 0.f;
    float AcceptanceRadius = 200.f;
    
    // 是否平滑到达（接近目标时减速），默认 true
    bool bSmoothArrival = true;

    // 配置参数 (从 ConfigManager 加载)
    float MaxFlightSpeed = 600.f;
    float MinFlightAltitude = 800.f;
    float DefaultFlightAltitude = 1000.f;
    float ObstacleDetectionRange = 800.f;
    float ObstacleAvoidanceRadius = 150.f;
    float AvoidanceSmoothingFactor = 0.15f;
};

//=============================================================================
// 固定翼飞行控制器
//=============================================================================

enum class EMAFixedWingMode : uint8
{
    Cruising,       // 巡航
    Navigating,     // 导航到目标
    Orbiting        // 盘旋
};

class FMAFixedWingFlightController : public IMAFlightController
{
public:
    virtual void Initialize(ACharacter* InOwner) override;
    virtual bool TakeOff(float TargetAltitude = 0.f) override;
    virtual bool Land(const FVector& LandingTarget) override;
    virtual bool FlyTo(const FVector& Destination) override;
    virtual void UpdateFollowTarget(const FVector& TargetLocation) override;
    virtual void UpdateFlight(float DeltaTime) override;
    virtual void CancelFlight() override;
    virtual void StopMovement() override;
    virtual void SetSmoothArrival(bool bSmooth) override { /* 固定翼不支持，忽略 */ }
    virtual EMAFlightControlState GetState() const override { return State; }
    virtual bool IsFlying() const override { return bIsFlying; }
    virtual bool HasArrived() const override { return State == EMAFlightControlState::Arrived; }
    virtual float GetAcceptanceRadius() const override { return AcceptanceRadius; }
    virtual void SetAcceptanceRadius(float Radius) override { AcceptanceRadius = Radius; }

    /** 开始盘旋 */
    void StartOrbit(const FVector& OrbitCenter);

private:
    void UpdateNavigation(float DeltaTime);
    void UpdateOrbit(float DeltaTime);
    void UpdateCruising(float DeltaTime);
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);
    void SetState(EMAFlightControlState NewState);

    ACharacter* Owner = nullptr;
    EMAFlightControlState State = EMAFlightControlState::Idle;
    EMAFixedWingMode FlightMode = EMAFixedWingMode::Cruising;
    FVector TargetLocation = FVector::ZeroVector;
    FVector OrbitCenterPoint = FVector::ZeroVector;
    float OrbitAngle = 0.f;
    float CurrentSpeed = 0.f;
    float TargetAltitude = 0.f;
    float AcceptanceRadius = 200.f;
    bool bIsFlying = false;

    // 配置参数 (从 ConfigManager 加载)
    float MinSpeed = 300.f;
    float MaxSpeed = 800.f;
    float CruiseAltitude = 1500.f;
    float TurnRate = 45.f;
    float OrbitRadius = 1000.f;
    float ObstacleDetectionRange = 1200.f;
    float ObstacleAvoidanceRadius = 150.f;
};
