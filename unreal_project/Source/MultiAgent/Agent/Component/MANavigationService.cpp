// MANavigationService.cpp
// 统一导航服务组件实现（核心状态/生命周期）

#include "MANavigationService.h"
#include "../../Core/Shared/Types/MATypes.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../../Utils/MAFlightController.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
UMANavigationService::UMANavigationService()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMANavigationService::BeginPlay()
{
    Super::BeginPlay();
    
    // 从 ConfigManager 加载统一配置
    InitializeFromConfig();
    
    // 缓存拥有者 (必须是 ACharacter)
    OwnerCharacter = Cast<ACharacter>(GetOwner());
    
    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] Attached to Character: %s"), *OwnerCharacter->GetName());
        // 注意：FlightController 在 EnsureFlightControllerInitialized() 中延迟初始化
        // 因为 bIsFlying 可能在子类 BeginPlay 中才被设置
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MANavigationService] Owner is not ACharacter! Navigation will not work."));
    }
}

void UMANavigationService::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopFollowing();
    CleanupNavigation();
    Super::EndPlay(EndPlayReason);
}

void UMANavigationService::TickComponent(float DeltaTime, ELevelTick TickType, 
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsNavigationPaused) return;
}

//=========================================================================
// 暂停/恢复实现
//=========================================================================

void UMANavigationService::PauseNavigation()
{
    if (bIsNavigationPaused || CurrentState == EMANavigationState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: PauseNavigation SKIPPED (bIsNavigationPaused=%s, CurrentState=%d, bIsFlying=%s, FlightTimer=%s)"),
            OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
            bIsNavigationPaused ? TEXT("true") : TEXT("false"),
            (int32)CurrentState,
            bIsFlying ? TEXT("true") : TEXT("false"),
            FlightCheckTimerHandle.IsValid() ? TEXT("valid") : TEXT("invalid"));
        return;
    }

    CapturePauseSnapshot();
    PauseActiveDrivers();

    bIsNavigationPaused = true;

    const bool bPausedFollowing = PauseSnapshot.Mode == EMANavigationPauseMode::Following;
    const bool bPausedManual = PauseSnapshot.Mode == EMANavigationPauseMode::Manual;
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Navigation PAUSED (flying=%s, following=%s, manualNav=%s)"),
        OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
        bIsFlying ? TEXT("true") : TEXT("false"),
        bPausedFollowing ? TEXT("true") : TEXT("false"),
        bPausedManual ? TEXT("true") : TEXT("false"));
}

void UMANavigationService::ResumeNavigation()
{
    if (!bIsNavigationPaused)
    {
        return;
    }

    bIsNavigationPaused = false;

    ResumeFromPauseSnapshot();
    ClearPauseSnapshot();

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Navigation RESUMED"),
        OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"));
}

//=========================================================================
// 核心 API 实现
//=========================================================================

void UMANavigationService::SetMoveSpeed(float Speed)
{
    if (!OwnerCharacter) return;
    
    UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
    if (!MovementComp) return;
    
    // 保存原始速度（仅在第一次修改时）
    if (!bSpeedModified)
    {
        OriginalMoveSpeed = MovementComp->MaxWalkSpeed;
        bSpeedModified = true;
    }
    
    if (Speed > 0.f)
    {
        MovementComp->MaxWalkSpeed = Speed;
        UE_LOG(LogTemp, Verbose, TEXT("[MANavigationService] %s: SetMoveSpeed to %.0f"), 
            *OwnerCharacter->GetName(), Speed);
    }
    else
    {
        // Speed <= 0 表示恢复默认
        RestoreDefaultSpeed();
    }
}

void UMANavigationService::RestoreDefaultSpeed()
{
    if (!OwnerCharacter || !bSpeedModified) return;
    
    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = OriginalMoveSpeed;
        UE_LOG(LogTemp, Verbose, TEXT("[MANavigationService] %s: RestoreDefaultSpeed to %.0f"), 
            *OwnerCharacter->GetName(), OriginalMoveSpeed);
    }
    
    bSpeedModified = false;
}

