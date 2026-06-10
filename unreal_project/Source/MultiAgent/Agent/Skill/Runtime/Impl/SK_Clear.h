// SK_Clear.h
// 清洗技能 - 对平面型目标对象的大平整面进行清洗
//
// 执行流程:
// 1. 解析目标对象，依据其包围盒推导最大平整面及其外法向量
// 2. 在距离该平面 StandoffDistance 的平行平面上生成航点（四角 + 中心）
// 3. 机器人依次飞经各航点，朝向跟随移动方向
// 4. 全程开启直线喷水特效，喷射方向平行于平面法向量（指向目标面）
//
// 假设：清洗目标为静止物体。

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Clear.generated.h"

class UMANavigationService;
class AMAWaterSpray;
class UMASkillComponent;
class AMACharacter;

/** 清洗技能执行阶段 */
UENUM()
enum class EClearPhase : uint8
{
    MoveToWaypoint,  // 飞往当前航点
    Complete         // 完成
};

/**
 * 清洗技能
 */
UCLASS()
class MULTIAGENT_API USK_Clear : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Clear();

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

private:
    //=========================================================================
    // 配置 (从 simulation.json 的 clear 段加载)
    //=========================================================================

    /** 清洗作业平面与目标平整面的间距 (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float StandoffDistance = 1000.f;

    /** 直线喷水初速度 (cm/s) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float SpraySpeed = 4000.f;

    /** 直线喷水水柱宽度 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float SprayWidth = 8.f;

    /** 航点间移动速度 (cm/s)，0 表示使用默认速度 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float MoveSpeed = 0.f;

    /** 到达航点判定半径 (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float AcceptanceRadius = 150.f;

    //=========================================================================
    // 状态
    //=========================================================================

    EClearPhase CurrentPhase = EClearPhase::MoveToWaypoint;
    bool bClearSucceeded = false;
    FString ClearResultMessage;
    float StartTime = 0.f;

    bool bIsAircraft = false;
    float MinFlightAltitude = 800.f;

    /** 作业航点（已含 standoff 偏移与高度修正） */
    TArray<FVector> Waypoints;
    int32 CurrentWaypointIndex = 0;

    /** 喷射方向：平行于目标平面法向量、指向目标面（即内法向量，世界空间单位向量） */
    FVector SprayDirection = FVector::ForwardVector;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    //=========================================================================
    // 引用
    //=========================================================================

    UPROPERTY()
    TObjectPtr<UMANavigationService> NavigationService;

    UPROPERTY()
    TObjectPtr<AMAWaterSpray> WaterSpray;

    TWeakObjectPtr<AActor> TargetActor;

    FTimerHandle SprayRefreshTimerHandle;

    //=========================================================================
    // 清洗视觉效果 - 目标材质渐变（脏色 → 白色）
    //=========================================================================

    /** 目标 mesh 的动态材质实例（用于运行时调色） */
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> TargetDynMaterial;

    /** 脏色（清洗开始时的初始颜色乘数） */
    FLinearColor DirtyTintColor = FLinearColor(0.55f, 0.50f, 0.40f, 1.f);

    /** 干净色（清洗结束时的最终颜色乘数 = 不染色） */
    FLinearColor CleanTintColor = FLinearColor::White;

    //=========================================================================
    // 流程
    //=========================================================================

    bool InitializeClearContext(AMACharacter& Character, UMASkillComponent& SkillComp);

    /** 依据目标包围盒计算最大平整面的外法向量与作业航点 */
    bool ComputeFaceWaypoints(const AMACharacter& Character, const AActor& Target);

    void StartSpray();
    void RefreshSprayDirection();
    void CleanupSpray();

    /** 初始化目标材质为脏色 DMI */
    void InitializeTargetMaterial();

    /** 按清洗进度更新目标材质颜色 */
    void UpdateTargetMaterialProgress();

    void MoveToCurrentWaypoint();
    void AdvanceWaypoint();
    void CompleteClear();
    void FailClear(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage = FString());

    void ResetClearRuntimeState();

    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);
};
