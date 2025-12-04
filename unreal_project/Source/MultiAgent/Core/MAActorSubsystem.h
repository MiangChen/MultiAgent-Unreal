// MAActorSubsystem.h
// Actor 管理子系统 - 负责所有 Character 和 Sensor 的生命周期管理

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "../Character/MACharacter.h"
#include "MAActorSubsystem.generated.h"

class AMACharacter;
class AMASensor;

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
    UPROPERTY()
    TArray<AMACharacter*> SpawnedCharacters;

    UPROPERTY()
    TArray<AMASensor*> SpawnedSensors;

    int32 NextActorID = 0;
    int32 NextSensorID = 0;
};