bool UMANavigationService::NavigateTo(FVector Destination, float InAcceptanceRadius, bool bSmoothArrival)
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] NavigateTo failed: No owner character"));
        return false;
    }
    
    // 如果正在进行其他操作，先取消
    if (CurrentState == EMANavigationState::Navigating || 
        CurrentState == EMANavigationState::TakingOff || 
        CurrentState == EMANavigationState::Landing)
    {
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: NavigateTo - Cancelling previous operation (state=%d)"),
            *OwnerCharacter->GetName(), (int32)CurrentState);
        CleanupNavigation();
    }
    
    // 初始化导航参数
    TargetLocation = Destination;
    AcceptanceRadius = InAcceptanceRadius;
    StartLocation = OwnerCharacter->GetActorLocation();
    bUsingManualNavigation = false;
    bHasActiveNavMeshRequest = false;
    ManualNavStuckTime = 0.f;
    bIsReturnHomeActive = false;
    
    // 设置飞行控制器的平滑到达选项
    if (bIsFlying)
    {
        EnsureFlightControllerInitialized();
        if (FlightController.IsValid())
        {
            FlightController->SetSmoothArrival(bSmoothArrival);
        }
    }
    
    // 设置状态为导航中
    SetNavigationState(EMANavigationState::Navigating);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: NavigateTo (%.0f, %.0f, %.0f), AcceptanceRadius=%.0f, bIsFlying=%s, bSmoothArrival=%s"),
        *OwnerCharacter->GetName(), Destination.X, Destination.Y, Destination.Z, AcceptanceRadius,
        bIsFlying ? TEXT("true") : TEXT("false"),
        bSmoothArrival ? TEXT("true") : TEXT("false"));
    
    // 根据导航模式选择策略
    if (bIsFlying)
    {
        return StartFlightNavigation();
    }
    else
    {
        return StartGroundNavigation();
    }
}


void UMANavigationService::CancelNavigation()
{
    FString OwnerName = OwnerCharacter ? OwnerCharacter->GetName() : TEXT("NULL");
    
    // 只有在活跃状态下才能取消
    if (CurrentState == EMANavigationState::Idle || 
        CurrentState == EMANavigationState::Arrived ||
        CurrentState == EMANavigationState::Failed ||
        CurrentState == EMANavigationState::Cancelled)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: CancelNavigation (CurrentState=%d)"), 
        *OwnerName, (int32)CurrentState);
    
    // 清理定时器和状态
    CleanupNavigation();
    
    // 设置状态并通知
    SetNavigationState(EMANavigationState::Cancelled);
    CompleteNavigation(false, TEXT("Navigation cancelled"));
}


bool UMANavigationService::IsNavigating() const
{
    return CurrentState == EMANavigationState::Navigating ||
           CurrentState == EMANavigationState::TakingOff ||
           CurrentState == EMANavigationState::Landing;
}

float UMANavigationService::GetNavigationProgress() const
{
    if (CurrentState != EMANavigationState::Navigating || !OwnerCharacter)
    {
        return CurrentState == EMANavigationState::Arrived ? 1.0f : 0.0f;
    }
    
    float StartDistance = FVector::Dist(StartLocation, TargetLocation);
    if (StartDistance < KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }
    
    float CurrentDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetLocation);
    float Progress = 1.0f - (CurrentDistance / StartDistance);
    
    return FMath::Clamp(Progress, 0.0f, 1.0f);
}

//=========================================================================
// 内部方法实现
//=========================================================================

void UMANavigationService::SetNavigationState(EMANavigationState NewState)
{
    if (CurrentState != NewState)
    {
        CurrentState = NewState;
        OnNavigationStateChanged.Broadcast(NewState);
    }
}

EMANavigationPauseMode UMANavigationService::ResolvePauseMode() const
{
    if (bIsFollowingActor)
    {
        return EMANavigationPauseMode::Following;
    }
    if (bIsFlying)
    {
        return EMANavigationPauseMode::Flying;
    }
    if (bUsingManualNavigation)
    {
        return EMANavigationPauseMode::Manual;
    }
    return EMANavigationPauseMode::Ground;
}

void UMANavigationService::CapturePauseSnapshot()
{
    PauseSnapshot.TargetLocation = TargetLocation;
    PauseSnapshot.AcceptanceRadius = AcceptanceRadius;
    PauseSnapshot.Mode = ResolvePauseMode();
    PauseSnapshot.State = CurrentState;
    PauseSnapshot.FollowTarget = FollowTarget;
    PauseSnapshot.FollowDistance = FollowDistance;
    PauseSnapshot.bReturnHomeIsLanding = bReturnHomeIsLanding;
    PauseSnapshot.bReturnHomeActive = bIsReturnHomeActive;
    PauseSnapshot.ReturnHomeLandAltitude = ReturnHomeLandAltitude;
    PauseSnapshot.bIsValid = true;
}

void UMANavigationService::ClearPauseSnapshot()
{
    PauseSnapshot.Reset();
}

