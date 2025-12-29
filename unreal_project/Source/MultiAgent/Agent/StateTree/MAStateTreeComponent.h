// MAStateTreeComponent.h
// 自定义 StateTreeComponent，支持在 C++ 中动态设置 StateTree Asset
// 支持根据配置禁用 StateTree

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "MAStateTreeComponent.generated.h"

class UStateTree;

/**
 * 自定义 StateTreeComponent
 * 暴露 SetStateTree 方法，允许在 C++ 中动态设置 StateTree Asset
 * 支持根据 simulation.json 中的 use_state_tree 配置禁用
 * 注意: StateTree Asset 需要使用 "StateTree Component" Schema (不是 AI Component)
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMAStateTreeComponent : public UStateTreeComponent
{
    GENERATED_BODY()

public:
    UMAStateTreeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 生命周期
    virtual void BeginPlay() override;

    // 设置 StateTree Asset
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    void SetStateTreeAsset(UStateTree* InStateTree);
    
    // 检查是否已设置 StateTree
    UFUNCTION(BlueprintPure, Category = "StateTree")
    bool HasStateTree() const;
    
    // 检查 StateTree 是否启用（根据配置）
    UFUNCTION(BlueprintPure, Category = "StateTree")
    bool IsStateTreeEnabled() const { return bStateTreeEnabled; }
    
    // 禁用 StateTree（停止运行，不再初始化）
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    void DisableStateTree();
    
    // 启用 StateTree
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    void EnableStateTree();

private:
    // 从配置管理器读取 use_state_tree 设置
    bool ShouldUseStateTree() const;
    
    // StateTree 是否启用
    bool bStateTreeEnabled = true;
};
