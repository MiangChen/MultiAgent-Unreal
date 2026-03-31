// MANavigationService.cpp
// 统一导航服务组件实现
// 支持所有继承 ACharacter 的实体

#include "MANavigationService.h"
#include "../../Core/Types/MATypes.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../../Utils/MAPathPlanner.h"
#include "../../Utils/MAFlightController.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
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

    // 保存当前导航状态
    PausedTargetLocation = TargetLocation;
    bWasFollowing = bIsFollowingActor;
    bWasUsingManualNav = bUsingManualNavigation;

    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;

    // 停止飞行控制器（悬停）
    if (bIsFlying && FlightController.IsValid())
    {
        FlightController->StopMovement();
    }

    // 停止地面 NavMesh 路径跟随
    // 注意：必须先设 bHasActiveNavMeshRequest = false，再调用 StopMovement()
    // 因为 StopMovement() 会同步触发 OnNavMeshMoveCompleted 回调
    // 如果不先清除标记，回调会认为是正常完成，导致 CompleteNavigation 被调用
    // 进而触发技能完成链条，破坏暂停状态
    if (!bIsFlying && bHasActiveNavMeshRequest && OwnerCharacter)
    {
        bHasActiveNavMeshRequest = false;
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
    }

    // 暂停跟随模式定时器
    if (bIsFollowingActor && FollowModeTimerHandle.IsValid() && World)
    {
        World->GetTimerManager().PauseTimer(FollowModeTimerHandle);
    }

    // 暂停手动导航定时器
    if (bUsingManualNavigation && ManualNavTimerHandle.IsValid() && World)
    {
        World->GetTimerManager().PauseTimer(ManualNavTimerHandle);
    }

    // 暂停飞行检查定时器
    if (FlightCheckTimerHandle.IsValid() && World)
    {
        World->GetTimerManager().PauseTimer(FlightCheckTimerHandle);
    }

    bIsNavigationPaused = true;

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Navigation PAUSED (flying=%s, following=%s, manualNav=%s)"),
        OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
        bIsFlying ? TEXT("true") : TEXT("false"),
        bWasFollowing ? TEXT("true") : TEXT("false"),
        bWasUsingManualNav ? TEXT("true") : TEXT("false"));
}

void UMANavigationService::ResumeNavigation()
{
    if (!bIsNavigationPaused)
    {
        return;
    }

    bIsNavigationPaused = false;

    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;

    if (bWasFollowing)
    {
        // 恢复跟随模式定时器
        if (FollowModeTimerHandle.IsValid() && World)
        {
            World->GetTimerManager().UnPauseTimer(FollowModeTimerHandle);
        }
    }
    else if (bIsFlying)
    {
        // 恢复飞行导航
        if (FlightController.IsValid())
        {
            FlightController->FlyTo(PausedTargetLocation);
        }
        if (FlightCheckTimerHandle.IsValid() && World)
        {
            World->GetTimerManager().UnPauseTimer(FlightCheckTimerHandle);
        }
    }
    else if (bWasUsingManualNav)
    {
        // 恢复手动导航定时器
        if (ManualNavTimerHandle.IsValid() && World)
        {
            World->GetTimerManager().UnPauseTimer(ManualNavTimerHandle);
        }
    }
    else
    {
        // 恢复地面 NavMesh 导航：重新发起导航请求
        NavigateTo(PausedTargetLocation, AcceptanceRadius);
    }

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

bool UMANavigationService::TakeOff(float TargetAltitude)
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] TakeOff failed: No owner character"));
        return false;
    }
    
    if (!bIsFlying)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: TakeOff failed: Not a flying vehicle"),
            *OwnerCharacter->GetName());
        return false;
    }
    
    // 如果正在进行其他操作，先取消
    if (CurrentState != EMANavigationState::Idle && CurrentState != EMANavigationState::Arrived)
    {
        CleanupNavigation();
    }
    
    EnsureFlightControllerInitialized();
    
    if (!FlightController.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: TakeOff failed: FlightController not initialized"),
            *OwnerCharacter->GetName());
        return false;
    }
    
    // 记录目标高度用于完成消息
    TargetLocation = OwnerCharacter->GetActorLocation();
    TargetLocation.Z = TargetAltitude;
    
    SetNavigationState(EMANavigationState::TakingOff);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: TakeOff to altitude %.0f"),
        *OwnerCharacter->GetName(), TargetAltitude);
    
    // 调用 FlightController 的 TakeOff
    if (!FlightController->TakeOff(TargetAltitude))
    {
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, TEXT("TakeOff failed: FlightController rejected request"));
        return false;
    }
    
    // 启动更新定时器（使用 FlightController 的 UpdateFlight）
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FlightCheckTimerHandle,
            FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateFlightOperation, FlightCheckInterval),
            FlightCheckInterval,
            true
        );
    }
    
    return true;
}