void UMANavigationService::PauseActiveDrivers()
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;

    // 停止飞行控制器（悬停）
    if (bIsFlying && FlightController.IsValid())
    {
        FlightController->StopMovement();
    }

    // 停止地面 NavMesh 路径跟随
    // 注意：必须先设 bHasActiveNavMeshRequest = false，再调用 StopMovement()
    // 因为 StopMovement() 会同步触发 OnNavMeshMoveCompleted 回调
    if (!bIsFlying && bHasActiveNavMeshRequest && OwnerCharacter)
    {
        bHasActiveNavMeshRequest = false;
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
    }

    if (World && FollowModeTimerHandle.IsValid())
    {
        World->GetTimerManager().PauseTimer(FollowModeTimerHandle);
    }
    if (World && ManualNavTimerHandle.IsValid())
    {
        World->GetTimerManager().PauseTimer(ManualNavTimerHandle);
    }
    if (World && FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().PauseTimer(FlightCheckTimerHandle);
    }
}

void UMANavigationService::ResumeFromPauseSnapshot()
{
    if (!PauseSnapshot.bIsValid)
    {
        return;
    }

    TargetLocation = PauseSnapshot.TargetLocation;
    AcceptanceRadius = PauseSnapshot.AcceptanceRadius;
    FollowDistance = PauseSnapshot.FollowDistance;
    bReturnHomeIsLanding = PauseSnapshot.bReturnHomeIsLanding;
    bIsReturnHomeActive = PauseSnapshot.bReturnHomeActive;
    ReturnHomeLandAltitude = PauseSnapshot.ReturnHomeLandAltitude;

    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    switch (PauseSnapshot.Mode)
    {
        case EMANavigationPauseMode::Following:
            if (World && FollowModeTimerHandle.IsValid())
            {
                World->GetTimerManager().UnPauseTimer(FollowModeTimerHandle);
            }
            else if (PauseSnapshot.FollowTarget.IsValid())
            {
                FollowActor(PauseSnapshot.FollowTarget.Get(), FollowDistance, AcceptanceRadius);
            }
            break;
        case EMANavigationPauseMode::Flying:
            ResumePausedFlyingOperation(World);
            break;
        case EMANavigationPauseMode::Manual:
            if (World && ManualNavTimerHandle.IsValid())
            {
                World->GetTimerManager().UnPauseTimer(ManualNavTimerHandle);
            }
            else
            {
                StartManualNavigation();
            }
            break;
        case EMANavigationPauseMode::Ground:
            // 恢复地面 NavMesh 导航：重新发起导航请求
            NavigateTo(TargetLocation, AcceptanceRadius);
            break;
        case EMANavigationPauseMode::None:
        default:
            break;
    }
}

void UMANavigationService::ResumePausedFlyingOperation(UWorld* World)
{
    EnsureFlightControllerInitialized();
    if (FlightController.IsValid())
    {
        FlightController->SetAcceptanceRadius(AcceptanceRadius);

        if (PauseSnapshot.bReturnHomeActive)
        {
            if (PauseSnapshot.bReturnHomeIsLanding || PauseSnapshot.State == EMANavigationState::Landing)
            {
                FlightController->Land();
            }
            else
            {
                FlightController->FlyTo(TargetLocation);
            }
        }
        else if (PauseSnapshot.State == EMANavigationState::TakingOff)
        {
            FlightController->TakeOff(TargetLocation.Z);
        }
        else if (PauseSnapshot.State == EMANavigationState::Landing)
        {
            FlightController->Land();
        }
        else
        {
            FlightController->FlyTo(TargetLocation);
        }
    }

    if (World && FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().UnPauseTimer(FlightCheckTimerHandle);
        return;
    }

    if (PauseSnapshot.bReturnHomeActive)
    {
        StartFlightTimer(&UMANavigationService::UpdateReturnHome);
    }
    else if (PauseSnapshot.State == EMANavigationState::TakingOff || PauseSnapshot.State == EMANavigationState::Landing)
    {
        StartFlightTimer(&UMANavigationService::UpdateFlightOperation);
    }
    else
    {
        StartFlightTimer(&UMANavigationService::UpdateFlightNavigation);
    }
}

