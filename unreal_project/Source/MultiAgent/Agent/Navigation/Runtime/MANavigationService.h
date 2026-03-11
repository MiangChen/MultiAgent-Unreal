// MANavigationService.h
// 统一导航服务组件
// 为 Navigate、Follow、Guide 技能提供统一的导航接口
// 支持所有继承 ACharacter 的实体（机器人、行人、车辆、船只等）

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Agent/Navigation/Domain/MANavigationTypes.h"
#include "../Infrastructure/MAPathPlanner.h"
#include "../Infrastructure/MAFlightController.h"
#include "MANavigationService.generated.h"

class ACharacter;
class AAIController;
class UWorld;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMANavigationService : public UActorComponent
{
    GENERATED_BODY()

public:
    UMANavigationService();

    //=========================================================================
    // 核心 API
    //=========================================================================

    /**
     * 导航到目标位置
     * @param Destination 目标位置
     * @param InAcceptanceRadius 到达判定半径
     * @param bSmoothArrival 是否平滑到达（接近目标时减速），默认 true
     * @return 是否成功启动导航
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    bool NavigateTo(FVector Destination, float InAcceptanceRadius = 200.f, bool bSmoothArrival = true);

    /**
     * 起飞到指定高度 (仅飞行机器人)
     * @param TargetAltitude 目标高度
     * @return 是否成功启动起飞
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    bool TakeOff(float TargetAltitude);

    /**
     * 降落到指定高度 (仅飞行机器人)
     * @param TargetAltitude 目标高度 (通常为地面高度)
     * @return 是否成功启动降落
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    bool Land(float TargetAltitude);

    /**
     * 返航：飞到指定位置并降落 (仅飞行机器人)
     * @param HomePosition 返航位置 (XY)
     * @param LandAltitude 降落高度
     * @return 是否成功启动返航
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    bool ReturnHome(FVector HomePosition, float LandAltitude);

    /**
     * 设置移动速度（仅对地面机器人有效）
     * @param Speed 目标速度，0 表示恢复默认速度
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void SetMoveSpeed(float Speed);

    /**
     * 恢复默认移动速度
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void RestoreDefaultSpeed();

    /**
     * 跟随目标 Actor（混合模式：远距离 NavMesh + 近距离直接跟随）
     * @param Target 目标 Actor
     * @param InFollowDistance 跟随距离
     * @param InAcceptanceRadius 到达判定半径
     * @return 是否成功启动跟随
     */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    bool FollowActor(AActor* Target, float InFollowDistance = 300.f, float InAcceptanceRadius = 100.f);

    /** 停止跟随 */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void StopFollowing();

    /** 是否正在跟随 */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    bool IsFollowing() const { return bIsFollowingActor; }

    /** 取消当前导航 */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void CancelNavigation();

    /** 是否正在导航 */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    bool IsNavigating() const;

    /** 获取导航进度 (0.0 - 1.0) */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    float GetNavigationProgress() const;

    /** 获取当前导航状态 */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    EMANavigationState GetNavigationState() const { return CurrentState; }

    /** 获取目标位置 */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    FVector GetDestination() const { return TargetLocation; }

    /** 获取当前跟随位置（供外部查询，如技能判断是否到达） */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    FVector GetCurrentFollowPosition() const;

    //=========================================================================
    // 暂停/恢复 API
    //=========================================================================

    /** 暂停导航（冻结当前移动，保存状态） */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void PauseNavigation();

    /** 恢复导航（从暂停位置继续原来的导航任务） */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void ResumeNavigation();

    /** 是否处于暂停状态 */
    UFUNCTION(BlueprintPure, Category = "Navigation")
    bool IsNavigationPaused() const { return bIsNavigationPaused; }

    //=========================================================================
    // 委托
    //=========================================================================

    /** 导航完成事件 */
    UPROPERTY(BlueprintAssignable, Category = "Navigation")
    FOnNavigationCompleted OnNavigationCompleted;

    /** 状态变化事件 */
    UPROPERTY(BlueprintAssignable, Category = "Navigation")
    FOnNavigationStateChanged OnNavigationStateChanged;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 是否为飞行模式（UAV 等） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    bool bIsFlying = false;

