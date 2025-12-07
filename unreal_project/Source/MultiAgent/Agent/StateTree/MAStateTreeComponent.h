// MAStateTreeComponent.h
// 自定义 StateTreeComponent，支持在 C++ 中动态设置 StateTree Asset

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "MAStateTreeComponent.generated.h"

class UStateTree;

/**
 * 自定义 StateTreeComponent
 * 暴露 SetStateTree 方法，允许在 C++ 中动态设置 StateTree Asset
 * 注意: StateTree Asset 需要使用 "StateTree Component" Schema (不是 AI Component)
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMAStateTreeComponent : public UStateTreeComponent
{
    GENERATED_BODY()

public:
    UMAStateTreeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 设置 StateTree Asset
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    void SetStateTreeAsset(UStateTree* InStateTree);
    
    // 检查是否已设置 StateTree
    UFUNCTION(BlueprintPure, Category = "StateTree")
    bool HasStateTree() const;
};
