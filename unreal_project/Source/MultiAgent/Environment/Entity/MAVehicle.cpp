// MAVehicle.cpp
// 车辆环境对象实现
// 参考 MAUGVCharacter 的配置

#include "MAVehicle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../../Agent/Navigation/Runtime/MANavigationService.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../Effect/MAFire.h"
#include "AIController.h"

AMAVehicle::AMAVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    // 启用碰撞阻挡
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 地面移动设置 - 禁用自动旋转，改用自定义平滑转向
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->MaxWalkSpeed = 400.f;
    MovementComp->bOrientRotationToMovement = false;
    MovementComp->RotationRate = FRotator(0.f, 0.f, 0.f);
    bUseControllerRotationYaw = false;

    // 创建车辆网格组件
    VehicleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMeshComponent->SetupAttachment(RootComponent);
    VehicleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // 车辆模型前向是 Y 轴，需要旋转 -90 度使其面向 X 轴
    VehicleMeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

    // 创建导航服务组件
    NavigationService = CreateDefaultSubobject<UMANavigationService>(TEXT("NavigationService"));

    // 设置 AI 控制
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
}

void AMAVehicle::BeginPlay()
{
    Super::BeginPlay();

    if (!GetController())
    {
        SpawnDefaultController();
    }

    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.AddDynamic(this, &AMAVehicle::OnNavigationCompleted);
    }
}

void AMAVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 平滑转向处理
    UpdateVehicleSteering(DeltaTime);
}

void AMAVehicle::UpdateVehicleSteering(float DeltaTime)
{
    // 只在移动时处理转向
    FVector Velocity = GetVelocity();
    float Speed = Velocity.Size2D();
    
    if (Speed < 10.f)
    {
        return;  // 静止时不转向
    }
    
    // 计算目标朝向（基于移动方向）
    FVector MoveDirection = Velocity.GetSafeNormal2D();
    if (MoveDirection.IsNearlyZero())
    {
        return;
    }
    
    FRotator CurrentRotation = GetActorRotation();
    FRotator TargetRotation = MoveDirection.Rotation();
    TargetRotation.Pitch = 0.f;
    TargetRotation.Roll = 0.f;
    
    // 计算角度差
    float YawDiff = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw);
    
    // 根据速度动态调整转向速率
    // 速度越快，转向越慢（模拟真实车辆转弯半径）
    // 基础转向速率 30°/s，最大 60°/s（低速时）
    float SpeedFactor = FMath::Clamp(Speed / GetCharacterMovement()->MaxWalkSpeed, 0.1f, 1.0f);
    float DynamicTurnRate = FMath::Lerp(MaxTurnRate, MinTurnRate, SpeedFactor);
    
    // 使用平滑插值
    float MaxYawChange = DynamicTurnRate * DeltaTime;
    float ActualYawChange = FMath::Clamp(YawDiff, -MaxYawChange, MaxYawChange);
    
    FRotator NewRotation = CurrentRotation;
    NewRotation.Yaw += ActualYawChange;
    
    SetActorRotation(NewRotation);
}

//=============================================================================
// IMAEnvironmentObject 接口实现
//=============================================================================

FString AMAVehicle::GetObjectLabel() const { return ObjectLabel; }
FString AMAVehicle::GetObjectType() const { return ObjectType; }
const TMap<FString, FString>& AMAVehicle::GetObjectFeatures() const { return Features; }

//=============================================================================
// 配置方法
//=============================================================================

