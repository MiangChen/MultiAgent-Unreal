#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MAAgent.generated.h"

class UMAAbilitySystemComponent;
class AMAPickupItem;

// Agent 类型枚举
UENUM(BlueprintType)
enum class EAgentType : uint8
{
    Human,
    RobotDog,
    Drone,
    Camera
};

UCLASS()
class MULTIAGENT_API AMAAgent : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMAAgent();

    // ========== IAbilitySystemInterface ==========
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ========== 司能 (GAS Abilities) ==========
    // 尝试拾取物品
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryPickup();

    // 尝试放下物品
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryDrop();

    // 尝试导航到目标位置 (GAS 版本)
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryNavigateTo(FVector Destination);

    // 取消导航
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void CancelNavigation();

    // 追踪另一个 Agent (使用 Ground Truth 位置)
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryFollowAgent(AMAAgent* TargetAgent, float FollowDistance = 200.f);

    // 停止追踪
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void StopFollowing();

    // 获取当前持有的物品
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    AMAPickupItem* GetHeldItem() const;

    // 是否持有物品
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool IsHoldingItem() const;

    // ========== Properties ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    int32 AgentID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    EAgentType AgentType;

    // 移动状态 (由 GA_Navigate 管理)
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

    // ========== 头顶状态显示 ==========
    // 显示技能激活信息
    UFUNCTION(BlueprintCallable, Category = "Status")
    void ShowStatus(const FString& Text, float Duration = 3.0f);

    // 显示技能激活（带参数）
    void ShowAbilityStatus(const FString& AbilityName, const FString& Params = TEXT(""));

    // 是否启用头顶状态显示
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool bShowStatusAboveHead = true;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PossessedBy(AController* NewController) override;

    // 导航期间每帧调用，子类可重写以实现动画驱动等逻辑
    virtual void OnNavigationTick();

    // 司能组件 (ASC)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UMAAbilitySystemComponent* AbilitySystemComponent;

private:
    // 头顶状态显示
    FString CurrentStatusText;
    float StatusDisplayEndTime = 0.f;
    void DrawStatusText();
};
