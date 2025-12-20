// MACharacter.h
// 可控角色基类 - 支持 GAS 技能系统
// Agent = Character + Components (Sensors, GAS, StateTree)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "../../Core/MATypes.h"
#include "MACharacter.generated.h"

class UMAAbilitySystemComponent;
class AMAPickupItem;
class UMASensorComponent;
class UMACameraSensorComponent;
class UMASquad;

UCLASS()
class MULTIAGENT_API AMACharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMACharacter();

    // ========== IAbilitySystemInterface ==========
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ========== 技能 (GAS Abilities) ==========
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryPickup();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryDrop();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    virtual bool TryNavigateTo(FVector Destination);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    virtual void CancelNavigation();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryFollowActor(AMACharacter* TargetActor, float FollowDistance = 200.f);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void StopFollowing();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    AMAPickupItem* GetHeldItem() const;

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool IsHoldingItem() const;

    // ========== Properties (Agent 命名) ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    EMAAgentType AgentType;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

    // ========== Squad 关联 ==========
    // 当前所属 Squad (nullptr 表示独立行动)
    UPROPERTY(BlueprintReadOnly, Category = "Squad")
    UMASquad* CurrentSquad = nullptr;

    // ========== Direct Control (WASD 直接控制) ==========
    // 设置直接控制状态
    UFUNCTION(BlueprintCallable, Category = "Control")
    void SetDirectControl(bool bEnabled);

    // 获取直接控制状态
    UFUNCTION(BlueprintCallable, Category = "Control")
    bool IsUnderDirectControl() const { return bIsUnderDirectControl; }

    // 取消 AI 移动（停止导航和 StateTree 移动命令）
    UFUNCTION(BlueprintCallable, Category = "Control")
    void CancelAIMovement();

    // 应用直接移动输入（基于世界方向）
    UFUNCTION(BlueprintCallable, Category = "Control")
    void ApplyDirectMovement(FVector WorldDirection);

    // ========== 跳跃 ==========
    /** 是否能执行跳跃 */
    UFUNCTION(BlueprintCallable, Category = "Jump")
    virtual bool CanPerformJump() const;

    /** 执行跳跃 (手动或自动触发) */
    UFUNCTION(BlueprintCallable, Category = "Jump")
    virtual void PerformJump();

    // ========== 头顶状态显示 ==========
    UFUNCTION(BlueprintCallable, Category = "Status")
    void ShowStatus(const FString& Text, float Duration = 3.0f);

    void ShowAbilityStatus(const FString& AbilityName, const FString& Params = TEXT(""));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool bShowStatusAboveHead = true;

    // ========== Sensor 管理 ==========
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    UMACameraSensorComponent* AddCameraSensor(FVector RelativeLocation, FRotator RelativeRotation = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    bool RemoveSensor(UMASensorComponent* Sensor);

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    TArray<UMASensorComponent*> GetAllSensors() const;

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    UMACameraSensorComponent* GetCameraSensor() const;

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    int32 GetSensorCount() const;

    // ========== Action 动态发现与执行 ==========
    
    // 获取该 Agent 所有可用的 Actions（包括自身和所有 Sensor 的）
    UFUNCTION(BlueprintCallable, Category = "Actions")
    TArray<FString> GetAvailableActions() const;
    
    // 执行指定的 Action
    UFUNCTION(BlueprintCallable, Category = "Actions")
    bool ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params);

protected:
    // 直接控制状态标志
    UPROPERTY(BlueprintReadOnly, Category = "Control")
    bool bIsUnderDirectControl = false;

    // ========== 跳跃参数 ==========
    /** 跳跃冷却计时器 */
    float JumpCooldownTimer = 0.f;
    
    /** 跳跃冷却时间 */
    float JumpCooldownDuration = 1.0f;
    
    /** 卡住检测时间 */
    float StuckTime = 0.f;
    
    /** 卡住阈值 (秒) */
    float StuckThreshold = 0.5f;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PossessedBy(AController* NewController) override;

    // 导航期间每帧调用，子类可重写
    virtual void OnNavigationTick();
    
    /** 检测卡住并自动跳跃 */
    virtual void CheckStuckAndAutoJump(float DeltaTime);
    
    // 根据 SkeletalMesh 自动计算 CapsuleComponent 大小
    void AutoFitCapsuleToMesh();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UMAAbilitySystemComponent* AbilitySystemComponent;

private:
    FString CurrentStatusText;
    float StatusDisplayEndTime = 0.f;
    void DrawStatusText();
};
