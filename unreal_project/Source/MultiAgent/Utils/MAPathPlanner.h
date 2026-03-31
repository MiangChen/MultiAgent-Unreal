// MAPathPlanner.h
// 路径规划器工具类
// 提供多层射线扫描和 2.5D 高程图两种路径规划算法

#pragma once

#include "CoreMinimal.h"

/**
 * 路径规划算法类型
 */
UENUM(BlueprintType)
enum class EMAPathPlannerType : uint8
{
    MultiLayerRaycast,  // 多层射线扫描
    ElevationMap        // 2.5D 高程图 A* 规划
};

/**
 * 路径规划器配置结构体
 * 包含所有可配置参数
 */
struct MULTIAGENT_API FMAPathPlannerConfig
{
    //=========================================================================
    // 通用配置
    //=========================================================================
    
    /** 机器人半径 */
    float AgentRadius = 50.f;
    
    /** 卡住超时时间 (秒) */
    float StuckTimeThreshold = 10.f;
    
    //=========================================================================
    // 多层射线扫描配置
    //=========================================================================
    
    /** 射线层数 (2-5) */
    int32 RaycastLayerCount = 3;
    
    /** 每层距离 */
    float RaycastLayerDistance = 300.f;
    
    /** 扫描角度范围 (±度) */
    float ScanAngleRange = 120.f;
    
    /** 扫描角度步长 */
    float ScanAngleStep = 15.f;
    
    //=========================================================================
    // 高程图配置 (2.5D A*)
    //=========================================================================
    
    /** 高程图栅格单元大小 (cm) */
    float ElevationCellSize = 100.f;
    
    /** 高程图搜索半径 (cm) */
    float ElevationSearchRadius = 3000.f;
    
    /** 最大可通行坡度角 (度) */
    float ElevationMaxSlopeAngle = 30.f;
    
    /** 最大可通行台阶高度 (cm) */
    float ElevationMaxStepHeight = 50.f;
    
    /** 路径平滑因子 (0.1-0.5) */
    float PathSmoothingFactor = 0.15f;
    
    /** 路径点到达阈值 (cm) */
    float ElevationWaypointReachThreshold = 100.f;
    
    /** 重规划阈值 (cm) */
    float ElevationReplanThreshold = 200.f;
    
    //=========================================================================
    // 调试配置
    //=========================================================================
    
    /** 是否启用调试绘制 */
    bool bEnableDebugDraw = false;
    
    FMAPathPlannerConfig() = default;
};


/**
 * 高程图单元格结构体
 * 存储每个栅格位置的地面高度和可通行性信息
 */
struct MULTIAGENT_API FMAElevationCell
{
    /** 地面高度 (FLT_MAX 表示无地面/悬崖) */
    float GroundZ = FLT_MAX;
    
    /** 是否可通行 (综合考虑地面、障碍物、坡度等) */
    bool bTraversable = false;
    
    /** 是否有障碍物 */
    bool bHasObstacle = false;
    
    //=========================================================================
    // A* 搜索状态
    //=========================================================================
    
    /** 从起点到此格的代价 */
    float G = FLT_MAX;
    
    /** 启发式估计（到终点的代价） */
    float H = 0.f;
    
    /** 总代价 F = G + H */
    float F() const { return G + H; }
    
    /** 父节点坐标 */
    int32 ParentX = -1;
    int32 ParentY = -1;
    
    /** 是否已访问 */
    bool bVisited = false;
    
    FMAElevationCell() = default;
    
    /** 检查是否有有效地面 */
    bool HasGround() const { return GroundZ < FLT_MAX * 0.5f; }
    
    /** 重置 A* 搜索状态 */
    void ResetSearchState()
    {
        G = FLT_MAX;
        H = 0.f;
        ParentX = -1;
        ParentY = -1;
        bVisited = false;
    }
};


/**
 * 2.5D 高程图类
 * 存储地形高度信息，支持可通行性评估和路径规划
 */
class MULTIAGENT_API FMAElevationMap
{
public:
    FMAElevationMap() = default;
    
    //=========================================================================
    // 构建方法
    //=========================================================================
    
    /**
     * 构建高程图
     * @param World 世界指针
     * @param Center 中心位置
     * @param Radius 搜索半径
     * @param CellSize 栅格单元大小
     * @param IgnoreActor 忽略的 Actor（通常是自身）
     * @param AgentRadius 机器人半径（用于障碍物检测）
     */
    void Build(
        UWorld* World,
        const FVector& Center,
        float Radius,
        float CellSize,
        AActor* IgnoreActor,
        float AgentRadius
    );
    