bool UMANavigationService::Land(float TargetAltitude)
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] Land failed: No owner character"));
        return false;
    }
    
    if (!bIsFlying)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: Land failed: Not a flying vehicle"),
            *OwnerCharacter->GetName());
        return false;
    }
    
    // 如果正在进行其他操作，先取消
    if (CurrentState != EMANavigationState::Idle && CurrentState != EMANavigationState::Arrived)
    {
        CleanupNavigation();
    }
    
    EnsureFlightControllerInitialized();
    
    if (!FlightController.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: Land failed: FlightController not initialized"),
            *OwnerCharacter->GetName());
        return false;
    }
    
    // 记录目标高度用于完成消息
    TargetLocation = OwnerCharacter->GetActorLocation();
    TargetLocation.Z = TargetAltitude;
    
    SetNavigationState(EMANavigationState::Landing);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Land to altitude %.0f"),
        *OwnerCharacter->GetName(), TargetAltitude);
    
    // 调用 FlightController 的 Land
    if (!FlightController->Land())
    {
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, TEXT("Land failed: FlightController rejected request"));
        return false;
    }
    
    // 启动更新定时器
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FlightCheckTimerHandle,
            FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateFlightOperation, FlightCheckInterval),
            FlightCheckInterval,
            true
        );
    }
    
    return true;
}

bool UMANavigationService::ReturnHome(FVector HomePosition, float LandAltitude)
{
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] ReturnHome failed: No owner character"));
        return false;
    }
    
    if (!bIsFlying)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: ReturnHome failed: Not a flying vehicle"),
            *OwnerCharacter->GetName());
        return false;
    }
    
    // 如果正在进行其他操作，先取消
    if (CurrentState != EMANavigationState::Idle && CurrentState != EMANavigationState::Arrived)
    {
        CleanupNavigation();
    }
    
    EnsureFlightControllerInitialized();
    
    if (!FlightController.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: ReturnHome failed: FlightController not initialized"),
            *OwnerCharacter->GetName());
        return false;
    }
    
    // 保持当前高度飞回 HomePosition
    StartLocation = OwnerCharacter->GetActorLocation();
    TargetLocation = HomePosition;
    TargetLocation.Z = StartLocation.Z;  // 保持当前高度
    ReturnHomeLandAltitude = LandAltitude;
    bReturnHomeIsLanding = false;
    AcceptanceRadius = 100.f;
    
    SetNavigationState(EMANavigationState::Navigating);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: ReturnHome to (%.0f, %.0f), then land at %.0f"),
        *OwnerCharacter->GetName(), HomePosition.X, HomePosition.Y, LandAltitude);
    
    // 使用飞行控制器导航到 HomePosition
    FlightController->SetAcceptanceRadius(AcceptanceRadius);
    FlightController->FlyTo(TargetLocation);
    
    // 启动更新定时器
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FlightCheckTimerHandle,
            FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateReturnHome, FlightCheckInterval),
            FlightCheckInterval,
            true
        );
    }
    
    return true;
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
    
    // 停止 AIController 移动
    if (OwnerCharacter)
    {
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
    }
    
    // 清理定时器和状态
    CleanupNavigation();
    
    // 设置状态并通知
    SetNavigationState(EMANavigationState::Cancelled);
    CompleteNavigation(false, TEXT("Navigation cancelled"));
}

//=========================================================================
// 跟随模式实现
//=========================================================================

