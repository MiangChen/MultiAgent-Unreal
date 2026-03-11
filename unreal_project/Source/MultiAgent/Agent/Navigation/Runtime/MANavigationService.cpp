// MANavigationService.cpp
// 统一导航服务组件实现（核心状态/生命周期）

#include "MANavigationService.h"
#include "Agent/Navigation/Bootstrap/MANavigationBootstrap.h"
#include "Core/Shared/Types/MATypes.h"
#include "../Infrastructure/MAFlightController.h"
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

    InitializeFromConfig();
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
    if (HasActiveNavigationOperation())
    {
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: NavigateTo - Cancelling previous operation (state=%d)"),
            *OwnerCharacter->GetName(), (int32)CurrentState);
        CleanupNavigation();
    }
    
    InitializeNavigateRequest(Destination, InAcceptanceRadius);
    
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
    if (!HasActiveNavigationOperation())
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
    return HasActiveNavigationOperation();
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

bool UMANavigationService::HasActiveNavigationOperation() const
{
    return CurrentState == EMANavigationState::Navigating ||
           CurrentState == EMANavigationState::TakingOff ||
           CurrentState == EMANavigationState::Landing;
}

bool UMANavigationService::IsNavigationTerminalState() const
{
    return CurrentState == EMANavigationState::Arrived ||
           CurrentState == EMANavigationState::Failed ||
           CurrentState == EMANavigationState::Cancelled;
}

void UMANavigationService::InitializeNavigateRequest(const FVector& Destination, const float InAcceptanceRadius)
{
    TargetLocation = Destination;
    AcceptanceRadius = InAcceptanceRadius;
    StartLocation = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
    bUsingManualNavigation = false;
    bHasActiveNavMeshRequest = false;
    ManualNavStuckTime = 0.f;
    bIsReturnHomeActive = false;
}

void UMANavigationService::ScheduleResetToIdle()
{
    if (!OwnerCharacter)
    {
        return;
    }

    UWorld* World = OwnerCharacter->GetWorld();
    if (!World)
    {
        return;
    }

    FTimerHandle ResetHandle;
    World->GetTimerManager().SetTimer(
        ResetHandle,
        [this]()
        {
            if (IsNavigationTerminalState())
            {
                SetNavigationState(EMANavigationState::Idle);
            }
        },
        0.1f,
        false
    );
}


