// MAUESceneQuery.h
// 场景查询辅助工具 - 根据语义标签查找场景对象
//
// 本模块提供基于 UE5 场景的对象查询功能，作为场景图查询的回退方案
// 主要查询应优先使用 MASceneGraphQuery，本模块用于:
// 1. 场景图中未找到对象时的回退查询
// 2. 需要获取实际 Actor 引用时的查询
//

#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Types/MASceneGraphTypes.h"
#include "../../../Environment/IMAEnvironmentObject.h"

class AActor;
class AMACharacter;

// 查询结果
struct FMAUESceneQueryResult
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
 */
class MULTIAGENT_API FMAUESceneQuery
{
public:
    //=========================================================================
    // GUID 查找接口 (通用)
    //=========================================================================
    
    /**
     * 根据 GUID 查找 Actor
     * 
     * 遍历场景中所有 Actor，查找 GUID 匹配的 Actor
     * 这是将场景图节点关联到实际 Actor 的核心方法
     * 
     * @param World 世界指针
     * @param Guid Actor 的 GUID 字符串 (通过 Actor->GetActorGuid().ToString() 获取)
     * @return 找到的 Actor，未找到返回 nullptr
     */
    static AActor* FindActorByGuid(UWorld* World, const FString& Guid);
    
    /**
     * 根据 GUID 数组查找多个 Actor
     * 
     * @param World 世界指针
     * @param GuidArray GUID 字符串数组
     * @return 找到的 Actor 数组
     */
    static TArray<AActor*> FindActorsByGuidArray(UWorld* World, const TArray<FString>& GuidArray);

    //=========================================================================
    // 语义标签查找接口
    //=========================================================================
    
    /**
     * 根据语义标签查找场景中的对象
     * 
     * @param World 世界指针
     * @param Label 语义标签
     * @return 查询结果
     */
    static FMAUESceneQueryResult FindObjectByLabel(UWorld* World, const FMASemanticLabel& Label);
    
    /**
     * 查找边界内的所有匹配对象
     * 
     * @param World 世界指针
     * @param Label 语义标签
     * @param BoundaryVertices 边界多边形顶点
     * @return 查询结果数组
     */
    static TArray<FMAUESceneQueryResult> FindObjectsInBoundary(UWorld* World, const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices);
    
    /**
     * 查找最近的匹配对象
     * 
     * @param World 世界指针
     * @param Label 语义标签
     * @param FromLocation 起始位置
     * @return 查询结果
     */
    static FMAUESceneQueryResult FindNearestObject(UWorld* World, const FMASemanticLabel& Label, const FVector& FromLocation);
    
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
     * 根据标签查找机器人 (回退方法)
     * 
     * 当场景图中未找到机器人时，使用此方法从 UE5 场景中查找
     * 
     * @param World 世界指针
     * @param RobotLabel 机器人标签 (如 "UAV-1", "UGV-2")
     * @return 找到的机器人 Actor，未找到返回 nullptr
     */
    static AMACharacter* FindRobotByLabel(UWorld* World, const FString& RobotLabel);
};