bool UMANavigationService::FollowActor(AActor* Target, float InFollowDistance, float InAcceptanceRadius)
{
    if (!OwnerCharacter || !Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] FollowActor failed: Invalid owner or target"));
        return false;
    }
    
    // 停止之前的导航/跟随
    if (bIsFollowingActor) StopFollowing();
    if (CurrentState == EMANavigationState::Navigating) CancelNavigation();
    
    // 初始化跟随参数
    FollowTarget = Target;
    FollowDistance = InFollowDistance;
    AcceptanceRadius = InAcceptanceRadius;
    bIsFollowingActor = true;
    LastNavMeshNavigateTime = 0.f;
    
    // 初始化路径规划器（用于地面近距离避障）
    if (!bIsFlying && !PathPlanner.IsValid())
    {
        FMAPathPlannerConfig PlannerConfig;
        if (UWorld* World = OwnerCharacter->GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
                {
                    PlannerConfig = ConfigMgr->GetPathPlannerConfig();
                }
            }
        }
        PathPlanner = MakeUnique<FMAMultiLayerRaycast>(PlannerConfig);
    }
    
    SetNavigationState(EMANavigationState::Navigating);
    
    // 根据飞行/地面选择不同的更新逻辑
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        if (bIsFlying)
        {
            // 飞行模式：使用飞行控制器
            if (FlightController.IsValid())
            {
                FlightController->SetAcceptanceRadius(AcceptanceRadius);
            }
            
            World->GetTimerManager().SetTimer(
                FollowModeTimerHandle,
                FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateFlightFollowMode, FollowModeUpdateInterval),
                FollowModeUpdateInterval,
                true
            );
        }
        else
        {
            // 地面模式
            World->GetTimerManager().SetTimer(
                FollowModeTimerHandle,
                FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateFollowMode, FollowModeUpdateInterval),
                FollowModeUpdateInterval,
                true
            );
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Started following %s (distance=%.0f, flying=%s)"),
        *OwnerCharacter->GetName(), *Target->GetName(), FollowDistance, bIsFlying ? TEXT("true") : TEXT("false"));
    
    return true;
}

void UMANavigationService::StopFollowing()
{
    if (!bIsFollowingActor) return;
    
    bIsFollowingActor = false;
    FollowTarget.Reset();
    LastFollowTargetPos = FVector::ZeroVector;
    
    // 清理跟随定时器
    if (UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
    }
    
    // 停止飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->CancelFlight();
        FlightController->StopMovement();
    }
    
    // 停止 AIController 移动
    if (OwnerCharacter)
    {
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] StopFollowing"));
}

void UMANavigationService::UpdateFollowMode(float DeltaTime)
{
    if (!bIsFollowingActor || !OwnerCharacter || !FollowTarget.IsValid())
    {
        StopFollowing();
        SetNavigationState(EMANavigationState::Failed);
        OnNavigationCompleted.Broadcast(false, TEXT("Follow failed: Target lost"));
        return;
    }
    
    FVector MyLoc = OwnerCharacter->GetActorLocation();
    FVector TargetLoc = FollowTarget->GetActorLocation();
    FVector FollowPos = CalculateFollowPosition();
    float DistanceToFollowPos = FVector::Dist2D(MyLoc, FollowPos);
    float DistanceToTarget = FVector::Dist2D(MyLoc, TargetLoc);
    
    // 已到达跟随位置
    if (DistanceToFollowPos < AcceptanceRadius)
    {
        // 停止 AIController 移动，避免抖动
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Arrived at follow position, waiting..."),
            *OwnerCharacter->GetName());
        return;  // 保持跟随状态，等待目标移动
    }
    
    // 地面机器人：统一使用 AIController 的 MoveToLocation
    // 这样可以利用 NavMesh 寻路和 CharacterMovementComponent 的速度控制
    AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController());
    if (!AICtrl)
    {
        // 没有 AIController，使用直接移动（备选方案）
        FVector MoveDir = (FollowPos - MyLoc).GetSafeNormal();
        MoveDir.Z = 0.f;
        OwnerCharacter->AddMovementInput(MoveDir, 1.0f);
        return;
    }
    
    // 检查是否需要更新导航目标
    // 只有当目标位置变化超过阈值时才重新发起导航请求
    float FollowPosChangeThreshold = 100.f;  // 跟随位置变化阈值
    bool bNeedUpdateNav = false;
    
    if (LastFollowTargetPos.IsZero())
    {
        bNeedUpdateNav = true;
    }
    else
    {
        float FollowPosChange = FVector::Dist2D(FollowPos, LastFollowTargetPos);
        if (FollowPosChange > FollowPosChangeThreshold)
        {
            bNeedUpdateNav = true;
        }
    }
    
    if (bNeedUpdateNav)
    {
        LastFollowTargetPos = FollowPos;
        
        UE_LOG(LogTemp, Verbose, TEXT("[MANavigationService] %s: Updating follow navigation to (%.0f,%.0f,%.0f)"),
            *OwnerCharacter->GetName(), FollowPos.X, FollowPos.Y, FollowPos.Z);
        
        // 使用较小的 AcceptanceRadius 让机器人更精确地到达跟随位置
        AICtrl->MoveToLocation(FollowPos, AcceptanceRadius * 0.5f, true, true, false, true, nullptr);
    }
}

