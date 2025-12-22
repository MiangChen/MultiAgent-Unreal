# Direct Control 直接控制功能分析

## 问题描述

当前系统中，通过 UI 部署的 Agent（使用 `SpawnAgentByType`）无法被直接控制，因为：

1. **切换视角依赖摄像头**：`ViewportManager::SwitchToAgentCamera()` 要求 Agent 有 `MACameraSensorComponent`
2. **UI 部署不添加摄像头**：`SpawnAgentByType()` 只生成 Agent，不添加任何 Sensor

```cpp
// MAViewportManager.cpp - 切换视角时检查摄像头
void UMAViewportManager::SwitchToAgentCamera(APlayerController* PC, AMACharacter* Agent)
{
    UMACameraSensorComponent* Camera = Agent->GetCameraSensor();
    if (!Camera) return;  // 没有摄像头就直接返回
    // ...
}
```

## 现有架构

### 摄像头组件 (MACameraSensorComponent)

- 继承自 `MASensorComponent`
- 包含 `UCameraComponent` 用于视角渲染
- 包含 `USceneCaptureComponent2D` 用于拍照/录像
- 支持 TCP 流、拍照、录像等功能

### 直接控制流程

```
用户按 C 键
    ↓
ViewportManager::SwitchToNextCamera()
    ↓
CollectCameras() - 收集所有有摄像头的 Agent
    ↓
SetViewTargetWithBlend() - 切换视角
    ↓
EnterAgentViewMode() - 启用直接控制
    ↓
Agent::SetDirectControl(true) - Agent 进入直接控制模式
```

## 解决方案

### 方案 A：为所有 Agent 自动添加第三人称摄像头 ✅ 已实现

在 `SpawnAgentByType()` 中自动为新生成的 Agent 添加默认摄像头。

```cpp
// MAAgentManager.cpp - SpawnAgentByType() 末尾添加
AMACharacter* Agent = SpawnAgent(AgentClass, SpawnLocation, Rotation, AgentID, AgentType);

if (Agent)
{
    // 自动添加第三人称摄像头
    // 位置：后方 150 单位，上方 80 单位
    // 角度：向下 15 度
    Agent->AddCameraSensor(FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
}

return Agent;
```

**优点**：
- 所有 Agent 都可以被直接控制
- 统一的第三人称视角体验
- 无需修改 ViewportManager 逻辑

**缺点**：
- 增加了每个 Agent 的组件数量
- 可能不是所有场景都需要摄像头

### 方案 B：支持无摄像头的直接控制

修改 `ViewportManager` 支持没有摄像头的 Agent：

```cpp
// MAViewportManager.cpp
void UMAViewportManager::SwitchToAgentCamera(APlayerController* PC, AMACharacter* Agent)
{
    if (!PC || !Agent) return;

    // 保存原始 Pawn
    if (!OriginalPawn.IsValid() && PC->GetPawn())
    {
        OriginalPawn = PC->GetPawn();
    }

    // 直接将 Agent 设为 ViewTarget（即使没有摄像头）
    PC->SetViewTargetWithBlend(Agent, 0.3f);
    
    // 进入 Agent View Mode，启用直接控制
    EnterAgentViewMode(Agent);
}
```

**问题**：没有摄像头时，UE5 会使用 Agent 的默认视角（通常是第一人称或无视角），体验不佳。

### 方案 C：动态创建临时摄像头

在进入直接控制时，如果 Agent 没有摄像头，动态创建一个：

```cpp
void UMAViewportManager::EnterAgentViewMode(AMACharacter* Agent)
{
    // 检查是否有摄像头，没有则创建
    if (!Agent->GetCameraSensor())
    {
        Agent->AddCameraSensor(FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
    }
    
    // ... 原有逻辑
}
```

## 推荐实现：方案 A

修改 `MAAgentManager::SpawnAgentByType()`，在生成 Agent 后自动添加第三人称摄像头。

### 不同 Agent 类型的摄像头配置

| Agent 类型 | 摄像头位置 | 摄像头角度 | 说明 |
|-----------|-----------|-----------|------|
| Human | (-150, 0, 80) | (-15, 0, 0) | 标准第三人称 |
| RobotDog | (-150, 0, 80) | (-15, 0, 0) | 标准第三人称 |
| DronePhantom4 | (-200, 0, 50) | (-10, 0, 0) | 稍远，适应飞行 |
| DroneInspire2 | (-200, 0, 50) | (-10, 0, 0) | 稍远，适应飞行 |

### 代码修改

```cpp
// MAAgentManager.cpp - SpawnAgentByType() 末尾

AMACharacter* Agent = SpawnAgent(AgentClass, SpawnLocation, Rotation, AgentID, AgentType);

if (Agent)
{
    // 根据类型配置第三人称摄像头
    FVector CameraOffset;
    FRotator CameraRotation;
    
    if (TypeName.Contains(TEXT("Drone")))
    {
        // 无人机：稍远的视角
        CameraOffset = FVector(-200.f, 0.f, 50.f);
        CameraRotation = FRotator(-10.f, 0.f, 0.f);
    }
    else
    {
        // 地面单位：标准第三人称
        CameraOffset = FVector(-150.f, 0.f, 80.f);
        CameraRotation = FRotator(-15.f, 0.f, 0.f);
    }
    
    Agent->AddCameraSensor(CameraOffset, CameraRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[AgentManager] Added default camera to %s"), *Agent->AgentID);
}

return Agent;
```

## 测试验证

1. 通过 UI 部署一个 Human Agent
2. 按 `C` 键切换到该 Agent 的视角
3. 使用 WASD 控制移动
4. 按 `V` 键返回观察者视角

## 相关文件

- `MAViewportManager.cpp/h` - 视角管理
- `MAAgentManager.cpp/h` - Agent 生成管理
- `MACharacter.cpp/h` - Agent 基类
- `MACameraSensorComponent.cpp/h` - 摄像头组件
- `MAAgentInputComponent.cpp/h` - 直接控制输入处理
