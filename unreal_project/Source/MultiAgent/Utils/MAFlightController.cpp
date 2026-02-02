// MAFlightController.cpp
// 飞行控制器实现

#include "MAFlightController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

//=============================================================================
// 多旋翼飞行控制器实现
//=============================================================================

void FMAMultiRotorFlightController::Initialize(ACharacter* InOwner)
{
    Owner = InOwner;
    State = EMAFlightControlState::Idle;
    CurrentSpeed = 0.f;
    SmoothedAvoidanceDirection = FVector::ZeroVector;
}

bool FMAMultiRotorFlightController::TakeOff(float TargetAltitude)
{
    if (!Owner || State != EMAFlightControlState::Idle) return false;

    float GroundZ = GetGroundHeight();
    float Altitude = (TargetAltitude > 0.f) ? TargetAltitude : (GroundZ + DefaultFlightAltitude);

    FVector CurrentLocation = Owner->GetActorLocation();
    TargetLocation = FVector(CurrentLocation.X, CurrentLocation.Y, Altitude);
    SetState(EMAFlightControlState::TakingOff);
    return true;
}

bool FMAMultiRotorFlightController::Land()
{
    if (!Owner || State == EMAFlightControlState::Idle) return false;

    FVector CurrentLocation = Owner->GetActorLocation();
    float GroundZ = GetGroundHeight();
    TargetLocation = FVector(CurrentLocation.X, CurrentLocation.Y, GroundZ + 50.f);
    SetState(EMAFlightControlState::Landing);
    return true;
}

void FMAMultiRotorFlightController::Hover()
{
    if (!Owner || State == EMAFlightControlState::Idle) return;
    
    TargetLocation = Owner->GetActorLocation();
    CurrentSpeed = 0.f;
    SetState(EMAFlightControlState::Idle);
}

void FMAMultiRotorFlightController::StopMovement()
{
    if (!Owner) return;
    
    // // 清零内部速度
    // CurrentSpeed = 0.f;
    // SmoothedAvoidanceDirection = FVector::ZeroVector;
    
    // 清零 CharacterMovementComponent 的速度
    if (UCharacterMovementComponent* MovementComp = Owner->GetCharacterMovement())
    {
        MovementComp->StopMovementImmediately();
    }
}

bool FMAMultiRotorFlightController::FlyTo(const FVector& Destination)
{
    if (!Owner) return false;

    // 确保高度不低于最低飞行高度
    TargetLocation = Destination;
    if (TargetLocation.Z < MinFlightAltitude)
    {
        TargetLocation.Z = MinFlightAltitude;
    }

    // 如果在 Idle 状态，直接进入 Flying
    if (State == EMAFlightControlState::Idle)
    {
        SetState(EMAFlightControlState::Flying);
    }
    else if (State != EMAFlightControlState::TakingOff && State != EMAFlightControlState::Landing)
    {
        SetState(EMAFlightControlState::Flying);
    }

    return true;
}

void FMAMultiRotorFlightController::UpdateFollowTarget(const FVector& NewTargetLocation)
{
    TargetLocation = NewTargetLocation;
    if (TargetLocation.Z < MinFlightAltitude)
    {
        TargetLocation.Z = MinFlightAltitude;
    }

    if (State == EMAFlightControlState::Idle)
    {
        TakeOff(TargetLocation.Z);
    }
    else if (State != EMAFlightControlState::TakingOff && State != EMAFlightControlState::Landing)
    {
        SetState(EMAFlightControlState::Flying);
    }
}

