// MAFixedWingUAVCharacter.cpp
// 固定翼无人机实现

#include "MAFixedWingUAVCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../Skill/MASkillComponent.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "StateTree.h"

AMAFixedWingUAVCharacter::AMAFixedWingUAVCharacter()
{
    AgentType = EMAAgentType::FixedWingUAV;
    
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->DefaultLandMovementMode = MOVE_Flying;
    MovementComp->MaxFlySpeed = MaxSpeed;
    MovementComp->GravityScale = 0.f;
}

void AMAFixedWingUAVCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (SkillComponent)
    {
        SkillComponent->OnEnergyDepleted.AddDynamic(this, &AMAFixedWingUAVCharacter::OnEnergyDepleted);
        SkillComponent->EnergyDrainRate = 0.3f;
        SkillComponent->MaxEnergy = 150.f;
        SkillComponent->Energy = 150.f;
    }
    
    // 加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled() && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_FixedWingUAV.ST_FixedWingUAV"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
        }
    }
    
    // 固定翼起始在空中，立即开始飞行
    FVector Loc = GetActorLocation();
    SetActorLocation(FVector(Loc.X, Loc.Y, CruiseAltitude));
    TargetAltitude = CruiseAltitude;
    
    // 初始化飞行状态
    bIsFlying = true;
    CurrentSpeed = MinSpeed;
    FlightMode = EMAFixedWingFlightMode::Cruising;
}

void AMAFixedWingUAVCharacter::InitializeSkillSet()
{
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Search);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAFixedWingUAVCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bIsFlying)
    {
        UpdateFlight(DeltaTime);
        
        if (ShouldDrainEnergy() && SkillComponent && SkillComponent->HasEnergy())
        {
            SkillComponent->DrainEnergy(DeltaTime);
        }
    }
}

void AMAFixedWingUAVCharacter::OnEnergyDepleted()
{
    StopFlight();
}

bool AMAFixedWingUAVCharacter::StartFlight()
{
    if (!SkillComponent || !SkillComponent->HasEnergy()) return false;
    
    bIsFlying = true;
    CurrentSpeed = MinSpeed;
    FlightMode = EMAFixedWingFlightMode::Cruising;
    return true;
}

void AMAFixedWingUAVCharacter::StopFlight()
{
    bIsFlying = false;
    FlightMode = EMAFixedWingFlightMode::Cruising;
    CurrentSpeed = 0.f;
    bIsMoving = false;
}

bool AMAFixedWingUAVCharacter::FlyTowards(FVector Destination)
{
    if (!bIsFlying) StartFlight();
    
    TargetLocation = Destination;
    TargetAltitude = Destination.Z;
    FlightMode = EMAFixedWingFlightMode::Navigating;
    bIsMoving = true;
    return true;
}

void AMAFixedWingUAVCharacter::StartOrbit(FVector InOrbitCenter)
{
    OrbitCenter = InOrbitCenter;
    OrbitCenter.Z = TargetAltitude;
    FlightMode = EMAFixedWingFlightMode::Orbiting;
    
    // 计算当前相对于盘旋中心的角度
    FVector ToPlane = GetActorLocation() - OrbitCenter;
    OrbitAngle = FMath::Atan2(ToPlane.Y, ToPlane.X);
}

void AMAFixedWingUAVCharacter::UpdateFlight(float DeltaTime)
{
    // 固定翼必须保持最低速度
    CurrentSpeed = FMath::Clamp(CurrentSpeed, MinSpeed, MaxSpeed);
    
    switch (FlightMode)
    {
        case EMAFixedWingFlightMode::Navigating:
            UpdateNavigation(DeltaTime);
            break;
        case EMAFixedWingFlightMode::Orbiting:
            UpdateOrbit(DeltaTime);
            break;
        case EMAFixedWingFlightMode::Cruising:
        default:
            // 巡航模式：保持直线飞行
            {
                FVector CurrentLocation = GetActorLocation();
                FVector ForwardDir = GetActorForwardVector();
                
                // 避障检测
                FVector SafeDir = CalculateAvoidanceDirection(ForwardDir);
                
                FVector NewLocation = CurrentLocation + SafeDir * CurrentSpeed * DeltaTime;
                NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetAltitude, DeltaTime, 2.f);
                SetActorLocation(NewLocation);
                
                // 更新朝向
                if (!SafeDir.Equals(ForwardDir, 0.01f))
                {
                    FRotator TargetRot = SafeDir.Rotation();
                    TargetRot.Pitch = 0.f;
                    TargetRot.Roll = 0.f;
                    SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnRate / 45.f));
                }
            }
            break;
    }
}