void UMANavigationService::UpdateFlightFollowMode(float DeltaTime)
{
    if (!bIsFollowingActor || !OwnerCharacter || !FollowTarget.IsValid())
    {
        StopFollowing();
        SetNavigationState(EMANavigationState::Failed);
        OnNavigationCompleted.Broadcast(false, TEXT("Follow failed: Target lost"));
        return;
    }
    
    FVector FollowPos = CalculateFollowPosition();
    FVector MyLoc = OwnerCharacter->GetActorLocation();
    float DistanceToFollowPos = FVector::Dist(MyLoc, FollowPos);
    
    // 如果已经在跟随位置附近，只做轻微调整，避免抖动
    if (DistanceToFollowPos < AcceptanceRadius)
    {
        // 在容差范围内，只更新朝向，不移动
        FVector DirToTarget = (FollowTarget->GetActorLocation() - MyLoc).GetSafeNormal2D();
        if (!DirToTarget.IsNearlyZero())
        {
            FRotator TargetRot = DirToTarget.Rotation();
            TargetRot.Pitch = 0.f;
            TargetRot.Roll = 0.f;
            FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, 2.f);
            OwnerCharacter->SetActorRotation(NewRot);
        }
        return;
    }
    
    // 使用飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->UpdateFollowTarget(FollowPos);
        FlightController->UpdateFlight(DeltaTime);
    }
    else
    {
        // 回退：简单直线跟随
        FVector MoveDir = (FollowPos - MyLoc).GetSafeNormal();
        
        OwnerCharacter->AddMovementInput(MoveDir, 1.0f);
        
        if (!MoveDir.IsNearlyZero())
        {
            FRotator TargetRot = MoveDir.Rotation();
            FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, 5.f);
            OwnerCharacter->SetActorRotation(NewRot);
        }
    }
}

FVector UMANavigationService::CalculateFollowPosition() const
{
    if (!FollowTarget.IsValid() || !OwnerCharacter) return FVector::ZeroVector;
    
    FVector TargetLoc = FollowTarget->GetActorLocation();
    FVector MyLoc = OwnerCharacter->GetActorLocation();
    
    FVector FollowPos;
    
    // 计算跟随方向：使用目标的反向前向（在目标身后跟随）
    // 而不是从目标指向机器人的方向（会导致机器人永远追不上）
    FVector TargetForward = FollowTarget->GetActorForwardVector().GetSafeNormal2D();
    FVector FollowDir = -TargetForward;  // 目标身后的方向
    
    // 如果目标没有明确的前向（静止），则使用从目标指向机器人的方向
    if (FollowDir.IsNearlyZero())
    {
        FollowDir = (MyLoc - TargetLoc).GetSafeNormal2D();
        if (FollowDir.IsNearlyZero())
        {
            FollowDir = FVector::ForwardVector;
        }
    }
    
    if (bIsFlying)
    {
        // 飞行机器人：XY 平面上的跟随位置
        FollowPos = TargetLoc + FollowDir * FollowDistance;
        // 高度固定在目标上方 MinFlightAltitude
        FollowPos.Z = FMath::Max(TargetLoc.Z + MinFlightAltitude, MinFlightAltitude);
    }
    else
    {
        // 地面机器人
        FollowPos = TargetLoc + FollowDir * FollowDistance;
        FollowPos.Z = TargetLoc.Z;
    }
    
    return FollowPos;
}

bool UMANavigationService::IsNavigating() const
{
    return CurrentState == EMANavigationState::Navigating ||
           CurrentState == EMANavigationState::TakingOff ||
           CurrentState == EMANavigationState::Landing;
}

