# 场景图管理系统

> 日期: 2025-12-29
> 状态: ✅ 已实现

## 功能概述

实现了完整的场景图管理系统，支持在 Modify 模式下对场景 Actor 进行语义标注，包括单选和多选两种模式，数据持久化到 JSON 文件。

## 新增文件

| 文件 | 说明 |
|------|------|
| `Core/MASceneGraphManager.h/cpp` | 场景图管理器 (UGameInstanceSubsystem) |
| `Core/MAGeometryUtils.h/cpp` | 几何计算工具类 |
| `UI/MAModifyTypes.h` | Modify 模式类型定义 |

## 核心功能

### 1. 场景标签输入与解析

支持 `id:值,type:值,shape:值` 格式的输入解析：

| 模式 | 输入格式 | 示例 |
|------|----------|------|
| 单选 | `id:值,type:值` | `id:building_01,type:building` |
| 多选 Polygon | `id:值,type:值,shape:polygon` | `id:area_01,type:building,shape:polygon` |
| 多选 LineString | `id:值,type:值,shape:linestring` | `id:road_01,type:road,shape:linestring` |

### 2. 多选模式 (Shift+Click)

- Shift+Click 添加/移除 Actor 到选择集 (toggle 行为)
- 普通 Click 清除之前选择，单选当前 Actor
- 支持同时高亮多个 Actor
- HintText 显示当前选择数量

### 3. 几何计算

| 方法 | 算法 | 用途 |
|------|------|------|
| `ComputeConvexHull2D()` | Graham Scan | 计算多选建筑群的凸包 |
| `SortByNearestNeighbor()` | 最近邻贪心 | 排序道路片段点序列 |
| `CalculatePolygonCentroid()` | 算术平均 | 计算多边形几何中心 |
| `CalculateLineStringCentroid()` | 算术平均 | 计算线串几何中心 |

### 4. GUID 字段

- 每个节点包含 `guid` 字段，通过 `Actor->GetActorGuid().ToString()` 获取
- 多选节点包含 `Guid` 数组，存储所有选中 Actor 的 GUID
- 支持向后兼容，旧 JSON 文件缺少 guid 字段时显示 "N/A"

### 5. 场景标签可视化

- 进入 Modify 模式时自动加载 JSON 并显示绿色文本标签
- 标签内容: "GUID: [guid]\nLabel: [label]"
- 位置: 节点中心 + Z 偏移 100
- 使用 `DrawDebugString` 绘制

### 6. GUID 反向查询

- 点击 Actor 时通过 `FindNodesByGuid()` 查询所属节点
- 如果 Actor 属于 Polygon/LineString 节点，显示完整节点 JSON
- 支持 Actor 属于多个节点的情况

## 数据结构

### FMASceneGraphNode

```cpp
USTRUCT(BlueprintType)
struct FMASceneGraphNode
{
    FString Id;           // 用户指定的 ID
    FString Guid;         // Actor GUID (单选)
    TArray<FString> GuidArray;  // Actor GUID 数组 (多选)
    FString Type;         // 语义类型
    FString Label;        // 自动生成的标签 (Type-N)
    FVector Center;       // 显示位置
    FString ShapeType;    // point/polygon/linestring
    FString RawJson;      // 原始 JSON
};
```

### EMAShapeType

```cpp
UENUM(BlueprintType)
enum class EMAShapeType : uint8
{
    Point,      // 单选默认
    Polygon,    // 多选建筑群
    LineString, // 多选道路片段
    Rectangle   // 预留
};
```

## JSON 输出格式

```json
// Point 类型
{
  "id": "building_01",
  "guid": "A1B2C3D4-...",
  "properties": { "type": "building", "label": "Building-1" },
  "shape": { "type": "point", "center": [100, 200, 0] }
}

// Polygon 类型
{
  "id": "area_01",
  "count": 3,
  "Guid": ["GUID1", "GUID2", "GUID3"],
  "properties": { "type": "building", "label": "Building-2" },
  "shape": { "type": "polygon", "vertices": [[x1,y1,z1], ...] }
}

// LineString 类型
{
  "id": "road_01",
  "count": 5,
  "Guid": ["GUID1", "GUID2", "GUID3", "GUID4", "GUID5"],
  "properties": { "type": "road", "label": "Road-1" },
  "shape": { "type": "linestring", "points": [[x1,y1,z1], ...] }
}
```

## 修改文件

- `Core/MASceneGraphManager.h/cpp` - 新增
- `Core/MAGeometryUtils.h/cpp` - 新增
- `UI/MAModifyTypes.h` - 新增
- `UI/MAModifyWidget.h/cpp` - 扩展多选支持
- `Input/MAPlayerController.h/cpp` - 扩展多选交互
- `UI/MAHUD.h/cpp` - 添加场景标签可视化

## 相关 Spec

- `.kiro/specs/scene-label-tagging/` - 场景标签功能
- `.kiro/specs/multi-select-mode/` - 多选模式
- `.kiro/specs/modify-mode-guid-visualization/` - GUID 可视化
- `.kiro/specs/modify-mode-label-display-fix/` - 标签显示修复
