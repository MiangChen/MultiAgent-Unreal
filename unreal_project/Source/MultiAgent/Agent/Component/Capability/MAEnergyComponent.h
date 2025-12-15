// MAEnergyComponent.h
// 能量组件 - 实现 IMAChargeable 接口
// 可附加到任何需要能量系统的 Agent

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Interface/MAAgentInterfaces.h"
#include "MAEnergyComponent.generated.h"

class UMAAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnergyChanged, float, NewEnergy, float, MaxEnergy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnergyDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLowEnergy);

/**
 * 能量组件 - 管理 Agent 的能量系统
 * 
 * 使用方式:
 *   1. 在 Character 构造函数中创建: EnergyComp = CreateDefaultSubobject<UMAEnergyComponent>(TEXT("EnergyComponent"));
 *   2. 通过 Interface 访问: IMAChargeable* Chargeable = Cast<IMAChargeable>(Actor->GetComponentByClass(UMAEnergyComponent::StaticClass()));
 */
UCLASS(ClassGroup=(MultiAgent), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMAEnergyComponent : public UActorComponent, public IMAChargeable
{
    GENERATED_BODY()

public:
    UMAEnergyComponent();

    // ========== IMAChargeable Interface ==========
    virtual float GetEnergy() const override { return Energy; }
    virtual float GetMaxEnergy() const override { return MaxEnergy; }
    virtual float GetEnergyPercent() const override { return (MaxEnergy > 0.f) ? (Energy / MaxEnergy * 100.f) : 0.f; }
    virtual bool HasEnergy() const override { return Energy > 0.f; }
    virtual void RestoreEnergy(float Amount) override;
    virtual void DrainEnergy(float DeltaTime) override;

    // ========== 属性 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float Energy = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float MaxEnergy = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float EnergyDrainRate = 1.f;  // 每秒消耗

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float LowEnergyThreshold = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    bool bShowEnergyDisplay = true;

    // ========== 委托 ==========
    UPROPERTY(BlueprintAssignable, Category = "Energy")
    FOnEnergyChanged OnEnergyChanged;

    UPROPERTY(BlueprintAssignable, Category = "Energy")
    FOnEnergyDepleted OnEnergyDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Energy")
    FOnLowEnergy OnLowEnergy;

    // ========== 辅助方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool IsLowEnergy() const { return Energy < LowEnergyThreshold; }

    UFUNCTION(BlueprintCallable, Category = "Energy")
    void SetEnergy(float NewEnergy);

    // 更新能量显示 (每帧调用)
    void UpdateEnergyDisplay();

    // 检查并更新低电量状态 Tag
    void CheckLowEnergyStatus();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // 缓存的 ASC 引用
    UPROPERTY()
    TWeakObjectPtr<UMAAbilitySystemComponent> CachedASC;

    // 获取 Owner 的 ASC
    UMAAbilitySystemComponent* GetOwnerASC();

    // 上一帧是否处于低电量状态
    bool bWasLowEnergy = false;
};