void FMAMultiRotorFlightController::UpdateFlight(float DeltaTime)
{
    if (!Owner) return;
    if (State == EMAFlightControlState::Idle || State == EMAFlightControlState::Arrived) return;

    FVector CurrentLocation = Owner->GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLocation;
    float Distance = ToTarget.Size();

    // 检查是否到达
    if (Distance < AcceptanceRadius)
    {
        CurrentSpeed = 0.f;

        if (State == EMAFlightControlState::TakingOff)
        {
            SetState(EMAFlightControlState::Idle);  // 起飞完成，进入悬停
        }
        else if (State == EMAFlightControlState::Landing)
        {
            SetState(EMAFlightControlState::Idle);  // 降落完成
        }
        else
        {
            SetState(EMAFlightControlState::Arrived);
        }
        return;
    }

    // 计算速度（距离越近速度越慢）
    float TargetSpeed = FMath::Min(MaxFlightSpeed, Distance * 0.5f);
    TargetSpeed = FMath::Max(100.f, TargetSpeed);
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, 2.f);

    // 计算移动方向（带避障）
    FVector Direction = ToTarget.GetSafeNormal();
    FVector AvoidanceDirection = CalculateAvoidanceDirection(Direction);

    // 移动
    FVector NewLocation = CurrentLocation + AvoidanceDirection * CurrentSpeed * DeltaTime;
    Owner->SetActorLocation(NewLocation);

    // 朝向
    if (!AvoidanceDirection.IsNearlyZero())
    {
        FRotator TargetRotation = AvoidanceDirection.Rotation();
        TargetRotation.Pitch = 0.f;
        TargetRotation.Roll = 0.f;
        Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), TargetRotation, DeltaTime, 5.f));
    }
}

void FMAMultiRotorFlightController::CancelFlight()
{
    Hover();
}

bool FMAMultiRotorFlightController::IsFlying() const
{
    return State == EMAFlightControlState::Flying ||
           State == EMAFlightControlState::TakingOff ||
           State == EMAFlightControlState::Landing;
}

void FMAMultiRotorFlightController::SetState(EMAFlightControlState NewState)
{
    State = NewState;
}

