// MAExternalCameraManager.cpp

#include "MAExternalCameraManager.h"
#include "Core/Manager/MAAgentManager.h"
#include "Agent/Character/MACharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"

void UMAExternalCameraManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Initialized"));
}

void UMAExternalCameraManager::Deinitialize()
{
    bInitialized = false;
    
    // 清理所有摄像头
    for (FMAExternalCameraInstance& Instance : ExternalCameras)
    {
        if (Instance.CameraActor)
        {
            Instance.CameraActor->Destroy();
        }
        if (Instance.SpringArmCameraActor)
        {
            Instance.SpringArmCameraActor->Destroy();
        }
    }
    ExternalCameras.Empty();

    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Deinitialized"));
}

void UMAExternalCameraManager::Tick(float DeltaTime)
{
    // 更新跟随摄像头位置
    UpdateFollowCameras();
}

void UMAExternalCameraManager::InitializeDefaultCameras()
{
    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Initializing default cameras..."));

    // 1. UAV-1 身后 500 距离摄像头
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UAV-1_Behind");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UAV-1");
        Config.FollowViewType = EMAFollowViewType::Behind;
        Config.FollowDistance = 500.f;
        Config.FOV = 90.f;
        AddExternalCamera(Config);
    }

    // 2. UAV-2 头顶上 500 距离摄像头
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UAV-2_TopDown");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UAV-2");
        Config.FollowViewType = EMAFollowViewType::TopDown;
        Config.FollowDistance = 500.f;
        Config.FOV = 90.f;
        AddExternalCamera(Config);
    }

    // 3. UGV 身前摄像头 - 距离更远、位置更高、俯视，带弹簧臂延迟
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UGV_Front");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UGV-1");
        Config.FollowViewType = EMAFollowViewType::BehindElevated;
        Config.FollowDistance = 1200.f;
        Config.FollowHeight = 500.f;
        Config.FOV = 90.f;
        Config.bUseSpringArm = true;
        Config.CameraLagSpeed = 4.f;
        Config.CameraRotationLagSpeed = 4.f;
        AddExternalCamera(Config);
    }

    // 4. UGV 身后摄像头 - 在 Agent 背后
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UGV_Behind");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UGV-1");
        Config.FollowViewType = EMAFollowViewType::BehindElevatedReverse;
        Config.FollowDistance = 1000.f;
        Config.FollowHeight = 500.f;
        Config.FOV = 90.f;
        Config.bUseSpringArm = true;
        Config.CameraLagSpeed = 4.f;
        Config.CameraRotationLagSpeed = 4.f;
        AddExternalCamera(Config);
    }

    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Initialized %d default cameras"), ExternalCameras.Num());
}

ACameraActor* UMAExternalCameraManager::AddExternalCamera(const FMAExternalCameraConfig& Config)
{
    FMAExternalCameraInstance Instance;
    Instance.Config = Config;

    // 如果是跟随摄像头且启用弹簧臂，创建带弹簧臂的相机
    if (Config.CameraType == EMAExternalCameraType::Follow && Config.bUseSpringArm)
    {
        Instance.SpringArmCameraActor = CreateSpringArmCameraActor(Config, Instance.SpringArmComponent, Instance.CameraComponent);
        if (!Instance.SpringArmCameraActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ExternalCameraManager] Failed to create spring arm camera: %s"), *Config.CameraName);
            return nullptr;
        }
    }
    else
    {
        // 创建普通相机
        Instance.CameraActor = CreateCameraActor(Config);
        if (!Instance.CameraActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ExternalCameraManager] Failed to create camera: %s"), *Config.CameraName);
            return nullptr;
        }
    }

    // 如果是跟随摄像头，查找目标 Agent
    if (Config.CameraType == EMAExternalCameraType::Follow)
    {
        Instance.TargetAgent = FindAgentByLabel(Config.TargetAgentLabel);
        if (!Instance.TargetAgent.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[ExternalCameraManager] Target agent '%s' not found for camera '%s'"),
                *Config.TargetAgentLabel, *Config.CameraName);
        }
    }

    ExternalCameras.Add(Instance);

    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Added camera: %s (Type: %s, SpringArm: %s)"),
        *Config.CameraName,
        Config.CameraType == EMAExternalCameraType::Fixed ? TEXT("Fixed") : TEXT("Follow"),
        Config.bUseSpringArm ? TEXT("Yes") : TEXT("No"));

    return Instance.CameraActor;
}

ACameraActor* UMAExternalCameraManager::CreateCameraActor(const FMAExternalCameraConfig& Config)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACameraActor* CameraActor = World->SpawnActor<ACameraActor>(
        ACameraActor::StaticClass(),
        Config.Location,
        Config.Rotation,
        SpawnParams
    );

    if (CameraActor)
    {
        // 设置摄像头参数
        if (UCameraComponent* CameraComp = CameraActor->GetCameraComponent())
        {
            CameraComp->SetFieldOfView(Config.FOV);
        }

        // 设置 Actor 标签
        CameraActor->Tags.Add(FName(*Config.CameraName));

#if WITH_EDITOR
        CameraActor->SetActorLabel(Config.CameraName);
#endif
    }

    return CameraActor;
}

