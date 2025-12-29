# 机器人资产获取指南

## 当前资产状态

| 机器人类型 | 资产状态 | 资产路径 |
|-----------|---------|---------|
| UAV (多旋翼) | ✅ 已有 | `/Game/Robot/dji_inspire2/` |
| FixedWingUAV | ⚠️ 待添加 | `/Game/Robot/fixed_wing/` |
| UGV | ⚠️ 待添加 | `/Game/Robot/ugv/` |
| RobotDog | ✅ 已有 | `/Game/Robot/go1/` |
| Humanoid | ✅ 已有 | `/Game/Characters/Mannequins/` |

## 获取资产的方式

### 1. Unreal Marketplace
- 搜索 "drone", "UAV", "vehicle", "robot" 等关键词
- 免费资产: Unreal Engine 每月免费内容
- 付费资产: 根据需求购买

### 2. Sketchfab
- 下载 FBX/OBJ 格式模型
- 导入 UE5 后需要设置材质和骨骼

### 3. 自制简单几何体
- 使用 UE5 内置的基础形状
- 适合原型开发阶段

## 导入资产步骤

### 步骤 1: 准备资产
- 格式: FBX (推荐) 或 OBJ
- 包含: 网格、材质、骨骼(可选)、动画(可选)

### 步骤 2: 导入到 UE5
1. 在 Content Browser 中右键 → Import
2. 选择 FBX 文件
3. 设置导入选项:
   - Skeletal Mesh: 如果有骨骼动画
   - Static Mesh: 如果是静态模型
   - Import Materials: 导入材质

### 步骤 3: 创建角色类
1. 在对应的 Character 类构造函数中设置 Mesh:
```cpp
static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
    TEXT("/Game/Robot/your_robot/your_robot.your_robot"));
if (MeshAsset.Succeeded())
{
    GetMesh()->SetSkeletalMesh(MeshAsset.Object);
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
}
```

### 步骤 4: 设置动画 (可选)
- 如果有动画，创建 AnimSequence 或 AnimBlueprint
- 在 BeginPlay 中设置动画

## 推荐资产来源

### 固定翼无人机
- Sketchfab: "fixed wing drone", "UAV aircraft"
- 或使用简单的飞机形状几何体

### 无人车 (UGV)
- Unreal Marketplace: "military vehicle", "rover"
- Sketchfab: "unmanned ground vehicle", "robot car"

## 临时方案

在没有实际资产时，代码已设置为可以运行（使用默认 Capsule）。
获取资产后，只需更新对应 Character 类中的资产路径即可。
