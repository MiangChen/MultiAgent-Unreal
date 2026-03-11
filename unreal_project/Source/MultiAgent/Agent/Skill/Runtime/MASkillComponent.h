// MASkillComponent.h
// ========== 技能管理层 ==========
// 负责单个机器人的技能管理：参数设置、技能模拟、反馈生成

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Agent/Skill/Domain/MASkillTypes.h"
#include "Agent/Skill/Feedback/MASkillFeedbackTypes.h"
#include "Agent/Skill/Runtime/MASkillRuntimeTypes.h"
#include "MASkillComponent.generated.h"

class AMACharacter;
class FMASkillBootstrap;
struct FMASkillRuntimeGateway;

UCLASS()
class MULTIAGENT_API UMASkillComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UMASkillComponent();

    void InitializeSkills(AActor* InOwnerActor);

    // ========== 命令系统 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Command")
    void ClearAllCommands();
    
    /** 取消所有正在执行的技能 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void CancelAllSkills();

    // ========== 技能参数 ==========
    
    const FMASkillParams& GetSkillParams() const { return SkillParams; }
    FMASkillParams& GetSkillParamsMutable() { return SkillParams; }

    const FMASkillRuntimeTargets& GetSkillRuntimeTargets() const { return SkillRuntimeTargets; }
    FMASkillRuntimeTargets& GetSkillRuntimeTargetsMutable() { return SkillRuntimeTargets; }
    void ResetSkillRuntimeTargets() { SkillRuntimeTargets.Reset(); }

    // ========== 反馈上下文 ==========
    
    const FMAFeedbackContext& GetFeedbackContext() const { return FeedbackContext; }
    FMAFeedbackContext& GetFeedbackContextMutable() { return FeedbackContext; }
    void ResetFeedbackContext() { FeedbackContext.Reset(); }
    
    void UpdateFeedback(int32 Current, int32 Total);
    void AddFoundObject(const FString& ObjectName, const FVector& Location, const TMap<FString, FString>& Attributes);

    // ========== 搜索结果缓存 ==========
    
    const FMASearchRuntimeResults& GetSearchResults() const { return SearchResults; }
    FMASearchRuntimeResults& GetSearchResultsMutable() { return SearchResults; }
    void ResetSearchResults() { SearchResults.Reset(); }

    // ========== 能量系统 ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float Energy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float MaxEnergy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float EnergyDrainRate = 1.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float LowEnergyThreshold = 20.f;

    UFUNCTION(BlueprintCallable, Category = "Energy")
    void DrainEnergy(float DeltaTime);
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    void RestoreEnergy(float Amount);
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    float GetEnergy() const { return Energy; }
    
    /** 设置电量值（会检测低电量阈值并触发 OnLowEnergy） */
    UFUNCTION(BlueprintCallable, Category = "Energy")
    void SetEnergy(float NewEnergy);
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    float GetEnergyPercent() const { return MaxEnergy > 0.f ? (Energy / MaxEnergy) * 100.f : 0.f; }
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool IsLowEnergy() const { return GetEnergyPercent() < LowEnergyThreshold; }
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool IsFullEnergy() const { return Energy >= MaxEnergy; }
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool HasEnergy() const { return Energy > 0.f; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnergyDepleted);
    UPROPERTY(BlueprintAssignable, Category = "Energy")
    FOnEnergyDepleted OnEnergyDepleted;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLowEnergy);
    UPROPERTY(BlueprintAssignable, Category = "Energy")
    FOnLowEnergy OnLowEnergy;

    // ========== 技能完成委托 ==========
    
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillCompleted, AMACharacter*, Agent, bool, bSuccess, const FString&, Message);
    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillCompleted OnSkillCompleted;
    
    /** 通知技能完成（由 StateTree 或技能实现调用） */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void NotifySkillCompleted(bool bSuccess, const FString& Message = TEXT(""));

private:
    friend class FMASkillBootstrap;
    friend struct FMASkillRuntimeGateway;

    void PrepareNavigateActivation(FVector TargetLocation);
    void PrepareFollowActivation(AActor* TargetActor, float FollowDistance);
    void PrepareChargeActivation();

    bool TryActivateSkillHandle(FGameplayAbilitySpecHandle AbilityHandle, const TCHAR* SkillName);
    void CancelSkillHandleIfValid(FGameplayAbilitySpecHandle AbilityHandle);

    FGameplayAbilitySpecHandle NavigateSkillHandle;
    FGameplayAbilitySpecHandle FollowSkillHandle;
    FGameplayAbilitySpecHandle ChargeSkillHandle;
    FGameplayAbilitySpecHandle SearchSkillHandle;
    FGameplayAbilitySpecHandle PlaceSkillHandle;
    FGameplayAbilitySpecHandle TakeOffSkillHandle;
    FGameplayAbilitySpecHandle LandSkillHandle;
    FGameplayAbilitySpecHandle ReturnHomeSkillHandle;
    FGameplayAbilitySpecHandle TakePhotoSkillHandle;
    FGameplayAbilitySpecHandle BroadcastSkillHandle;
    FGameplayAbilitySpecHandle HandleHazardSkillHandle;
    FGameplayAbilitySpecHandle GuideSkillHandle;

    UPROPERTY()
    FMASkillParams SkillParams;

    UPROPERTY()
    FMASkillRuntimeTargets SkillRuntimeTargets;
    
    UPROPERTY()
    FMAFeedbackContext FeedbackContext;
    
    UPROPERTY()
    FMASearchRuntimeResults SearchResults;
};