FVector UMANavigationService::GetCurrentFollowPosition() const
{
    return CalculateFollowPosition();
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

FVector UMANavigationService::AdjustFlightAltitude(const FVector& Location) const
{
    FVector AdjustedLocation = Location;
    if (AdjustedLocation.Z < MinFlightAltitude)
    {
        AdjustedLocation.Z = MinFlightAltitude;
    }
    return AdjustedLocation;
}

bool UMANavigationService::StartFlightNavigation()
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character not found"));
        return false;
    }
    
    // 调整目标高度
    TargetLocation = AdjustFlightAltitude(TargetLocation);
    
    // 确保飞行控制器已初始化
    EnsureFlightControllerInitialized();
    
    // 使用飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->SetAcceptanceRadius(AcceptanceRadius);
        FlightController->FlyTo(TargetLocation);
        
        // 设置定时检查到达
        if (UWorld* World = OwnerCharacter->GetWorld())
        {
            if (FlightCheckTimerHandle.IsValid())
            {
                World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
            }
            
            World->GetTimerManager().SetTimer(
                FlightCheckTimerHandle,
                FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateFlightNavigation, FlightCheckInterval),
                FlightCheckInterval,
                true
            );
        }
        return true;
    }
    
    // 回退：使用 CharacterMovementComponent 的飞行模式
    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->SetMovementMode(MOVE_Flying);
    }
    
    // 设置定时检查到达
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        if (FlightCheckTimerHandle.IsValid())
        {
            World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
        }
        
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindWeakLambda(this, [this]()
        {
            UpdateFlightNavigation(FlightCheckInterval);
        });
        
        World->GetTimerManager().SetTimer(
            FlightCheckTimerHandle,
            TimerDelegate,
            FlightCheckInterval,
            true
        );
    }
    
    return true;
}

void UMANavigationService::UpdateFlightNavigation(float DeltaTime)
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character lost during flight"));
        return;
    }
    
    // 使用飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->UpdateFlight(DeltaTime);
        
        if (FlightController->HasArrived())
        {
            SetNavigationState(EMANavigationState::Arrived);
            CompleteNavigation(true, FString::Printf(
                TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"), 
                TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
        }
        return;
    }
    
    // 回退：简单直线导航
    FVector CurrentLoc = OwnerCharacter->GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLoc;
    float Distance3D = ToTarget.Size();
    
    if (Distance3D < AcceptanceRadius)
    {
        SetNavigationState(EMANavigationState::Arrived);
        CompleteNavigation(true, FString::Printf(
            TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"), 
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
        return;
    }
    
    FVector Direction = ToTarget.GetSafeNormal();
    OwnerCharacter->AddMovementInput(Direction, 1.0f);
    
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);
        OwnerCharacter->SetActorRotation(NewRotation);
    }
}

