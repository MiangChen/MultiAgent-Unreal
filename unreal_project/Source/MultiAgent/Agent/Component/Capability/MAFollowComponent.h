// MAFollowComponent.h
// 跟随能力组件 - 实现 IMAFollowable 接口
// 存储跟随目标参数

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Interface/MAAgentInterfaces.h"
#include "../../Character/MACharacter.h"
#include "MAFollowComponent.generated.h"

/**
 * 跟随能力组件 - 管理 Agent 的跟随参数
 */
UCLASS(ClassGroup=(MultiAgent), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMAFollowComponent : public UActorComponent, public IMAFollowable
{
    GENERATED_BODY()

public:
    UMAFollowComponent();

    // ========== IMAFollowable Interface ==========
    virtual AMACharacter* GetFollowTarget() const override { return FollowTarget.Get(); }
    virtual void SetFollowTarget(AMACharacter* Target) override { FollowTarget = Target; }
    virtual void ClearFollowTarget() override { FollowTarget.Reset(); }

    // ========== 属性 ==========
    
    // 跟随目标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    TWeakObjectPtr<AMACharacter> FollowTarget;

    // ========== 辅助方法 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    bool HasFollowTarget() const { return FollowTarget.IsValid(); }
};