AActor* UMAExternalCameraManager::CreateSpringArmCameraActor(const FMAExternalCameraConfig& Config, USpringArmComponent*& OutSpringArm, UCameraComponent*& OutCamera)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 创建一个空 Actor 作为弹簧臂相机的载体
    AActor* CameraActor = World->SpawnActor<AActor>(AActor::StaticClass(), Config.Location, Config.Rotation, SpawnParams);
    if (!CameraActor)
    {
        return nullptr;
    }

    // 创建场景根组件
    USceneComponent* RootComp = NewObject<USceneComponent>(CameraActor, TEXT("RootComponent"));
    CameraActor->SetRootComponent(RootComp);
    RootComp->RegisterComponent();

    // 创建弹簧臂组件
    USpringArmComponent* SpringArm = NewObject<USpringArmComponent>(CameraActor, TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComp);
    SpringArm->TargetArmLength = Config.FollowDistance;
    SpringArm->bEnableCameraLag = true;
    SpringArm->bEnableCameraRotationLag = true;
    SpringArm->CameraLagSpeed = Config.CameraLagSpeed;
    SpringArm->CameraRotationLagSpeed = Config.CameraRotationLagSpeed;
    SpringArm->bDoCollisionTest = false;  // 禁用碰撞检测，避免相机穿墙
    SpringArm->RegisterComponent();

    // 创建相机组件
    UCameraComponent* Camera = NewObject<UCameraComponent>(CameraActor, TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->SetFieldOfView(Config.FOV);
    Camera->RegisterComponent();

    // 设置 Actor 标签
    CameraActor->Tags.Add(FName(*Config.CameraName));

#if WITH_EDITOR
    CameraActor->SetActorLabel(Config.CameraName);
#endif

    OutSpringArm = SpringArm;
    OutCamera = Camera;

    return CameraActor;
}

TArray<ACameraActor*> UMAExternalCameraManager::GetAllExternalCameras() const
{
    TArray<ACameraActor*> Cameras;
    for (const FMAExternalCameraInstance& Instance : ExternalCameras)
    {
        if (Instance.CameraActor)
        {
            Cameras.Add(Instance.CameraActor);
        }
    }
    return Cameras;
}

ACameraActor* UMAExternalCameraManager::GetExternalCameraByIndex(int32 Index) const
{
    if (Index >= 0 && Index < ExternalCameras.Num())
    {
        // 优先返回普通相机，如果没有则返回 nullptr（弹簧臂相机需要通过 SpringArmCameraActor 访问）
        return ExternalCameras[Index].CameraActor;
    }
    return nullptr;
}

FString UMAExternalCameraManager::GetCameraName(int32 Index) const
{
    if (Index >= 0 && Index < ExternalCameras.Num())
    {
        return ExternalCameras[Index].Config.CameraName;
    }
    return TEXT("");
}

AActor* UMAExternalCameraManager::GetViewTargetByIndex(int32 Index) const
{
    if (Index >= 0 && Index < ExternalCameras.Num())
    {
        const FMAExternalCameraInstance& Instance = ExternalCameras[Index];
        // 优先返回弹簧臂相机 Actor
        if (Instance.SpringArmCameraActor)
        {
            return Instance.SpringArmCameraActor;
        }
        return Instance.CameraActor;
    }
    return nullptr;
}

