// MAWorldQuery.h
// 世界状态查询工具 - 提供查询 UE5 世界实体状态的接口

#pragma once

#include "CoreMinimal.h"
#include "MAWorldQuery.generated.h"

/**
 * 实体节点 - 表示世界中的一个实体
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAEntityNode
{
    GENERATED_BODY()

    /** 实体 ID */
    UPROPERTY(BlueprintReadOnly)
    FString Id;

    /** 实体类别: robot, building, prop, area, trans_facility */
    UPROPERTY(BlueprintReadOnly)
    FString Category;

    /** 实体类型: UAV, UGV, house, road 等 */
    UPROPERTY(BlueprintReadOnly)
    FString Type;

    /** 实体属性 */
    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Properties;

    FMAEntityNode() {}

    /** 序列化为 JSON 对象 */
    TSharedPtr<FJsonObject> ToJsonObject() const;
};

/**
 * 边界特征 - 表示实体的边界/轮廓
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMABoundaryFeature
{
    GENERATED_BODY()

    /** 实体标签/名称 */
    UPROPERTY(BlueprintReadOnly)
    FString Label;

    /** 边界类型: area, line, point, rectangle, circle */
    UPROPERTY(BlueprintReadOnly)
    FString Kind;

    /** 边界坐标 (用于 area, line, point, rectangle) */
    UPROPERTY(BlueprintReadOnly)
    TArray<FVector2D> Coords;

    /** 圆心 (用于 circle) */
    UPROPERTY(BlueprintReadOnly)
    FVector2D Center = FVector2D::ZeroVector;

    /** 半径 (用于 circle) */
    UPROPERTY(BlueprintReadOnly)
    float Radius = 0.f;

    FMABoundaryFeature() {}

    /** 序列化为 JSON 对象 */
    TSharedPtr<FJsonObject> ToJsonObject() const;
};

/**
 * 世界状态查询结果
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldQueryResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FMAEntityNode> Entities;

    UPROPERTY(BlueprintReadOnly)
    TArray<FMABoundaryFeature> Boundaries;

    /** 序列化为 JSON */
    FString ToJson() const;
};

/**
 * 世界状态查询工具
 * 
 * 提供查询 UE5 世界实体状态的静态方法
 */
class MULTIAGENT_API FMAWorldQuery
{
public:
    /**
     * 获取所有实体节点
     * @param World UE5 世界
     * @return 实体节点列表
     */
    static TArray<FMAEntityNode> GetAllEntities(UWorld* World);

    /**
     * 获取所有机器人实体
     * @param World UE5 世界
     * @return 机器人实体节点列表
     */
    static TArray<FMAEntityNode> GetAllRobots(UWorld* World);

    /**
     * 获取所有道具实体
     * @param World UE5 世界
     * @return 道具实体节点列表
     */
    static TArray<FMAEntityNode> GetAllProps(UWorld* World);

    /**
     * 获取所有场景物体（建筑、街道等静态网格体）
     * @param World UE5 世界
     * @return 场景物体节点列表
     */
    static TArray<FMAEntityNode> GetAllSceneObjects(UWorld* World);

    /**
     * 获取边界特征
     * @param World UE5 世界
     * @param Categories 要查询的类别 (空则查询所有)
     * @return 边界特征列表
     */
    static TArray<FMABoundaryFeature> GetBoundaryFeatures(UWorld* World, const TArray<FString>& Categories = TArray<FString>());

    /**
     * 根据 ID 获取实体节点
     * @param World UE5 世界
     * @param EntityId 实体 ID
     * @return 实体节点 (如果找到)
     */
    static FMAEntityNode GetEntityById(UWorld* World, const FString& EntityId);

    /**
     * 根据名称获取实体节点
     * @param World UE5 世界
     * @param EntityName 实体名称 (properties.name)
     * @return 实体节点 (如果找到)
     */
    static FMAEntityNode GetEntityByName(UWorld* World, const FString& EntityName);

    /**
     * 获取完整的世界状态
     * @param World UE5 世界
     * @return 包含所有实体和边界的查询结果
     */
    static FMAWorldQueryResult GetWorldState(UWorld* World);

private:
    /** 从 Actor 提取实体节点 */
    static FMAEntityNode ExtractEntityNode(AActor* Actor);
    
    /** 从 Actor 提取边界特征 */
    static FMABoundaryFeature ExtractBoundaryFeature(AActor* Actor);
    
    /** 获取 Actor 的俯视轮廓 */
    static TArray<FVector2D> GetActorFootprint(AActor* Actor);
};
