# UE5 GAS (Gameplay Ability System) API 变更指南

> 数据来源：UE 引擎源码头文件中的 `UE_DEPRECATED` 标记
> 路径：`Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/`

---

## 1. UGameplayAbility (GameplayAbility.h)

### 5.5 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `GetAbilitySystemComponentFromActorInfo_Checked()` | `GetAbilitySystemComponentFromActorInfo_Ensured()` | 函数重命名 |
| `AbilityTags` (直接访问) | `GetAssetTags()` / `SetAssetTags()` | 将变为私有只读，`SetAssetTags()` 只能在构造函数中调用 |

### 5.4 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `SetMarkPendingKillOnAbilityEnd()` | 无 | 不安全，已被忽略 |
| `IsMarkPendingKillOnAbilityEnd()` | 无 | 不安全，总是返回 false |
| `bMarkPendingKillOnAbilityEnd` | 无 | 不安全，不要使用 |

### 5.3 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `SetMovementSyncPoint()` | 无 | 无用途，将被移除 |

---

## 2. UAbilitySystemComponent (AbilitySystemComponent.h)

### 5.5 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `InternalServerTryActiveAbility()` (公开访问) | `RemoveActiveGameplayEffect()` 或 `RemoveActiveGameplayEffect_AllowClientRemoval()` | 不应公开 |

### 5.4 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `SetActiveGameplayEffectInhibit()` | 使用 `MoveTemp(ActiveGEHandle)` 版本 | Handle 使用后不再有效 |
| `ReinvokeActiveGameplayCues()` | 自行实现 | 未使用且逻辑不一致 |

### 5.3 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `FindAbilitySpecFromGEHandle()` | 无 | 不准确，现返回 nullptr |
| `OnImmunityBlockGameplayEffect` | `OnImmunityBlockGameplayEffectDelegate` | 直接使用委托 |

### 5.1 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `SpawnedAttributes` (直接访问) | `Add/RemoveSpawnedAttributes()` | 将变为私有 |
| `ReplicatedInstancedAbilities` | `GetReplicatedInstancedAbilities()` / `Add/RemoveReplicatedInstancedAbility()` | 将变为私有 |

### 4.26 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `ClientDebugStrings` | `Set/GetClientDebugStrings()` | 将变为私有 |
| `ServerDebugStrings` | `Set/GetServerDebugStrings()` | 将变为私有 |
| `RepAnimMontageInfo` | `Set/GetRepAnimMontageInfo()` | 将变为私有 |
| `BlockedAbilityBindings` | `Set/GetBlockedAbilityBindings()` | 将变为私有 |
| `MinimalReplicationTags` | `Set/GetMinimalReplicationTags()` | 将变为私有 |

---

## 3. UGameplayEffect (GameplayEffect.h)

### 5.5 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `SetBaseAttributeValueFromReplication()` | 使用 `FGameplayAttributeData` 版本 | 签名变更 |

