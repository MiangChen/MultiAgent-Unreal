// MAAbilitySystemComponent.h
// GAS 核心组件，挂载到 Agent 上管理所有 Ability

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Ability/GA_Formation.h"
#include "MAAbilitySystemComponent.generated.h"

class AMAPatrolPath;

UCLASS()
class MULTIAGENT_API UMAAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UMAAbilitySystemComponent();

    // 初始化默认 Abilities
    void InitializeAbilities(AActor* InOwnerActor);

    // 直接激活 Pickup Ability (推荐使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivatePickup();

    // 直接激活 Drop Ability (推荐使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateDrop();

    // 直接激活 Navigate Ability (推荐使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateNavigate(FVector TargetLocation);

    // 取消导航
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelNavigate();

    // 直接激活 Follow Ability (追踪目标 Character)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateFollow(class AMACharacter* TargetCharacter, float FollowDistance = 200.f);

    // 取消追踪
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelFollow();

    // 直接激活 TakePhoto Ability (Camera 使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateTakePhoto();

    // ========== 新增 Robot Abilities ==========
    // 注意: Patrol 已移至 StateTree，不再使用 GAS Ability
    
    // 搜索
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateSearch();
    
    // 取消搜索
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelSearch();
    
    // 观察
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateObserve(AActor* Target = nullptr);
    
    // 取消观察
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelObserve();
    
    // 报告
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateReport(const FString& Message);
    
    // 充电
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateCharge();
    
    // 编队
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateFormation(AMACharacter* Leader, EFormationType Type, int32 Position);
    
    // 取消编队
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelFormation();
    
    // 避障
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateAvoid(FVector Destination);
    
    // 取消避障
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelAvoid();

    // 通过 Tag 激活 Ability (StateTree 使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateAbilityByTag(FGameplayTag AbilityTag);

    // 通过 Tag 取消 Ability (StateTree 使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelAbilityByTag(FGameplayTag AbilityTag);

    // 检查是否有某个 Tag
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool HasGameplayTagFromContainer(FGameplayTag TagToCheck) const;

    // ========== 命令系统 (用于 StateTree) ==========
    
    // 发送命令 (添加 Command Tag)
    UFUNCTION(BlueprintCallable, Category = "Command")
    void SendCommand(FGameplayTag CommandTag);
    
    // 清除命令 (移除 Command Tag)
    UFUNCTION(BlueprintCallable, Category = "Command")
    void ClearCommand(FGameplayTag CommandTag);
    
    // 清除所有命令
    UFUNCTION(BlueprintCallable, Category = "Command")
    void ClearAllCommands();

protected:
    // 默认授予的 Ability 类列表
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 保存的 Ability Handle，用于直接激活
    FGameplayAbilitySpecHandle PickupAbilityHandle;
    FGameplayAbilitySpecHandle DropAbilityHandle;
    FGameplayAbilitySpecHandle NavigateAbilityHandle;
    FGameplayAbilitySpecHandle FollowAbilityHandle;
    FGameplayAbilitySpecHandle TakePhotoAbilityHandle;
    
    // 新增 Ability Handles
    // 注意: PatrolAbilityHandle 已移除，Patrol 改用 StateTree
    FGameplayAbilitySpecHandle SearchAbilityHandle;
    FGameplayAbilitySpecHandle ObserveAbilityHandle;
    FGameplayAbilitySpecHandle ReportAbilityHandle;
    FGameplayAbilitySpecHandle ChargeAbilityHandle;
    FGameplayAbilitySpecHandle FormationAbilityHandle;
    FGameplayAbilitySpecHandle AvoidAbilityHandle;
};
