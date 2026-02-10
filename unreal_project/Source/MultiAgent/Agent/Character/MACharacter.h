// MACharacter.h
// 机器人角色基类 - 支持技能系统
// Agent = Character + SkillComponent + Sensors + StateTree

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "../../Core/Types/MATypes.h"
#include "MACharacter.generated.h"

class UMASkillComponent;
class UMACameraSensorComponent;
class UMANavigationService;

UCLASS()
class MULTIAGENT_API AMACharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMACharacter();

    // ========== IAbilitySystemInterface ==========
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    UMASkillComponent* GetSkillComponent() const { return SkillComponent; }

    UFUNCTION(BlueprintCallable, Category = "Navigation")
    UMANavigationService* GetNavigationService() const { return NavigationService; }

    // ========== 技能接口 ==========
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool TryNavigateTo(FVector Destination);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void CancelNavigation();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryFollowActor(AActor* TargetActor, float FollowDistance = 300.f);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void StopFollowing();

    // ========== 技能集 ==========
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool HasSkill(EMASkillType Skill) const { return AvailableSkills.Contains(Skill); }
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    const TArray<EMASkillType>& GetAvailableSkills() const { return AvailableSkills; }

    // ========== 属性 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    EMAAgentType AgentType;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;
    
    /** 初始位置（用于返航） */
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    FVector InitialLocation;

    // ========== 直接控制 ==========
    UFUNCTION(BlueprintCallable, Category = "Control")
    void SetDirectControl(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Control")
    bool IsUnderDirectControl() const { return bIsUnderDirectControl; }

    UFUNCTION(BlueprintCallable, Category = "Control")
    void CancelAIMovement();

    UFUNCTION(BlueprintCallable, Category = "Control")
    void ApplyDirectMovement(FVector WorldDirection);

    // ========== 状态显示 ==========
    UFUNCTION(BlueprintCallable, Category = "Status")
    void ShowStatus(const FString& Text, float Duration = 3.0f);

    void ShowAbilityStatus(const FString& SkillName, const FString& Params = TEXT(""));

    /** 获取当前状态文字（供 HUD 绘制） */
    UFUNCTION(BlueprintPure, Category = "Status")
    FString GetCurrentStatusText() const;

    // ========== 能量系统 ==========
    /** 检查是否应该消耗能量（根据配置） */
    UFUNCTION(BlueprintPure, Category = "Energy")
    bool ShouldDrainEnergy() const;

    /** 低电量自动返航（由 OnLowEnergy 触发） */
    UFUNCTION()
    void OnLowEnergyReturn();

    /** 是否正在低电量返航中 */
    UFUNCTION(BlueprintPure, Category = "Energy")
    bool IsLowEnergyReturning() const { return bIsLowEnergyReturning; }

    /** 暂停状态变化回调（用于延迟执行低电量返航） */
    UFUNCTION()
    void OnPauseStateChanged(bool bPaused);

    /** 返航技能完成回调（清除低电量返航标记） */
    UFUNCTION()
    void OnReturnSkillCompleted(AMACharacter* Agent, bool bSuccess, const FString& Message);

    // ========== Sensor 管理 ==========
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    UMACameraSensorComponent* AddCameraSensor(FVector RelativeLocation, FRotator RelativeRotation = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    UMACameraSensorComponent* GetCameraSensor() const;

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    int32 GetSensorCount() const;

    // ========== Squad ==========
    UPROPERTY(BlueprintReadOnly, Category = "Squad")
    class UMASquad* CurrentSquad = nullptr;

    // ========== Action 系统 ==========
    virtual TArray<FString> GetAvailableActions() const { return TArray<FString>(); }
    virtual bool ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params) { return false; }

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Control")
    bool bIsUnderDirectControl = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TArray<EMASkillType> AvailableSkills;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnNavigationTick() {}
    
    // 子类重写以定义可用技能
    virtual void InitializeSkillSet() {}

    void AutoFitCapsuleToMesh();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    UMASkillComponent* SkillComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
    UMANavigationService* NavigationService;

private:
    FString CurrentStatusText;
    float StatusDisplayEndTime = 0.f;
    void DrawStatusText();
    
    /** 是否有待执行的低电量返航 */
    bool bPendingLowEnergyReturn = false;
    
    /** 是否正在执行低电量返航（用于条件检查豁免） */
    bool bIsLowEnergyReturning = false;
    
    /** 执行低电量返航的实际逻辑 */
    void ExecuteLowEnergyReturn();
};
