// MACargo.h
// 货物类 - cargo 类型的可搬运物体
//
// 实现 IMAEnvironmentObject 和 IMAPickupItem 接口。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../IMAEnvironmentObject.h"
#include "../IMAPickupItem.h"
#include "MACargo.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AMACharacter;
class AMAUGVCharacter;

/**
 * 货物 Actor
 * 
 * 实现 IMAEnvironmentObject 和 IMAPickupItem 接口，支持统一的场景查询和搬运操作。
 * 
 * 数据流:
 * - config/maps/{MapType}.json 中配置 environment 数组
 * - MAConfigManager 解析配置到 FMAEnvironmentObjectConfig
 * - MASceneGraphManager::LoadDynamicNodes() 创建场景图节点
 */
UCLASS()
class MULTIAGENT_API AMACargo : public AActor, public IMAEnvironmentObject, public IMAPickupItem
{
    GENERATED_BODY()

public:
    AMACargo();

    //=========================================================================
    // 原有属性 (保持向后兼容)
    //=========================================================================

    /** 物品名称 (用于显示) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString ItemName;

    /** 物品 ID (对应场景图节点 ID) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 ItemID;

    /** 物品颜色 (用于渲染) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FLinearColor ItemColor = FLinearColor::White;

    /** 是否可以被拾取 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bCanBePickedUp;

    //=========================================================================
    // 环境对象属性 (IMAEnvironmentObject 数据存储)
    //=========================================================================

    /** 对象标签 (用于语义查询，若空则使用 ItemName) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectLabel;

    /** 对象类型 (cargo) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectType = TEXT("cargo");

    /** 对象特征 (color, subtype, status 等键值对) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TMap<FString, FString> Features;

    //=========================================================================
    // IMAEnvironmentObject 接口实现
    //=========================================================================

    virtual FString GetObjectLabel() const override;
    virtual FString GetObjectType() const override;
    virtual const TMap<FString, FString>& GetObjectFeatures() const override;

    //=========================================================================
    // IMAPickupItem 接口实现
    //=========================================================================

    virtual FString GetItemName() const override { return ItemName; }
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
    // 物品操作
    //=========================================================================

    /** 设置物品颜色 */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetItemColor(FLinearColor NewColor);

    /** 获取 Mesh 组件 */
    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

    //=========================================================================
    // 承载者引用
    //=========================================================================
    
    /** 当前承载者（角色、UGV 或其他物体） */
    UPROPERTY(BlueprintReadOnly, Category = "Attachment")
    TWeakObjectPtr<AActor> CurrentCarrier;

protected:
    virtual void BeginPlay() override;

    /** 碰撞检测组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionComponent;

    /** 网格组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    /** Overlap 开始事件 */
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    /** Overlap 结束事件 */
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
