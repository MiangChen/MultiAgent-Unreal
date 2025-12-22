# 场景实体定义方案分析

## 需要定义的实体

| 实体类型 | 说明 | 数据结构 |
|---------|------|---------|
| Charge Station | 充电站位置 | 单点 (Position) |
| Patrol Path | 巡逻路径 | 多点序列 (Array of Points) |
| Coverage Area | 覆盖区域 | 多边形/多点 (Polygon or Points) |

---

## 方案对比

### 方案 A: 地图内预定义 (Level Actor)

在 UE 编辑器中直接放置 Actor 到地图中。

**实现方式:**
- `AMAChargeStation` - 放置在地图中的充电站 Actor
- `AMAPatrolPath` - Spline Actor 定义巡逻路径
- `AMACoverageArea` - Volume Actor 定义覆盖区域

**优点:**
- ✅ 可视化编辑，所见即所得
- ✅ 与地图紧密绑定，位置精确
- ✅ 支持复杂路径 (Spline 曲线)
- ✅ 可利用 UE 的碰撞检测、导航网格
- ✅ 不同地图可以有不同的预设

**缺点:**
- ❌ 需要打开 UE 编辑器修改
- ❌ 每个地图需要单独配置
- ❌ 运行时不易动态修改

**适合场景:**
- 固定场景，位置不常变化
- 需要精确的路径规划
- 多地图项目，每个地图有独特布局

---

### 方案 B: 运行时动态定义 (类似 Agent)

通过 UI 或配置文件在运行时创建。

**实现方式:**
- Setup UI 中添加 "场景元素" 配置页
- 用户在小地图上点击放置
- 或通过 JSON 配置文件定义

**优点:**
- ✅ 灵活，运行时可修改
- ✅ 不需要 UE 编辑器
- ✅ 可以保存/加载不同配置
- ✅ 适合实验和测试

**缺点:**
- ❌ 缺乏可视化预览
- ❌ 路径定义复杂 (需要多次点击)
- ❌ 难以精确对齐地形

**适合场景:**
- 研究/实验环境，需要频繁调整
- 简单场景，只需要几个点
- 需要程序化生成的场景

---

### 方案 C: 混合方案 (推荐)

地图预定义 + 运行时覆盖/扩展。

**实现方式:**
```
1. 地图中预放置默认的 Charge Station / Patrol Path
2. 运行时可以:
   - 使用地图预设 (默认)
   - 通过 UI 添加额外的点
   - 通过 UI 禁用某些预设点
```

**数据流:**
```
地图 Actor (预设) 
    ↓
SceneManager 收集
    ↓
UI 可修改/扩展
    ↓
Agent 使用
```

**优点:**
- ✅ 兼顾可视化编辑和运行时灵活性
- ✅ 地图有合理默认值
- ✅ 用户可以快速调整

---

## 建议实现

### 第一阶段: 地图预定义 (简单实现)

```cpp
// 充电站 Actor - 放置在地图中
UCLASS()
class AMAChargeStation : public AActor
{
    UPROPERTY(EditAnywhere)
    int32 StationID;
    
    UPROPERTY(EditAnywhere)
    float ChargeRate = 10.f;  // 每秒充电量
};

// 巡逻路径 Actor - 使用 Spline
UCLASS()
class AMAPatrolPath : public AActor
{
    UPROPERTY(VisibleAnywhere)
    USplineComponent* PathSpline;
    
    UPROPERTY(EditAnywhere)
    FString PathName;
    
    UPROPERTY(EditAnywhere)
    bool bIsLoop = true;
};

// 场景管理器 - 收集地图中的实体
UCLASS()
class UMASceneManager : public UWorldSubsystem
{
    TArray<AMAChargeStation*> ChargeStations;
    TArray<AMAPatrolPath*> PatrolPaths;
    
    // BeginPlay 时自动收集地图中的 Actor
    void CollectSceneEntities();
    
    // Agent 查询接口
    AMAChargeStation* FindNearestChargeStation(FVector Location);
    AMAPatrolPath* GetPatrolPath(FString PathName);
};
```

### 第二阶段: UI 扩展 (可选)

- 小地图上显示充电站图标
- 点击小地图添加临时充电站
- Setup UI 中配置巡逻路径点

---

## 结论

| 实体 | 推荐方案 | 原因 |
|-----|---------|------|
| Charge Station | 地图预定义 | 位置固定，与建筑/设施相关 |
| Patrol Path | 地图预定义 (Spline) | 路径复杂，需要可视化编辑 |
| Coverage Area | 地图预定义 (Volume) | 区域形状复杂 |

**建议先实现方案 A (地图预定义)**，后续根据需求添加运行时修改功能。
