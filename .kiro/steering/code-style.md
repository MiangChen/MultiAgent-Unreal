# Code Style Guidelines

详细规范请参考: #[[file:doc/prompt_code_style.md]]
项目架构文档: #[[file:doc/ARCHITECTURE.md]]
UE5 API 变更说明: #[[file:doc/UE5_API_CHANGES.md]]
项目说明与编译: #[[file:README.md]]

## 核心原则

### StateTree Task 设计
- Task 不存储可配置参数
- 参数存储在 Robot/Character 上
- Task 从 Robot 获取参数
- 同一参数可被多个 Task/Ability 复用

### 示例
```cpp
// 好的做法：从 Robot 获取参数
AMACharacter* Target = Robot->GetFollowTarget();
float Distance = Robot->ScanRadius;

// 不好的做法：在 Task 中配置参数
UPROPERTY(EditAnywhere)
float FollowDistance = 200.f;  // 避免这样做
```
