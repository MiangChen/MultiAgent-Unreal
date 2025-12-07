// MASensorComponent.h
// 传感器组件基类 - 可附加到 Character 上的模块化组件

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MASensorComponent.generated.h"

// 传感器类型
UENUM(BlueprintType)
enum class EMASensorType : uint8
{
    Camera,
    Lidar,
    Depth,
    IMU
};

UCLASS(Abstract, ClassGroup=(MultiAgent), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMASensorComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UMASensorComponent();

    // ========== Properties ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
    int32 SensorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
    FString SensorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor")
    EMASensorType SensorType;

protected:
    virtual void BeginPlay() override;
};
