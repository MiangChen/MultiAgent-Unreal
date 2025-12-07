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

    // ========== Action 动态发现与执行 ==========
    
    // 获取该 Sensor 支持的所有 Actions
    // 子类应重写此方法返回自己支持的 Actions
    UFUNCTION(BlueprintCallable, Category = "Actions")
    virtual TArray<FString> GetAvailableActions() const;
    
    // 执行指定的 Action
    // 子类应重写此方法实现具体的 Action 逻辑
    UFUNCTION(BlueprintCallable, Category = "Actions")
    virtual bool ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params);

protected:
    virtual void BeginPlay() override;
};
