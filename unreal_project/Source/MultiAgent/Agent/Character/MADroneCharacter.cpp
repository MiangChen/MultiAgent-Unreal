// MADroneCharacter.cpp
// 无人机飞行系统实现 - 不使用 NavMesh，直接位置控制

#include "MADroneCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTree.h"
#include "MAAbilitySystemComponent.h"
#include "MAGameplayTags.h"
#include "MAStateTreeComponent.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"

AMADroneCharacter::AMADroneCharacter()
{
    AgentType = EMAAgentType::Drone;
    
    // Energy defaults
    Energy = 100.f;
    MaxEnergy = 100.f;
    EnergyDrainRate = 0.5f;
    
    // StateTree 组件
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // 配置移动组件 - 飞行模式
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->DefaultLandMovementMode = MOVE_Flying;
    MovementComp->BrakingDecelerationFlying = 1000.f;
    MovementComp->MaxFlySpeed = 600.f;
    MovementComp->GravityScale = 0.f;  // 禁用重力
}

void AMADroneCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Drone BeginPlay"), *AgentName);
    
    // 动态加载 StateTree
    if (StateTreeComponent && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_Drone.ST_Drone"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
            UE_LOG(LogTemp, Log, TEXT("[%s] StateTree loaded: %s"), *AgentName, *ST->GetName());
        }
    }
    
    // 初始状态：停在地面，等待起飞命令
    SetFlightState(EMADroneFlightState::Landed);
    
    // 启动螺旋桨动画
    if (PropellerAnim)
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->SetAnimation(PropellerAnim);
        GetMesh()->Play(true);
    }
}

void AMADroneCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 更新飞行
    UpdateFlight(DeltaTime);
    
    // 螺旋桨动画
    UpdatePropellerAnimation();
    
    // 飞行时消耗能量
    if (IsInAir() && HasEnergy())
    {
        DrainEnergy(DeltaTime);
    }
    
    UpdateEnergyDisplay();
    CheckLowEnergyStatus();
}

// ========== Flight System ==========

void AMADroneCharacter::UpdateFlight(float DeltaTime)
{
    if (FlightState != EMADroneFlightState::Flying && 
        FlightState != EMADroneFlightState::TakingOff && 
        FlightState != EMADroneFlightState::Landing)
    {
        CurrentSpeed = 0.f;
        return;
    }
    
    FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = CurrentFlightTarget - CurrentLocation;
    float Distance = ToTarget.Size();
    
    // 到达目标
    if (Distance < AcceptanceRadius)
    {
        CurrentSpeed = 0.f;
        
        if (FlightState == EMADroneFlightState::TakingOff)
        {
            SetFlightState(EMADroneFlightState::Hovering);
            UE_LOG(LogTemp, Log, TEXT("[%s] TakeOff complete at Z=%.0f"), *AgentName, CurrentLocation.Z);
            
            // 检查是否有待执行的飞行目标
            if (bHasPendingFlyTarget)
            {
                bHasPendingFlyTarget = false;
                FlyTo(PendingFlyTarget);
                UE_LOG(LogTemp, Log, TEXT("[%s] Now flying to pending target (%.0f, %.0f, %.0f)"), 
                    *AgentName, PendingFlyTarget.X, PendingFlyTarget.Y, PendingFlyTarget.Z);
            }
        }
        else if (FlightState == EMADroneFlightState::Landing)
        {
            SetFlightState(EMADroneFlightState::Landed);
            UE_LOG(LogTemp, Log, TEXT("[%s] Landing complete"), *AgentName);
        }
        else
        {
            SetFlightState(EMADroneFlightState::Hovering);
            OnFlightCompleted.Broadcast(true, CurrentLocation);
            UE_LOG(LogTemp, Log, TEXT("[%s] Flight complete at (%.0f, %.0f, %.0f)"), 
                *AgentName, CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
        }
        return;
    }
    
    // 碰撞检测
    if (bEnableCollisionCheck && FlightState == EMADroneFlightState::Flying)
    {
        FHitResult HitResult;
        if (CheckForwardCollision(HitResult))
        {
            // 检测到障碍物，停止并悬停
            Hover();
            OnCollisionDetected.Broadcast(HitResult);
            OnFlightCompleted.Broadcast(false, CurrentLocation);
            UE_LOG(LogTemp, Warning, TEXT("[%s] Collision detected! Stopping at (%.0f, %.0f, %.0f)"), 
                *AgentName, CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
            return;
        }
    }
    
    // 计算速度 (加速/减速)
    float TargetSpeed = MaxFlightSpeed;
    
    // 接近目标时减速
    if (Distance < MaxFlightSpeed * 2.f)
    {
        TargetSpeed = FMath::Max(100.f, Distance * 0.5f);
    }
    
    // 平滑加速
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, FlightAcceleration / 100.f);
    
    // 移动 (使用 Sweep 检测碰撞)
    FVector Direction = ToTarget.GetSafeNormal();
    FVector NewLocation = CurrentLocation + Direction * CurrentSpeed * DeltaTime;
    FHitResult SweepHit;
    SetActorLocation(NewLocation, true, &SweepHit);  // true = sweep, 会检测碰撞
    
    // 如果 Sweep 检测到碰撞，停止并悬停
    if (SweepHit.bBlockingHit)
    {
        Hover();
        OnCollisionDetected.Broadcast(SweepHit);
        UE_LOG(LogTemp, Warning, TEXT("[%s] Sweep collision with %s! Hovering."), 
            *AgentName, *GetNameSafe(SweepHit.GetActor()));
        return;
    }
    
    // 朝向飞行方向 (只旋转 Yaw)
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        TargetRotation.Pitch = 0.f;
        TargetRotation.Roll = 0.f;
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.f));
    }
    
    bIsMoving = true;
}

