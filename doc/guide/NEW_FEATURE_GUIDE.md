# 新功能开发指南

本文档帮助开发者快速定位开发某类功能需要修改的文件。

## 三层架构

```
指令调度层 (MACommandManager)
    │ SendCommandToAgent()
    ▼
技能管理层 (MASkillComponent + StateTree)
    │ SkillParams → StateTree Tag → 技能激活
    ▼
技能内核层 (SK_Navigate, SK_Follow, SK_Charge, SK_Place)
```

## 功能类型 → 代码位置

### 1. 添加新的 Agent 命令/技能

```
1. Core/Manager/MACommandManager.h   - 添加 EMACommand 枚举
2. Core/Manager/MACommandManager.cpp - 实现参数预处理分支
3. Agent/Skill/MASkillComponent.h    - 添加 FMASkillParams 参数、技能激活接口
4. Agent/Skill/Impl/                 - 创建 SK_XXX.h/cpp 技能实现
5. Agent/Skill/MASkillTags.h/cpp     - 注册新 Tag
6. Agent/StateTree/Task/             - 创建 MASTTask_XXX.h/cpp 实现具体行为
7. Agent/Skill/MAFeedbackSystem.h/cpp - 添加反馈模板
```

### 2. 添加新的 Agent 类型 (如新无人机)

```
1. Agent/Character/              - 创建新 Character 类
2. Core/Types/MATypes.h          - 添加 EMAAgentType 枚举
3. UI/MASetupWidget.cpp          - 添加到 AvailableAgentTypes
4. config/agents.json            - 添加配置 (可选)
```

### 3. 添加新的技能实现

```
1. Agent/Skill/Impl/             - 创建 SK_XXX.h/cpp
2. Agent/Skill/MASkillTags.cpp   - 注册新 Tag
3. Agent/Skill/MASkillComponent.cpp - 在 InitializeSkills() 中注册技能
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

### 8. 添加新的环境实体 (如充电站)

```
1. Environment/                  - 创建 Actor 类
2. UE 编辑器中放置到地图
3. 相关 Task/Skill 中添加查找逻辑
```

## 常用代码模式

### 访问 Subsystem

```cpp
// 使用 MA_SUBS 宏
MA_SUBS.CommandManager->SendCommandToAgent(Agent, EMACommand::Navigate);
MA_SUBS.SelectionManager->GetSelectedAgents();
MA_SUBS.AgentManager->GetAllAgents();
```

### 发送指令（指令调度层）

```cpp
// 先设置技能参数
UMASkillComponent* SkillComp = Agent->GetSkillComponent();
FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
Params.TargetLocation = FVector(1000, 0, 0);

// 发送命令
CommandManager->SendCommandToAgent(Agent, EMACommand::Navigate);
```

### 技能激活（技能管理层）

```cpp
UMASkillComponent* SkillComp = Character->GetSkillComponent();
SkillComp->TryActivateNavigate(TargetLocation);
SkillComp->TryActivateFollow(TargetCharacter, 200.f);
SkillComp->TryActivateCharge();
SkillComp->TryActivatePlace();
```

### 设置 Gameplay Tag

```cpp
SkillComp->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Navigate")));
SkillComp->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
SkillComp->ClearAllCommands();  // 清除所有 Command.* Tag
```

### 能量系统

```cpp
UMASkillComponent* SkillComp = Character->GetSkillComponent();
float Energy = SkillComp->GetEnergy();
float Percent = SkillComp->GetEnergyPercent();
bool bLow = SkillComp->IsLowEnergy();
bool bHas = SkillComp->HasEnergy();
SkillComp->DrainEnergy(DeltaTime);
SkillComp->RestoreEnergy(Amount);
```

### 反馈系统

```cpp
// 获取反馈上下文
FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
Context.TargetLocation = TargetLocation;

// 生成反馈消息
FString Message = FMAFeedbackTemplates::Get().GenerateMessage(
    AgentName, Command, bSuccess, Context);
```