### 5.3 废弃 (大量重构为 Component 模式)

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `ChanceToApplyToTarget` | `UChanceToApplyGameplayEffectComponent` | 使用组件 |
| `ApplicationRequirements` | `UCustomCanApplyGameplayEffectComponent` | 使用组件 |
| `ConditionalGameplayEffects` | `UAdditionalEffectsGameplayEffectComponent` | 使用组件 |
| `PrematureExpirationEffectClasses` | `UAdditionalEffectsGameplayEffectComponent` | 使用组件 |
| `RoutineExpirationEffectClasses` | `UAdditionalEffectsGameplayEffectComponent` | 使用组件 |
| `UIData` | `UGameplayEffectUIData` (作为组件) | 使用 `FindComponent<>()` |
| `InheritableGameplayEffectTags` | `UAssetTagsGameplayEffectComponent` | 使用组件 |
| `InheritableOwnedTagsContainer` | `UTargetTagsGameplayEffectComponent` | 使用组件 |
| `InheritableBlockedAbilityTagsContainer` | `UTargetTagsGameplayEffectComponent` | 使用组件 |
| `OngoingTagRequirements` | `UTargetTagRequirementsGameplayEffectComponent` | 使用组件 |
| `ApplicationTagRequirements` | `UTargetTagRequirementsGameplayEffectComponent` | 使用组件 |
| `RemovalTagRequirements` | `URemoveOtherGameplayEffectComponent` | 使用组件 |
| `RemoveGameplayEffectsWithTags` | `UTargetTagRequirementsGameplayEffectComponent` | 使用组件 |
| `GrantedApplicationImmunityTags` | `UImmunityGameplayEffectComponent` | 使用组件 |
| `GrantedApplicationImmunityQuery` | `UImmunityGameplayEffectComponent` | 使用组件 |
| `RemoveGameplayEffectQuery` | `URemoveOtherGameplayEffectComponent` | 使用组件 |
| `GrantedAbilities` | `UAbilitiesGameplayEffectComponent` | 使用组件 |
| `CheckOngoingTagRequirements()` | `UTargetTagRequirementsGameplayEffectComponent` | 使用组件 |
| `CheckRemovalTagRequirements()` | `UTargetTagRequirementsGameplayEffectComponent` | 使用组件 |
| `GetOwnedTagsContainer()` | `GetGrantedTags()` | 方法名与实现不匹配 |
| `HasOwnedTag()` | `GetGrantedTags().HasTag()` | 方法名与实现不匹配 |
| `HasAllOwnedTags()` | `GetGrantedTags().HasAll()` | 方法名与实现不匹配 |
| `HasAnyOwnedTag()` | `GetGrantedTags().HasAny()` | 方法名与实现不匹配 |
| `StackCount` | `GetStackCount()` / `SetStackCount()` | 将变为私有 |

### 5.0 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `DynamicAssetTags` | `AddDynamicAssetTag()` / `AppendDynamicAssetTags()` / `GetDynamicAssetTags()` | 不再支持移除 |

### 4.17 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `GetGameplayAttributeValueChangeDelegate()` (旧签名) | 新签名版本 | 委托签名变更 |

---

## 4. FGameplayAbilitySpec (GameplayAbilitySpec.h)

### 5.5 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `ActivationInfo` | 实例化 Ability 的 `CurrentActivationInfo` | 仅适用于已废弃的 NonInstanced |
| `DynamicAbilityTags` | `GetDynamicSpecSourceTags()` | 更准确的命名 |

---

## 5. EGameplayAbilityInstancingPolicy (GameplayAbilityTypes.h)

### 5.5 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `NonInstanced` | `InstancedPerActor` | 避免混淆的边界情况 |

---

## 6. UAbilitySystemGlobals (AbilitySystemGlobals.h)

### 5.5 废弃 (大量配置移至 Project Settings)

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `AbilitySystemGlobalsClassName` | Project Settings + `GetDefault<UGameplayAbilitiesDeveloperSettings>()` | 配置移至项目设置 |
| `bUseDebugTargetFromHud` | Project Settings | 配置移至项目设置 |
| `ActivateFailCooldownTag` | Project Settings | 配置移至项目设置 |
| `ActivateFailCostTag` | Project Settings | 配置移至项目设置 |
| `ActivateFailTagsBlockedTag` | Project Settings | 配置移至项目设置 |
| `ActivateFailTagsMissingTag` | Project Settings | 配置移至项目设置 |
| `ActivateFailNetworkingTag` | Project Settings | 配置移至项目设置 |
| `MinimalReplicationTagCountBits` | Project Settings | 配置移至项目设置 |
| `bAllowGameplayModEvaluationChannels` | Project Settings | 配置移至项目设置 |
| `DefaultGameplayModEvaluationChannel` | Project Settings | 配置移至项目设置 |
| `GameplayModEvaluationChannelAliases` | Project Settings | 配置移至项目设置 |
| `GlobalCurveTableName` | Project Settings | 配置移至项目设置 |
| `GlobalAttributeMetaDataTableName` | Project Settings | 配置移至项目设置 |
| `GlobalAttributeSetDefaultsTableName` | `GetGlobalAttributeSetDefaultsTablePaths()` | 改为数组 |
| `GlobalAttributeSetDefaultsTableNames` | Project Settings | 配置移至项目设置 |
| `GlobalGameplayCueManagerClass` | Project Settings | 配置移至项目设置 |
| `GlobalGameplayCueManagerName` | Project Settings | 配置移至项目设置 |
| `GameplayCueNotifyPaths` | `Get/Add/RemoveGameplayCueNotifyPath()` | 将变为私有 |
| `GameplayTagResponseTableName` | Project Settings | 配置移至项目设置 |

