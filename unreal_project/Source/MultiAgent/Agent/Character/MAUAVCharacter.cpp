// MAUAVCharacter.cpp
// 多旋翼无人机实现

#include "MAUAVCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../Skill/MASkillComponent.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "StateTree.h"

AMAUAVCharacter::AMAUAVCharacter()
{
    AgentType = EMAAgentType::UAV;
    
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->DefaultLandMovementMode = MOVE_Flying;
    MovementComp->MaxFlySpeed = 600.f;
    MovementComp->GravityScale = 0.f;
    
    // 加载 Inspire 2 模型
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/dji_inspire2/dji_inspire2.dji_inspire2"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -75.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    }

    // 加载螺旋桨动画
    static ConstructorHelpers::FObjectFinder<UAnimSequence> PropellerAnimAsset(
        TEXT("/Game/Robot/dji_inspire2/dji_inspire2_2Hovering.dji_inspire2_2Hovering"));
    if (PropellerAnimAsset.Succeeded())
    {
        PropellerAnim = PropellerAnimAsset.Object;
    }
}

void AMAUAVCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (SkillComponent)
    {
        SkillComponent->OnEnergyDepleted.AddDynamic(this, &AMAUAVCharacter::OnEnergyDepleted);
        SkillComponent->EnergyDrainRate = 0.5f;
    }
    
    // 尝试加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled())
    {
        if (!StateTreeComponent->HasStateTree())
        {
            UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_UAV.ST_UAV"));
            if (ST)
            {
                StateTreeComponent->SetStateTreeAsset(ST);
                UE_LOG(LogTemp, Log, TEXT("[UAV %s] StateTree loaded: ST_UAV"), *AgentID);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[UAV %s] StateTree asset not found: /Game/StateTree/ST_UAV.ST_UAV"), *AgentID);
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[UAV %s] StateTree already set"), *AgentID);
        }
    }
    else if (StateTreeComponent && !StateTreeComponent->IsStateTreeEnabled())
    {
        UE_LOG(LogTemp, Log, TEXT("[UAV %s] StateTree disabled, using direct skill activation"), *AgentID);
    }
    
    SetFlightState(EMAFlightState::Landed);
    
    // 初始时螺旋桨不转，等待起飞
    if (PropellerAnim)
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->SetAnimation(PropellerAnim);
        GetMesh()->Stop();  // 初始停止
    }
}

void AMAUAVCharacter::SnapToGround()
{
    FVector CurrentLocation = GetActorLocation();
    FVector Start = CurrentLocation + FVector(0.f, 0.f, 1000.f);  // 从更高处开始
    FVector End = Start - FVector(0.f, 0.f, 5000.f);  // 向下检测
    
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params))
    {
        // UAV 放在地面上，稍微抬高一点（50cm）避免穿模
        FVector NewLocation = FVector(CurrentLocation.X, CurrentLocation.Y, HitResult.Location.Z + 50.f);
        SetActorLocation(NewLocation);
        UE_LOG(LogTemp, Log, TEXT("[UAV %s] Snapped to ground at Z=%.1f"), *AgentID, NewLocation.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UAV %s] SnapToGround failed - no ground detected, keeping Z=%.1f"), *AgentID, CurrentLocation.Z);
    }
}

void AMAUAVCharacter::InitializeSkillSet()
{
    // UAV 技能: Navigate, Search, Follow
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Search);
    AvailableSkills.Add(EMASkillType::Follow);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAUAVCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateFlight(DeltaTime);
    
    if (IsInAir() && ShouldDrainEnergy() && SkillComponent && SkillComponent->HasEnergy())
    {
        SkillComponent->DrainEnergy(DeltaTime);
    }
}

void AMAUAVCharacter::OnEnergyDepleted()
{
    Land();
}

// ========== 飞行系统 ==========

void AMAUAVCharacter::UpdateFlight(float DeltaTime)
{
    if (FlightState != EMAFlightState::Flying && 
        FlightState != EMAFlightState::TakingOff && 
        FlightState != EMAFlightState::Landing)
    {
        CurrentSpeed = 0.f;
        return;
    }
    
    FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = CurrentFlightTarget - CurrentLocation;
    float Distance = ToTarget.Size();
    
    if (Distance < AcceptanceRadius)
    {
        CurrentSpeed = 0.f;
        
        if (FlightState == EMAFlightState::TakingOff)
        {
            SetFlightState(EMAFlightState::Hovering);
            if (bHasPendingFlyTarget)
            {
                bHasPendingFlyTarget = false;
                FlyTo(PendingFlyTarget);
            }
        }
        else if (FlightState == EMAFlightState::Landing)
        {
            SetFlightState(EMAFlightState::Landed);
        }
        else
        {
            SetFlightState(EMAFlightState::Hovering);
        }
        return;
    }
    
    float TargetSpeed = FMath::Min(MaxFlightSpeed, Distance * 0.5f);
    TargetSpeed = FMath::Max(100.f, TargetSpeed);
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, 2.f);
    
    // 计算移动方向（带避障）
    FVector Direction = ToTarget.GetSafeNormal();
    FVector AvoidanceDirection = CalculateAvoidanceDirection(Direction);
    
    FVector NewLocation = CurrentLocation + AvoidanceDirection * CurrentSpeed * DeltaTime;
    SetActorLocation(NewLocation);
    
    // 朝向使用实际移动方向
    if (!AvoidanceDirection.IsNearlyZero())
    {
        FRotator TargetRotation = AvoidanceDirection.Rotation();
        TargetRotation.Pitch = 0.f;
        TargetRotation.Roll = 0.f;
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.f));
    }
    
    bIsMoving = true;
}