bool UMANavigationService::StartGroundNavigation()
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character not found"));
        return false;
    }
    
    AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController());
    if (!AICtrl)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: No AIController, trying manual navigation"), 
            *OwnerCharacter->GetName());
        StartManualNavigation();
        return true;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Starting ground navigation. Location=(%.0f, %.0f, %.0f), Target=(%.0f, %.0f, %.0f)"),
        *OwnerCharacter->GetName(),
        OwnerCharacter->GetActorLocation().X, OwnerCharacter->GetActorLocation().Y, OwnerCharacter->GetActorLocation().Z,
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    
    // 绑定 NavMesh 回调
    if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
    {
        PathComp->OnRequestFinished.AddUObject(this, &UMANavigationService::OnNavMeshMoveCompleted);
    }
    
    // 将目标点投影到 NavMesh 上
    FNavLocation ProjectedLocation;
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerCharacter->GetWorld());
    if (NavSys)
    {
        // 限制查询范围，避免投影到错误的 NavMesh 层（如地下）
        FVector QueryExtent(500.f, 500.f, 200.f);
        if (NavSys->ProjectPointToNavigation(TargetLocation, ProjectedLocation, QueryExtent))
        {
            // 检查投影结果是否合理（Z 轴偏移不应超过 150）
            float ZDiff = FMath::Abs(ProjectedLocation.Location.Z - TargetLocation.Z);
            if (ZDiff < 150.f)
            {
                if (!ProjectedLocation.Location.Equals(TargetLocation, 1.f))
                {
                    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Target projected from (%.0f, %.0f, %.0f) to (%.0f, %.0f, %.0f)"),
                        *OwnerCharacter->GetName(),
                        TargetLocation.X, TargetLocation.Y, TargetLocation.Z,
                        ProjectedLocation.Location.X, ProjectedLocation.Location.Y, ProjectedLocation.Location.Z);
                    TargetLocation = ProjectedLocation.Location;
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: Rejected projection with large Z diff (%.0f), keeping original target"),
                    *OwnerCharacter->GetName(), ZDiff);
            }
        }
    }
    
    // 检查距离
    float DistanceToTarget = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetLocation);
    
    // 调用 MoveToLocation
    EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(
        TargetLocation, AcceptanceRadius, true, true, false, true, nullptr);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: MoveToLocation result = %d"),
        *OwnerCharacter->GetName(), (int32)Result);
    
    // 保存当前请求 ID
    if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
    {
        CurrentRequestID = PathComp->GetCurrentRequestId();
        bHasActiveNavMeshRequest = true;
    }
    
    // 处理结果
    if (Result == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: NavMesh path failed, trying manual navigation"), 
            *OwnerCharacter->GetName());
        StartManualNavigation();
    }
    else if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        float ActualDistance = FVector::Dist2D(OwnerCharacter->GetActorLocation(), TargetLocation);
        if (ActualDistance < AcceptanceRadius * 2.f)
        {
            SetNavigationState(EMANavigationState::Arrived);
            CompleteNavigation(true, FString::Printf(
                TEXT("Navigate succeeded: Already at destination (%.0f, %.0f, %.0f)"), 
                TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: AlreadyAtGoal but distance=%.0f, trying manual navigation"), 
                *OwnerCharacter->GetName(), ActualDistance);
            StartManualNavigation();
        }
    }
    else if (Result == EPathFollowingRequestResult::RequestSuccessful && DistanceToTarget > AcceptanceRadius * 3.f)
    {
        // 设置延迟检查，防止假成功
        if (UWorld* World = OwnerCharacter->GetWorld())
        {
            FVector CheckStartLocation = OwnerCharacter->GetActorLocation();
            FTimerHandle CheckHandle;
            
            World->GetTimerManager().SetTimer(
                CheckHandle,
                [this, CheckStartLocation]()
                {
                    if (!OwnerCharacter || !bHasActiveNavMeshRequest) return;
                    
                    float MovedDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), CheckStartLocation);
                    if (MovedDistance < 30.f)
                    {
                        UE_LOG(LogTemp, Error, TEXT("[MANavigationService] %s: NavMesh FALSE SUCCESS detected!"),
                            *OwnerCharacter->GetName());
                        
                        if (AAIController* AI = Cast<AAIController>(OwnerCharacter->GetController()))
                        {
                            AI->StopMovement();
                        }
                        
                        bHasActiveNavMeshRequest = false;
                        StartManualNavigation();
                    }
                },
                0.5f,
                false
            );
        }
    }
    
    return true;
}


void UMANavigationService::StartManualNavigation()
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character lost"));
        return;
    }

    // 确保停止 AIController 的 NavMesh 寻路，避免与手动导航双重控制冲突
    if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
    {
        AICtrl->StopMovement();
        if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
    }
    bHasActiveNavMeshRequest = false;

    bUsingManualNavigation = true;
    ManualNavStuckTime = 0.f;
    LastManualNavLocation = OwnerCharacter->GetActorLocation();
    
    // 从 ConfigManager 读取路径规划配置
    EMAPathPlannerType PlannerType = EMAPathPlannerType::MultiLayerRaycast;
    FMAPathPlannerConfig PlannerConfig;
    
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
            {
                PlannerType = ConfigMgr->GetPathPlannerTypeEnum();
                PlannerConfig = ConfigMgr->GetPathPlannerConfig();
            }
        }
    }
    
    // 根据配置选择路径规划算法
    if (PlannerType == EMAPathPlannerType::ElevationMap)
    {
        PathPlanner = MakeUnique<FMAElevationMapPathfinder>(PlannerConfig);
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Using ElevationMap pathfinder"), 
            *OwnerCharacter->GetName());
    }
    else
    {
        PathPlanner = MakeUnique<FMAMultiLayerRaycast>(PlannerConfig);
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Using MultiLayerRaycast pathfinder"), 
            *OwnerCharacter->GetName());
    }
    PathPlanner->Reset();
    
    // 启动手动导航更新定时器
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ManualNavTimerHandle,
            FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateManualNavigation, ManualNavUpdateInterval),
            ManualNavUpdateInterval,
            true
        );
    }
}