void AMAFixedWingUAVCharacter::UpdateNavigation(float DeltaTime)
{
    FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLocation;
    float Distance = ToTarget.Size();
    
    if (Distance < 200.f)
    {
        // 到达目标，开始盘旋
        StartOrbit(TargetLocation);
        bIsMoving = false;
        return;
    }
    
    // 计算期望方向
    FVector DesiredDir = ToTarget.GetSafeNormal();
    
    // 避障检测
    FVector SafeDir = CalculateAvoidanceDirection(DesiredDir);
    
    // 逐渐转向
    FRotator CurrentRot = GetActorRotation();
    FRotator TargetRot = SafeDir.Rotation();
    TargetRot.Pitch = 0.f;
    TargetRot.Roll = 0.f;
    
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, TurnRate / 45.f);
    SetActorRotation(NewRot);
    
    // 使用实际朝向移动（固定翼不能侧飞）
    FVector ForwardDir = NewRot.Vector();
    FVector NewLocation = CurrentLocation + ForwardDir * CurrentSpeed * DeltaTime;
    NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetAltitude, DeltaTime, 2.f);
    SetActorLocation(NewLocation);
}

void AMAFixedWingUAVCharacter::UpdateOrbit(float DeltaTime)
{
    // 计算盘旋角速度（基于当前速度和盘旋半径）
    float AngularSpeed = CurrentSpeed / OrbitRadius;
    OrbitAngle += AngularSpeed * DeltaTime;
    
    // 计算目标位置（圆周上的点）
    FVector TargetPos;
    TargetPos.X = OrbitCenter.X + OrbitRadius * FMath::Cos(OrbitAngle);
    TargetPos.Y = OrbitCenter.Y + OrbitRadius * FMath::Sin(OrbitAngle);
    TargetPos.Z = TargetAltitude;
    
    FVector CurrentLocation = GetActorLocation();
    
    // 计算切线方向（圆周的切线）
    FVector TangentDir;
    TangentDir.X = -FMath::Sin(OrbitAngle);
    TangentDir.Y = FMath::Cos(OrbitAngle);
    TangentDir.Z = 0.f;
    
    // 避障检测
    FVector SafeDir = CalculateAvoidanceDirection(TangentDir);
    
    // 更新朝向
    FRotator TargetRot = SafeDir.Rotation();
    TargetRot.Pitch = 0.f;
    TargetRot.Roll = 0.f;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnRate / 30.f));
    
    // 移动到目标位置（平滑过渡）
    FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPos, DeltaTime, 3.f);
    NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetAltitude, DeltaTime, 2.f);
    SetActorLocation(NewLocation);
}

FVector AMAFixedWingUAVCharacter::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    FVector CurrentLocation = GetActorLocation();
    
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    FVector TraceEnd = CurrentLocation + DesiredDirection * ObstacleDetectionRange;
    
    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        CurrentLocation,
        TraceEnd,
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeSphere(ObstacleAvoidanceRadius),
        Params
    );
    
    if (!bHit)
    {
        return DesiredDirection;
    }
    
    float ObstacleDistance = HitResult.Distance;
    
    // 障碍物很近，优先向上爬升
    if (ObstacleDistance < ObstacleAvoidanceRadius * 3.f)
    {
        return FVector(DesiredDirection.X * 0.3f, DesiredDirection.Y * 0.3f, 1.f).GetSafeNormal();
    }
    
    // 尝试左右绕行
    TArray<FVector> AvoidanceDirections;
    AvoidanceDirections.Add(FVector(0.f, 0.f, 1.f));  // 上
    AvoidanceDirections.Add(FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal());  // 左
    AvoidanceDirections.Add(-FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal()); // 右
    
    for (const FVector& AvoidDir : AvoidanceDirections)
    {
        FVector TestEnd = CurrentLocation + AvoidDir * ObstacleDetectionRange;
        bool bTestHit = GetWorld()->SweepSingleByChannel(
            HitResult,
            CurrentLocation,
            TestEnd,
            FQuat::Identity,
            ECC_WorldStatic,
            FCollisionShape::MakeSphere(ObstacleAvoidanceRadius),
            Params
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

bool AMAFixedWingUAVCharacter::TryNavigateTo(FVector Destination)
{
    return FlyTowards(Destination);
}

void AMAFixedWingUAVCharacter::CancelNavigation()
{
    if (FlightMode == EMAFixedWingFlightMode::Navigating)
    {
        // 取消导航后开始在当前位置盘旋
        StartOrbit(GetActorLocation());
    }
    bIsMoving = false;
}
