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
    Drone
};

UCLASS()
class MULTIAGENT_API AMAAgent : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMAAgent();

    // ========== IAbilitySystemInterface ==========
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ========== Movement ==========
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(FVector Destination);

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopMovement();

    UFUNCTION(BlueprintCallable, Category = "Agent")
    FVector GetCurrentLocation() const;

    // ========== GAS Abilities ==========
    // 尝试拾取物品
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryPickup();

    // 尝试放下物品
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryDrop();

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

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PossessedBy(AController* NewController) override;

    // GAS 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UMAAbilitySystemComponent* AbilitySystemComponent;

    // 更新动画（为动画蓝图提供加速度输入）
    void UpdateAnimation();

private:
    FVector TargetLocation;
};