### 5.3 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `bIgnoreCooldowns` | `CVarAbilitySystemIgnoreCooldowns` | 使用 CVar |
| `bIgnoreCosts` | `CVarAbilitySystemIgnoreCosts` | 使用 CVar |
| `ShouldIgnoreCooldowns()` | `bIgnoreAbilitySystemCooldowns` (namespace) | 使用 CVar |
| `ShouldIgnoreCosts()` | `bIgnoreAbilitySystemCosts` (namespace) | 使用 CVar |

---

## 7. UAbilitySystemBlueprintLibrary (AbilitySystemBlueprintLibrary.h)

### 5.5 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `MakeSpecHandle()` | `MakeSpecHandleByClass()` | 更安全，需要 CDO |

### 5.3 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `AddLinkedGameplayEffect()` | `UAdditionalGameplayEffectsComponent` | Linked GE 不复制 |
| `AddLinkedGameplayEffectSpec()` | `UAdditionalGameplayEffectsComponent` | Linked GE 不复制 |
| `GetAllLinkedGameplayEffectSpecHandles()` | `UAdditionalGameplayEffectsComponent` | Linked GE 不复制 |

---

## 8. AbilityTask (AbilityTask_PlayMontageAndWait.h)

### 5.3 废弃

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `OnInterrupted` | `OnGameplayAbilityCancelled` | 命名更准确 |

---

## 9. 其他

### GameplayCue_Types.h (5.3)

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `FGCNotifyActorKey` | 见构造函数注释 | 结构体废弃 |

### GameplayCueNotify_Actor.h (5.3)

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `NotifyKey` | 无 | 未使用 |

### GameplayAbilityWorldReticle.h / GameplayAbilityTargetActor.h (5.1)

| 废弃 API | 替代方案 | 说明 |
|---------|---------|------|
| `MasterPC` | `PrimaryPC` | 属性重命名 |

---

## 总结：UE 5.3+ GAS 架构变化趋势

1. **Component 化**: GameplayEffect 的很多功能被拆分为独立的 `UGameplayEffectComponent`
2. **私有化**: 很多公开属性将变为私有，提供 Getter/Setter
3. **配置外移**: 全局配置从代码移至 Project Settings
4. **CVar 化**: 调试开关改用 Console Variable
5. **命名规范化**: 修正命名与实现不匹配的 API

---

## 推荐做法

```cpp
// UE 5.5 风格的 Ability 构造函数
UMyAbility::UMyAbility()
{
    // ✅ 正确：使用 SetAssetTags
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(MyTag);
    SetAssetTags(AssetTags);
    
    // ❌ 错误：直接访问 AbilityTags (已废弃)
    // AbilityTags.AddTag(MyTag);
    
    // ✅ 这些仍然可以直接设置
    ActivationOwnedTags.AddTag(...);
    ActivationRequiredTags.AddTag(...);
    ActivationBlockedTags.AddTag(...);
    BlockAbilitiesWithTag.AddTag(...);
    
    // ✅ 使用 InstancedPerActor 替代 NonInstanced
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
```