void UMAExternalCameraManager::UpdateFollowCameras()
{
    for (FMAExternalCameraInstance& Instance : ExternalCameras)
    {
        if (Instance.Config.CameraType != EMAExternalCameraType::Follow)
        {
            continue;
        }

        // 如果目标 Agent 无效，尝试重新查找
        if (!Instance.TargetAgent.IsValid())
        {
            Instance.TargetAgent = FindAgentByLabel(Instance.Config.TargetAgentLabel);
            if (!Instance.TargetAgent.IsValid())
            {
                continue;
            }
        }

        AMACharacter* Target = Instance.TargetAgent.Get();
        if (!Target)
        {
            continue;
        }

        FVector TargetLocation = Target->GetActorLocation();
        FRotator TargetRotation = Target->GetActorRotation();

        // 弹簧臂相机：更新位置和旋转，使用平滑插值实现延迟效果
        if (Instance.Config.bUseSpringArm && Instance.SpringArmCameraActor && Instance.SpringArmComponent)
        {
            // 将弹簧臂 Actor 放在目标位置
            Instance.SpringArmCameraActor->SetActorLocation(TargetLocation);

            // 根据视角类型计算目标旋转和长度
            FRotator DesiredRotation;
            float ArmLength = Instance.Config.FollowDistance;

            switch (Instance.Config.FollowViewType)
            {
            case EMAFollowViewType::Behind:
                {
                    // 身后视角：弹簧臂朝向 Agent 后方，稍微抬高
                    DesiredRotation = FRotator(-10.f, TargetRotation.Yaw + 180.f, 0.f);
                    Instance.SpringArmComponent->SocketOffset = FVector(0.f, 0.f, 100.f);
                }
                break;

            case EMAFollowViewType::BehindElevated:
                {
                    // 身前抬高俯视：计算俯视角度
                    float PitchAngle = FMath::RadiansToDegrees(FMath::Atan2(Instance.Config.FollowHeight, Instance.Config.FollowDistance));
                    DesiredRotation = FRotator(-PitchAngle, TargetRotation.Yaw + 180.f, 0.f);
                    // 计算实际弹簧臂长度（斜边）
                    ArmLength = FMath::Sqrt(FMath::Square(Instance.Config.FollowDistance) + FMath::Square(Instance.Config.FollowHeight));
                    Instance.SpringArmComponent->SocketOffset = FVector::ZeroVector;
                }
                break;

            case EMAFollowViewType::BehindElevatedReverse:
                {
                    // 身后抬高俯视：与 BehindElevated 方向相反
                    float PitchAngle = FMath::RadiansToDegrees(FMath::Atan2(Instance.Config.FollowHeight, Instance.Config.FollowDistance));
                    DesiredRotation = FRotator(-PitchAngle, TargetRotation.Yaw, 0.f);
                    ArmLength = FMath::Sqrt(FMath::Square(Instance.Config.FollowDistance) + FMath::Square(Instance.Config.FollowHeight));
                    Instance.SpringArmComponent->SocketOffset = FVector::ZeroVector;
                }
                break;

            case EMAFollowViewType::TopDown:
                {
                    // 头顶俯视
                    DesiredRotation = FRotator(-90.f, 0.f, 0.f);
                }
                break;
            }

            Instance.SpringArmComponent->TargetArmLength = ArmLength;
            
            // 使用平滑插值实现旋转延迟
            // 限制 DeltaTime 范围，避免帧率波动导致的抖动
            float DeltaTime = FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0.001f, 0.1f);
            float InterpSpeed = Instance.Config.CameraRotationLagSpeed;
            
            // 使用四元数插值获得更平滑的旋转
            FQuat CurrentQuat = Instance.SpringArmCameraActor->GetActorRotation().Quaternion();
            FQuat DesiredQuat = DesiredRotation.Quaternion();
            FQuat NewQuat = FMath::QInterpTo(CurrentQuat, DesiredQuat, DeltaTime, InterpSpeed);
            Instance.SpringArmCameraActor->SetActorRotation(NewQuat.Rotator());
        }
        // 普通相机：直接设置位置和旋转
        else if (Instance.CameraActor)
        {
            FVector CameraLocation;
            FRotator CameraRotation;

            switch (Instance.Config.FollowViewType)
            {
            case EMAFollowViewType::Behind:
                {
                    FVector BackwardDir = -Target->GetActorForwardVector();
                    CameraLocation = TargetLocation + BackwardDir * Instance.Config.FollowDistance + FVector(0.f, 0.f, 100.f);
                    CameraRotation = (TargetLocation - CameraLocation).Rotation();
                }
                break;

            case EMAFollowViewType::BehindElevated:
                {
                    FVector BackwardDir = -Target->GetActorForwardVector();
                    CameraLocation = TargetLocation + BackwardDir * Instance.Config.FollowDistance + FVector(0.f, 0.f, Instance.Config.FollowHeight);
                    CameraRotation = (TargetLocation - CameraLocation).Rotation();
                }
                break;

            case EMAFollowViewType::BehindElevatedReverse:
                {
                    FVector ForwardDir = Target->GetActorForwardVector();
                    CameraLocation = TargetLocation + ForwardDir * Instance.Config.FollowDistance + FVector(0.f, 0.f, Instance.Config.FollowHeight);
                    CameraRotation = (TargetLocation - CameraLocation).Rotation();
                }
                break;

            case EMAFollowViewType::TopDown:
                {
                    CameraLocation = TargetLocation + FVector(0.f, 0.f, Instance.Config.FollowDistance);
                    CameraRotation = FRotator(-90.f, 0.f, 0.f);
                }
                break;
            }

            Instance.CameraActor->SetActorLocation(CameraLocation);
            Instance.CameraActor->SetActorRotation(CameraRotation);
        }
    }
}

AMACharacter* UMAExternalCameraManager::FindAgentByLabel(const FString& Label) const
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return nullptr;

    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent && Agent->AgentLabel == Label)
        {
            return Agent;
        }
    }

    return nullptr;
}
