// MASkillComponent.h
// ========== 技能管理层 ==========
// 负责单个机器人的技能管理：参数设置、技能模拟、反馈生成

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Utils/MAFeedbackGenerator.h"
#include "MASkillComponent.generated.h"

class AMACharacter;

// 语义标签（用于 Search/Place）
USTRUCT(BlueprintType)
struct FMASemanticTarget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Class;  // object, robot, ground
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Type;   // boat, box, UGV
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> Features;
    
    bool IsEmpty() const { return Class.IsEmpty() && Type.IsEmpty(); }
    bool IsGround() const { return Class.Equals(TEXT("ground"), ESearchCase::IgnoreCase); }
    bool IsRobot() const { return Class.Equals(TEXT("robot"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("UGV"), ESearchCase::IgnoreCase); }
};

// 技能参数
USTRUCT(BlueprintType)
struct FMASkillParams
{
    GENERATED_BODY()

    // Navigate
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AcceptanceRadius = 100.f;

    // Follow
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AMACharacter> FollowTarget;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FollowDistance = 300.f;

    // Search
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> SearchBoundary;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget SearchTarget;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SearchScanWidth = 1600.f;  // 扫描宽度默认 16m，航点更稀疏

    // Place
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget PlaceObject1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget PlaceObject2;

    // TakeOff / Land / ReturnHome
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TakeOffHeight = 3000.f;  // 默认 30m (搜索高度)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LandHeight = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HomeLocation = FVector::ZeroVector;  // 返航点

    // Charge
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ChargingStationLocation = FVector::ZeroVector;  // 充电站位置
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChargingStationId;  // 充电站ID
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bChargingStationFound = false;  // 是否找到充电站

    // TakePhoto / Broadcast / HandleHazard 共用参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget CommonTarget;  // 目标对象语义标签
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CommonTargetObjectId;  // 目标对象 ID（优先使用）
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DefaultSearchRadius = 1000.f;  // 默认搜索半径 10m
    
    // Broadcast 专用参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString BroadcastMessage;  // 喊话内容
    
    // Guide 专用参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget GuideTarget;  // 引导目标
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString GuideTargetObjectId;  // 引导目标 ID
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector GuideDestination = FVector::ZeroVector;  // 引导目的地
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> GuideTargetActor;  // 引导目标 Actor（运行时缓存）
};

// 场景查询结果缓存
USTRUCT()
struct FMASearchResults
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Object1Actor;
    
    UPROPERTY()
    FVector Object1Location = FVector::ZeroVector;
    
    UPROPERTY()
    TWeakObjectPtr<AActor> Object2Actor;
    
    UPROPERTY()
    FVector Object2Location = FVector::ZeroVector;
    
    UPROPERTY()
    TArray<FVector> SearchPath;
    
    void Reset()
    {
        Object1Actor.Reset();
        Object2Actor.Reset();
        Object1Location = Object2Location = FVector::ZeroVector;
        SearchPath.Empty();
    }
};

UCLASS()
class MULTIAGENT_API UMASkillComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UMASkillComponent();

    void InitializeSkills(AActor* InOwnerActor);

    // ========== 技能激活接口 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateNavigate(FVector TargetLocation);
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void CancelNavigate();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateFollow(AMACharacter* TargetCharacter, float FollowDistance = 200.f);
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void CancelFollow();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateCharge();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateSearch();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void CancelSearch();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivatePlace();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void CancelPlace();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateTakeOff();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateLand();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateReturnHome();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void CancelReturnHome();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateTakePhoto();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateBroadcast();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateHandleHazard();
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool TryActivateGuide();

    // ========== 命令系统 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Command")
    void ClearAllCommands();
    
    /** 取消所有正在执行的技能 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void CancelAllSkills();

    // ========== 技能参数 ==========
    
    const FMASkillParams& GetSkillParams() const { return SkillParams; }
    FMASkillParams& GetSkillParamsMutable() { return SkillParams; }

    // ========== 反馈上下文 ==========
    
    const FMAFeedbackContext& GetFeedbackContext() const { return FeedbackContext; }
    FMAFeedbackContext& GetFeedbackContextMutable() { return FeedbackContext; }
    void ResetFeedbackContext() { FeedbackContext.Reset(); }
    
    void UpdateFeedback(int32 Current, int32 Total);
    void AddFoundObject(const FString& ObjectName, const FVector& Location, const TMap<FString, FString>& Attributes);

    // ========== 搜索结果缓存 ==========
    
    const FMASearchResults& GetSearchResults() const { return SearchResults; }
    FMASearchResults& GetSearchResultsMutable() { return SearchResults; }
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

    // ========== 技能完成委托 ==========
    
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillCompleted, AMACharacter*, Agent, bool, bSuccess, const FString&, Message);
    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillCompleted OnSkillCompleted;
    
    /** 通知技能完成（由 StateTree 或技能实现调用） */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void NotifySkillCompleted(bool bSuccess, const FString& Message = TEXT(""));

protected:
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

private:
    UPROPERTY()
    FMASkillParams SkillParams;
    
    UPROPERTY()
    FMAFeedbackContext FeedbackContext;
    
    UPROPERTY()
    FMASearchResults SearchResults;
};
