// MASceneGraphManager.h
// 场景图管理器 - 负责场景标签数据的解析、验证和持久化
// Requirements: 1.1, 1.2, 1.3, 1.4, 2.1, 2.2, 2.3, 2.4, 3.1, 3.3, 4.1, 4.2, 4.3, 4.4, 6.1, 6.2, 6.3, 6.4, 6.5, 7.1, 7.2, 7.3, 7.4, 8.1

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphManager.generated.h"

/**
 * 场景图节点数据结构
 * Requirements: 7.1, 7.2, 7.3, 7.4
 * 
 * 支持三种形状类型:
 * - point: 单个 Actor，使用 Guid 字段和 shape.center
 * - polygon: 多个 Actor 组成的多边形，使用 GuidArray 和 shape.vertices
 * - linestring: 多个 Actor 组成的线串，使用 GuidArray 和 shape.points
 */
USTRUCT(BlueprintType)
struct FMASceneGraphNode
{
    GENERATED_BODY()

    /** 节点唯一标识 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Id;

    /** Actor 的全局唯一标识符 (通过 Actor->GetActorGuid().ToString() 获取) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Guid;

    /** 节点类型 (intersection, building, etc.) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Type;

    /** 自动生成的标签 (Intersection-12) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Label;

    /** 世界坐标 [x, y, z] - 对于 polygon/linestring 类型，这是计算出的几何中心 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FVector Center;

    /** 形状类型: "point", "polygon", "linestring" */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString ShapeType;

    /** 多个 Actor 的 GUID 数组 (用于 polygon/linestring 类型) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    TArray<FString> GuidArray;

    /** 原始 JSON 字符串 (用于预览显示) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString RawJson;

    FMASceneGraphNode()
        : Center(FVector::ZeroVector)
    {
    }
};

/**
 * 场景图管理器
 * 
 * 职责:
 * - 解析用户输入的标签字符串 (id:值,type:值 格式)
 * - 验证输入数据的有效性
 * - 检查 ID 是否重复
 * - 自动生成标签 (Type-N 格式)
 * - 将场景节点持久化到 JSON 文件
 * 
 * Requirements: 8.1 - 实现为 UGameInstanceSubsystem 以便全局访问
 */
UCLASS()
class MULTIAGENT_API UMASceneGraphManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 生命周期
    //=========================================================================

    /** 初始化子系统 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    /** 反初始化子系统 */
    virtual void Deinitialize() override;

    //=========================================================================
    // 核心 API
    //=========================================================================

