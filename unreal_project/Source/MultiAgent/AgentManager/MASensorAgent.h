// MASensorAgent.h
// 传感器 Agent 基类，支持 Attach 到其他 Agent

#pragma once

#include "CoreMinimal.h"
#include "MAAgent.h"
#include "MASensorAgent.generated.h"

UCLASS()
class MULTIAGENT_API AMASensorAgent : public AMAAgent
{
    GENERATED_BODY()

public:
    AMASensorAgent();

    // ========== Attach 功能 ==========
    
    // 附着到另一个 Agent
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    void AttachToAgent(AMAAgent* ParentAgent, FVector RelativeLocation, FRotator RelativeRotation = FRotator::ZeroRotator);

    // 解除附着
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    void DetachFromAgent();

    // 获取附着的父 Agent
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    AMAAgent* GetAttachedAgent() const;

    // 是否已附着
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    bool IsAttached() const;

protected:
    // 附着的父 Agent
    UPROPERTY()
    TWeakObjectPtr<AMAAgent> AttachedToAgent;
};