FVector AMAUAVCharacter::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    FVector CurrentLocation = GetActorLocation();
    
    // 前方障碍物检测
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    float DetectionDistance = ObstacleDetectionRange;
    FVector TraceEnd = CurrentLocation + DesiredDirection * DetectionDistance;
    
    // 使用球形检测，更可靠
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
        // 前方无障碍，直接前进
        return DesiredDirection;
    }
    
    // 检测到障碍物，尝试绕行
    float ObstacleDistance = HitResult.Distance;
    
    // 如果障碍物很近，优先向上爬升
    if (ObstacleDistance < ObstacleAvoidanceRadius * 2.f)
    {
        return FVector(0.f, 0.f, 1.f);  // 向上
    }
    
    // 尝试多个绕行方向：上、左、右
    TArray<FVector> AvoidanceDirections;
    AvoidanceDirections.Add(FVector(0.f, 0.f, 1.f));  // 上
    AvoidanceDirections.Add(FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal());  // 左
    AvoidanceDirections.Add(-FVector::CrossProduct(DesiredDirection, FVector::UpVector).GetSafeNormal()); // 右
    
    for (const FVector& AvoidDir : AvoidanceDirections)
    {
        FVector TestEnd = CurrentLocation + AvoidDir * DetectionDistance;
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
            // 找到可行方向，混合原方向和避障方向
            return (DesiredDirection * 0.3f + AvoidDir * 0.7f).GetSafeNormal();
        }
    }
    
    // 所有方向都被阻挡，向上爬升
    return FVector(0.f, 0.f, 1.f);
}

bool AMAUAVCharacter::TakeOff(float TargetAltitude)
{
    if (!SkillComponent || !SkillComponent->HasEnergy()) return false;
    if (FlightState != EMAFlightState::Landed) return false;
    
    float GroundZ = GetGroundHeight();
    float Altitude = (TargetAltitude > 0.f) ? TargetAltitude : (GroundZ + DefaultFlightAltitude);
    
    FVector CurrentLocation = GetActorLocation();
    CurrentFlightTarget = FVector(CurrentLocation.X, CurrentLocation.Y, Altitude);
    SetFlightState(EMAFlightState::TakingOff);
    return true;
}

bool AMAUAVCharacter::Land()
{
    if (FlightState == EMAFlightState::Landed) return false;
    
    FVector CurrentLocation = GetActorLocation();
    float GroundZ = GetGroundHeight();
    CurrentFlightTarget = FVector(CurrentLocation.X, CurrentLocation.Y, GroundZ + 50.f);
    SetFlightState(EMAFlightState::Landing);
    return true;
}

void AMAUAVCharacter::Hover()
{
    if (FlightState == EMAFlightState::Landed) return;
    CurrentFlightTarget = GetActorLocation();
    CurrentSpeed = 0.f;
    bIsMoving = false;
    SetFlightState(EMAFlightState::Hovering);
}

bool AMAUAVCharacter::FlyTo(FVector Destination)
{
    UE_LOG(LogTemp, Log, TEXT("[UAV %s] FlyTo: Dest=(%.1f, %.1f, %.1f), FlightState=%d, HasEnergy=%d"),
        *AgentName, Destination.X, Destination.Y, Destination.Z,
        (int32)FlightState, (SkillComponent && SkillComponent->HasEnergy()) ? 1 : 0);
    
    if (!SkillComponent || !SkillComponent->HasEnergy()) return false;
    if (FlightState == EMAFlightState::Landed) return false;
    CurrentFlightTarget = Destination;
    SetFlightState(EMAFlightState::Flying);
    return true;
}

void AMAUAVCharacter::SetFlightState(EMAFlightState NewState)
{
    if (FlightState != NewState)
    {
        EMAFlightState OldState = FlightState;
        FlightState = NewState;
        bIsMoving = (NewState == EMAFlightState::Flying || 
                     NewState == EMAFlightState::TakingOff || 
                     NewState == EMAFlightState::Landing);
        
        // 控制螺旋桨动画
        if (NewState == EMAFlightState::Landed)
        {
            // 降落后停止螺旋桨
            GetMesh()->Stop();
        }
        else if (OldState == EMAFlightState::Landed)
        {
            // 从地面起飞时启动螺旋桨
            GetMesh()->Play(true);
        }
    }
}

float AMAUAVCharacter::GetGroundHeight() const
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

bool AMAUAVCharacter::TryNavigateTo(FVector Destination)
{
    UE_LOG(LogTemp, Log, TEXT("[UAV %s] TryNavigateTo: Dest=(%.1f, %.1f, %.1f), FlightState=%d, HasEnergy=%d"),
        *AgentName, Destination.X, Destination.Y, Destination.Z,
        (int32)FlightState, (SkillComponent && SkillComponent->HasEnergy()) ? 1 : 0);
    
    if (!SkillComponent || !SkillComponent->HasEnergy()) return false;
    
    // 使用传入的目标高度（参数处理阶段已确保高度可达）
    FVector FlyTarget = Destination;
    
    if (FlightState == EMAFlightState::Landed)
    {
        bHasPendingFlyTarget = true;
        PendingFlyTarget = FlyTarget;
        TakeOff(Destination.Z);
        return true;
    }
    
    if (FlightState == EMAFlightState::TakingOff)
    {
        bHasPendingFlyTarget = true;
        PendingFlyTarget = FlyTarget;
        return true;
    }
    
    return FlyTo(Destination);
}

void AMAUAVCharacter::CancelNavigation()
{
    Hover();
}