bool AMADroneCharacter::TakeOff(float TargetAltitude)
{
    if (!HasEnergy())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Cannot take off - no energy!"), *AgentName);
        return false;
    }
    
    if (FlightState != EMADroneFlightState::Landed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Already in air"), *AgentName);
        return false;
    }
    
    // 计算目标高度（绝对 Z 坐标）
    float GroundZ = GetGroundHeight();
    float Altitude;
    if (TargetAltitude > 0.f)
    {
        Altitude = TargetAltitude;  // 使用传入的绝对高度
    }
    else
    {
        Altitude = GroundZ + DefaultFlightAltitude;  // 地面 + 默认飞行高度
    }
    
    FVector CurrentLocation = GetActorLocation();
    CurrentFlightTarget = FVector(CurrentLocation.X, CurrentLocation.Y, Altitude);
    
    SetFlightState(EMADroneFlightState::TakingOff);
    UE_LOG(LogTemp, Log, TEXT("[%s] TakeOff: CurrentZ=%.0f, GroundZ=%.0f, TargetZ=%.0f"), 
        *AgentName, CurrentLocation.Z, GroundZ, Altitude);
    
    return true;
}

bool AMADroneCharacter::Land()
{
    if (FlightState == EMADroneFlightState::Landed)
    {
        return false;
    }
    
    FVector CurrentLocation = GetActorLocation();
    float GroundZ = GetGroundHeight();
    CurrentFlightTarget = FVector(CurrentLocation.X, CurrentLocation.Y, GroundZ + 50.f);
    
    SetFlightState(EMADroneFlightState::Landing);
    UE_LOG(LogTemp, Log, TEXT("[%s] Landing..."), *AgentName);
    
    return true;
}

void AMADroneCharacter::Hover()
{
    if (FlightState == EMADroneFlightState::Landed)
    {
        return;
    }
    
    CurrentFlightTarget = GetActorLocation();
    CurrentSpeed = 0.f;
    bIsMoving = false;
    SetFlightState(EMADroneFlightState::Hovering);
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Hovering at (%.0f, %.0f, %.0f)"), 
        *AgentName, CurrentFlightTarget.X, CurrentFlightTarget.Y, CurrentFlightTarget.Z);
}

bool AMADroneCharacter::FlyTo(FVector Destination)
{
    if (!HasEnergy())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Cannot fly - no energy!"), *AgentName);
        return false;
    }
    
    if (FlightState == EMADroneFlightState::Landed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Must take off first!"), *AgentName);
        return false;
    }
    
    CurrentFlightTarget = Destination;
    SetFlightState(EMADroneFlightState::Flying);
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Flying to (%.0f, %.0f, %.0f)"), 
        *AgentName, Destination.X, Destination.Y, Destination.Z);
    
    return true;
}

void AMADroneCharacter::CancelFlight()
{
    if (FlightState == EMADroneFlightState::Flying)
    {
        Hover();
        OnFlightCompleted.Broadcast(false, GetActorLocation());
    }
}

void AMADroneCharacter::SetFlightState(EMADroneFlightState NewState)
{
    if (FlightState != NewState)
    {
        FlightState = NewState;
        bIsMoving = (NewState == EMADroneFlightState::Flying || 
                     NewState == EMADroneFlightState::TakingOff || 
                     NewState == EMADroneFlightState::Landing);
    }
}

float AMADroneCharacter::GetGroundHeight() const
{
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0.f, 0.f, 10000.f);
    
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
    {
        return HitResult.Location.Z;
    }
    
    return 0.f;
}

// ========== Collision Detection ==========

bool AMADroneCharacter::CheckForwardCollision(FHitResult& OutHit) const
{
    FVector Direction = (CurrentFlightTarget - GetActorLocation()).GetSafeNormal();
    return CheckCollisionInDirection(Direction, CollisionCheckDistance, OutHit);
}

