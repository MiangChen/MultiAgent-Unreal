// MABoat.cpp
// 船只环境对象实现
// 使用简单的 Tick 直线移动，保持水面高度

#include "MABoat.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../Effect/MAFire.h"

AMABoat::AMABoat()
{
    PrimaryActorTick.bCanEverTick = true;

    // 禁用 CharacterMovement，使用自定义移动
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->GravityScale = 0.f;
    MovementComp->SetMovementMode(MOVE_Flying);

    // 启用碰撞阻挡
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 隐藏默认 SkeletalMesh
    GetMesh()->SetVisibility(false);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 创建船只网格组件
    BoatMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatMesh"));
    BoatMeshComponent->SetupAttachment(RootComponent);
    BoatMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoatMeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));  // 模型前向是 Y 轴
}

void AMABoat::BeginPlay()
{
    Super::BeginPlay();
    WaterLevel = GetActorLocation().Z;
}

void AMABoat::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!bIsMoving) return;
    
    FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLocation;
    ToTarget.Z = 0.f;  // 只考虑水平距离
    float Distance = ToTarget.Size();
    
    // 到达判定
    if (Distance < AcceptanceRadius)
    {
        bIsMoving = false;
        OnMoveCompleted(true);
        return;
    }
    
    // 计算移动
    FVector Direction = ToTarget.GetSafeNormal();
    float MoveDistance = FMath::Min(MoveSpeed * DeltaTime, Distance);
    FVector NewLocation = CurrentLocation + Direction * MoveDistance;
    NewLocation.Z = CurrentLocation.Z;  // 保持水面高度
    
    SetActorLocation(NewLocation);
    
    // 平滑转向
    FRotator TargetRotation = Direction.Rotation();
    TargetRotation.Pitch = 0.f;
    TargetRotation.Roll = 0.f;
    FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, TurnSpeed);
    SetActorRotation(NewRotation);
}

//=============================================================================
// IMAEnvironmentObject 接口实现
//=============================================================================

FString AMABoat::GetObjectLabel() const { return ObjectLabel; }
FString AMABoat::GetObjectType() const { return ObjectType; }
const TMap<FString, FString>& AMABoat::GetObjectFeatures() const { return Features; }

//=============================================================================
// 配置方法
//=============================================================================

void AMABoat::Configure(const FMAEnvironmentObjectConfig& Config)
{
    ObjectLabel = Config.Label;
    ObjectType = Config.Type;
    Features = Config.Features;
    SetActorRotation(Config.Rotation);

    // 根据 subtype 选择船只模型
    FString Subtype = Features.FindRef(TEXT("subtype"));
    if (Subtype.IsEmpty()) Subtype = TEXT("lifeboat");
    SetBoatMesh(Subtype);
    
    // Mesh 设置后，调整胶囊体和船体位置
    AutoFitCapsuleToMesh();
    
    // 设置船体在水面的位置
    // Config.Position.Z 是水面高度，船底需要略低于水面（吃水）
    WaterLevel = Config.Position.Z;
    float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    // Actor 位置 = 水面高度 - 吃水深度 + 胶囊体半高（使船底在水面下 WaterlineDraft）
    float BoatZ = WaterLevel - WaterlineDraft + CapsuleHalfHeight;
    SetActorLocation(FVector(Config.Position.X, Config.Position.Y, BoatZ));

    // 配置巡逻
    if (Config.Patrol.bEnabled && Config.Patrol.Waypoints.Num() > 0)
    {
        PatrolWaypoints = Config.Patrol.Waypoints;
        bPatrolLoop = Config.Patrol.bLoop;
        PatrolWaitTime = Config.Patrol.WaitTime;
        
        if (UWorld* World = GetWorld())
        {
            FTimerHandle StartTimerHandle;
            World->GetTimerManager().SetTimer(StartTimerHandle, this, &AMABoat::StartPatrol, 1.0f, false);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MABoat] %s: WaterLevel=%.1f, Draft=%.1f, ActorZ=%.1f"), 
        *ObjectLabel, WaterLevel, WaterlineDraft, BoatZ);
}

void AMABoat::SetBoatMesh(const FString& Subtype)
{
    FString MeshPath = GetBoatMeshPath(Subtype);
    UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    
    if (!LoadedMesh)
    {
        MeshPath = GetBoatMeshPath(TEXT("lifeboat"));
        LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    }

    if (LoadedMesh && BoatMeshComponent)
    {
        BoatMeshComponent->SetStaticMesh(LoadedMesh);
    }
}

//=============================================================================
// 导航接口 - 简单直线移动
//=============================================================================

bool AMABoat::NavigateTo(FVector Destination, float InAcceptanceRadius)
{
    TargetLocation = Destination;
    TargetLocation.Z = GetActorLocation().Z;  // 保持水面高度
    AcceptanceRadius = InAcceptanceRadius;
    bIsMoving = true;
    return true;
}

void AMABoat::CancelNavigation()
{
    bIsMoving = false;
}

bool AMABoat::IsNavigating() const
{
    return bIsMoving;
}

