// MAPickupItem.h
// 可拾取物体基类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAPickupItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AMACharacter;
class AMAUGVCharacter;

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

    // ========== 附着/分离系统 ==========
    
    /** 当前承载者（角色、UGV 或其他物体） */
    UPROPERTY(BlueprintReadOnly, Category = "Attachment")
    TWeakObjectPtr<AActor> CurrentCarrier;
    
    /** 附着到角色手部位置 */
    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void AttachToHand(AMACharacter* Character);
    
    /** 附着到 UGV 货舱 */
    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void AttachToUGV(AMAUGVCharacter* UGV);
    
    /** 放置到地面指定位置 */
    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void PlaceOnGround(FVector Location);
    
    /** 放置到另一个物体上方 */
    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void PlaceOnObject(AMAPickupItem* TargetObject);
    
    /** 从当前承载者分离 */
    UFUNCTION(BlueprintCallable, Category = "Attachment")
    void DetachFromCarrier();
    
    /** 检查是否被承载 */
    UFUNCTION(BlueprintPure, Category = "Attachment")
    bool IsBeingCarried() const { return CurrentCarrier.IsValid(); }
    
    /** 获取当前承载者 */
    UFUNCTION(BlueprintPure, Category = "Attachment")
    AActor* GetCurrentCarrier() const { return CurrentCarrier.Get(); }

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
