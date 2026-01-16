// MASceneGraphManager.h
// 场景图管理器 - 负责场景标签数据的解析、验证和持久化

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphTypes.h"
#include "MASceneGraphManager.generated.h"

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
 * - pickup_item: 可拾取物品
 * - charging_station: 充电站
 */
USTRUCT(BlueprintType)
struct FMASceneGraphNode
{
    GENERATED_BODY()

    //=========================================================================
    // 基础字段 (现有)
    //=========================================================================

    /** 节点唯一标识 */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Id;

    /** Actor 的全局唯一标识符 (通过 Actor->GetActorGuid().ToString() 获取) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Guid;

    /** 节点类型 (intersection, building, robot, pickup_item, etc.) */
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
    // 新增字段 - 分类
    //=========================================================================

    /** 节点类别: building, trans_facility, prop, robot */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString Category;

    //=========================================================================
    // 新增字段 - 动态节点专用
    //=========================================================================

    /** 是否为动态节点 (机器人、可移动物体等) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    bool bIsDynamic = false;

    /** 旋转角度 (仅动态节点) */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FRotator Rotation;

    //=========================================================================
    // 新增字段 - PickupItem 专用
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
    // 新增字段 - 位置标签
    //=========================================================================

    /** 所在地点标签（如Building-3, Intersection-1）
     *  对于非地点节点（机器人、道具、货物等），表示其当前所在的最近地点
     */
    UPROPERTY(BlueprintReadWrite, Category = "SceneGraph")
    FString LocationLabel;

    //=========================================================================
    // 构造函数
    //=========================================================================

    FMASceneGraphNode()
        : Center(FVector::ZeroVector)
        , bIsDynamic(false)
        , Rotation(FRotator::ZeroRotator)
        , bIsCarried(false)
    {
    }

    //=========================================================================
    // 辅助方法 - 类型判断
    //=========================================================================

    /** 是否为机器人节点 */
    bool IsRobot() const { return Category == TEXT("robot"); }

    /** 是否为可拾取物品节点 (Type == "cargo") */
    bool IsPickupItem() const { return Type == TEXT("cargo"); }

    /** 是否为充电站节点 */
    bool IsChargingStation() const { return Type == TEXT("charging_station"); }

    /** 是否为建筑物节点 */
    bool IsBuilding() const { return Category == TEXT("building") || Type == TEXT("building"); }

    /** 是否为道路节点 */
    bool IsRoad() const { return Type == TEXT("road_segment") || Type == TEXT("street_segment"); }

    /** 是否为路口节点 */
    bool IsIntersection() const { return Type == TEXT("intersection"); }

    /** 是否为道具节点 */
    bool IsProp() const { return Category == TEXT("prop"); }

    /** 是否为交通设施节点 (道路或路口) */
    bool IsTransFacility() const { return Category == TEXT("trans_facility"); }

    /** 节点是否有效 (至少有 ID) */
    bool IsValid() const { return !Id.IsEmpty(); }

    /** 获取显示名称 (优先使用 Label，否则使用 Id) */
    FString GetDisplayName() const { return Label.IsEmpty() ? Id : Label; }
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
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool AddMultiSelectNode(const FString& JsonString, FString& OutErrorMessage);

