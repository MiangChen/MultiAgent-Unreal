# Agent 生成位置问题分析与修复方案

> 日期: 2025-12-20
> 问题: Agent 生成在地下 / 框选位置不匹配
> 状态: ✅ 已修复

## 问题一：Agent 生成在地下

### 日志分析
```
[DronePhantom4_0] Hovering at (5308, -4214, -119)
[DronePhantom4_1] Hovering at (6079, 1989, -119)
[DronePhantom4_2] Hovering at (2556, -3957, -43)
```

### 根本原因
1. 射线检测起点相对于传入位置，如果传入位置已经很低，检测范围不够
2. 某些地图的地面 Z 坐标本身就是负值（这是正常的）

### 修复方案
修改 `MAAgentManager::SpawnAgentByType()` 和 `MAPlayerController::ProjectToGround()`：

```cpp
// 从固定高空位置开始检测，确保能检测到任何高度的地面
FVector TraceStart = FVector(SpawnLocation.X, SpawnLocation.Y, 10000.f);
FVector TraceEnd = FVector(SpawnLocation.X, SpawnLocation.Y, -20000.f);
```

---

## 问题二：框选位置与生成位置不匹配

### 原因分析
原来的 Deployment 框选逻辑：
1. 将屏幕四角投影到世界（延伸 10000 单位）
2. 在世界坐标系做双线性插值
3. 受透视影响，小屏幕框 → 大世界范围

Select 框选逻辑：
1. 遍历 Agent，将世界位置投影到屏幕
2. 在屏幕坐标系比较
3. 精确匹配

### 修复方案
重写 `ProjectSelectionBoxToWorld()`，使用和 Select 一样的屏幕空间逻辑：

```cpp
TArray<FVector> AMAPlayerController::ProjectSelectionBoxToWorld(FVector2D Start, FVector2D End, int32 Count)
{
    TArray<FVector> Points;
    
    // 计算屏幕框边界
    float MinX = FMath::Min(Start.X, End.X);
    float MaxX = FMath::Max(Start.X, End.X);
    float MinY = FMath::Min(Start.Y, End.Y);
    float MaxY = FMath::Max(Start.Y, End.Y);
    
    // 在屏幕空间网格分布
    int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt((float)Count)));
    int32 Rows = FMath::Max(1, FMath::CeilToInt((float)Count / Cols));
    
    for (int32 Row = 0; Row < Rows && Spawned < Count; ++Row)
    {
        for (int32 Col = 0; Col < Cols && Spawned < Count; ++Col)
        {
            // 计算屏幕坐标
            float ScreenX = FMath::Lerp(MinX, MaxX, U);
            float ScreenY = FMath::Lerp(MinY, MaxY, V);
            
            // 从屏幕坐标射线检测地面
            FVector WorldPos, WorldDir;
            DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldPos, WorldDir);
            
            // 沿视线方向射线检测
            FHitResult HitResult;
            FVector TraceStart = WorldPos;
            FVector TraceEnd = WorldPos + WorldDir * 50000.f;
            
            if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
            {
                Points.Add(HitResult.Location);
            }
        }
    }
    
    return Points;
}
```

### 关键改动

| 方面 | 旧逻辑 | 新逻辑 |
|------|--------|--------|
| 插值空间 | 世界坐标 | 屏幕坐标 |
| 射线方向 | 垂直向下 | 沿相机视线 |
| 精度 | 受透视影响 | 精确匹配屏幕框 |

---

## 修改文件

- `Source/MultiAgent/Core/MAAgentManager.cpp` - `SpawnAgentByType()` 固定高空射线检测
- `Source/MultiAgent/Input/MAPlayerController.cpp` - `ProjectToGround()` 固定高空射线检测
- `Source/MultiAgent/Input/MAPlayerController.cpp` - `ProjectSelectionBoxToWorld()` 屏幕空间逻辑

---

## 两种框选逻辑对比

### Select 模式（选择 Agent）
```
屏幕框 → 遍历 Agent → Agent 世界位置投影到屏幕 → 检查是否在框内
```
- 方向：World → Screen
- 比较空间：屏幕坐标

### Deployment 模式（放置 Agent）- 修复后
```
屏幕框 → 屏幕空间网格分布 → 每个点沿视线射线检测地面 → 生成位置
```
- 方向：Screen → World（沿视线）
- 精度：与屏幕框精确匹配
