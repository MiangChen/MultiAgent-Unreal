// SK_TakePhoto.h
// 拍照技能 - 多阶段实现
// 1. 移动到安全距离
// 2. 转向目标
// 3. 显示相机画中画 (通过 Skill PIP bridge)
// 4. 拍照完成

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_TakePhoto.generated.h"

class UMANavigationService;
class UMASkillComponent;
class AMACharacter;

/** 技能执行阶段 */
UENUM()
enum class ETakePhotoPhase : uint8
{
    MoveToDistance,   // 移动到拍照距离
    TurnToTarget,     // 转向目标
    TakePhoto,        // 拍照中（显示相机画面）
    Complete          // 完成
};

UCLASS()
class MULTIAGENT_API USK_TakePhoto : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_TakePhoto();

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

    void ResetTakePhotoRuntimeState();
    void FailTakePhoto(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage = FString());
    bool InitializeTakePhotoContext(AMACharacter& Character, UMASkillComponent& SkillComp);
    void HandleNavigationFailure(const FString& Message);
    void CompleteTakePhoto();
    void CleanupTakePhotoRuntime(AMACharacter* Character, bool bWasCancelled, bool& bOutSuccessToNotify, FString& InOutMessageToNotify);

private:
    //=========================================================================
    // 配置
    //=========================================================================
    
    /** 拍照距离 (cm) - 与目标保持的水平距离 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float PhotoDistance = 500.f;

    /** 拍照持续时间 (秒) - 相机画面显示时长 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float PhotoDuration = 3.0f;

    /** 相机视场角 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float CameraFOV = 60.f;

    /** 相机前置偏移 (cm) - 模拟眼睛在前部 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float CameraForwardOffset = 50.f;

    //=========================================================================
    // 状态
    //=========================================================================
    
    ETakePhotoPhase CurrentPhase = ETakePhotoPhase::MoveToDistance;
    bool bPhotoSucceeded = false;
    FString PhotoResultMessage;
    
    /** 是否是飞行机器人 */
    bool bIsAircraft = false;
    
    /** 飞行机器人最小高度限制 (cm) */
    float MinFlightAltitude = 800.f;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    //=========================================================================
    // 引用
    //=========================================================================
    
    UPROPERTY()
    TObjectPtr<UMANavigationService> NavigationService;

    /** 画中画相机 ID */
    FGuid PIPCameraId;

    /** 目标 Actor */
    TWeakObjectPtr<AActor> TargetActor;
    
    /** 目标位置 */
    FVector TargetLocation;
    
    /** 目标名称 */
    FString TargetName;
    
    FTimerHandle PhotoTimerHandle;
    FTimerHandle TurnTimerHandle;

    //=========================================================================
    // 阶段处理
    //=========================================================================
    
    void UpdatePhase();
    void HandleMoveToDistance();
    void HandleTurnToTarget();
    void HandleTakePhoto();
    void HandleComplete();

    //=========================================================================
    // 回调
    //=========================================================================
    
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);

    void OnTurnTick();
    void OnPhotoComplete();

    //=========================================================================
    // 辅助
    //=========================================================================
    
    FVector CalculatePhotoPosition() const;
    void CreatePIPCamera();
    void ShowPIPCamera();
    void HidePIPCamera();
    void CleanupPIPCamera();
    FString GetPhaseString() const;
};