    /**
     * 评估两个相邻格子之间的可通行性
     * @param FromX 起始格 X 坐标
     * @param FromY 起始格 Y 坐标
     * @param ToX 目标格 X 坐标
     * @param ToY 目标格 Y 坐标
     * @param MaxSlopeAngle 最大可通行坡度角（度）
     * @param MaxStepHeight 最大可通行台阶高度（cm）
     * @return 是否可通行
     */
    bool EvaluateTraversability(
        int32 FromX, int32 FromY,
        int32 ToX, int32 ToY,
        float MaxSlopeAngle,
        float MaxStepHeight
    ) const;
    
    //=========================================================================
    // 查询方法
    //=========================================================================
    
    /** 获取指定格子的地面高度 */
    float GetGroundHeight(int32 X, int32 Y) const;
    
    /** 检查指定格子是否可通行 */
    bool IsTraversable(int32 X, int32 Y) const;
    
    /** 检查指定格子是否有有效地面 */
    bool HasGround(int32 X, int32 Y) const;
    
    /** 检查坐标是否在栅格范围内 */
    bool IsValidCell(int32 X, int32 Y) const;
    
    /** 世界坐标转栅格坐标 */
    FIntPoint WorldToGrid(const FVector& WorldPos) const;
    
    /** 栅格坐标转世界坐标（返回单元格中心，Z 为地面高度） */
    FVector GridToWorld(int32 X, int32 Y) const;
    
    /** 获取 8 方向邻居 */
    TArray<FIntPoint> GetNeighbors(int32 X, int32 Y) const;
    
    //=========================================================================
    // A* 搜索支持
    //=========================================================================
    
    /** 重置所有格子的 A* 搜索状态 */
    void ResetSearchState();
    
    /** 获取格子引用（用于 A* 搜索） */
    FMAElevationCell& GetCell(int32 X, int32 Y);
    const FMAElevationCell& GetCell(int32 X, int32 Y) const;
    
    //=========================================================================
    // Getter
    //=========================================================================
    
    int32 GetWidth() const { return Width; }
    int32 GetHeight() const { return Height; }
    float GetCellSize() const { return CellSize; }
    const FVector& GetOrigin() const { return Origin; }
    bool IsBuilt() const { return Width > 0 && Height > 0; }

private:
    /** 栅格数据 */
    TArray<TArray<FMAElevationCell>> Cells;
    
    /** 栅格宽度 */
    int32 Width = 0;
    
    /** 栅格高度 */
    int32 Height = 0;
    
    /** 栅格原点（世界坐标，左下角） */
    FVector Origin = FVector::ZeroVector;
    
    /** 栅格单元大小 */
    float CellSize = 100.f;
};

/**
 * 路径规划器抽象接口
 * 定义统一的路径规划 API
 */
class MULTIAGENT_API IMAPathPlanner
{
public:
    virtual ~IMAPathPlanner() = default;
    
    /**
     * 计算下一步移动方向
     * @param World 世界指针
     * @param CurrentLocation 当前位置
     * @param TargetLocation 目标位置
     * @param IgnoreActor 忽略的 Actor（通常是自身）
     * @return 移动方向向量（已归一化）
     */
    virtual FVector CalculateDirection(
        UWorld* World,
        const FVector& CurrentLocation,
        const FVector& TargetLocation,
        AActor* IgnoreActor
    ) = 0;
    
    /**
     * 检查是否有有效路径
     * @return 是否有有效路径
     */
    virtual bool HasValidPath() const = 0;
    
    /**
     * 重置状态（导航开始时调用）
     */
    virtual void Reset() = 0;
    
    /**
     * 获取算法类型
     * @return 算法类型枚举
     */
    virtual EMAPathPlannerType GetType() const = 0;
};


/**
 * 多层射线扫描路径规划器
 * 通过多层射线检测评估各方向的可行性
 */
class MULTIAGENT_API FMAMultiLayerRaycast : public IMAPathPlanner
{
public:
    /**
     * 构造函数
     * @param Config 配置参数
     */
    explicit FMAMultiLayerRaycast(const FMAPathPlannerConfig& Config);
    
    //=========================================================================
    // IMAPathPlanner 接口实现
    //=========================================================================
    
    virtual FVector CalculateDirection(
        UWorld* World,
        const FVector& CurrentLocation,
        const FVector& TargetLocation,
        AActor* IgnoreActor
    ) override;
    
    virtual bool HasValidPath() const override;
    
    virtual void Reset() override;
    
    virtual EMAPathPlannerType GetType() const override 
    { 
        return EMAPathPlannerType::MultiLayerRaycast; 
    }
    
