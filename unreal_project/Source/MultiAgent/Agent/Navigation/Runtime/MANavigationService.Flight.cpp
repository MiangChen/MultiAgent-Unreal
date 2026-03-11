// MANavigationService.Flight.cpp
// 飞行导航与飞行操作相关实现

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "../Infrastructure/MAFlightController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UMANavigationService::EnsureCanRunFlightCommand(const TCHAR* CommandName)
{
    if (!OwnerCharacter || !bIsFlying)
    {
        const FMANavigationCommandFeedback Feedback =
            FMANavigationUseCases::BuildFlightCommandPrecheck(OwnerCharacter, bIsFlying, false, CommandName);
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s"),
            *Feedback.Message);
        return false;
    }

    if (CurrentState != EMANavigationState::Idle && CurrentState != EMANavigationState::Arrived)
    {
        CleanupNavigation();
    }

    EnsureFlightControllerInitialized();
    const FMANavigationCommandFeedback Feedback =
        FMANavigationUseCases::BuildFlightCommandPrecheck(
            OwnerCharacter,
            bIsFlying,
            FlightController.IsValid(),
            CommandName);
    if (!Feedback.bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: %s"),
            *OwnerCharacter->GetName(), *Feedback.Message);
        return false;
    }

    return true;
}

void UMANavigationService::StartFlightTimer(void (UMANavigationService::*UpdateFunction)(float))
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    if (FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
    }

    World->GetTimerManager().SetTimer(
        FlightCheckTimerHandle,
        FTimerDelegate::CreateUObject(this, UpdateFunction, FlightCheckInterval),
        FlightCheckInterval,
        true
    );
}

bool UMANavigationService::TakeOff(float TargetAltitude)
{
    if (!EnsureCanRunFlightCommand(TEXT("TakeOff")))
    {
        return false;
    }
    
    // 记录目标高度用于完成消息
    TargetLocation = OwnerCharacter->GetActorLocation();
    TargetLocation.Z = TargetAltitude;
    bIsReturnHomeActive = false;
    
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
    
    StartFlightTimer(&UMANavigationService::UpdateFlightOperation);
    
    return true;
}

bool UMANavigationService::Land(float TargetAltitude)
{
    if (!EnsureCanRunFlightCommand(TEXT("Land")))
    {
        return false;
    }
    
    // 记录目标高度用于完成消息
    TargetLocation = OwnerCharacter->GetActorLocation();
    TargetLocation.Z = TargetAltitude;
    bIsReturnHomeActive = false;
    
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
    
    StartFlightTimer(&UMANavigationService::UpdateFlightOperation);
    
    return true;
}

bool UMANavigationService::ReturnHome(FVector HomePosition, float LandAltitude)
{
    if (!EnsureCanRunFlightCommand(TEXT("ReturnHome")))
    {
        return false;
    }
    
    // 保持当前高度飞回 HomePosition
    StartLocation = OwnerCharacter->GetActorLocation();
    TargetLocation = HomePosition;
    TargetLocation.Z = StartLocation.Z;  // 保持当前高度
    ReturnHomeLandAltitude = LandAltitude;
    bReturnHomeIsLanding = false;
    bIsReturnHomeActive = true;
    AcceptanceRadius = 100.f;
    
    SetNavigationState(EMANavigationState::Navigating);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: ReturnHome to (%.0f, %.0f), then land at %.0f"),
        *OwnerCharacter->GetName(), HomePosition.X, HomePosition.Y, LandAltitude);
    
    // 使用飞行控制器导航到 HomePosition
    FlightController->SetAcceptanceRadius(AcceptanceRadius);
    FlightController->FlyTo(TargetLocation);
    
    StartFlightTimer(&UMANavigationService::UpdateReturnHome);
    
    return true;
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
    bIsReturnHomeActive = false;
    
    // 确保飞行控制器已初始化
    EnsureFlightControllerInitialized();
    
    // 使用飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->SetAcceptanceRadius(AcceptanceRadius);
        FlightController->FlyTo(TargetLocation);
        
        StartFlightTimer(&UMANavigationService::UpdateFlightNavigation);
        return true;
    }
    
    // 回退：使用 CharacterMovementComponent 的飞行模式
    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->SetMovementMode(MOVE_Flying);
    }
    
    StartFlightTimer(&UMANavigationService::UpdateFlightNavigation);
    
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
            CompleteNavigateSuccess();
        }
        return;
    }
    
    // 回退：简单直线导航
    FVector CurrentLoc = OwnerCharacter->GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLoc;
    float Distance3D = ToTarget.Size();
    
    if (Distance3D < AcceptanceRadius)
    {
        CompleteNavigateSuccess();
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