    /**
     * 生成标签
     * 
     * @param Type 节点类型
     * @return 生成的标签，格式: "Type-N"
     * 
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    FString GenerateLabel(const FString& Type) const;

    /**
     * 获取指定类型的节点数量
     * 
     * @param Type 节点类型
     * @return 该类型的节点数量
     * 
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    int32 GetTypeCount(const FString& Type) const;

    /**
     * 获取场景图文件路径
     * 
     * @return JSON 文件的完整路径
     * 
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    FString GetSceneGraphFilePath() const;

    /**
     * 获取所有场景节点
     * 
     * @return 所有节点的数组
     * 
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    TArray<FMASceneGraphNode> GetAllNodes() const;

    //=========================================================================
    // 分类查询接口 (委托给 MASceneGraphQuery)
    //=========================================================================

    /**
     * 获取所有建筑物节点
     * @return 建筑物节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllBuildings() const;

    /**
     * 获取所有道路节点
     * @return 道路节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllRoads() const;

    /**
     * 获取所有路口节点
     * @return 路口节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllIntersections() const;

    /**
     * 获取所有道具节点
     * @return 道具节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllProps() const;

    /**
     * 获取所有机器人节点
     * @return 机器人节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllRobots() const;

    /**
     * 获取所有可拾取物品节点
     * @return 可拾取物品节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllPickupItems() const;

    /**
     * 获取所有充电站节点
     * @return 充电站节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllChargingStations() const;

    //=========================================================================
    // 语义查询接口 (委托给 MASceneGraphQuery)
    //=========================================================================

    /**
     * 根据语义标签查找节点
     * @param Label 语义标签
     * @return 第一个匹配的节点，如果未找到则返回无效节点
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    FMASceneGraphNode FindNodeByLabel(const FMASemanticLabel& Label) const;

    /**
     * 查找最近的匹配节点
     * @param Label 语义标签
     * @param FromLocation 起始位置
     * @return 最近的匹配节点，如果未找到则返回无效节点
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    FMASceneGraphNode FindNearestNode(const FMASemanticLabel& Label, const FVector& FromLocation) const;

    /**
     * 在边界内查找匹配节点
     * @param Label 语义标签
     * @param BoundaryVertices 边界多边形顶点
     * @return 在边界内且匹配标签的节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> FindNodesInBoundary(const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices) const;

    /**
     * 判断点是否在任意建筑物内部
     * @param Point 要检查的点
     * @return 是否在建筑物内部
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    bool IsPointInsideBuilding(const FVector& Point) const;

    /**
     * 查找最近的地标
     * @param Location 起始位置
     * @param MaxDistance 最大搜索距离
     * @return 最近的地标节点，如果未找到则返回无效节点
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    FMASceneGraphNode FindNearestLandmark(const FVector& Location, float MaxDistance = 1000.f) const;

    //=========================================================================
    // 动态节点管理接口
    //=========================================================================

    /**
     * 更新机器人位置
     * @param RobotId 机器人ID
     * @param NewPosition 新位置
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    void UpdateRobotPosition(const FString& RobotId, const FVector& NewPosition);

    /**
     * 更新可拾取物品位置
     * @param ItemId 物品ID
     * @param NewPosition 新位置
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    void UpdatePickupItemPosition(const FString& ItemId, const FVector& NewPosition);

    /**
     * 更新可拾取物品携带状态
     * @param ItemId 物品ID
     * @param bIsCarried 是否被携带
     * @param CarrierId 携带者ID
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    void UpdatePickupItemCarrierStatus(const FString& ItemId, bool bIsCarried, const FString& CarrierId = TEXT(""));

    /**
     * 根据 Actor GUID 查找包含该 GUID 的所有节点
     * 
     * 遍历所有节点，检查 GuidArray 是否包含目标 GUID，
     * 返回所有匹配的节点。支持一个 Actor 属于多个节点的情况。
     * 
     * @param ActorGuid Actor 的 GUID 字符串 (通过 Actor->GetActorGuid().ToString() 获取)
     * @return 包含该 GUID 的所有节点数组，如果未找到则返回空数组
     * 
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    TArray<FMASceneGraphNode> FindNodesByGuid(const FString& ActorGuid) const;

    //=========================================================================
    // HTTP API 支持 - 世界状态查询
    //=========================================================================

    /**
     * 将场景图节点转换为JSON对象
     * 
     * 根据节点类型正确构建shape对象:
     * - prism (building): type, vertices, height
     * - linestring (street): type, points, vertices
     * - point (intersection): type, center, vertices
     * - point (prop): type, center
     * - point (robot/dynamic): type, center
     * 
     * @param Node 场景图节点
     * @return JSON对象
     * 
     */
    TSharedPtr<FJsonObject> NodeToJsonObject(const FMASceneGraphNode& Node) const;

    /**
     * 构建世界状态JSON响应
     * 
     * @param CategoryFilter 类别过滤条件 (可选)
     * @param TypeFilter 类型过滤条件 (可选)
     * @param LabelFilter 标签过滤条件 (可选)
     * @return JSON字符串，包含nodes和edges数组
     * 
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|API")
    FString BuildWorldStateJson(const FString& CategoryFilter = TEXT(""), const FString& TypeFilter = TEXT(""), const FString& LabelFilter = TEXT("")) const;

private:
    //=========================================================================
    // JSON 文件 I/O
    //=========================================================================

    /**
     * 加载场景图数据
     * 
     * @return 加载是否成功
     * 
     */
    bool LoadSceneGraph();

    /**
     * 保存场景图数据
     * 
     * @return 保存是否成功
     * 
     */
    bool SaveSceneGraph();

    /**
     * 创建空的场景图结构
     * 
     */
    void CreateEmptySceneGraph();

    //=========================================================================
    // 动态节点加载
    //=========================================================================

    /**
     * 加载动态节点 (机器人、可拾取物品、充电站)
     * 从 agents.json 和 environment.json 加载
     */
    void LoadDynamicNodes();

    //=========================================================================
    // 缓存管理
    //=========================================================================

    /**
     * 使缓存失效
     * 当静态节点或动态节点发生变化时调用
     */
    void InvalidateCache();

    /**
     * 重建缓存
     * 合并静态节点和动态节点到 CachedAllNodes
     */
    void RebuildCache() const;

    //=========================================================================
    // 内部辅助方法
    //=========================================================================

    /**
     * 将类型字符串首字母大写
     * 
     * @param Type 原始类型字符串
     * @return 首字母大写的类型字符串
     * 
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
     */
    bool ValidateNodeJsonStructure(const TSharedPtr<FJsonObject>& NodeObject, FString& OutErrorMessage) const;

    /**
     * 在动态节点数组中查找节点
     * @param NodeId 节点ID
     * @return 节点指针，如果未找到则返回 nullptr
     */
    FMASceneGraphNode* FindDynamicNodeById(const FString& NodeId);

    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 内存中的场景图数据 (原始 JSON) */
    TSharedPtr<FJsonObject> SceneGraphData;

    /** 静态节点 (从 JSON 加载) */
    TArray<FMASceneGraphNode> StaticNodes;

    /** 动态节点 (运行时创建) */
    TArray<FMASceneGraphNode> DynamicNodes;

    /** 合并后的所有节点缓存 */
    mutable TArray<FMASceneGraphNode> CachedAllNodes;

    /** 缓存是否有效 */
    mutable bool bCacheValid = false;

    /** 场景图文件名 */
    static const FString SceneGraphFileName;
};