void AMABoat::OnMoveCompleted(bool bSuccess)
{
    // 如果正在巡逻，处理巡逻逻辑
    if (bIsPatrolling)
    {
        if (bSuccess)
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(PatrolWaitTimerHandle, this, &AMABoat::MoveToNextWaypoint, PatrolWaitTime, false);
            }
        }
        else
        {
            MoveToNextWaypoint();
        }
    }
}

//=============================================================================
// 私有方法
//=============================================================================

void AMABoat::AutoFitCapsuleToMesh()
{
    if (!BoatMeshComponent || !BoatMeshComponent->GetStaticMesh()) return;
    
    FBoxSphereBounds Bounds = BoatMeshComponent->GetStaticMesh()->GetBounds();
    
    float Radius = (Bounds.BoxExtent.X + Bounds.BoxExtent.Y) * 0.5f;
    float HalfHeight = Bounds.BoxExtent.Z * 0.85f;
    Radius = FMath::Min(Radius, 60.f);
    HalfHeight = FMath::Max(HalfHeight, Radius);
    
    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
    
    float MeshBottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    float MeshOffsetZ = -HalfHeight - MeshBottomZ;
    
    BoatMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, MeshOffsetZ));
    
    UE_LOG(LogTemp, Log, TEXT("[MABoat] AutoFit: Capsule(R=%.1f, H=%.1f), MeshOffset=%.1f"),
        Radius, HalfHeight, MeshOffsetZ);
}

FString AMABoat::GetBoatMeshPath(const FString& Subtype)
{
    // 路径基于 /Game/Boatyard/Meshes/
    static TMap<FString, FString> BoatMeshMap = {
        {TEXT("metal_01"), TEXT("/Game/Boatyard/Meshes/SM_Boat_Metal_NN_01a.SM_Boat_Metal_NN_01a")},
        {TEXT("metal_02"), TEXT("/Game/Boatyard/Meshes/SM_Boat_Metal_NN_02a.SM_Boat_Metal_NN_02a")},
        {TEXT("metal_03"), TEXT("/Game/Boatyard/Meshes/SM_Boat_Metal_NN_03a.SM_Boat_Metal_NN_03a")},
        {TEXT("metal_06"), TEXT("/Game/Boatyard/Meshes/SM_Boat_Metal_NN_06a.SM_Boat_Metal_NN_06a")},
        {TEXT("boat_04"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_04a.SM_Boat_NN_04a")},
        {TEXT("boat_07"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_07a.SM_Boat_NN_07a")},
        {TEXT("boat_14"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_14a.SM_Boat_NN_14a")},
        {TEXT("boat_17"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_17a.SM_Boat_NN_17a")},
        {TEXT("boat_54"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_54a.SM_Boat_NN_54a")},
        {TEXT("pontoon"), TEXT("/Game/Boatyard/Meshes/SM_Pontoon_Boat_NN_01a.SM_Pontoon_Boat_NN_01a")},
        // 别名
        {TEXT("lifeboat"), TEXT("/Game/Boatyard/Meshes/SM_Boat_Metal_NN_01a.SM_Boat_Metal_NN_01a")},
        {TEXT("fishing"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_04a.SM_Boat_NN_04a")},
        {TEXT("speedboat"), TEXT("/Game/Boatyard/Meshes/SM_Boat_NN_17a.SM_Boat_NN_17a")},
    };

    FString SubtypeLower = Subtype.ToLower();
    if (const FString* Path = BoatMeshMap.Find(SubtypeLower))
    {
        return *Path;
    }
    return BoatMeshMap[TEXT("lifeboat")];
}

//=============================================================================
// 巡逻功能
//=============================================================================

void AMABoat::StartPatrol()
{
    if (PatrolWaypoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MABoat] %s: No patrol waypoints configured"), *ObjectLabel);
        return;
    }
    
    bIsPatrolling = true;
    CurrentWaypointIndex = 0;
    MoveToNextWaypoint();
    
    UE_LOG(LogTemp, Log, TEXT("[MABoat] %s: Started patrol with %d waypoints"), *ObjectLabel, PatrolWaypoints.Num());
}

void AMABoat::StopPatrol()
{
    bIsPatrolling = false;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PatrolWaitTimerHandle);
    }
    
    CancelNavigation();
    UE_LOG(LogTemp, Log, TEXT("[MABoat] %s: Stopped patrol"), *ObjectLabel);
}

void AMABoat::MoveToNextWaypoint()
{
    if (!bIsPatrolling || PatrolWaypoints.Num() == 0) return;
    
    FVector NextWaypoint = PatrolWaypoints[CurrentWaypointIndex];
    
    CurrentWaypointIndex++;
    if (CurrentWaypointIndex >= PatrolWaypoints.Num())
    {
        if (bPatrolLoop)
        {
            CurrentWaypointIndex = 0;
        }
        else
        {
            bIsPatrolling = false;
            UE_LOG(LogTemp, Log, TEXT("[MABoat] %s: Patrol completed"), *ObjectLabel);
            return;
        }
    }
    
    NavigateTo(NextWaypoint, 150.f);
}

//=============================================================================
// 附着火焰管理
//=============================================================================

void AMABoat::SetAttachedFire(AMAFire* Fire)
{
    AttachedFire = Fire;
}

AMAFire* AMABoat::GetAttachedFire() const
{
    return AttachedFire.Get();
}
