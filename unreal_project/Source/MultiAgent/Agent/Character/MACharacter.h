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

    // ========== 技能接口 ==========
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool TryNavigateTo(FVector Destination);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void CancelNavigation();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryFollowActor(AMACharacter* TargetActor, float FollowDistance = 200.f);

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
    FString AgentName;

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

    // ========== 能量系统 ==========
    /** 检查是否应该消耗能量（根据配置） */
    UFUNCTION(BlueprintPure, Category = "Energy")
    bool ShouldDrainEnergy() const;

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

    // ========== 描边效果 ==========
    UFUNCTION(BlueprintCallable, Category = "Outline")
    virtual void EnableOutline();
    
    UFUNCTION(BlueprintCallable, Category = "Outline")
    virtual void DisableOutline();

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

private:
    FString CurrentStatusText;
    float StatusDisplayEndTime = 0.f;
    void DrawStatusText();
};
