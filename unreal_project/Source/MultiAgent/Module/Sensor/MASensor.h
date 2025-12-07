// MASensor.h
// 传感器基类 - 可附着到 Character 上

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MASensor.generated.h"

class AMACharacter;

// 传感器类型
UENUM(BlueprintType)
enum class EMASensorType : uint8
{
    Camera,
    Lidar,
    Depth,
    IMU
};

UCLASS()
class MULTIAGENT_API AMASensor : public AActor
{
    GENERATED_BODY()

public:
    AMASensor();

    // ========== Attach 功能 ==========
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    void AttachToCharacter(AMACharacter* ParentCharacter, FVector RelativeLocation, FRotator RelativeRotation = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    void DetachFromCharacter();

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    AMACharacter* GetAttachedCharacter() const;

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    bool IsAttached() const;

    // ========== Properties ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
    int32 SensorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
    FString SensorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
    EMASensorType SensorType;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sensor")
    USceneComponent* RootSceneComponent;

    UPROPERTY()
    TWeakObjectPtr<AMACharacter> AttachedToCharacter;
};
