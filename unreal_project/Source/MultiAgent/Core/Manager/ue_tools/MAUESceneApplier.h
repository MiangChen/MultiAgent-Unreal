// MAUESceneApplier.h
// UE 场景应用器 - 将场景图节点变更同步到 UE 场景中的 Actor
//
// 与 MASceneGraphUpdater 互为镜像:
// - MASceneGraphUpdater: UE 场景 → 场景图 (技能完成后更新节点数据)
// - MAUESceneApplier:    场景图 → UE 场景 (节点编辑后同步 Actor 状态)
//
// 使用场景:
// - Edit/Modify 模式下用户编辑了节点 JSON 后，将变更应用到对应的 Actor
// - 初始化后将 Actor 的实际位置回写到场景图节点 (解决配置文件 Z 不准确)
// - 支持位置、旋转、标签等属性的同步

#pragma once

#include "CoreMinimal.h"

struct FMASceneGraphNode;

/**
 * UE 场景应用器
 * 
 * 职责:
 * - 根据场景图节点数据，更新 UE 场景中对应 Actor 的状态
 * - 通过 GUID 桥接场景图节点与 UE Actor
 * - 提供地面投影能力，确保移动后的 Actor 贴合地面
 * - 将 Actor 的实际位置回写到场景图节点 (初始化校准)
 * 
 * 设计原则:
 * - 纯静态工具类，无状态
 * - 对不同类型的 Actor (MACharacter, IMAEnvironmentObject, 普通 AActor) 统一处理
 */
class MULTIAGENT_API FMAUESceneApplier
{
public:
    //=========================================================================
    // 统一入口
    //=========================================================================

    /**
     * 将场景图节点的变更应用到 UE 场景中对应的 Actor
     * 位置变更会自动进行地面投影，确保 Actor 不会悬空或嵌入地下
     * 
     * @param World 世界指针
     * @param Node 变更后的场景图节点
     * @return 成功应用变更的 Actor 数量
     */
    static int32 ApplyNodeToScene(UWorld* World, const FMASceneGraphNode& Node);

    //=========================================================================
    // 细粒度接口
    //=========================================================================

    /**
     * 将位置同步到 Actor (自动地面投影)
     * 使用节点提供的 XY 坐标，通过射线检测确定正确的 Z 坐标
     * 
     * @param World 世界指针
     * @param ActorGuid Actor 的 GUID
     * @param NewLocation 新的世界坐标 (Z 分量会被地面投影覆盖)
     * @return 是否成功
     */
    static bool ApplyPosition(UWorld* World, const FString& ActorGuid, const FVector& NewLocation);

    /**
     * 将旋转同步到 Actor
     */
    static bool ApplyRotation(UWorld* World, const FString& ActorGuid, const FRotator& NewRotation);

    /**
     * 将标签同步到 Actor (MACharacter 的 AgentLabel 或 ActorLabel)
     */
    static bool ApplyLabel(UWorld* World, const FString& ActorGuid, const FString& NewLabel);

    //=========================================================================
    // 批量接口
    //=========================================================================

    /**
     * 对比新旧节点，仅应用差异部分到 UE 场景
     */
    static int32 ApplyNodeDiff(UWorld* World, const FMASceneGraphNode& OldNode, const FMASceneGraphNode& NewNode);

    //=========================================================================
    // 场景图校准 (Actor → 场景图)
    //=========================================================================

    /**
     * 将 Actor 的实际位置回写到场景图动态节点
     * 
     * 用于初始化后校准：配置文件中的坐标 (尤其是 Z) 可能与 Actor 实际
     * 被放置到场景后的位置不同 (因为 Spawn 时会做地面投影和碰撞调整)。
     * 调用此方法可将 Actor 的真实位置同步回场景图节点。
     * 
     * @param World 世界指针
     * @param Node 要校准的动态节点 (会被修改)
     * @return 是否成功校准
     */
    static bool CalibrateNodeFromActor(UWorld* World, FMASceneGraphNode& Node);

    //=========================================================================
    // 地面投影工具
    //=========================================================================

    /**
     * 将世界坐标投影到地面 (使用固定偏移)
     * 从 (X, Y, 10000) 向下射线检测到 (X, Y, -20000)，找到地面碰撞点
     * 
     * @param World 世界指针
     * @param Location 输入坐标 (仅使用 XY，Z 会被替换)
     * @param HeightOffset 地面之上的偏移量
     * @return 投影后的坐标，如果射线未命中则返回原始坐标
     */
    static FVector SnapToGround(UWorld* World, const FVector& Location, float HeightOffset = 5.f);

    /**
     * 将 Actor 投影到地面 (根据 Actor 类型自动计算偏移)
     * - MACharacter: 使用胶囊体半高
     * - 其他 Actor: 使用边界框底部到原点的距离
     * 
     * @param World 世界指针
     * @param Actor 要投影的 Actor (用于计算偏移)
     * @param Location 目标 XY 坐标
     * @return 投影后的坐标
     */
    static FVector SnapActorToGround(UWorld* World, AActor* Actor, const FVector& Location);

    /**
     * 计算 Actor 原点到底部的偏移量
     * - MACharacter: 胶囊体半高
     * - 其他 Actor: 边界框底部到原点的距离
     */
    static float CalculateGroundOffset(AActor* Actor);

private:
    static AActor* FindActorByGuid(UWorld* World, const FString& Guid);
    static bool ApplyNodeToActor(AActor* Actor, const FMASceneGraphNode& Node, UWorld* World);
};