    /** 是否为固定翼飞行器 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    bool bIsFixedWing = false;

    /** 飞行机器人最低飞行高度 (从 ConfigManager 加载，可覆盖) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float MinFlightAltitude = 800.f;

    /** 默认跟随距离 (从 ConfigManager 加载) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float DefaultFollowDistance = 300.f;

    /** 默认跟随位置容差 (从 ConfigManager 加载) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float DefaultFollowPositionTolerance = 350.f;

    /** 初始化配置 (从 ConfigManager 加载统一参数) */
    void InitializeFromConfig();
    void ApplyBootstrapConfig(
        EMAPathPlannerType InPathPlannerType,
        const FMAPathPlannerConfig& InPathPlannerConfig,
        float InMinFlightAltitude,
        float InFollowDistance,
        float InFollowPositionTolerance,
        float InStuckTimeout
    );

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 设置导航状态 */
    void SetNavigationState(EMANavigationState NewState);

    /** 当前是否处于活跃导航操作状态 */
    bool HasActiveNavigationOperation() const;

    /** 当前是否处于导航终态 */
    bool IsNavigationTerminalState() const;

    /** 初始化一次 NavigateTo 请求的运行期状态 */
    void InitializeNavigateRequest(const FVector& Destination, float InAcceptanceRadius);

    /** 延迟将终态重置回 Idle，避免覆盖新请求 */
    void ScheduleResetToIdle();

    /** 启动地面导航 (NavMesh 优先) */
    bool StartGroundNavigation();

    /** 启动飞行导航 */
    bool StartFlightNavigation();

    /** 飞行命令通用前置校验（Owner/飞行模式/FlightController） */
    bool EnsureCanRunFlightCommand(const TCHAR* CommandName);

    /** 启动飞行相关轮询定时器 */
    void StartFlightTimer(void (UMANavigationService::*UpdateFunction)(float));

    /** 启动手动导航 (NavMesh 失败后的备选) */
    void StartManualNavigation();

    /** 更新手动导航 */
    void UpdateManualNavigation(float DeltaTime);

    /** 更新飞行导航 */
    void UpdateFlightNavigation(float DeltaTime);

    /** 更新飞行操作（起飞/降落/导航的统一更新） */
    void UpdateFlightOperation(float DeltaTime);

    /** 更新返航 */
    void UpdateReturnHome(float DeltaTime);

    /** 更新飞行跟随模式 */
    void UpdateFlightFollowMode(float DeltaTime);

    /** 启动跟随模式更新定时器 */
    void StartFollowUpdateTimer();

    /** 处理跟随失败并统一发事件 */
    void HandleFollowFailure(const TCHAR* FailureReason);

    /** 地面远距离跟随：通过 NavMesh 驱动 */
    void UpdateGroundFollowNavMesh(AAIController* AICtrl, const FVector& FollowPos, float CurrentTimeSeconds);

    /** 地面近距离跟随：直接移动驱动 */
    void UpdateGroundFollowDirect(const FVector& CurrentLocation, const FVector& FollowPos, float DeltaTime);

    /** 更新跟随模式 */
    void UpdateFollowMode(float DeltaTime);

    /** 计算跟随位置（内部使用，外部通过 GetCurrentFollowPosition 访问） */
    FVector CalculateFollowPosition() const;

    /** NavMesh 导航完成回调 */
    void OnNavMeshMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

    /** 绑定 NavMesh 完成回调 */
    void BindNavMeshCompletionCallback(AAIController* AICtrl);

    /** 尝试将目标点投影到 NavMesh */
    void TryProjectTargetToNavMesh();

    /** 处理 MoveToLocation 请求结果 */
    void HandleGroundMoveRequestResult(EPathFollowingRequestResult::Type Result, float DistanceToTarget);

    /** 启动 NavMesh 假成功检测 */
    void ScheduleNavMeshFalseSuccessCheck(const FVector& CheckStartLocation);

    /** 完成导航 */
    void CompleteNavigation(bool bSuccess, const FString& Message);

    /** 导航成功完成（到达 TargetLocation） */
    void CompleteNavigateSuccess();

    /** 推导当前暂停模式 */
    EMANavigationPauseMode ResolvePauseMode() const;

    /** 记录暂停前快照 */
    void CapturePauseSnapshot();

    /** 清空暂停快照 */
    void ClearPauseSnapshot();

    /** 暂停当前导航驱动 */
    void PauseActiveDrivers();

    /** 从暂停快照恢复导航驱动 */
    void ResumeFromPauseSnapshot();

    /** 恢复飞行相关导航驱动 */
    void ResumePausedFlyingOperation(UWorld* World);

    /** 清理所有导航定时器 */
    void ClearAllNavigationTimers();

    /** 停止所有导航驱动（飞行/地面） */
    void StopAllNavigationDrivers();

    /** 重置运行期导航标志 */
    void ResetNavigationRuntimeFlags();

    /** 调整飞行目标高度 */
    FVector AdjustFlightAltitude(const FVector& Location) const;

    /** 清理所有定时器和状态 */
    void CleanupNavigation();

