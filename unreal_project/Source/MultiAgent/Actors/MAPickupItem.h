// MAPickupItem.h
// 可拾取物体基类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAPickupItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class MULTIAGENT_API AMAPickupItem : public AActor
{
    GENERATED_BODY()

public:
    AMAPickupItem();

    // 物品名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemName;

    // 物品 ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemID;

    // 是否可以被拾取
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bCanBePickedUp;

    // 被拾取时调用
    UFUNCTION(BlueprintCallable, Category = "Item")
    void OnPickedUp(AActor* PickerActor);

    // 被放下时调用
    UFUNCTION(BlueprintCallable, Category = "Item")
    void OnDropped(FVector DropLocation);

    // 设置物理模拟
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetPhysicsEnabled(bool bEnabled);

    // 获取 Mesh 组件
    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
    virtual void BeginPlay() override;

    // 碰撞检测组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionComponent;

    // 网格组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    // Overlap 事件
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
