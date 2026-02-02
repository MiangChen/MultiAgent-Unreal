// MABoat.cpp
// 船只环境对象实现
// 参考 MAUGVCharacter 的配置，使用飞行模式保持水面高度

#include "MABoat.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../../Agent/Component/MANavigationService.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../Effect/MAFire.h"
#include "AIController.h"

AMABoat::AMABoat()
{
    PrimaryActorTick.bCanEverTick = false;

    // 参考 UGV: 胶囊体设置
    GetCapsuleComponent()->SetCapsuleSize(60.f, 40.f);  // 半径60，半高40
    // 启用碰撞阻挡
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 参考 UGV: 移动设置，但使用飞行模式保持水面高度
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->MaxWalkSpeed = 300.f;
    MovementComp->MaxFlySpeed = 300.f;
    MovementComp->bOrientRotationToMovement = true;
    MovementComp->RotationRate = FRotator(0.f, 90.f, 0.f);
    // 使用飞行模式，禁用重力
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->GravityScale = 0.f;

    // 隐藏默认 SkeletalMesh
    GetMesh()->SetVisibility(false);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 创建船只网格组件 (参考 UGV 的 StaticMeshComponent 设置)
    BoatMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatMesh"));
    BoatMeshComponent->SetupAttachment(RootComponent);
    // 参考 UGV: 禁用 StaticMesh 碰撞
    BoatMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // 调整模型位置和旋转
    BoatMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
    // 船只模型前向是 Y 轴，需要旋转 -90 度使其面向 X 轴（Actor 前向）
    BoatMeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

    // 创建导航服务组件
    NavigationService = CreateDefaultSubobject<UMANavigationService>(TEXT("NavigationService"));

    // 设置 AI 控制
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
}

void AMABoat::BeginPlay()
{
    Super::BeginPlay();

    // 确保 AIController 已创建
    if (!GetController())
    {
        SpawnDefaultController();
    }

    // 记录初始水面高度
    WaterLevel = GetActorLocation().Z;

    // 绑定导航完成回调
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.AddDynamic(this, &AMABoat::OnNavigationCompleted);
        // 使用飞行导航模式保持水面高度
        NavigationService->bIsFlying = true;
        NavigationService->MinFlightAltitude = WaterLevel;
    }
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

    // 配置巡逻
    if (Config.Patrol.bEnabled && Config.Patrol.Waypoints.Num() > 0)
    {
        PatrolWaypoints = Config.Patrol.Waypoints;
        bPatrolLoop = Config.Patrol.bLoop;
        PatrolWaitTime = Config.Patrol.WaitTime;
        
        // 延迟启动巡逻
        if (UWorld* World = GetWorld())
        {
            FTimerHandle StartTimerHandle;
            World->GetTimerManager().SetTimer(StartTimerHandle, this, &AMABoat::StartPatrol, 1.0f, false);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MABoat] Configured: %s (subtype=%s, patrol=%s)"), 
        *ObjectLabel, *Subtype, Config.Patrol.bEnabled ? TEXT("enabled") : TEXT("disabled"));
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
// 导航接口
//=============================================================================

bool AMABoat::NavigateTo(FVector Destination, float AcceptanceRadius)
{
    // 保持水面高度
    Destination.Z = WaterLevel;
    return NavigationService ? NavigationService->NavigateTo(Destination, AcceptanceRadius) : false;
}

void AMABoat::CancelNavigation()
{
    if (NavigationService) NavigationService->CancelNavigation();
}

bool AMABoat::IsNavigating() const
{
    return NavigationService && NavigationService->IsNavigating();
}

void AMABoat::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    // 如果正在巡逻，处理巡逻逻辑
    if (bIsPatrolling)
    {
        if (bSuccess)
        {
            // 到达巡逻点，等待后前往下一个
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(PatrolWaitTimerHandle, this, &AMABoat::MoveToNextWaypoint, PatrolWaitTime, false);
            }
        }
        else
        {
            // 导航失败，尝试下一个点
            MoveToNextWaypoint();
        }
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MABoat] %s: Navigation %s - %s"), 
        *ObjectLabel, bSuccess ? TEXT("succeeded") : TEXT("failed"), *Message);
}

//=============================================================================
// 私有方法
//=============================================================================

FString AMABoat::GetBoatMeshPath(const FString& Subtype)
{
    static TMap<FString, FString> BoatMeshMap = {
        {TEXT("lifeboat"), TEXT("/Game/Boats/Meshes/SM_Lifeboat.SM_Lifeboat")},
        {TEXT("canoe"), TEXT("/Game/Boats/Meshes/SM_Canoe.SM_Canoe")},
        {TEXT("metal"), TEXT("/Game/Boats/Meshes/SM_MetalBoat.SM_MetalBoat")},
        {TEXT("pontoon"), TEXT("/Game/Boats/Meshes/SM_Pontoon.SM_Pontoon")},
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
    
    FVector TargetLocation = PatrolWaypoints[CurrentWaypointIndex];
    
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
    
    NavigateTo(TargetLocation, 150.f);
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
