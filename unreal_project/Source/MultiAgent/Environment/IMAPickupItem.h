// IMAPickupItem.h
// 可搬运物体接口 - 所有可被 Place 技能搬运的物体都应实现此接口
//
// 实现此接口的类:
// - AMACargo (cargo 类型 - 货物箱)
// - AMAComponent (assembly_component 类型 - 组装组件)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IMAPickupItem.generated.h"

class AMACharacter;
class AMAUGVCharacter;

UINTERFACE(MinimalAPI, Blueprintable)
class UMAPickupItem : public UInterface
{
    GENERATED_BODY()
};

/**
 * 可搬运物体接口
 * 
 * 定义了所有可被 Humanoid 搬运的物体必须实现的方法。
 * 包括附着到角色手部、附着到 UGV、放置到地面等操作。
 */
class MULTIAGENT_API IMAPickupItem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 核心属性访问
    //=========================================================================

    /** 获取物品名称 */
    virtual FString GetItemName() const = 0;

    /** 是否可以被拾取 */
    virtual bool CanBePickedUp() const = 0;

    /** 设置是否可以被拾取 */
    virtual void SetCanBePickedUp(bool bCanPickup) = 0;

    //=========================================================================
    // 边界信息 - 用于精确放置计算
    //=========================================================================

    /**
     * 获取物体底部相对于 Actor 原点的 Z 偏移
     * 用于精确计算放置高度，使物体底部贴合目标表面
     * 
     * @return 底部 Z 偏移（负值表示底部在原点下方）
     */
    virtual float GetBottomOffset() const = 0;

    /**
     * 获取物体的边界尺寸
     * @return 边界半尺寸 (BoxExtent)
     */
    virtual FVector GetBoundsExtent() const = 0;

    //=========================================================================
    // 附着/分离系统
    //=========================================================================

    /** 附着到角色手部位置 */
    virtual void AttachToHand(AMACharacter* Character) = 0;

    /** 附着到 UGV 货舱 */
    virtual void AttachToUGV(AMAUGVCharacter* UGV) = 0;

    /** 
     * 放置到地面指定位置
     * @param Location 目标位置
     * @param bUprightPlacement 是否摆正放置（true=摆正姿态，false=保持当前姿态）
     */
    virtual void PlaceOnGround(FVector Location, bool bUprightPlacement = true) = 0;

    /** 
     * 放置到另一个物体上方
     * @param TargetObject 目标物体
     * @param bUprightPlacement 是否摆正放置
     */
    virtual void PlaceOnObject(AActor* TargetObject, bool bUprightPlacement = true) = 0;

    /** 从当前承载者分离 */
    virtual void DetachFromCarrier() = 0;

    /** 检查是否被承载 */
    virtual bool IsBeingCarried() const = 0;

    /** 获取当前承载者 */
    virtual AActor* GetCurrentCarrier() const = 0;

    //=========================================================================
    // 物理控制
    //=========================================================================

    /** 设置物理模拟开关 */
    virtual void SetPhysicsEnabled(bool bEnabled) = 0;

    /** 放置后是否应该启用物理模拟（某些不稳定物体如天线需要禁用） */
    virtual bool ShouldEnablePhysicsOnPlace() const { return true; }

    //=========================================================================
    // 事件回调
    //=========================================================================

    /** 被拾取时调用 */
    virtual void OnPickedUp(AActor* PickerActor) = 0;

    /** 被放下时调用 */
    virtual void OnDropped(FVector DropLocation) = 0;
};
