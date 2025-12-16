// MAAgentInterfaces.h
// Agent 能力接口定义 - 支持多种 Agent 类型共享行为

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MAAgentInterfaces.generated.h"

class AMACharacter;
class AMAPatrolPath;

// ============================================================================
// IMAPatrollable - 可巡逻能力
// ============================================================================

UINTERFACE(MinimalAPI, Blueprintable)
class UMAPatrollable : public UInterface
{
    GENERATED_BODY()
};

class MULTIAGENT_API IMAPatrollable
{
    GENERATED_BODY()

public:
    virtual AMAPatrolPath* GetPatrolPath() const = 0;
    virtual void SetPatrolPath(AMAPatrolPath* Path) = 0;
    virtual float GetScanRadius() const = 0;
};

// ============================================================================
// IMAFollowable - 可跟随能力
// ============================================================================

UINTERFACE(MinimalAPI, Blueprintable)
class UMAFollowable : public UInterface
{
    GENERATED_BODY()
};

class MULTIAGENT_API IMAFollowable
{
    GENERATED_BODY()

public:
    virtual AMACharacter* GetFollowTarget() const = 0;
    virtual void SetFollowTarget(AMACharacter* Target) = 0;
    virtual void ClearFollowTarget() = 0;
};

// ============================================================================
// IMACoverable - 可覆盖扫描能力
// ============================================================================

UINTERFACE(MinimalAPI, Blueprintable)
class UMACoverable : public UInterface
{
    GENERATED_BODY()
};

class MULTIAGENT_API IMACoverable
{
    GENERATED_BODY()

public:
    virtual AActor* GetCoverageArea() const = 0;
    virtual void SetCoverageArea(AActor* Area) = 0;
    virtual float GetScanRadius() const = 0;
};

// ============================================================================
// IMAChargeable - 可充电能力
// ============================================================================

UINTERFACE(MinimalAPI, Blueprintable)
class UMAChargeable : public UInterface
{
    GENERATED_BODY()
};

class MULTIAGENT_API IMAChargeable
{
    GENERATED_BODY()

public:
    virtual float GetEnergy() const = 0;
    virtual float GetMaxEnergy() const = 0;
    virtual float GetEnergyPercent() const = 0;
    virtual bool HasEnergy() const = 0;
    virtual void RestoreEnergy(float Amount) = 0;
    virtual void DrainEnergy(float DeltaTime) = 0;
};