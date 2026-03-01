// MASceneGraphManager.h
// 场景图管理器 - 负责场景标签数据的解析、验证和持久化
//
// 重构后的架构:
// - 单一数据源: WorkingCopy 是场景图数据的唯一内存副本
// - 模式感知保存: 只有 Modify 模式才会将修改持久化到源文件
// - 统一图操作: AddNode/DeleteNode/EditNode 是所有图操作的统一入口

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphNodeTypes.h"
#include "MASceneGraphTypes.h"
#include "../Config/MAConfigManager.h"
#include "MASceneGraphManager.generated.h"

class IMASceneGraphQueryPort;
class IMASceneGraphCommandPort;
class IMASceneGraphRepositoryPort;
class IMASceneGraphEventPublisherPort;
class FMASceneGraphCommandPortComposer;
enum class EMASceneChangeType : uint8;

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
     * 获取下一个可用的节点 ID
     * 遍历所有节点找到最大数字 ID，返回 最大ID + 1
     * 
     * @return 下一个可用的 ID 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    FString GetNextAvailableId() const;

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
    // 统一图操作 API (重构后的核心入口)
    //=========================================================================

    /**
     * 添加节点到 Working Copy
     * 
     * @param NodeJson 节点 JSON 字符串
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool AddNode(const FString& NodeJson, FString& OutError);

    /**
     * 从 Working Copy 删除节点
     * 
     * @param NodeId 节点 ID
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool DeleteNode(const FString& NodeId, FString& OutError);

    /**
     * 编辑节点 (统一入口，自动分发到静态/动态节点)
     * 
     * @param NodeId 节点 ID
     * @param NewNodeJson 新的节点 JSON
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError);

    /**
     * 编辑静态节点 (WorkingCopy 中的节点)
     * 
     * @param NodeId 节点 ID
     * @param NewNodeJson 新的节点 JSON
     * @param OutError 错误信息
     * @return 是否成功
     */
    bool EditStaticNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError);

    /**
     * 保存 Working Copy 到源文件
     * 仅在 Modify 模式下生效，Edit 模式下返回 false 并记录警告
     * 
     * @return 是否成功保存
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool SaveToSource();

    /**
     * 将节点标记为 Goal
     * 在节点的 properties 中添加 is_goal: true
     * 
     * @param NodeId 节点 ID
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool SetNodeAsGoal(const FString& NodeId, FString& OutError);

    /**
     * 取消节点的 Goal 标记
     * 从节点的 properties 中移除 is_goal 字段
     * 
     * @param NodeId 节点 ID
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph")
    bool UnsetNodeAsGoal(const FString& NodeId, FString& OutError);

    /**
     * 检查节点是否被标记为 Goal
     * 
     * @param NodeId 节点 ID
     * @return 是否为 Goal
     */
    UFUNCTION(BlueprintPure, Category = "SceneGraph")
    bool IsNodeGoal(const FString& NodeId) const;

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

    /**
     * 获取所有 Goal 节点
     * @return Goal 节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllGoals() const;

    /**
     * 获取所有 Zone 节点
     * @return Zone 节点数组
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    TArray<FMASceneGraphNode> GetAllZones() const;

    /**
     * 根据节点 ID 获取节点
     * @param NodeId 节点 ID
     * @return 节点，如果未找到则返回无效节点
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Query")
    FMASceneGraphNode GetNodeById(const FString& NodeId) const;

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
     * 绑定动态节点的 GUID
     * 在 Actor 生成后调用，将 Actor 的 GUID 绑定到对应的动态节点
     * 
     * @param NodeIdOrLabel 节点 ID 或 Label (用于匹配节点)
     * @param ActorGuid Actor 的 GUID 字符串 (通过 Actor->GetActorGuid().ToString() 获取)
     * @return 是否绑定成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    bool BindDynamicNodeGuid(const FString& NodeIdOrLabel, const FString& ActorGuid);

    /**
     * 编辑动态节点
     * 验证 JSON 格式并更新节点的内存数据
     * 
     * @param NodeId 节点 ID
     * @param NewNodeJson 新的节点 JSON 字符串
     * @param OutError 错误信息 (如果失败)
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    bool EditDynamicNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError);

    /**
     * 更新动态节点位置 (机器人或环境对象)
     * @param NodeId 节点ID
     * @param NewPosition 新位置
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    void UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition);

    /**
     * 更新动态节点的 Feature
     * @param NodeId 节点ID
     * @param Key Feature 键
     * @param Value Feature 值
     */
    UFUNCTION(BlueprintCallable, Category = "SceneGraph|Dynamic")
    void UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value);

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
    friend class FMASceneGraphCommandPortComposer;

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
    UMAConfigManager* ResolveConfigManager() const;
    void BuildDynamicNodesFromRuntimeConfig(const UMAConfigManager& ConfigManager);
    void RefreshDynamicNodeLocationLabels();
    void CalibrateDynamicNodeFromWorldActor(const FString& NodeIdOrLabel);

    //=========================================================================
    // 内部辅助方法
    //=========================================================================

    /**
     * 从 JSON 对象中提取节点数组
     * 
     * @return 节点数组指针，如果不存在则返回 nullptr
     */
    TArray<TSharedPtr<FJsonValue>>* GetNodesArray() const;

    //=========================================================================
    // 统一图操作内部方法
    //=========================================================================

    /**
     * 初始化 Working Copy
     * 从源文件加载数据到 WorkingCopy
     * @return 是否成功
     */
    bool InitializeWorkingCopy();
    void CacheRuntimeConfig();
    void InitializePorts();
    void EnsureWorkingCopyForStartup();
    void RebuildStaticNodesFromWorkingCopy();
    void InitializeCommandPort();

    /**
     * 写路径内部实现 (由 CommandPort 转发调用)
     */
    bool AddNodeInternal(const FString& NodeJson, FString& OutError);
    bool DeleteNodeInternal(const FString& NodeId, FString& OutError);
    bool EditNodeInternal(const FString& NodeId, const FString& NewNodeJson, FString& OutError);
    bool UpdateNodeGoalFlagInternal(const FString& NodeId, bool bIsGoal, FString& OutError);
    bool SetNodeAsGoalInternal(const FString& NodeId, FString& OutError);
    bool UnsetNodeAsGoalInternal(const FString& NodeId, FString& OutError);
    bool SaveToSourceInternal();
    void UpdateDynamicNodePositionInternal(const FString& NodeId, const FVector& NewPosition);
    void UpdateDynamicNodeFeatureInternal(const FString& NodeId, const FString& Key, const FString& Value);

    /**
     * 发送场景变更消息 (仅 Edit 模式)
     * @param ChangeType 变更类型
     * @param NodeJson 节点数据 JSON 字符串
     */
    void NotifySceneChange(EMASceneChangeType ChangeType, const FString& NodeJson);

    //=========================================================================
    // 内部状态
    //=========================================================================

    /** Working Copy - 内存中的场景图数据 (原始 JSON) */
    TSharedPtr<FJsonObject> WorkingCopy;

    /** 静态节点 (从 JSON 加载) */
    TArray<FMASceneGraphNode> StaticNodes;

    /** 动态节点 (运行时创建) */
    TArray<FMASceneGraphNode> DynamicNodes;

    /** 源文件路径 */
    FString SourceFilePath;

    /** 缓存的运行模式 */
    EMARunMode CachedRunMode;

    /** 查询端口实现 (P1c: manager 内部查询转发) */
    TSharedPtr<IMASceneGraphQueryPort> QueryPort;

    /** 写端口实现 (P1c-b2: Command 转发) */
    TSharedPtr<IMASceneGraphCommandPort> CommandPort;

    /** 仓储端口实现 (P1c-b: I/O 适配) */
    TSharedPtr<IMASceneGraphRepositoryPort> RepositoryPort;

    /** 事件发布端口实现 (P1c-b: 通信适配) */
    TSharedPtr<IMASceneGraphEventPublisherPort> EventPublisherPort;
};