void UMANavigationService::CompleteNavigateSuccess()
{
    SetNavigationState(EMANavigationState::Arrived);
    CompleteNavigation(true, FString::Printf(
        TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"),
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
}

void UMANavigationService::CompleteNavigation(bool bSuccess, const FString& Message)
{
    FString OwnerName = OwnerCharacter ? OwnerCharacter->GetName() : TEXT("NULL");
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: CompleteNavigation - Success=%s, Message=%s"),
        *OwnerName, bSuccess ? TEXT("true") : TEXT("false"), *Message);
    
    // 清理
    CleanupNavigation();
    
    // 广播完成事件
    OnNavigationCompleted.Broadcast(bSuccess, Message);
    
    // 延迟重置状态为 Idle（仅在仍处于终态时才重置，避免覆盖新导航的状态）
    if (OwnerCharacter && OwnerCharacter->GetWorld())
    {
        FTimerHandle ResetHandle;
        OwnerCharacter->GetWorld()->GetTimerManager().SetTimer(
            ResetHandle,
            [this]()
            {
                // 只有当状态仍然是终态时才重置为 Idle
                // 如果已经开始了新的导航（Navigating/TakingOff/Landing），不要覆盖
                if (CurrentState == EMANavigationState::Arrived ||
                    CurrentState == EMANavigationState::Failed ||
                    CurrentState == EMANavigationState::Cancelled)
                {
                    SetNavigationState(EMANavigationState::Idle);
                }
            },
            0.1f,
            false
        );
    }
}

void UMANavigationService::CleanupNavigation()
{
    ClearAllNavigationTimers();
    StopAllNavigationDrivers();
    ResetNavigationRuntimeFlags();
    ClearPauseSnapshot();
}

void UMANavigationService::ClearAllNavigationTimers()
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    if (FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
        FlightCheckTimerHandle.Invalidate();
    }
    if (ManualNavTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(ManualNavTimerHandle);
        ManualNavTimerHandle.Invalidate();
    }
    if (FollowModeTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
        FollowModeTimerHandle.Invalidate();
    }
}

void UMANavigationService::StopAllNavigationDrivers()
{
    if (FlightController.IsValid())
    {
        FlightController->CancelFlight();
        FlightController->StopMovement();
    }

    if (!OwnerCharacter)
    {
        return;
    }

    if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
    {
        AICtrl->StopMovement();
        if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
    }
}

void UMANavigationService::ResetNavigationRuntimeFlags()
{
    bHasActiveNavMeshRequest = false;
    bUsingManualNavigation = false;
    bIsFollowingActor = false;
    bReturnHomeIsLanding = false;
    bIsReturnHomeActive = false;
    ReturnHomeLandAltitude = 0.f;
    bIsNavigationPaused = false;
    FollowTarget.Reset();
    LastFollowTargetPos = FVector::ZeroVector;
    LastNavMeshNavigateTime = 0.f;
}

void UMANavigationService::InitializeFromConfig()
{
    if (AActor* Owner = GetOwner())
    {
        if (UWorld* World = Owner->GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
                {
                    // 加载飞行配置
                    const FMAFlightConfig& FlightCfg = ConfigMgr->GetFlightConfig();
                    MinFlightAltitude = FlightCfg.MinAltitude;
                    
                    // 加载地面导航配置
                    const FMAGroundNavigationConfig& GroundCfg = ConfigMgr->GetGroundNavigationConfig();
                    StuckTimeout = GroundCfg.StuckTimeout;
                    
                    // 加载跟随配置 (适用于所有机器人)
                    const FMAFollowConfig& FollowCfg = ConfigMgr->GetFollowConfig();
                    DefaultFollowDistance = FollowCfg.Distance;
                    DefaultFollowPositionTolerance = FollowCfg.PositionTolerance;
                    
                    UE_LOG(LogTemp, Verbose, TEXT("[MANavigationService] Loaded config: MinFlightAlt=%.0f, FollowDist=%.0f, FollowTolerance=%.0f, StuckTimeout=%.0f"), 
                        MinFlightAltitude, DefaultFollowDistance, DefaultFollowPositionTolerance, StuckTimeout);
                }
            }
        }
    }
}

void UMANavigationService::EnsureFlightControllerInitialized()
{
    if (FlightController.IsValid() || !bIsFlying || !OwnerCharacter)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Initializing FlightController (bIsFixedWing=%s)"),
        *OwnerCharacter->GetName(), bIsFixedWing ? TEXT("true") : TEXT("false"));
    
    if (bIsFixedWing)
    {
        FlightController = MakeUnique<FMAFixedWingFlightController>();
    }
    else
    {
        FlightController = MakeUnique<FMAMultiRotorFlightController>();
    }
    FlightController->Initialize(OwnerCharacter);
}