    /**
     * 解析标签输入字符串
     * 
     * @param InputText 输入文本，格式: "id:值,type:值"
     * @param OutId 解析出的 ID
     * @param OutType 解析出的 Type
     * @param OutErrorMessage 错误信息（如果解析失败）
     * @return 解析是否成功
     * 
     * Requirements: 1.1, 1.2, 1.3, 1.4
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool ParseLabelInput(const FString& InputText, FString& OutId, FString& OutType, FString& OutErrorMessage);

    /**
     * 添加场景节点
     * 
     * @param Id 节点 ID
     * @param Type 节点类型
     * @param WorldLocation Actor 世界坐标
     * @param Actor 源 Actor (用于提取 GUID)
     * @param OutLabel 生成的标签
     * @param OutErrorMessage 错误信息
     * @return 添加是否成功
     * 
     * Requirements: 3.1, 3.3, 6.3, 7.1, 7.2, 7.3, 7.4
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool AddSceneNode(const FString& Id, const FString& Type, const FVector& WorldLocation,
                      AActor* Actor, FString& OutLabel, FString& OutErrorMessage);

    /**
     * 检查 ID 是否已存在
     * 
     * @param Id 要检查的 ID
     * @return ID 是否已存在
     * 
     * Requirements: 3.1
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    bool IsIdExists(const FString& Id) const;

    /**
     * 添加多选节点 (Polygon/LineString)
     * 
     * @param JsonString 生成的 JSON 字符串 (包含 id, Guid[], properties, shape 等)
     * @param OutErrorMessage 错误信息
     * @return 添加是否成功
     * 
     * Requirements: 4.1, 4.2, 5.1, 5.2, 8.1
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool AddMultiSelectNode(const FString& JsonString, FString& OutErrorMessage);

    /**
     * 生成标签
     * 
     * @param Type 节点类型
     * @return 生成的标签，格式: "Type-N"
     * 
     * Requirements: 4.1, 4.2, 4.3, 4.4
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    FString GenerateLabel(const FString& Type) const;

    /**
     * 获取指定类型的节点数量
     * 
     * @param Type 节点类型
     * @return 该类型的节点数量
     * 
     * Requirements: 4.3
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    int32 GetTypeCount(const FString& Type) const;

    /**
     * 获取场景图文件路径
     * 
     * @return JSON 文件的完整路径
     * 
     * Requirements: 6.1
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    FString GetSceneGraphFilePath() const;

    /**
     * 获取所有场景节点
     * 
     * @return 所有节点的数组
     * 
     * Requirements: 3.4, 6.1, 6.2
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    TArray<FMASceneGraphNode> GetAllNodes() const;

    /**
     * 根据 Actor GUID 查找包含该 GUID 的所有节点
     * 
     * 遍历所有节点，检查 GuidArray 是否包含目标 GUID，
     * 返回所有匹配的节点。支持一个 Actor 属于多个节点的情况。
     * 
     * @param ActorGuid Actor 的 GUID 字符串 (通过 Actor->GetActorGuid().ToString() 获取)
     * @return 包含该 GUID 的所有节点数组，如果未找到则返回空数组
     * 
     * Requirements: 2.1, 2.2, 2.4
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    TArray<FMASceneGraphNode> FindNodesByGuid(const FString& ActorGuid) const;

    /**
     * 计算多边形顶点的几何中心 (算术平均值)
     * 
     * @param Vertices 顶点数组 (每个顶点为 [x, y, z] 的 JSON 数组)
     * @return 几何中心点
     * 
     * Requirements: 1.1
     */
    static FVector CalculatePolygonCentroid(const TArray<TSharedPtr<FJsonValue>>& Vertices);

    /**
     * 计算线串端点的几何中心 (算术平均值)
     * 
     * @param Points 端点数组 (每个点为 [x, y, z] 的 JSON 数组)
     * @return 几何中心点
     * 
     * Requirements: 1.2
     */
    static FVector CalculateLineStringCentroid(const TArray<TSharedPtr<FJsonValue>>& Points);

private:
    //=========================================================================
    // JSON 文件 I/O
    //=========================================================================

    /**
     * 加载场景图数据
     * 
     * @return 加载是否成功
     * 
     * Requirements: 6.1, 6.2
     */
    bool LoadSceneGraph();

    /**
     * 保存场景图数据
     * 
     * @return 保存是否成功
     * 
     * Requirements: 6.4
     */
    bool SaveSceneGraph();

    /**
     * 创建空的场景图结构
     * 
     * Requirements: 6.2
     */
    void CreateEmptySceneGraph();

    //=========================================================================
    // 内部辅助方法
    //=========================================================================

    /**
     * 将类型字符串首字母大写
     * 
     * @param Type 原始类型字符串
     * @return 首字母大写的类型字符串
     * 
     * Requirements: 4.2
     */
    FString CapitalizeFirstLetter(const FString& Type) const;

    /**
     * 从 JSON 对象中提取节点数组
     * 
     * @return 节点数组指针，如果不存在则返回 nullptr
     */
    TArray<TSharedPtr<FJsonValue>>* GetNodesArray() const;

    /**
     * 验证场景图节点 JSON 结构
     * 
     * 根据 shape.type 验证必需字段:
     * - prism: 需要 shape.vertices 和 shape.height
     * - linestring: 需要 shape.points 和 shape.vertices
     * - point: 需要 shape.center
     * 
     * @param NodeObject 要验证的 JSON 对象
     * @param OutErrorMessage 验证失败时的错误信息
     * @return 验证是否通过
     * 
     * Requirements: 7.4, 8.3, 8.4
     */
    bool ValidateNodeJsonStructure(const TSharedPtr<FJsonObject>& NodeObject, FString& OutErrorMessage) const;

    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 内存中的场景图数据 */
    TSharedPtr<FJsonObject> SceneGraphData;

    /** 场景图文件名 */
    static const FString SceneGraphFileName;
};
