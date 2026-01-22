// MAExternalCameraManager.cpp

#include "MAExternalCameraManager.h"
#include "MAAgentManager.h"
#include "../Agent/Character/MACharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
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

    // 1. 固定摄像头: 位置 (-2667, 9311, 100)，朝向 X 正方向 Y 负方向 45 度
    // Yaw = -45 度 (X 正方向是 0 度，Y 负方向是 -90 度，45 度之间就是 -45 度)
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("Fixed_Overview");
        Config.CameraType = EMAExternalCameraType::Fixed;
        Config.Location = FVector(-2667.f, 9311.f, 100.f);
        Config.Rotation = FRotator(0.f, -45.f, 0.f);  // Pitch, Yaw, Roll
        Config.FOV = 60.f;  // 减小视场角，焦距变长，画面放大
        AddExternalCamera(Config);
    }

    // 2. UAV_01 身后 500 距离摄像头
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UAV_01_Behind");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UAV_01");
        Config.FollowViewType = EMAFollowViewType::Behind;
        Config.FollowDistance = 500.f;
        Config.FOV = 90.f;
        AddExternalCamera(Config);
    }

    // 3. UAV_02 头顶上 500 距离摄像头
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UAV_02_TopDown");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UAV_02");
        Config.FollowViewType = EMAFollowViewType::TopDown;
        Config.FollowDistance = 500.f;
        Config.FOV = 90.f;
        AddExternalCamera(Config);
    }

    // 4. UGV 身后 500 距离摄像头
    {
        FMAExternalCameraConfig Config;
        Config.CameraName = TEXT("UGV_Behind");
        Config.CameraType = EMAExternalCameraType::Follow;
        Config.TargetAgentLabel = TEXT("UGV_01");
        Config.FollowViewType = EMAFollowViewType::Behind;
        Config.FollowDistance = 500.f;
        Config.FOV = 90.f;
        AddExternalCamera(Config);
    }

    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Initialized %d default cameras"), ExternalCameras.Num());
}

ACameraActor* UMAExternalCameraManager::AddExternalCamera(const FMAExternalCameraConfig& Config)
{
    ACameraActor* CameraActor = CreateCameraActor(Config);
    if (!CameraActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExternalCameraManager] Failed to create camera: %s"), *Config.CameraName);
        return nullptr;
    }

    FMAExternalCameraInstance Instance;
    Instance.Config = Config;
    Instance.CameraActor = CameraActor;

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

    UE_LOG(LogTemp, Log, TEXT("[ExternalCameraManager] Added camera: %s (Type: %s)"),
        *Config.CameraName,
        Config.CameraType == EMAExternalCameraType::Fixed ? TEXT("Fixed") : TEXT("Follow"));

    return CameraActor;
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
        if (!Target || !Instance.CameraActor)
        {
            continue;
        }

        FVector TargetLocation = Target->GetActorLocation();
        FRotator TargetRotation = Target->GetActorRotation();
        FVector CameraLocation;
        FRotator CameraRotation;

        switch (Instance.Config.FollowViewType)
        {
        case EMAFollowViewType::Behind:
            {
                // 身后视角：在 Agent 后方，稍微抬高
                FVector BackwardDir = -Target->GetActorForwardVector();
                CameraLocation = TargetLocation + BackwardDir * Instance.Config.FollowDistance + FVector(0.f, 0.f, 100.f);
                CameraRotation = (TargetLocation - CameraLocation).Rotation();
            }
            break;

        case EMAFollowViewType::TopDown:
            {
                // 头顶俯视：在 Agent 正上方
                CameraLocation = TargetLocation + FVector(0.f, 0.f, Instance.Config.FollowDistance);
                CameraRotation = FRotator(-90.f, 0.f, 0.f);  // 向下看
            }
            break;
        }

        Instance.CameraActor->SetActorLocation(CameraLocation);
        Instance.CameraActor->SetActorRotation(CameraRotation);
    }
}

AMACharacter* UMAExternalCameraManager::FindAgentByLabel(const FString& Label) const
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return nullptr;

    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent && Agent->AgentName == Label)
        {
            return Agent;
        }
    }

    return nullptr;
}