void AMAVehicle::Configure(const FMAEnvironmentObjectConfig& Config)
{
    ObjectLabel = Config.Label;
    ObjectType = Config.Type;
    Features = Config.Features;
    SetActorRotation(Config.Rotation);

    // 根据 subtype 选择车辆模型
    FString Subtype = Features.FindRef(TEXT("subtype"));
    if (Subtype.IsEmpty()) Subtype = TEXT("sedan");
    SetVehicleMesh(Subtype);
    
    // Mesh 设置后，自动调整胶囊体尺寸和位置
    AutoFitCapsuleToMesh();

    // 设置车身颜色
    FString ColorStr = Features.FindRef(TEXT("color"));
    if (!ColorStr.IsEmpty())
    {
        SetBodyColor(ParseColorString(ColorStr));
    }

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
            World->GetTimerManager().SetTimer(StartTimerHandle, this, &AMAVehicle::StartPatrol, 1.0f, false);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MAVehicle] Configured: %s (subtype=%s, patrol=%s)"), 
        *ObjectLabel, *Subtype, Config.Patrol.bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void AMAVehicle::SetVehicleMesh(const FString& Subtype)
{
    FString MeshPath = GetVehicleMeshPath(Subtype);
    UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    
    if (!LoadedMesh)
    {
        MeshPath = GetVehicleMeshPath(TEXT("sedan"));
        LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    }

    if (LoadedMesh && VehicleMeshComponent)
    {
        VehicleMeshComponent->SetStaticMesh(LoadedMesh);
    }
}

void AMAVehicle::SetBodyColor(const FLinearColor& Color)
{
    if (!VehicleMeshComponent) return;

    int32 NumMaterials = VehicleMeshComponent->GetNumMaterials();
    for (int32 i = 0; i < NumMaterials; ++i)
    {
        if (UMaterialInterface* Material = VehicleMeshComponent->GetMaterial(i))
        {
            UMaterialInstanceDynamic* DynMaterial = VehicleMeshComponent->CreateAndSetMaterialInstanceDynamic(i);
            if (DynMaterial)
            {
                DynMaterial->SetVectorParameterValue(TEXT("BodyColor"), Color);
                DynMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynMaterial->SetVectorParameterValue(TEXT("Color"), Color);
            }
        }
    }
}

//=============================================================================
// 导航接口
//=============================================================================

bool AMAVehicle::NavigateTo(FVector Destination, float AcceptanceRadius)
{
    return NavigationService ? NavigationService->NavigateTo(Destination, AcceptanceRadius) : false;
}

void AMAVehicle::CancelNavigation()
{
    if (NavigationService) NavigationService->CancelNavigation();
}

bool AMAVehicle::IsNavigating() const
{
    return NavigationService && NavigationService->IsNavigating();
}

void AMAVehicle::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    // 如果正在巡逻，处理巡逻逻辑
    if (bIsPatrolling)
    {
        if (bSuccess)
        {
            // 到达巡逻点，等待后前往下一个
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(PatrolWaitTimerHandle, this, &AMAVehicle::MoveToNextWaypoint, PatrolWaitTime, false);
            }
        }
        else
        {
            // 导航失败，尝试下一个点
            MoveToNextWaypoint();
        }
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MAVehicle] %s: Navigation %s - %s"), 
        *ObjectLabel, bSuccess ? TEXT("succeeded") : TEXT("failed"), *Message);
}

//=============================================================================
// 私有方法
//=============================================================================

void AMAVehicle::AutoFitCapsuleToMesh()
{
    // 根据 StaticMesh 边界自动调整胶囊体尺寸和 Mesh 位置
    if (!VehicleMeshComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAVehicle] AutoFit: VehicleMeshComponent is NULL!"));
        return;
    }
    
    if (!VehicleMeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAVehicle] AutoFit: StaticMesh is NULL! (Mesh not loaded yet)"));
        return;
    }
    
    FBoxSphereBounds Bounds = VehicleMeshComponent->GetStaticMesh()->GetBounds();
    
    float Radius = (Bounds.BoxExtent.X + Bounds.BoxExtent.Y) * 0.5f;
    float HalfHeight = Bounds.BoxExtent.Z * 0.85f;
    Radius = FMath::Min(Radius, 80.f);
    HalfHeight = FMath::Max(HalfHeight, Radius);
    
    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
    
    // Mesh 底部对齐胶囊体底部
    float MeshBottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    float MeshOffsetZ = -HalfHeight - MeshBottomZ;
    
    VehicleMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, MeshOffsetZ));
    
    UE_LOG(LogTemp, Log, TEXT("[MAVehicle] AutoFit: Capsule(R=%.1f, H=%.1f), MeshOffset=%.1f"),
        Radius, HalfHeight, MeshOffsetZ);
}