    /** 确保飞行控制器已初始化 (首次使用时调用) */
    void EnsureFlightControllerInitialized();

    //=========================================================================
    // 成员变量
    //=========================================================================

    /** 当前导航状态 */
    EMANavigationState CurrentState = EMANavigationState::Idle;

    /** 目标位置 */
    FVector TargetLocation = FVector::ZeroVector;

    /** 起始位置 (用于计算进度) */
    FVector StartLocation = FVector::ZeroVector;

    /** 到达判定半径 */
    float AcceptanceRadius = 200.f;

    /** 是否使用手动导航 */
    bool bUsingManualNavigation = false;

    /** 是否正在跟随 Actor */
    bool bIsFollowingActor = false;

    /** 跟随目标 */
    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTarget;

    /** 跟随距离 */
    float FollowDistance = 300.f;

    /** 跟随模式定时器 */
    FTimerHandle FollowModeTimerHandle;

    /** 上次 NavMesh 导航时间（用于控制远距离导航频率） */
    float LastNavMeshNavigateTime = 0.f;

    /** 上次跟随目标位置（用于判断是否需要更新导航） */
    FVector LastFollowTargetPos = FVector::ZeroVector;

    /** 手动导航卡住时间 */
    float ManualNavStuckTime = 0.f;

    /** 上次手动导航位置 */
    FVector LastManualNavLocation = FVector::ZeroVector;

    /** 路径规划器 */
    TUniquePtr<IMAPathPlanner> PathPlanner;
    EMAPathPlannerType PathPlannerType = EMAPathPlannerType::MultiLayerRaycast;
    FMAPathPlannerConfig PathPlannerConfig;

    /** 当前 NavMesh 请求 ID */
    FAIRequestID CurrentRequestID;

    /** 是否有活跃的 NavMesh 请求 */
    bool bHasActiveNavMeshRequest = false;

    /** 飞行检查定时器 */
    FTimerHandle FlightCheckTimerHandle;

    /** 手动导航定时器 */
    FTimerHandle ManualNavTimerHandle;

    /** 飞行控制器 (多旋翼或固定翼) */
    TUniquePtr<IMAFlightController> FlightController;

    /** 暂停快照 */
    struct FMANavigationPauseSnapshot
    {
        FVector TargetLocation = FVector::ZeroVector;
        float AcceptanceRadius = 0.f;
        EMANavigationPauseMode Mode = EMANavigationPauseMode::None;
        EMANavigationState State = EMANavigationState::Idle;
        TWeakObjectPtr<AActor> FollowTarget;
        float FollowDistance = 0.f;
        bool bReturnHomeIsLanding = false;
        bool bReturnHomeActive = false;
        float ReturnHomeLandAltitude = 0.f;
        bool bIsValid = false;

        void Reset()
        {
            TargetLocation = FVector::ZeroVector;
            AcceptanceRadius = 0.f;
            Mode = EMANavigationPauseMode::None;
            State = EMANavigationState::Idle;
            FollowTarget.Reset();
            FollowDistance = 0.f;
            bReturnHomeIsLanding = false;
            bReturnHomeActive = false;
            ReturnHomeLandAltitude = 0.f;
            bIsValid = false;
        }
    } PauseSnapshot;

    /** 是否处于导航暂停状态 */
    bool bIsNavigationPaused = false;

    /** 返航降落高度 */
    float ReturnHomeLandAltitude = 0.f;

    /** 返航是否进入降落阶段 */
    bool bReturnHomeIsLanding = false;

    /** 是否处于返航流程中 */
    bool bIsReturnHomeActive = false;

    /** 拥有者角色缓存 */
    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter = nullptr;

    /** 原始移动速度（用于恢复） */
    float OriginalMoveSpeed = 0.f;

    /** 是否已修改移动速度 */
    bool bSpeedModified = false;

    /** 手动导航卡住超时时间 (秒) - 从 ConfigManager 加载 */
    float StuckTimeout = 10.f;

    /** 手动导航更新间隔 (秒) */
    static constexpr float ManualNavUpdateInterval = 0.05f;

    /** 飞行到达检查间隔 (秒) */
    static constexpr float FlightCheckInterval = 0.1f;

    /** 跟随模式更新间隔 (秒) */
    static constexpr float FollowModeUpdateInterval = 0.05f;

    /** 远距离阈值：超过此距离使用 NavMesh (cm) */
    static constexpr float NavMeshDistanceThreshold = 2000.f;

    /** 远距离 NavMesh 导航更新间隔 (秒) */
    static constexpr float NavMeshFollowUpdateInterval = 2.0f;
};
