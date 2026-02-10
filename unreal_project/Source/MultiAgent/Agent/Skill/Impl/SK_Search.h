// SK_Search.h
// 搜索技能 - 在指定区域内搜索目标
// 支持两种模式：Coverage (覆盖搜索) 和 Patrol (巡逻模式)
// 支持飞行机器人和地面机器人

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Search.generated.h"

class UMANavigationService;
class UMAPIPCameraManager;

/**
 * 搜索模式枚举
 * Coverage: 覆盖搜索模式，使用割草机式路径完整覆盖搜索区域
 * Patrol: 巡逻模式，在区域内关键点之间循环巡视
 */
UENUM(BlueprintType)
enum class ESearchMode : uint8
{
    Coverage    UMETA(DisplayName = "Coverage Search"),   // 覆盖搜索模式 (割草机路径)
    Patrol      UMETA(DisplayName = "Patrol")             // 巡逻模式 (关键点循环)
};

UCLASS()
class MULTIAGENT_API USK_Search : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Search();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // 生成搜索路径 (根据模式和机器人类型)
    void GenerateSearchPath();
    
    // 导航完成回调
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);
    
    // 处理航点到达
    void HandleWaypointArrival();
    
    // 导航到下一个航点
    void NavigateToNextWaypoint();
    
    // 开始航点等待 (巡逻模式)
    void StartWaypointWait();
    
    // 等待完成回调
    void OnWaitCompleted();
    
    // 画中画相机展示
    void StartPIPCapture();                    // 开始画中画展示序列 (Patrol 模式)
    void ShowNextPIPView();                    // 展示下一个视角
    void TurnToDirection(const FRotator& TargetRotation);  // 转向指定方向
    void OnTurnComplete();                     // 转向完成回调
    void CreateAndShowPIP(const FRotator& CameraRotation); // 创建并展示画中画
    void OnPIPDisplayComplete();               // 单张画中画展示完成
    void CleanupPIP();                         // 清理画中画资源
    TArray<FRotator> GenerateCameraDirections() const;     // 生成相机方向列表
    
    // Coverage 模式持续相机
    void CreateCoverageCamera();               // 创建持续跟随相机

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    // 搜索模式
    ESearchMode CurrentSearchMode = ESearchMode::Coverage;
    
    // 搜索航线
    TArray<FVector> SearchPath;
    int32 CurrentWaypointIndex = 0;
    int32 LastUIUpdateIndex = -1;  // 上次 UI 更新时的航点索引
    
    // 巡逻循环计数
    int32 PatrolCycleCount = 0;
    
    // 巡逻循环次数限制 (默认1次)
    int32 PatrolCycleLimit = 1;
    
    // 巡逻等待时间 (秒)
    float PatrolWaitTime = 2.0f;
    
    // 搜索参数
    UPROPERTY()
    float ScanWidth = 1600.f;  // 扫描宽度（默认 16m）
    
    // 导航服务引用
    UPROPERTY()
    TWeakObjectPtr<UMANavigationService> NavigationService;
    
    // 等待定时器
    FTimerHandle WaitTimerHandle;
    
    // 画中画相机
    UPROPERTY()
    TObjectPtr<UMAPIPCameraManager> PIPCameraManager;
    FGuid PIPCameraId;
    TArray<FRotator> PendingCameraDirections;  // 待展示的相机方向队列
    int32 CurrentPIPIndex = 0;                 // 当前展示的画中画索引
    FTimerHandle TurnTimerHandle;              // 转向定时器
    FTimerHandle PIPDisplayTimerHandle;        // 画中画展示定时器
    FRotator TargetTurnRotation;               // 目标转向角度
    static constexpr float PIPDisplayDuration = 2.0f;  // 每张画中画展示时长
    static constexpr float CameraForwardOffset = 50.f; // 相机前向偏移
    static constexpr float CameraFOV = 90.f;           // 相机视场角
    
    // 提前结束相关
    TArray<FVector> FoundTargetLocations;  // 已找到的目标位置
    bool bHasFoundTarget = false;  // 是否有找到目标
    
    // 连续导航失败计数
    int32 ConsecutiveNavFailures = 0;
    static constexpr int32 MaxConsecutiveNavFailures = 3;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bSearchSucceeded = false;
    FString SearchResultMessage;
};
