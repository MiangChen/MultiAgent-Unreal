# Bug 记录

## Bug #1: 物品拾取后不跟随角色移动

**日期**: 2025-12-03

**现象**: 
- 机器人拾取物品后，物体保持悬浮在原位置
- 角色移动时，物体不跟随
- 执行 Drop 后，物体才有重力掉落

**调试过程**:
1. 添加详细日志发现 `AttachToComponent()` 返回成功
2. 日志显示 `RootComponent: CollisionComponent`，而不是 `MeshComponent`
3. 发现问题：MeshComponent 有独立物理模拟，不跟随父组件移动

**根本原因**: 
UE 组件层级与物理模拟的关系问题

```
错误的层级：
Actor
└── CollisionComponent (Root, 无物理) ← AttachToComponent 移动的是这个
    └── MeshComponent (有物理) ← 物理引擎独立控制，不跟随移动

正确的层级：
Actor  
└── MeshComponent (Root, 有物理) ← 现在 Attach 移动的是可见的 Mesh
    └── CollisionComponent ← 跟着 Mesh 走
```

**核心原则**:
1. `AttachToComponent()` 只移动 Actor 的 RootComponent
2. 有物理模拟的组件会被物理引擎独立控制位置
3. **想要附着移动的组件必须是 Root，且关闭物理后才会跟随父组件**

**修复方案**:
修改 `MAPickupItem.cpp`，把 MeshComponent 设为 Root：

```cpp
// 修复前
RootComponent = CollisionComponent;
MeshComponent->SetupAttachment(RootComponent);

// 修复后
RootComponent = MeshComponent;
CollisionComponent->SetupAttachment(RootComponent);
```

**相关文件**:
- `Source/MultiAgent/Interaction/MAPickupItem.cpp`
- `Source/MultiAgent/GAS/Abilities/GA_Pickup.cpp`