bool AMADroneCharacter::CheckCollisionInDirection(FVector Direction, float Distance, FHitResult& OutHit) const
{
    if (Direction.IsNearlyZero())
    {
        return false;
    }
    
    FVector Start = GetActorLocation();
    FVector End = Start + Direction * Distance;
    
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    bool bHit = GetWorld()->SweepSingleByChannel(
        OutHit,
        Start,
        End,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeSphere(CollisionCheckRadius),
        Params
    );
    
    // Debug 绘制
    #if WITH_EDITOR
    if (bEnableCollisionCheck)
    {
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Red : FColor::Green, false, 0.1f);
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), OutHit.Location, 20.f, 8, FColor::Red, false, 0.1f);
        }
    }
    #endif
    
    return bHit;
}

// ========== Energy System ==========

void AMADroneCharacter::DrainEnergy(float DeltaTime)
{
    Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);
    
    if (Energy <= 0.f)
    {
        Land();
        UE_LOG(LogTemp, Warning, TEXT("[%s] Energy depleted! Emergency landing."), *AgentName);
    }
}

void AMADroneCharacter::RestoreEnergy(float Amount)
{
    Energy = FMath::Min(MaxEnergy, Energy + Amount);
}

void AMADroneCharacter::UpdateEnergyDisplay()
{
    FVector TextLocation = GetActorLocation() + FVector(0.f, 0.f, 80.f);
    FString EnergyText = FString::Printf(TEXT("%.0f%%"), GetEnergyPercent());
    
    FColor DisplayColor = FColor::Green;
    if (Energy < LowEnergyThreshold)
    {
        DisplayColor = FColor::Red;
    }
    else if (Energy < 50.f)
    {
        DisplayColor = FColor::Yellow;
    }
    
    DrawDebugString(GetWorld(), TextLocation, EnergyText, nullptr, DisplayColor, 0.f, true, 1.0f);
}

void AMADroneCharacter::CheckLowEnergyStatus()
{
    if (!AbilitySystemComponent) return;
    
    const FMAGameplayTags& Tags = FMAGameplayTags::Get();
    bool bHasLowEnergyTag = AbilitySystemComponent->HasGameplayTagFromContainer(Tags.Status_LowEnergy);
    
    if (Energy < LowEnergyThreshold && !bHasLowEnergyTag)
    {
        AbilitySystemComponent->AddLooseGameplayTag(Tags.Status_LowEnergy);
        UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy warning! %.0f%%"), *AgentName, GetEnergyPercent());
    }
    else if (Energy >= LowEnergyThreshold && bHasLowEnergyTag)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(Tags.Status_LowEnergy);
    }
}

// ========== Animation ==========

void AMADroneCharacter::UpdatePropellerAnimation()
{
    if (PropellerAnim && !GetMesh()->IsPlaying())
    {
        GetMesh()->SetAnimation(PropellerAnim);
        GetMesh()->Play(true);
    }
}

// ========== 重写基类导航 ==========

bool AMADroneCharacter::TryNavigateTo(FVector Destination)
{
    if (!HasEnergy())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Cannot navigate - no energy!"), *AgentName);
        return false;
    }
    
    // 计算目标飞行高度
    float GroundZ = GetGroundHeight();
    float TargetFlightZ = GroundZ + DefaultFlightAltitude;
    FVector FlyTarget = FVector(Destination.X, Destination.Y, TargetFlightZ);
    
    // 如果在地面，先起飞
    if (FlightState == EMADroneFlightState::Landed)
    {
        // 保存待飞行目标，起飞完成后执行
        bHasPendingFlyTarget = true;
        PendingFlyTarget = FlyTarget;
        
        TakeOff(TargetFlightZ);
        UE_LOG(LogTemp, Log, TEXT("[%s] TakeOff first to Z=%.0f, then will fly to (%.0f, %.0f, %.0f)"), 
            *AgentName, TargetFlightZ, FlyTarget.X, FlyTarget.Y, FlyTarget.Z);
        return true;
    }
    
    // 如果正在起飞，更新待飞行目标
    if (FlightState == EMADroneFlightState::TakingOff)
    {
        bHasPendingFlyTarget = true;
        PendingFlyTarget = FlyTarget;
        UE_LOG(LogTemp, Log, TEXT("[%s] Still taking off, updated pending target to (%.0f, %.0f, %.0f)"), 
            *AgentName, FlyTarget.X, FlyTarget.Y, FlyTarget.Z);
        return true;
    }
    
    // 已在空中且悬停/飞行中，直接飞向目标（保持当前高度）
    FVector Target = FVector(Destination.X, Destination.Y, GetActorLocation().Z);
    return FlyTo(Target);
}

void AMADroneCharacter::CancelNavigation()
{
    CancelFlight();
}

// ========== Drone Abilities ==========

bool AMADroneCharacter::TrySearch()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateSearch();
    }
    return false;
}

void AMADroneCharacter::StopSearch()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelSearch();
    }
}

bool AMADroneCharacter::TryCharge()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateCharge();
    }
    return false;
}
