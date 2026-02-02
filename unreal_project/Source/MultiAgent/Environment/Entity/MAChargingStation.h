// MAChargingStation.h
// 充电站 - 机器人可在此充电恢复电量

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAChargingStation.generated.h"

class AMACharacter;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class MULTIAGENT_API AMAChargingStation : public AActor
{
    GENERATED_BODY()

public:
    AMAChargingStation();

    // 交互范围
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
    float InteractionRadius = 300.f;

    // 检查机器人是否在范围内
    UFUNCTION(BlueprintCallable, Category = "Charging")
    bool IsRobotInRange(AMACharacter* Robot) const;

    // 获取范围内的所有机器人
    UFUNCTION(BlueprintCallable, Category = "Charging")
    TArray<AMACharacter*> GetRobotsInRange() const;

    // 设置 Mesh（可在编辑器或运行时调用）
    UFUNCTION(BlueprintCallable, Category = "Charging")
    void SetStationMesh(UStaticMesh* NewMesh);

protected:
    virtual void BeginPlay() override;

    // 软引用 - 可在编辑器中配置的 Mesh（移动资产时自动更新）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TSoftObjectPtr<UStaticMesh> StationMeshAsset;

    // 软引用 - 可在编辑器中配置的材质
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TSoftObjectPtr<UMaterialInterface> StationMaterialAsset;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* InteractionSphere;

    // Overlap 事件
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    // 当前在范围内的机器人
    UPROPERTY()
    TArray<TWeakObjectPtr<AMACharacter>> RobotsInRange;
};
