// MAComponent.h
// 通用组件环境对象 - 静态道具类（太阳能板、LED屏幕、音响、支架等）
// 
// 现在也支持搬运操作，实现 IMAPickupItem 接口

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../IMAEnvironmentObject.h"
#include "../IMAPickupItem.h"
#include "MAComponent.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class AMACharacter;
class AMAUGVCharacter;
struct FMAEnvironmentObjectConfig;

/**
 * 通用组件环境对象
 * 
 * 用于表示组装组件类环境对象，如太阳能板、天线、音响、支架等。
 * 这些对象可以被 Humanoid 搬运，实现 IMAPickupItem 接口。
 * 
 * 配置示例:
 * {
 *     "label": "SolarPanel_1",
 *     "type": "assembly_component",
 *     "position": [1000, 2000, 0],
 *     "rotation": [0, 0, 0],
 *     "features": {
 *         "subtype": "solar_panel",
 *         "scale": "2.0"
 *     }
 * }
 * 
 * 支持的 subtype:
 * - solar_panel: 太阳能板
 * - antenna_module: 卫星天线
 * - address_speaker: 广播音响
 * - stand: 支架/高脚凳
 */
UCLASS()
class MULTIAGENT_API AMAComponent : public AActor, public IMAEnvironmentObject, public IMAPickupItem
{
    GENERATED_BODY()

public:
    AMAComponent();

    //=========================================================================
    // IMAEnvironmentObject 接口实现
    //=========================================================================

    virtual FString GetObjectLabel() const override { return ObjectLabel; }
    virtual FString GetObjectType() const override { return ObjectType; }
    virtual const TMap<FString, FString>& GetObjectFeatures() const override { return Features; }

    //=========================================================================
    // IMAPickupItem 接口实现
    //=========================================================================

    virtual FString GetItemName() const override { return ObjectLabel; }
    virtual bool CanBePickedUp() const override { return bCanBePickedUp; }
    virtual void SetCanBePickedUp(bool bCanPickup) override { bCanBePickedUp = bCanPickup; }

    virtual float GetBottomOffset() const override;
    virtual FVector GetBoundsExtent() const override;

    virtual void AttachToHand(AMACharacter* Character) override;
    virtual void AttachToUGV(AMAUGVCharacter* UGV) override;
    virtual void PlaceOnGround(FVector Location, bool bUprightPlacement = true) override;
    virtual void PlaceOnObject(AActor* TargetObject, bool bUprightPlacement = true) override;
    virtual void DetachFromCarrier() override;
    virtual bool IsBeingCarried() const override { return CurrentCarrier.IsValid(); }
    virtual AActor* GetCurrentCarrier() const override { return CurrentCarrier.Get(); }
    virtual void SetPhysicsEnabled(bool bEnabled) override;
    virtual bool ShouldEnablePhysicsOnPlace() const override;
    virtual void OnPickedUp(AActor* PickerActor) override;
    virtual void OnDropped(FVector DropLocation) override;

    //=========================================================================
    // 配置方法
    //=========================================================================

    /** 根据配置初始化组件 */
    UFUNCTION(BlueprintCallable, Category = "Component")
    void Configure(const FMAEnvironmentObjectConfig& Config);

    /** 启用物理模拟（掉落） */
    UFUNCTION(BlueprintCallable, Category = "Component")
    void EnablePhysics();

    /** 禁用物理模拟 */
    UFUNCTION(BlueprintCallable, Category = "Component")
    void DisablePhysics();

    /** 获取网格组件 */
    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
    virtual void BeginPlay() override;

    //=========================================================================
    // 环境对象属性
    //=========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectType = TEXT("assembly_component");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TMap<FString, FString> Features;

    //=========================================================================
    // 可搬运属性
    //=========================================================================

    /** 是否可以被拾取 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    bool bCanBePickedUp = true;

    /** 当前承载者（角色、UGV 或其他物体） */
    UPROPERTY(BlueprintReadOnly, Category = "Pickup")
    TWeakObjectPtr<AActor> CurrentCarrier;

    //=========================================================================
    // 组件
    //=========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    /** 碰撞检测组件 (用于检测 Character 进入范围) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionComponent;

    /** Overlap 开始事件 */
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    /** Overlap 结束事件 */
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    /** 设置组件网格 */
    void SetComponentMesh(const FString& Subtype);

    /** 获取组件网格路径 */
    static FString GetComponentMeshPath(const FString& Subtype);

    /** 获取组件默认缩放 */
    static FVector GetComponentDefaultScale(const FString& Subtype);

    /** 获取组件默认偏移 */
    static FVector GetComponentDefaultOffset(const FString& Subtype);

    /** 获取组件堆叠偏移（补偿 mesh 不对称，确保堆叠物品视觉居中） */
    static FVector GetComponentStackOffset(const FString& Subtype);
};
