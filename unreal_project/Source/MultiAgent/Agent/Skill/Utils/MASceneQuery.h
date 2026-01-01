// MASceneQuery.h
// 场景查询辅助工具 - 根据语义标签查找场景对象
//
// 本模块提供基于 UE5 场景的对象查询功能，作为场景图查询的回退方案
// 主要查询应优先使用 MASceneGraphQuery，本模块用于:
// 1. 场景图中未找到对象时的回退查询
// 2. 需要获取实际 Actor 引用时的查询
//
// Requirements: 2.4

#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Types/MASceneGraphTypes.h"

class AActor;
class AMAPickupItem;
class AMACharacter;

// 查询结果
struct FMASceneQueryResult
{
    AActor* Actor = nullptr;
    FString Name;
    FVector Location = FVector::ZeroVector;
    FMASemanticLabel Label;
    bool bFound = false;
};

/**
 * 场景查询工具类
 * 
 * 提供基于 UE5 场景的对象查询功能
 * 作为 MASceneGraphQuery 的回退方案
 * 
 * Requirements: 2.4
 */
class MULTIAGENT_API FMASceneQuery
{
public:
    /**
     * 根据语义标签查找场景中的对象
     * 
     * @param World 世界指针
     * @param Label 语义标签
     * @return 查询结果
     */
    static FMASceneQueryResult FindObjectByLabel(UWorld* World, const FMASemanticLabel& Label);
    
    /**
     * 查找边界内的所有匹配对象
     * 
     * @param World 世界指针
     * @param Label 语义标签
     * @param BoundaryVertices 边界多边形顶点
     * @return 查询结果数组
     */
    static TArray<FMASceneQueryResult> FindObjectsInBoundary(UWorld* World, const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices);
    
    /**
     * 查找最近的匹配对象
     * 
     * @param World 世界指针
     * @param Label 语义标签
     * @param FromLocation 起始位置
     * @return 查询结果
     */
    static FMASceneQueryResult FindNearestObject(UWorld* World, const FMASemanticLabel& Label, const FVector& FromLocation);
    
    /**
     * 检查对象是否匹配语义标签
     * 
     * @param Actor 要检查的 Actor
     * @param Label 语义标签
     * @return 是否匹配
     */
    static bool MatchesLabel(AActor* Actor, const FMASemanticLabel& Label);
    
    /**
     * 从 Actor 提取语义标签
     * 
     * @param Actor 源 Actor
     * @return 提取的语义标签
     */
    static FMASemanticLabel ExtractLabel(AActor* Actor);
    
    /**
     * 查找机器人 (回退方法)
     * 
     * 当场景图中未找到机器人时，使用此方法从 UE5 场景中查找
     * 
     * @param World 世界指针
     * @param RobotName 机器人名称
     * @return 找到的机器人 Actor，未找到返回 nullptr
     */
    static AMACharacter* FindRobotByName(UWorld* World, const FString& RobotName);

private:
    /**
     * 检查 PickupItem 是否匹配语义标签
     * 
     * 匹配策略:
     * 1. 如果 features 中有 "name"，直接用 name 匹配 ItemName
     * 2. 如果 features 中有 "color"，检查 ItemName 是否包含该颜色
     * 3. 如果有 type，检查 ItemName 是否包含该类型
     * 
     * @param Item 要检查的 PickupItem
     * @param Label 语义标签
     * @return 是否匹配
     */
    static bool MatchesPickupItemLabel(AMAPickupItem* Item, const FMASemanticLabel& Label);
    
    /**
     * 从 PickupItem 提取语义标签
     * 
     * @param Item 源 PickupItem
     * @return 提取的语义标签
     */
    static FMASemanticLabel ExtractPickupItemLabel(AMAPickupItem* Item);
};