void UMANavigationService::UpdateManualNavigation(float DeltaTime)
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character lost during manual navigation"));
        return;
    }
    
    FVector CurrentLocation = OwnerCharacter->GetActorLocation();
    float Distance2D = FVector::Dist2D(CurrentLocation, TargetLocation);
    
    // 检查是否到达目标
    if (Distance2D < AcceptanceRadius)
    {
        SetNavigationState(EMANavigationState::Arrived);
        CompleteNavigation(true, FString::Printf(
            TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"), 
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
        return;
    }
    
    // 检查是否卡住
    float MovedDistance = FVector::Dist(CurrentLocation, LastManualNavLocation);
    if (MovedDistance < 50.f)
    {
        ManualNavStuckTime += DeltaTime;
        if (ManualNavStuckTime > StuckTimeout)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: Manual navigation stuck for %.0fs, giving up"), 
                *OwnerCharacter->GetName(), StuckTimeout);
            SetNavigationState(EMANavigationState::Failed);
            CompleteNavigation(false, TEXT("Navigate failed: Path blocked, unable to reach destination"));
            return;
        }
    }
    else
    {
        ManualNavStuckTime = 0.f;
        LastManualNavLocation = CurrentLocation;
    }
    
    // 计算移动方向
    FVector MoveDirection;
    if (PathPlanner.IsValid())
    {
        MoveDirection = PathPlanner->CalculateDirection(
            OwnerCharacter->GetWorld(),
            CurrentLocation,
            TargetLocation,
            OwnerCharacter
        );
    }
    else
    {
        MoveDirection = (TargetLocation - CurrentLocation).GetSafeNormal();
        MoveDirection.Z = 0.f;
    }
    
    // 使用 AddMovementInput 移动
    OwnerCharacter->AddMovementInput(MoveDirection, 1.0f);
    
    // 更新朝向
    if (!MoveDirection.IsNearlyZero())
    {
        FRotator TargetRotation = MoveDirection.Rotation();
        TargetRotation.Pitch = 0.f;
        TargetRotation.Roll = 0.f;
        
        FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);
        OwnerCharacter->SetActorRotation(NewRotation);
    }
}

void UMANavigationService::OnNavMeshMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    FString OwnerName = OwnerCharacter ? OwnerCharacter->GetName() : TEXT("NULL");
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: OnNavMeshMoveCompleted - RequestID=%d, Result.Code=%d"),
        *OwnerName, RequestID.GetID(), (int32)Result.Code);
    
    // 检查是否是当前请求的回调
    if (!bHasActiveNavMeshRequest || RequestID != CurrentRequestID)
    {
        return;
    }
    
    bHasActiveNavMeshRequest = false;
    
    if (OwnerCharacter)
    {
        float ActualDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetLocation);
        
        // 检查假成功
        if (Result.IsSuccess() && ActualDistance > AcceptanceRadius * 2.f)
        {
            UE_LOG(LogTemp, Error, TEXT("[MANavigationService] %s: FALSE SUCCESS! Distance %.1f, switching to manual navigation"),
                *OwnerCharacter->GetName(), ActualDistance);
            StartManualNavigation();
            return;
        }
        
        // 如果 NavMesh 报告 Blocked/OffPath 但距离还很远，尝试手动导航
        if (!Result.IsSuccess() && ActualDistance > AcceptanceRadius * 2.f)
        {
            if (Result.Code == EPathFollowingResult::Blocked || Result.Code == EPathFollowingResult::OffPath)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: NavMesh %s, trying manual navigation"),
                    *OwnerCharacter->GetName(), 
                    Result.Code == EPathFollowingResult::Blocked ? TEXT("BLOCKED") : TEXT("OFF_PATH"));
                StartManualNavigation();
                return;
            }
        }
    }
    
    // 根据结果设置状态
    if (Result.IsSuccess())
    {
        SetNavigationState(EMANavigationState::Arrived);
        CompleteNavigation(true, FString::Printf(
            TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"), 
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
    }
    else
    {
        FString Message;
        switch (Result.Code)
        {
            case EPathFollowingResult::Blocked: 
                Message = TEXT("Navigate failed: Path blocked by obstacle"); 
                break;
            case EPathFollowingResult::OffPath: 
                Message = TEXT("Navigate failed: Lost navigation path"); 
                break;
            case EPathFollowingResult::Aborted: 
                Message = TEXT("Navigate failed: Navigation aborted"); 
                break;
            default: 
                Message = TEXT("Navigate failed: Unknown error"); 
                break;
        }
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, Message);
    }
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
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    
    // 清理飞行检查定时器
    if (FlightCheckTimerHandle.IsValid() && World)
    {
        World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
        FlightCheckTimerHandle.Invalidate();
    }
    
    // 清理手动导航定时器
    if (ManualNavTimerHandle.IsValid() && World)
    {
        World->GetTimerManager().ClearTimer(ManualNavTimerHandle);
        ManualNavTimerHandle.Invalidate();
    }
    
    // 清理跟随模式定时器
    if (FollowModeTimerHandle.IsValid() && World)
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
        FollowModeTimerHandle.Invalidate();
    }
    
    // 停止飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->CancelFlight();
        FlightController->StopMovement();
    }
    
    // 停止地面导航
    if (OwnerCharacter)
    {
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
            
            if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
            {
                PathComp->OnRequestFinished.RemoveAll(this);
            }
        }
    }
    
    // 重置状态
    bHasActiveNavMeshRequest = false;
    bUsingManualNavigation = false;
    bIsFollowingActor = false;
    bReturnHomeIsLanding = false;
    bIsNavigationPaused = false;
    FollowTarget.Reset();
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

