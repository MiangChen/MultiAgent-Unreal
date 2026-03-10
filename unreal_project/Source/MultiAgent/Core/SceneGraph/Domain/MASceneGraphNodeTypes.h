// MASceneGraphNodeTypes.h
// 场景图节点基础类型定义

#pragma once

#include "CoreMinimal.h"
#include "MASceneGraphNodeTypes.generated.h"

/**
 * 场景图节点数据结构
 *
 * 支持三种形状类型:
 * - point: 单个 Actor，使用 Guid 字段和 shape.center
 * - polygon: 多个 Actor 组成的多边形，使用 GuidArray 和 shape.vertices
 * - linestring: 多个 Actor 组成的线串，使用 GuidArray 和 shape.points
 *
 * 支持的节点类别:
 * - building: 建筑物
 * - trans_facility: 交通设施 (道路、路口)
 * - prop: 道具 (雕像、天线、水塔等)
 * - robot: 机器人 (UAV, UGV, Quadruped, Humanoid)
 */
USTRUCT(BlueprintType)
struct FMASceneGraphNode
{
    GENERATED_BODY()

    //=========================================================================
    // 基础字段
    //=========================================================================

    /** 节点唯一标识 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Id;

    /** Actor 的全局唯一标识符 (通过 Actor->GetActorGuid().ToString() 获取) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Guid;

    /** 节点类型 (intersection, building, robot, etc.) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Type;

    /** 自动生成的标签 (Intersection-12, UAV-1, RedBox) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Label;

    /** 世界坐标 [x, y, z] - 对于 polygon/linestring 类型，这是计算出的几何中心 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FVector Center;

    /** 形状类型: "point", "polygon", "linestring", "prism" */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString ShapeType;

    /** 多个 Actor 的 GUID 数组 (用于 polygon/linestring 类型) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    TArray<FString> GuidArray;

    /** 原始 JSON 字符串 (用于预览显示) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString RawJson;

    //=========================================================================
    // 分类与动态字段
    //=========================================================================

    /** 节点类别: building, trans_facility, prop, robot */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Category;

    /** 是否为动态节点 (机器人、可移动物体等) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    bool bIsDynamic = false;

    /** 旋转角度 (仅动态节点) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FRotator Rotation;

    //=========================================================================
    // PickupItem 专用字段
    //=========================================================================

    /** 特征属性: color, name, item_type 等 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    TMap<FString, FString> Features;

    /** 是否被携带 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    bool bIsCarried = false;

    /** 携带者 ID (机器人 ID) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString CarrierId;

    //=========================================================================
    // 位置标签
    //=========================================================================

    /** 所在地点标签（如Building-3, Intersection-1） */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString LocationLabel;

    FMASceneGraphNode()
        : Center(FVector::ZeroVector)
        , bIsDynamic(false)
        , Rotation(FRotator::ZeroRotator)
        , bIsCarried(false)
    {
    }

    //=========================================================================
    // 辅助方法
    //=========================================================================

    bool IsRobot() const { return Category == TEXT("robot"); }
    bool IsPickupItem() const { return Type == TEXT("cargo") || Type == TEXT("assembly_component"); }
    bool IsChargingStation() const { return Type == TEXT("charging_station"); }
    bool IsBuilding() const { return Category == TEXT("building") || Type == TEXT("building"); }
    bool IsRoad() const { return Type == TEXT("road_segment") || Type == TEXT("street_segment"); }
    bool IsIntersection() const { return Type == TEXT("intersection"); }
    bool IsProp() const { return Category == TEXT("prop"); }
    bool IsTransFacility() const { return Category == TEXT("trans_facility"); }
    bool IsValid() const { return !Id.IsEmpty(); }
    FString GetDisplayName() const { return Label.IsEmpty() ? Id : Label; }
};