    //=========================================================================
    // 配置参数 Getter/Setter
    //=========================================================================
    
    /** 获取射线层数 */
    int32 GetLayerCount() const { return Config.RaycastLayerCount; }
    
    /** 设置射线层数（自动限制在 2-5 范围内） */
    void SetLayerCount(int32 NewLayerCount) 
    { 
        Config.RaycastLayerCount = FMath::Clamp(NewLayerCount, 2, 5); 
    }
    
    /** 获取每层距离 */
    float GetLayerDistance() const { return Config.RaycastLayerDistance; }
    
    /** 设置每层距离 */
    void SetLayerDistance(float NewDistance) 
    { 
        Config.RaycastLayerDistance = FMath::Max(NewDistance, 50.f); 
    }
    
    /** 获取扫描角度范围 */
    float GetScanAngleRange() const { return Config.ScanAngleRange; }
    
    /** 设置扫描角度范围 */
    void SetScanAngleRange(float NewRange) 
    { 
        Config.ScanAngleRange = FMath::Clamp(NewRange, 30.f, 180.f); 
    }
    
    /** 获取扫描角度步长 */
    float GetScanAngleStep() const { return Config.ScanAngleStep; }
    
    /** 设置扫描角度步长 */
    void SetScanAngleStep(float NewStep) 
    { 
        Config.ScanAngleStep = FMath::Clamp(NewStep, 5.f, 45.f); 
    }
    
    /** 获取完整配置 */
    const FMAPathPlannerConfig& GetConfig() const { return Config; }
    
    /** 设置完整配置（会验证参数） */
    void SetConfig(const FMAPathPlannerConfig& NewConfig);

private:
    //=========================================================================
    // 内部方法
    //=========================================================================
    
    /**
     * 检查某方向是否畅通（多高度射线）
     * @param World 世界指针
     * @param Start 起始位置
     * @param Direction 方向
     * @param Distance 检测距离
     * @param IgnoreActor 忽略的 Actor
     * @return 是否畅通
     */
    bool IsDirectionClear(
        UWorld* World, 
        const FVector& Start, 
        const FVector& Direction, 
        float Distance, 
        AActor* IgnoreActor
    );
    
    /**
     * 检查地面连续性（悬崖检测）
     * @param World 世界指针
     * @param Start 起始位置
     * @param Direction 方向
     * @param Distance 检测距离
     * @param IgnoreActor 忽略的 Actor
     * @return 是否有地面
     */
    bool HasGroundAhead(
        UWorld* World, 
        const FVector& Start, 
        const FVector& Direction,
        float Distance, 
        AActor* IgnoreActor
    );
    
    /**
     * 多层评估某方向的得分
     * @param World 世界指针
     * @param CurrentLoc 当前位置
     * @param TargetLoc 目标位置
     * @param Direction 方向
     * @param IgnoreActor 忽略的 Actor
     * @return 方向得分（越高越好）
     */
    float EvaluateDirection(
        UWorld* World, 
        const FVector& CurrentLoc, 
        const FVector& TargetLoc,
        const FVector& Direction, 
        AActor* IgnoreActor
    );
    
    /**
     * 检测振荡（左右摇摆）
     * @return 是否处于振荡状态
     */
    bool IsOscillating() const;
    
    //=========================================================================
    // 成员变量
    //=========================================================================
    
    /** 配置 */
    FMAPathPlannerConfig Config;
    
    /** 上一次的方向 */
    FVector LastDirection;
    
    /** 最近的方向历史（用于振荡检测） */
    TArray<FVector> RecentDirections;
    
    /** 是否有有效路径 */
    bool bHasValidPath = true;
    
    /** 振荡锁定帧数 */
    int32 OscillationLockFrames = 0;
    
    /** 平滑后的方向 */
    FVector SmoothedDirection = FVector::ZeroVector;
    
    /** 方向平滑因子 */
    static constexpr float DirectionSmoothingFactor = 0.15f;

    /** 最大历史方向数量 */
    static constexpr int32 MaxRecentDirections = 6;

    /** 振荡检测阈值（方向变化次数） */
    static constexpr int32 OscillationThreshold = 3;

    /** 振荡锁定持续帧数 */
    static constexpr int32 OscillationLockDuration = 30;
};


/**
 * 2.5D 高程图路径规划器
 * 基于高程图的 A* 路径规划，支持复杂 3D 地形导航
 */
class MULTIAGENT_API FMAElevationMapPathfinder : public IMAPathPlanner
{
public:
    /**
     * 构造函数
     * @param InConfig 配置参数
     */
    explicit FMAElevationMapPathfinder(const FMAPathPlannerConfig& InConfig);
    