float FMAMultiRotorFlightController::GetGroundHeight() const
{
    if (!Owner) return 0.f;

    FVector Start = Owner->GetActorLocation();
    FVector End = Start - FVector(0.f, 0.f, 10000.f);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    if (Owner->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
    {
        return HitResult.Location.Z;
    }
    return 0.f;
}

FVector FMAMultiRotorFlightController::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    if (!Owner) return DesiredDirection;

    FVector CurrentLocation = Owner->GetActorLocation();
    float GroundZ = GetGroundHeight();

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);
    Params.bTraceComplex = true;

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    // 检测某方向是否畅通
    auto GetClearDistance = [&](const FVector& Dir, float MaxDistance) -> float
    {
        if (Dir.IsNearlyZero()) return 0.f;
        FHitResult Hit;
        bool bHit = Owner->GetWorld()->SweepSingleByObjectType(
            Hit, CurrentLocation, CurrentLocation + Dir * MaxDistance,
            FQuat::Identity, ObjectParams,
            FCollisionShape::MakeSphere(ObstacleAvoidanceRadius), Params
        );
        return bHit ? Hit.Distance : MaxDistance;
    };

    // 多层检测距离
    float NearDist = ObstacleDetectionRange * 0.3f;
    float MidDist = ObstacleDetectionRange * 0.6f;
    float FarDist = ObstacleDetectionRange;

    float ForwardClearDist = GetClearDistance(DesiredDirection, FarDist);

    // 计算避障紧急程度
    float AvoidanceUrgency = 0.f;
    if (ForwardClearDist < NearDist)
        AvoidanceUrgency = 1.0f;
    else if (ForwardClearDist < MidDist)
        AvoidanceUrgency = 0.6f;
    else if (ForwardClearDist < FarDist)
        AvoidanceUrgency = 0.3f;

    // 无需避障
    if (AvoidanceUrgency < 0.01f)
    {
        if (SmoothedAvoidanceDirection.IsNearlyZero())
            SmoothedAvoidanceDirection = DesiredDirection;
        else
        {
            SmoothedAvoidanceDirection = FMath::Lerp(SmoothedAvoidanceDirection, DesiredDirection, AvoidanceSmoothingFactor * 2.f);
            SmoothedAvoidanceDirection.Normalize();
        }
        return SmoothedAvoidanceDirection;
    }

    // 评估候选方向
    FVector Right = FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal();
    float CurrentAltitude = CurrentLocation.Z - GroundZ;

    struct FCandidate { FVector Dir; float Priority; };
    TArray<FCandidate> Candidates;

    Candidates.Add({(DesiredDirection * 0.7f + FVector::UpVector * 0.3f).GetSafeNormal(), 1.0f});
    Candidates.Add({(DesiredDirection * 0.5f + FVector::UpVector * 0.5f + Right * 0.2f).GetSafeNormal(), 0.9f});
    Candidates.Add({(DesiredDirection * 0.5f + FVector::UpVector * 0.5f - Right * 0.2f).GetSafeNormal(), 0.9f});
    Candidates.Add({FVector::UpVector, 0.7f});
    Candidates.Add({(DesiredDirection * 0.3f + Right * 0.7f).GetSafeNormal(), 0.5f});
    Candidates.Add({(DesiredDirection * 0.3f - Right * 0.7f).GetSafeNormal(), 0.5f});
    if (CurrentAltitude > MinFlightAltitude + 500.f)
    {
        Candidates.Add({(DesiredDirection * 0.7f - FVector::UpVector * 0.3f).GetSafeNormal(), 0.4f});
    }

    // 找最佳方向
    FVector BestDirection = FVector::UpVector;
    float BestScore = -1.f;

    for (const auto& C : Candidates)
    {
        float ClearDist = GetClearDistance(C.Dir, MidDist);
        if (ClearDist > NearDist)
        {
            float AlignScore = FMath::Max(0.f, FVector::DotProduct(C.Dir, DesiredDirection));
            float DistScore = ClearDist / MidDist;
            float TotalScore = C.Priority * 0.4f + AlignScore * 0.3f + DistScore * 0.3f;

            if (TotalScore > BestScore)
            {
                BestScore = TotalScore;
                BestDirection = C.Dir;
            }
        }
    }

    // 混合方向
    FVector TargetDirection;
    if (BestScore > 0.f)
    {
        float AvoidWeight = AvoidanceUrgency;
        TargetDirection = (DesiredDirection * (1.f - AvoidWeight) + BestDirection * AvoidWeight).GetSafeNormal();
    }
    else
    {
        TargetDirection = FVector::UpVector;
    }

    // 平滑
    if (SmoothedAvoidanceDirection.IsNearlyZero())
        SmoothedAvoidanceDirection = DesiredDirection;

    float SmoothFactor = FMath::Lerp(AvoidanceSmoothingFactor, AvoidanceSmoothingFactor * 4.f, AvoidanceUrgency);
    SmoothedAvoidanceDirection = FMath::Lerp(SmoothedAvoidanceDirection, TargetDirection, SmoothFactor);
    SmoothedAvoidanceDirection.Normalize();

    return SmoothedAvoidanceDirection;
}

//=============================================================================
// 固定翼飞行控制器实现
//=============================================================================

void FMAFixedWingFlightController::Initialize(ACharacter* InOwner)
{
    Owner = InOwner;
    State = EMAFlightControlState::Idle;
    FlightMode = EMAFixedWingMode::Cruising;
    CurrentSpeed = MinSpeed;
    bIsFlying = true;  // 固定翼始终在飞行
    TargetAltitude = CruiseAltitude;
}

bool FMAFixedWingFlightController::TakeOff(float InTargetAltitude)
{
    // 固定翼不支持垂直起飞，但可以爬升到目标高度
    if (!Owner) return false;
    
    TargetAltitude = (InTargetAltitude > 0.f) ? InTargetAltitude : CruiseAltitude;
    FlightMode = EMAFixedWingMode::Cruising;
    SetState(EMAFlightControlState::Flying);
    bIsFlying = true;
    return true;
}

bool FMAFixedWingFlightController::Land()
{
    // 固定翼不支持垂直降落，进入盘旋模式
    if (!Owner) return false;
    
    StartOrbit(Owner->GetActorLocation());
    // 固定翼降落需要跑道，这里只是进入低空盘旋
    TargetAltitude = 500.f;  // 低空盘旋
    SetState(EMAFlightControlState::Landing);
    return true;
}