FString AMAVehicle::GetVehicleMeshPath(const FString& Subtype)
{
    static TMap<FString, FString> VehicleMeshMap = {
        {TEXT("sedan"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Executive_Sedan.SM_Executive_Sedan")},
        {TEXT("suv"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Full-size_SUV.SM_Full-size_SUV")},
        {TEXT("pickup"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Pickup.SM_Pickup")},
        {TEXT("van"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Compact_Panel_Van.SM_Compact_Panel_Van")},
        {TEXT("minivan"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Minivan.SM_Minivan")},
        {TEXT("hatchback"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Family_Hatchback.SM_Family_Hatchback")},
        {TEXT("crossover"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Compact_Crossover.SM_Compact_Crossover")},
        {TEXT("luxury"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Luxury_Car.SM_Luxury_Car")},
        {TEXT("wagon"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Station_Wagon.SM_Station_Wagon")},
        {TEXT("sporty"), TEXT("/Game/City_Traffic_Pro/Cars/Meshes/SM_Sporty_Compact_Hatchback.SM_Sporty_Compact_Hatchback")},
    };

    FString SubtypeLower = Subtype.ToLower();
    if (const FString* Path = VehicleMeshMap.Find(SubtypeLower))
    {
        return *Path;
    }
    return VehicleMeshMap[TEXT("sedan")];
}

FLinearColor AMAVehicle::ParseColorString(const FString& ColorStr)
{
    if (ColorStr.Equals(TEXT("red"), ESearchCase::IgnoreCase)) return FLinearColor::Red;
    if (ColorStr.Equals(TEXT("green"), ESearchCase::IgnoreCase)) return FLinearColor::Green;
    if (ColorStr.Equals(TEXT("blue"), ESearchCase::IgnoreCase)) return FLinearColor::Blue;
    if (ColorStr.Equals(TEXT("yellow"), ESearchCase::IgnoreCase)) return FLinearColor::Yellow;
    if (ColorStr.Equals(TEXT("white"), ESearchCase::IgnoreCase)) return FLinearColor::White;
    if (ColorStr.Equals(TEXT("black"), ESearchCase::IgnoreCase)) return FLinearColor::Black;
    if (ColorStr.Equals(TEXT("gray"), ESearchCase::IgnoreCase) || ColorStr.Equals(TEXT("grey"), ESearchCase::IgnoreCase))
        return FLinearColor::Gray;
    if (ColorStr.Equals(TEXT("silver"), ESearchCase::IgnoreCase)) return FLinearColor(0.75f, 0.75f, 0.75f);
    if (ColorStr.Equals(TEXT("orange"), ESearchCase::IgnoreCase)) return FLinearColor(1.f, 0.5f, 0.f);

    if (ColorStr.StartsWith(TEXT("#")) && ColorStr.Len() == 7)
    {
        return FLinearColor(FColor::FromHex(ColorStr));
    }
    return FLinearColor::White;
}

//=============================================================================
// 巡逻功能
//=============================================================================

void AMAVehicle::StartPatrol()
{
    if (PatrolWaypoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAVehicle] %s: No patrol waypoints configured"), *ObjectLabel);
        return;
    }
    
    bIsPatrolling = true;
    CurrentWaypointIndex = 0;
    MoveToNextWaypoint();
    
    UE_LOG(LogTemp, Log, TEXT("[MAVehicle] %s: Started patrol with %d waypoints"), *ObjectLabel, PatrolWaypoints.Num());
}

void AMAVehicle::StopPatrol()
{
    bIsPatrolling = false;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PatrolWaitTimerHandle);
    }
    
    CancelNavigation();
    UE_LOG(LogTemp, Log, TEXT("[MAVehicle] %s: Stopped patrol"), *ObjectLabel);
}

void AMAVehicle::MoveToNextWaypoint()
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
            UE_LOG(LogTemp, Log, TEXT("[MAVehicle] %s: Patrol completed"), *ObjectLabel);
            return;
        }
    }
    
    NavigateTo(TargetLocation, 150.f);
}

//=============================================================================
// 附着火焰管理
//=============================================================================

void AMAVehicle::SetAttachedFire(AMAFire* Fire)
{
    AttachedFire = Fire;
}

AMAFire* AMAVehicle::GetAttachedFire() const
{
    return AttachedFire.Get();
}