    //=========================================================================
    // IMAPathPlanner 接口实现
    //=========================================================================
    
    virtual FVector CalculateDirection(
        UWorld* World,
        const FVector& CurrentLocation,
        const FVector& TargetLocation,
        AActor* IgnoreActor
    ) override;
    
    virtual bool HasValidPath() const override;
    
    virtual void Reset() override;
    
    virtual EMAPathPlannerType GetType() const override 
    { 
        return EMAPathPlannerType::ElevationMap; 
    }
    
    //=========================================================================
    // 配置参数 Getter/Setter
    //=========================================================================
    
    /** 获取完整配置 */
    const FMAPathPlannerConfig& GetConfig() const { return Config; }
    
    /** 设置完整配置 */
    void SetConfig(const FMAPathPlannerConfig& NewConfig);
    
    //=========================================================================
    // 调试可视化
    //=========================================================================
    
    /**
     * 绘制高程图调试可视化
     * @param World 世界指针
     * @param bDrawTraversable 是否绘制可通行区域（绿色）
     * @param bDrawObstacles 是否绘制障碍物（红色）
     * @param bDrawNoGround 是否绘制无地面区域（黄色）
     * @param Duration 绘制持续时间（秒）
     */
    void DrawDebugElevationMap(
        UWorld* World,
        bool bDrawTraversable = true,
        bool bDrawObstacles = true,
        bool bDrawNoGround = true,
        float Duration = 0.1f
    ) const;
    
    /**
     * 绘制规划路径调试可视化
     * @param World 世界指针
     * @param Duration 绘制持续时间（秒）
     */
    void DrawDebugPath(UWorld* World, float Duration = 0.1f) const;
    
    /** 获取缓存的路径（用于外部调试） */
    const TArray<FVector>& GetCachedPath() const { return CachedPath; }
    
    /** 获取当前路径点索引 */
    int32 GetCurrentWaypointIndex() const { return CurrentWaypointIndex; }

private:
    //=========================================================================
    // 内部方法
    //=========================================================================
    
    /**
     * 检查是否需要重新规划路径
     * @param CurrentLocation 当前位置
     * @param TargetLocation 目标位置
     * @return 是否需要重规划
     */
    bool NeedsReplan(const FVector& CurrentLocation, const FVector& TargetLocation) const;
    
    /**
     * 执行 A* 路径搜索
     * @param Start 起始位置
     * @param Target 目标位置
     * @return 是否找到路径
     */
    bool FindPath(const FVector& Start, const FVector& Target);
    
    /**
     * 计算移动代价（考虑距离和坡度）
     * @param FromX 起始格 X 坐标
     * @param FromY 起始格 Y 坐标
     * @param ToX 目标格 X 坐标
     * @param ToY 目标格 Y 坐标
     * @return 移动代价
     */
    float CalculateMoveCost(int32 FromX, int32 FromY, int32 ToX, int32 ToY) const;
    
    /**
     * 计算启发式估计（欧几里得距离，考虑高度差）
     * @param FromX 起始格 X 坐标
     * @param FromY 起始格 Y 坐标
     * @param ToX 目标格 X 坐标
     * @param ToY 目标格 Y 坐标
     * @return 启发式估计值
     */
    float CalculateHeuristic(int32 FromX, int32 FromY, int32 ToX, int32 ToY) const;
    
    /**
     * 从目标回溯重建路径
     * @param TargetGrid 目标栅格坐标
     */
    void ReconstructPath(const FIntPoint& TargetGrid);
    
    //=========================================================================
    // 成员变量
    //=========================================================================
    
    /** 配置 */
    FMAPathPlannerConfig Config;
    
    /** 高程图 */
    FMAElevationMap ElevationMap;
    
    /** 缓存路径（带 XYZ 坐标） */
    TArray<FVector> CachedPath;
    
    /** 当前路径点索引 */
    int32 CurrentWaypointIndex = 0;
    
    /** 是否有有效路径 */
    bool bHasValidPath = false;
    
    /** 上次规划的目标位置 */
    FVector LastTargetLocation = FVector::ZeroVector;
    
    /** 上次规划的起始位置 */
    FVector LastStartLocation = FVector::ZeroVector;
    
    /** 平滑后的方向 */
    FVector SmoothedDirection = FVector::ZeroVector;
    
    /** 缓存的世界指针 */
    TWeakObjectPtr<UWorld> CachedWorld;
    
    /** 缓存的忽略 Actor */
    TWeakObjectPtr<AActor> CachedIgnoreActor;
};