bool FMAFixedWingFlightController::FlyTo(const FVector& Destination)
{
    if (!Owner) return false;

    TargetLocation = Destination;
    TargetAltitude = FMath::Max(Destination.Z, CruiseAltitude);
    FlightMode = EMAFixedWingMode::Navigating;
    SetState(EMAFlightControlState::Flying);
    return true;
}

void FMAFixedWingFlightController::UpdateFollowTarget(const FVector& NewTargetLocation)
{
    TargetLocation = NewTargetLocation;
    TargetAltitude = FMath::Max(NewTargetLocation.Z, CruiseAltitude);
    
    if (FlightMode != EMAFixedWingMode::Navigating)
    {
        FlightMode = EMAFixedWingMode::Navigating;
        SetState(EMAFlightControlState::Flying);
    }
}

void FMAFixedWingFlightController::UpdateFlight(float DeltaTime)
{
    if (!Owner) return;

    // 固定翼必须保持最低速度
    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinSpeed, MaxSpeed);

    switch (FlightMode)
    {
        case EMAFixedWingMode::Navigating:
            UpdateNavigation(DeltaTime);
            break;
        case EMAFixedWingMode::Orbiting:
            UpdateOrbit(DeltaTime);
            break;
        case EMAFixedWingMode::Cruising:
        default:
            UpdateCruising(DeltaTime);
            break;
    }
}

void FMAFixedWingFlightController::CancelFlight()
{
    // 固定翼取消导航后开始盘旋
    if (Owner)
    {
        StartOrbit(Owner->GetActorLocation());
    }
}

void FMAFixedWingFlightController::StopMovement()
{
    if (!Owner) return;
    
    // 固定翼不能真正停止，但可以减速到最低速度
    CurrentSpeed = MinSpeed;
    
    // 清零 CharacterMovementComponent 的速度
    if (UCharacterMovementComponent* MovementComp = Owner->GetCharacterMovement())
    {
        MovementComp->StopMovementImmediately();
    }
}

void FMAFixedWingFlightController::StartOrbit(const FVector& InOrbitCenter)
{
    OrbitCenterPoint = InOrbitCenter;
    OrbitCenterPoint.Z = TargetAltitude;
    FlightMode = EMAFixedWingMode::Orbiting;

    if (Owner)
    {
        FVector ToPlane = Owner->GetActorLocation() - OrbitCenterPoint;
        OrbitAngle = FMath::Atan2(ToPlane.Y, ToPlane.X);
    }
}

void FMAFixedWingFlightController::UpdateNavigation(float DeltaTime)
{
    if (!Owner) return;

    FVector CurrentLocation = Owner->GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLocation;
    float Distance = ToTarget.Size();

    // 到达目标，开始盘旋
    if (Distance < AcceptanceRadius)
    {
        StartOrbit(TargetLocation);
        SetState(EMAFlightControlState::Arrived);
        return;
    }

    // 计算期望方向并避障
    FVector DesiredDir = ToTarget.GetSafeNormal();
    FVector SafeDir = CalculateAvoidanceDirection(DesiredDir);

    // 逐渐转向（固定翼转弯较慢）
    FRotator CurrentRot = Owner->GetActorRotation();
    FRotator TargetRot = SafeDir.Rotation();
    TargetRot.Pitch = 0.f;
    TargetRot.Roll = 0.f;
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, TurnRate / 45.f);
    Owner->SetActorRotation(NewRot);

    // 使用实际朝向移动（固定翼不能侧飞）
    FVector ForwardDir = NewRot.Vector();
    FVector NewLocation = CurrentLocation + ForwardDir * CurrentSpeed * DeltaTime;
    NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetAltitude, DeltaTime, 2.f);
    Owner->SetActorLocation(NewLocation);
}

