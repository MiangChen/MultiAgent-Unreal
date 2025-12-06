// MAActorSubsystem.h
// Actor 管理子系统 - 负责所有 Character 和 Sensor 的生命周期管理
// 同时作为集群管理器，处理编队等集群行为（类似 CARLA TrafficManager）

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "../Character/MACharacter.h"
#include "MAActorSubsystem.generated.h"

class AMACharacter;
class AMASensor;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSpawned, AMACharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDestroyed, AMACharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSensorSpawned, AMASensor*, Sensor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSensorDestroyed, AMASensor*, Sensor);

UCLASS()
class MULTIAGENT_API UMAActorSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== Character 管理 ==========
    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    AMACharacter* SpawnCharacter(TSubclassOf<AMACharacter> CharacterClass, FVector Location, FRotator Rotation, int32 ActorID, EMAActorType Type);

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    bool DestroyCharacter(AMACharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    TArray<AMACharacter*> GetAllCharacters() const { return SpawnedCharacters; }

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    TArray<AMACharacter*> GetCharactersByType(EMAActorType Type) const;

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    AMACharacter* GetCharacterByID(int32 ActorID) const;

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    AMACharacter* GetCharacterByName(const FString& Name) const;

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    int32 GetCharacterCount() const { return SpawnedCharacters.Num(); }

    // ========== Sensor 管理 ==========
    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    AMASensor* SpawnSensor(TSubclassOf<AMASensor> SensorClass, FVector Location, FRotator Rotation, int32 SensorID);

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    bool DestroySensor(AMASensor* Sensor);

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    TArray<AMASensor*> GetAllSensors() const { return SpawnedSensors; }

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    int32 GetSensorCount() const { return SpawnedSensors.Num(); }

    // ========== 批量操作 ==========
    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    void StopAllCharacters();

    UFUNCTION(BlueprintCallable, Category = "ActorManager")
    void MoveAllCharactersTo(FVector Destination, float Radius = 150.f);

    // ========== 编队管理 (Formation Manager) ==========
    
    // 启动编队 - Leader 带领所有 RobotDog
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void StartFormation(AMACharacter* Leader, EMAFormationType Type);
    
    // 停止编队
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void StopFormation();
    
    // 切换编队类型
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void SetFormationType(EMAFormationType Type);
    
    // 获取当前编队类型
    UFUNCTION(BlueprintCallable, Category = "Formation")
    EMAFormationType GetFormationType() const { return CurrentFormationType; }
    
    // 检查是否在编队中
    UFUNCTION(BlueprintCallable, Category = "Formation")
    bool IsInFormation() const { return CurrentFormationType != EMAFormationType::None; }
    
    // 编队参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationSpacing = 200.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationUpdateInterval = 0.3f;
    
    // 滞后区参数：开始移动阈值 > 停止移动阈值，避免抖动
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationStartMoveThreshold = 150.f;  // 距离超过此值开始移动
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationStopMoveThreshold = 80.f;    // 距离小于此值停止移动

    // ========== 委托 ==========
    UPROPERTY(BlueprintAssignable, Category = "ActorManager")
    FOnCharacterSpawned OnCharacterSpawned;

    UPROPERTY(BlueprintAssignable, Category = "ActorManager")
    FOnCharacterDestroyed OnCharacterDestroyed;

    UPROPERTY(BlueprintAssignable, Category = "ActorManager")
    FOnSensorSpawned OnSensorSpawned;

    UPROPERTY(BlueprintAssignable, Category = "ActorManager")
    FOnSensorDestroyed OnSensorDestroyed;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // ========== Actor 管理 ==========
    UPROPERTY()
    TArray<AMACharacter*> SpawnedCharacters;

    UPROPERTY()
    TArray<AMASensor*> SpawnedSensors;

    int32 NextActorID = 0;
    int32 NextSensorID = 0;
    
    // ========== 编队状态 ==========
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> FormationLeader;
    
    UPROPERTY()
    TArray<AMACharacter*> FormationMembers;
    
    EMAFormationType CurrentFormationType = EMAFormationType::None;
    
    FTimerHandle FormationTimerHandle;
    
    // 编队更新逻辑
    void UpdateFormation();
    
    // 计算编队位置
    TArray<FVector> CalculateFormationPositions(const FVector& LeaderLocation, const FRotator& LeaderRotation) const;
    
    // 计算单个位置的偏移
    FVector CalculateFormationOffset(int32 Position, int32 TotalCount) const;
};
