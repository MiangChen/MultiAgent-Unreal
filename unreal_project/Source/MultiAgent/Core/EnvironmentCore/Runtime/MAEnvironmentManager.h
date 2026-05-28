// MAEnvironmentManager.h
// 环境实体管理器 - 负责所有环境对象的生命周期管理

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MAEnvironmentManager.generated.h"

class UMAConfigManager;
class AMACargo;
class AMAPerson;
class AMAVehicle;
class AMABoat;
class AMAFire;
class AMASmoke;
class AMAWind;
class AMAChargingStation;
class AMAComponent;
struct FMAEnvironmentObjectConfig;

DECLARE_LOG_CATEGORY_EXTERN(LogMAEnvironmentManager, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnvironmentObjectSpawned, AActor*, Object);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnvironmentObjectDestroyed, AActor*, Object);

/**
 * 环境实体管理器
 * 
 * 职责:
 * - 从 ConfigManager 获取配置并生成环境对象
 * - 管理环境对象生命周期 (person, vehicle, boat, cargo, fire, smoke, wind, charging_station)
 * - 按类型/标签查询环境对象
 * 
 * 支持的环境对象类型:
 * - person: 行人 (AMAPerson)
 * - vehicle: 车辆 (AMAVehicle)
 * - boat: 船只 (AMABoat)
 * - cargo: 货物 (AMACargo)
 * - fire: 火焰特效 (AMAFire)
 * - smoke: 烟雾特效 (AMASmoke)
 * - wind: 强风特效 (AMAWind)
 * - charging_station: 充电站 (AMAChargingStation)
 */
UCLASS()
class MULTIAGENT_API UMAEnvironmentManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== 初始化 ==========
    
    /** 从 ConfigManager 加载配置并生成所有环境对象 */
    UFUNCTION(BlueprintCallable, Category = "EnvironmentManager")
    void SpawnEnvironmentObjectsFromConfig();

    // ========== 环境对象查询 ==========
    
    /** 获取所有环境对象 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AActor*> GetAllEnvironmentObjects() const { return SpawnedObjects; }

    /** 按类型获取环境对象 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AActor*> GetEnvironmentObjectsByType(const FString& Type) const;

    /** 按标签获取环境对象 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    AActor* GetEnvironmentObjectByLabel(const FString& Label) const;

    /** 获取环境对象数量 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    int32 GetEnvironmentObjectCount() const { return SpawnedObjects.Num(); }

    // ========== 特定类型查询 ==========

    /** 获取所有行人 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AMAPerson*> GetAllPersons() const { return SpawnedPersons; }

    /** 获取所有车辆 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AMAVehicle*> GetAllVehicles() const { return SpawnedVehicles; }

    /** 获取所有船只 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AMABoat*> GetAllBoats() const { return SpawnedBoats; }

    /** 获取所有可拾取物品 (cargo) */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AMACargo*> GetAllPickupItems() const { return SpawnedPickupItems; }

    /** 获取所有充电站 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AMAChargingStation*> GetAllChargingStations() const { return SpawnedChargingStations; }

    /** 获取所有组件 */
    UFUNCTION(BlueprintPure, Category = "EnvironmentManager")
    TArray<AMAComponent*> GetAllComponents() const { return SpawnedComponents; }

    // ========== 环境对象管理 ==========

    /** 销毁环境对象 */
    UFUNCTION(BlueprintCallable, Category = "EnvironmentManager")
    bool DestroyEnvironmentObject(AActor* Object);

    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "EnvironmentManager")
    FOnEnvironmentObjectSpawned OnEnvironmentObjectSpawned;

    UPROPERTY(BlueprintAssignable, Category = "EnvironmentManager")
    FOnEnvironmentObjectDestroyed OnEnvironmentObjectDestroyed;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // ========== 生成的对象存储 ==========

    /** 所有生成的环境对象 */
    UPROPERTY()
    TArray<AActor*> SpawnedObjects;

    /** 行人列表 */
    UPROPERTY()
    TArray<AMAPerson*> SpawnedPersons;

    /** 车辆列表 */
    UPROPERTY()
    TArray<AMAVehicle*> SpawnedVehicles;

    /** 船只列表 */
    UPROPERTY()
    TArray<AMABoat*> SpawnedBoats;

    /** 货物列表 (cargo 类型) */
    UPROPERTY()
    TArray<AMACargo*> SpawnedPickupItems;

    /** 充电站列表 */
    UPROPERTY()
    TArray<AMAChargingStation*> SpawnedChargingStations;

    /** 组件列表 */
    UPROPERTY()
    TArray<AMAComponent*> SpawnedComponents;

    /** 特效列表 (fire, smoke, wind) */
    UPROPERTY()
    TArray<AActor*> SpawnedEffects;

    // ========== 内部方法 ==========

    UMAConfigManager* GetConfigManager() const;

    /** 生成环境对象 (从 EnvironmentObjects 配置) */
    void SpawnEnvironmentObjects();

    /** 生成行人 */
    AMAPerson* SpawnPerson(const FMAEnvironmentObjectConfig& Config);

    /** 生成车辆 */
    AMAVehicle* SpawnVehicle(const FMAEnvironmentObjectConfig& Config);

    /** 生成船只 */
    AMABoat* SpawnBoat(const FMAEnvironmentObjectConfig& Config);

    /** 生成货物 (cargo 类型) */
    AMACargo* SpawnCargo(const FMAEnvironmentObjectConfig& Config);

    /** 生成充电站 (charging_station 类型) */
    AMAChargingStation* SpawnChargingStation(const FMAEnvironmentObjectConfig& Config);

    /** 生成组件 (assembly_component 类型) */
    AMAComponent* SpawnComponent(const FMAEnvironmentObjectConfig& Config);

    /** 生成火焰特效 */
    AMAFire* SpawnFire(const FMAEnvironmentObjectConfig& Config);

    /** 生成烟雾特效 */
    AMASmoke* SpawnSmoke(const FMAEnvironmentObjectConfig& Config);

    /** 生成强风特效 */
    AMAWind* SpawnWind(const FMAEnvironmentObjectConfig& Config);

    /** 调整地面对象生成高度 */
    FVector AdjustGroundSpawnHeight(FVector Location) const;

    /** 解析颜色字符串 */
    FLinearColor ParseColorString(const FString& ColorString) const;
    
    /** 为环境对象生成附着火焰（如果 is_fire=true） */
    void SpawnAttachedFireIfNeeded(AActor* OwnerActor, const FMAEnvironmentObjectConfig& Config);
};