void FMAFixedWingFlightController::UpdateOrbit(float DeltaTime)
{
    if (!Owner) return;

    // 计算盘旋角速度
    float AngularSpeed = CurrentSpeed / OrbitRadius;
    OrbitAngle += AngularSpeed * DeltaTime;

    // 计算目标位置
    FVector TargetPos;
    TargetPos.X = OrbitCenterPoint.X + OrbitRadius * FMath::Cos(OrbitAngle);
    TargetPos.Y = OrbitCenterPoint.Y + OrbitRadius * FMath::Sin(OrbitAngle);
    TargetPos.Z = TargetAltitude;

    FVector CurrentLocation = Owner->GetActorLocation();

    // 计算切线方向
    FVector TangentDir(-FMath::Sin(OrbitAngle), FMath::Cos(OrbitAngle), 0.f);
    FVector SafeDir = CalculateAvoidanceDirection(TangentDir);

    // 更新朝向
    FRotator TargetRot = SafeDir.Rotation();
    TargetRot.Pitch = 0.f;
    TargetRot.Roll = 0.f;
    Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, DeltaTime, TurnRate / 30.f));

    // 移动到目标位置
    FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPos, DeltaTime, 3.f);
    NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetAltitude, DeltaTime, 2.f);
    Owner->SetActorLocation(NewLocation);
}

void FMAFixedWingFlightController::UpdateCruising(float DeltaTime)
{
    if (!Owner) return;

    FVector CurrentLocation = Owner->GetActorLocation();
    FVector ForwardDir = Owner->GetActorForwardVector();

    // 避障检测
    FVector SafeDir = CalculateAvoidanceDirection(ForwardDir);

    FVector NewLocation = CurrentLocation + SafeDir * CurrentSpeed * DeltaTime;
    NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetAltitude, DeltaTime, 2.f);
    Owner->SetActorLocation(NewLocation);

    // 更新朝向
    if (!SafeDir.Equals(ForwardDir, 0.01f))
    {
        FRotator TargetRot = SafeDir.Rotation();
        TargetRot.Pitch = 0.f;
        TargetRot.Roll = 0.f;
        Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, DeltaTime, TurnRate / 45.f));
    }
}

FVector FMAFixedWingFlightController::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    if (!Owner) return DesiredDirection;

    FVector CurrentLocation = Owner->GetActorLocation();

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    FVector TraceEnd = CurrentLocation + DesiredDirection * ObstacleDetectionRange;

    bool bHit = Owner->GetWorld()->SweepSingleByChannel(
        HitResult, CurrentLocation, TraceEnd, FQuat::Identity,
        ECC_WorldStatic, FCollisionShape::MakeSphere(ObstacleAvoidanceRadius), Params
    );

    if (!bHit) return DesiredDirection;

    float ObstacleDistance = HitResult.Distance;

    // 障碍物很近，优先向上爬升
    if (ObstacleDistance < ObstacleAvoidanceRadius * 3.f)
    {
        return FVector(DesiredDirection.X * 0.3f, DesiredDirection.Y * 0.3f, 1.f).GetSafeNormal();
    }

    // 尝试左右绕行
    TArray<FVector> AvoidanceDirections = {
        FVector(0.f, 0.f, 1.f),  // 上
        FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal(),   // 左
        -FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal()   // 右
    };

    for (const FVector& AvoidDir : AvoidanceDirections)
    {
        FVector TestEnd = CurrentLocation + AvoidDir * ObstacleDetectionRange;
        bool bTestHit = Owner->GetWorld()->SweepSingleByChannel(
            HitResult, CurrentLocation, TestEnd, FQuat::Identity,
            ECC_WorldStatic, FCollisionShape::MakeSphere(ObstacleAvoidanceRadius), Params
        );

        if (!bTestHit)
        {
            // 固定翼转弯较慢，混合更多原方向
            return (DesiredDirection * 0.5f + AvoidDir * 0.5f).GetSafeNormal();
        }
    }

    // 所有方向都被阻挡，向上爬升
    return FVector(DesiredDirection.X * 0.2f, DesiredDirection.Y * 0.2f, 1.f).GetSafeNormal();
}

void FMAFixedWingFlightController::SetState(EMAFlightControlState NewState)
{
    State = NewState;
}
