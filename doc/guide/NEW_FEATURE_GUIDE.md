# 新功能开发指南

本文档帮助开发者快速定位开发某类功能需要修改的文件。

## 功能类型 → 代码位置

### 1. 添加新的 Agent 命令 (如巡逻、跟随)

```
1. Core/MACommandManager.h       - 添加 EMACommand 枚举
2. Core/MACommandManager.cpp     - 实现 SendCommand() 分支
3. Input/MAPlayerController.cpp  - 绑定快捷键
4. Agent/StateTree/Task/         - 创建 MASTTask_XXX.h/cpp 实现具体行为
5. doc/KEYBINDINGS.md            - 更新按键文档
```

### 2. 添加新的 Agent 类型 (如新无人机)

```
1. Agent/Character/              - 创建新 Character 类
2. Agent/Interface/MAAgentInterfaces.h - 实现需要的 Interface
3. Core/MATypes.h                - 添加 EMAAgentType 枚举
4. UI/MASetupWidget.cpp          - 添加到 AvailableAgentTypes
5. config/SimConfig.json         - 添加配置 (可选)
```

### 3. 添加新的 GAS 技能

```
1. Agent/GAS/Ability/            - 创建 GA_XXX.h/cpp
2. Agent/GAS/MAGameplayTags.cpp  - 注册新 Tag
3. Agent/GAS/MAAbilitySystemComponent.cpp - 注册 Ability
4. Config/DefaultGameplayTags.ini - 配置 Tag
```

### 4. 添加新的 StateTree Task

```
1. Agent/StateTree/Task/         - 创建 MASTTask_XXX.h/cpp
2. UE 编辑器中添加到 StateTree Asset
```

### 5. 添加新的 StateTree Condition

```
1. Agent/StateTree/Condition/    - 创建 MASTCondition_XXX.h/cpp
2. UE 编辑器中添加到 StateTree Asset
```

### 6. 添加新的 UI 界面

```
1. UI/                           - 创建 Widget 类
2. UI/MAHUD.cpp                  - 管理 Widget 显示
3. Input/MAPlayerController.cpp  - 绑定切换快捷键
4. Input/MAInputActions.cpp      - 添加 Input Action
```

### 7. 添加新的传感器

```
1. Agent/Component/Sensor/       - 创建 Sensor Component
2. Agent/Character/MACharacter.cpp - 添加 Sensor 管理
3. config/SimConfig.json         - 配置传感器参数
```

### 8. 添加新的能力组件 (Capability)

```
1. Agent/Component/Capability/   - 创建 Component 类
2. Agent/Interface/MAAgentInterfaces.h - 定义对应 Interface
3. Agent/Character/              - 在需要的 Character 中添加组件
```

### 9. 添加新的环境实体 (如充电站、巡逻路径)

```
1. Environment/                  - 创建 Actor 类
2. UE 编辑器中放置到地图
3. 相关 Task/Ability 中添加查找逻辑
```

### 10. 添加新的快捷键

```
1. Input/MAInputActions.cpp      - 创建 UInputAction 并绑定按键
2. Input/MAPlayerController.cpp  - 添加回调函数
3. doc/KEYBINDINGS.md            - 更新文档
```

## 常用代码模式

### 访问 Subsystem

```cpp
// 使用 MA_SUBS 宏
MA_SUBS.CommandManager->SendCommand(EMACommand::Patrol);
MA_SUBS.SelectionManager->GetSelectedAgents();
MA_SUBS.AgentManager->GetAllAgents();
```

### 通过 Interface 获取能力

```cpp
if (IMAChargeable* Chargeable = GetCapabilityInterface<IMAChargeable>(Actor))
{
    float Energy = Chargeable->GetEnergy();
}
```

### 设置 Gameplay Tag

```cpp
ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
```
