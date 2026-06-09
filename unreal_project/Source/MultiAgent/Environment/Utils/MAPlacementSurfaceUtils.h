// MAPlacementSurfaceUtils.h
// 计算将物体放置到另一个物体顶面时的世界坐标。
//
// 用于 IMAPickupItem::PlaceOnObject() 的实现，避免多个物体被堆叠在
// 目标中心同一点上。会扫描场景中已经摆在目标顶面的其他可拾取物体，
// 选取一块没有冲突的区域；若实在没有空余空间则回退到目标中心。

#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * 放置面计算工具
 *
 * 仅做几何计算，不修改任何 Actor 状态。
 * 调用方在拿到放置坐标后，自己负责把物体移到该位置、调整朝向、启用/禁用物理。
 */
class MULTIAGENT_API FMAPlacementSurfaceUtils
{
public:
    /**
     * 计算将 ItemActor 放置到 TargetActor 顶面时的世界坐标。
     *
     * 算法：
     * 1. 由 IMAPickupItem 接口（或 RootComponent->Bounds）推算目标顶面 Z 与 XY 半尺寸。
     * 2. 扫描世界中所有 IMAPickupItem，筛选出未被携带、且当前坐落在目标顶面附近的物体，
     *    把它们的 XY 包围盒视为占用矩形。
     * 3. 以目标中心为起点，按螺旋格点顺序寻找一个能完整放下 ItemActor 且不与任何
     *    占用矩形相交的位置。
     * 4. 若找不到任何合法位置，回退到目标中心。
     *
     * @param TargetActor 被放置到的目标物体（如 Grate_1）
     * @param ItemActor   待放置的物体本身
     * @param Margin      占用矩形外扩的安全间距，单位 cm
     * @return Item 应被移动到的世界坐标
     */
    static FVector ComputePlacementWorldLocation(
        AActor& TargetActor,
        AActor& ItemActor,
        float Margin = 5.f);

    /**
     * 查找当前坐落在 TargetActor 顶面上的所有可拾取物体。
     *
     * 用于运输/抬起时把"乘客"物体一并跟随承载者移动；判定条件与
     * ComputePlacementWorldLocation 中的占用判定一致：
     * - 物体实现 IMAPickupItem 接口
     * - 当前未被任何机器人携带
     * - 物体底面 Z 与 TargetActor 顶面 Z 接近（80cm 容差）
     * - 物体中心在 TargetActor 的 XY 范围内
     *
     * 返回的物体集合不包含 TargetActor 自身。
     */
    static TArray<AActor*> CollectItemsOnSurface(AActor& TargetActor);
};