void UMANavigationService::UpdateFlightOperation(float DeltaTime)
{
    if (!OwnerCharacter || !FlightController.IsValid())
    {
        FString OpName = (CurrentState == EMANavigationState::TakingOff) ? TEXT("TakeOff") : TEXT("Land");
        CompleteNavigation(false, FString::Printf(TEXT("%s failed: Character or FlightController lost"), *OpName));
        return;
    }
    
    // 调用 FlightController 的统一更新
    FlightController->UpdateFlight(DeltaTime);
    
    // 检查是否完成
    EMAFlightControlState FlightState = FlightController->GetState();
    
    if (FlightState == EMAFlightControlState::Idle || FlightState == EMAFlightControlState::Arrived)
    {
        FVector CurrentLocation = OwnerCharacter->GetActorLocation();
        
        if (CurrentState == EMANavigationState::TakingOff)
        {
            SetNavigationState(EMANavigationState::Arrived);
            CompleteNavigation(true, FString::Printf(
                TEXT("TakeOff succeeded: Reached altitude %.0fm"), CurrentLocation.Z / 100.f));
        }
        else if (CurrentState == EMANavigationState::Landing)
        {
            SetNavigationState(EMANavigationState::Arrived);
            CompleteNavigation(true, FString::Printf(
                TEXT("Land succeeded: Landed at altitude %.0fm"), CurrentLocation.Z / 100.f));
        }
    }
}

void UMANavigationService::UpdateReturnHome(float DeltaTime)
{
    if (!OwnerCharacter || !FlightController.IsValid())
    {
        CompleteNavigation(false, TEXT("ReturnHome failed: Character or FlightController lost"));
        return;
    }
    
    if (bReturnHomeIsLanding)
    {
        // 降落阶段：使用 FlightController
        FlightController->UpdateFlight(DeltaTime);
        
        EMAFlightControlState FlightState = FlightController->GetState();
        if (FlightState == EMAFlightControlState::Idle)
        {
            // 降落完成
            SetNavigationState(EMANavigationState::Arrived);
            FVector CurrentLocation = OwnerCharacter->GetActorLocation();
            CompleteNavigation(true, FString::Printf(
                TEXT("ReturnHome succeeded: Landed at (%.0f, %.0f, %.0f)"),
                CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z));
        }
    }
    else
    {
        // 飞行阶段：使用 FlightController
        FlightController->UpdateFlight(DeltaTime);
        
        if (FlightController->HasArrived())
        {
            // 到达 HomePosition 上空，开始降落
            bReturnHomeIsLanding = true;
            SetNavigationState(EMANavigationState::Landing);
            FlightController->Land();
            
            UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: ReturnHome - Arrived at home, starting landing"),
                *OwnerCharacter->GetName());
        }
    }
}
