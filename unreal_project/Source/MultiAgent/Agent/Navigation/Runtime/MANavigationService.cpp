// MANavigationService.cpp
// 统一导航服务组件实现（核心状态/生命周期）

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
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
