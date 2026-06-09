// MAMetalGrate.h
// 金属网格平板 - metal_grate 类型的静态承载平台
//
// 平板默认静止于地面，可被一个或多个机器人抬起或拉起；
// 平板表面可承载其他物品。实现 IMAEnvironmentObject 和 IMAPickupItem 接口，
// 因此可以被场景查询定位，也可以经 Place/Carry 类技能被搬运。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../IMAEnvironmentObject.h"
#include "../IMAPickupItem.h"
#include "MAMetalGrate.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class AMACharacter;
class AMAUGVCharacter;
struct FMAEnvironmentObjectConfig;

/**
 * 金属网格平板
 *
 * 配置示例:
 * {
 *     "label": "Grate_1",
 *     "type": "metal_grate",
 *     "position": [7000, 9000, 0],
 *     "rotation": [0, 0, 0],
 *     "features": {
 *         "scale": "1.0",
 *         "mass": "80"
 *     }
 * }
 *
 * 支持的 features:
 * - scale: 平板整体缩放倍数（在默认尺寸基础上相乘），默认 1.0
 * - mass: 平板质量（kg），用于物理模拟，默认 80
 */
UCLASS()
class MULTIAGENT_API AMAMetalGrate : public AActor, public IMAEnvironmentObject, public IMAPickupItem
{
    GENERATED_BODY()

public:
    AMAMetalGrate();

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
    virtual void OnPickedUp(AActor* PickerActor) override;
    virtual void OnDropped(FVector DropLocation) override;

    //=========================================================================
    // 配置方法
    //=========================================================================

    /** 根据配置初始化平板 */
    UFUNCTION(BlueprintCallable, Category = "MetalGrate")
    void Configure(const FMAEnvironmentObjectConfig& Config);

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
    FString ObjectType = TEXT("metal_grate");

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
    // 外观资源（可在编辑器中替换）
    //=========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TSoftObjectPtr<UStaticMesh> GrateMeshAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TSoftObjectPtr<UMaterialInterface> GrateMaterialAsset;

    //=========================================================================
    // 物理参数
    //=========================================================================

    /** 平板默认质量（kg） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float DefaultMassKg = 80.f;

    /** 线性阻尼（避免被轻微外力推动后滑行） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float LinearDamping = 1.5f;

    /** 角阻尼（避免长时间旋转） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float AngularDamping = 2.0f;

    //=========================================================================
    // 组件
    //=========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    /** 拾取交互范围检测组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionComponent;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    /** 应用 features 中的 scale / mass 到运行时组件 */
    void ApplyFeatureOverrides();
};
