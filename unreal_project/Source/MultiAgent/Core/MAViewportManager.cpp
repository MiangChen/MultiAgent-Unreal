// MAViewportManager.cpp

#include "MAViewportManager.h"
#include "MAAgentManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MADroneCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../UI/MAHUD.h"
#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

void UMAViewportManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[ViewportManager] Initialized"));
}

void UMAViewportManager::CollectCameras(TArray<UMACameraSensorComponent*>& OutCameras, TArray<AMACharacter*>& OutOwners) const
{
    OutCameras.Empty();
    OutOwners.Empty();

    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            if (UMACameraSensorComponent* Camera = Agent->GetCameraSensor())
            {
                OutCameras.Add(Camera);
                OutOwners.Add(Agent);
            }
        }
    }
}

TArray<UMACameraSensorComponent*> UMAViewportManager::GetAllCameras() const
{
    TArray<UMACameraSensorComponent*> Cameras;
    TArray<AMACharacter*> Owners;
    CollectCameras(Cameras, Owners);
    return Cameras;
}

UMACameraSensorComponent* UMAViewportManager::GetCurrentCamera() const
{
    TArray<UMACameraSensorComponent*> Cameras;
    TArray<AMACharacter*> Owners;
    CollectCameras(Cameras, Owners);

    if (Cameras.Num() == 0) return nullptr;

    int32 Index = (CurrentCameraIndex >= 0 && CurrentCameraIndex < Cameras.Num()) ? CurrentCameraIndex : 0;
    return Cameras[Index];
}

void UMAViewportManager::SwitchToNextCamera(APlayerController* PC)
{
    if (!PC) return;

    TArray<UMACameraSensorComponent*> Cameras;
    TArray<AMACharacter*> Owners;
    CollectCameras(Cameras, Owners);

    if (Cameras.Num() == 0) return;

    // 保存原始 Pawn
    if (!OriginalPawn.IsValid() && PC->GetPawn())
    {
        OriginalPawn = PC->GetPawn();
    }

    // 切换到下一个
    CurrentCameraIndex = (CurrentCameraIndex + 1) % Cameras.Num();

    AMACharacter* TargetAgent = Owners[CurrentCameraIndex];
    if (TargetAgent)
    {
        PC->SetViewTargetWithBlend(TargetAgent, 0.3f);
        
        // 进入 Agent View Mode，启用直接控制
        EnterAgentViewMode(TargetAgent);
        
        // 显示 Direct Control 消息
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Direct Control: %s (%d/%d)"), 
                *TargetAgent->AgentName, 
                CurrentCameraIndex + 1, 
                Cameras.Num()));
    }
}

void UMAViewportManager::SwitchToAgentCamera(APlayerController* PC, AMACharacter* Agent)
{
    if (!PC || !Agent) return;

    UMACameraSensorComponent* Camera = Agent->GetCameraSensor();
    if (!Camera) return;

    // 保存原始 Pawn
    if (!OriginalPawn.IsValid() && PC->GetPawn())
    {
        OriginalPawn = PC->GetPawn();
    }

    // 更新索引
    TArray<UMACameraSensorComponent*> Cameras;
    TArray<AMACharacter*> Owners;
    CollectCameras(Cameras, Owners);
    CurrentCameraIndex = Owners.Find(Agent);

    PC->SetViewTargetWithBlend(Agent, 0.3f);
    
    // 进入 Agent View Mode，启用直接控制
    EnterAgentViewMode(Agent);
    
    // 显示 Direct Control 消息
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
        FString::Printf(TEXT("Direct Control: %s"), *Agent->AgentName));
}

void UMAViewportManager::ReturnToSpectator(APlayerController* PC)
{
    if (!PC) return;

    // 退出 Agent View Mode
    ExitAgentViewMode();

    if (OriginalPawn.IsValid())
    {
        // 将 Spectator Pawn 移动到默认位置俯瞰场景
        FVector SpectatorLocation = FVector(790.f, 1810.f, 502.f);
        FRotator SpectatorRotation = FRotator(-45.f, 0.f, 0.f);  // 向下看 45 度
        
        OriginalPawn->SetActorLocation(SpectatorLocation);
        OriginalPawn->SetActorRotation(SpectatorRotation);
        
        PC->SetViewTargetWithBlend(OriginalPawn.Get(), 0.3f);
        CurrentCameraIndex = -1;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Spectator view"));
        
        UE_LOG(LogTemp, Log, TEXT("[ViewportManager] Returned to Spectator at (%.0f, %.0f, %.0f)"), 
            SpectatorLocation.X, SpectatorLocation.Y, SpectatorLocation.Z);
    }
}


// ========== Agent View Mode ==========

void UMAViewportManager::EnterAgentViewMode(AMACharacter* Agent)
{
    if (!Agent)
    {
        return;
    }
    
    // 如果已经在控制另一个 Agent，先退出
    if (bIsInAgentViewMode && ControlledAgent.IsValid() && ControlledAgent.Get() != Agent)
    {
        ControlledAgent->SetDirectControl(false);
    }
    
    bIsInAgentViewMode = true;
    ControlledAgent = Agent;
    
    // 启用 Agent 的直接控制模式
    Agent->SetDirectControl(true);
    
    // 重置相机俯仰角
    CurrentCameraPitch = DefaultCameraPitch;
    
    // 显示 Direct Control 指示器 (Requirements: 3.3)
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD()))
            {
                HUD->ShowDirectControlIndicator(Agent);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ViewportManager] Entered Agent View Mode: %s"), *Agent->AgentName);
}

void UMAViewportManager::ExitAgentViewMode()
{
    if (!bIsInAgentViewMode)
    {
        return;
    }
    
    // 禁用 Agent 的直接控制模式
    if (ControlledAgent.IsValid())
    {
        ControlledAgent->SetDirectControl(false);
        UE_LOG(LogTemp, Log, TEXT("[ViewportManager] Exited Agent View Mode: %s"), *ControlledAgent->AgentName);
    }
    
    bIsInAgentViewMode = false;
    ControlledAgent.Reset();
    
    // 重置相机俯仰角
    CurrentCameraPitch = DefaultCameraPitch;
    
    // 隐藏 Direct Control 指示器 (Requirements: 3.3)
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD()))
            {
                HUD->HideDirectControlIndicator();
            }
        }
    }
}

void UMAViewportManager::ApplyMovementInput(FVector2D Input)
{
    if (!bIsInAgentViewMode || !ControlledAgent.IsValid())
    {
        return;
    }
    
    AMACharacter* Agent = ControlledAgent.Get();
    
    // 如果输入为零，不需要处理
    if (Input.IsNearlyZero())
    {
        return;
    }
    
    // 获取 Agent 的朝向
    FRotator AgentRotation = Agent->GetActorRotation();
    
    // 计算基于 Agent 朝向的移动方向
    // Input.Y = 前后 (W/S), Input.X = 左右 (A/D)
    FVector ForwardDirection = FRotationMatrix(AgentRotation).GetUnitAxis(EAxis::X);
    FVector RightDirection = FRotationMatrix(AgentRotation).GetUnitAxis(EAxis::Y);
    
    // 组合移动方向
    FVector WorldDirection = ForwardDirection * Input.Y + RightDirection * Input.X;
    WorldDirection.Z = 0.f;  // 保持水平移动
    
    if (!WorldDirection.IsNearlyZero())
    {
        WorldDirection.Normalize();
    }
    
    // 应用移动
    Agent->ApplyDirectMovement(WorldDirection);
}

void UMAViewportManager::ApplyVerticalInput(float Input)
{
    if (!bIsInAgentViewMode || !ControlledAgent.IsValid())
    {
        return;
    }
    
    // 检查是否为 Drone
    AMADroneCharacter* Drone = Cast<AMADroneCharacter>(ControlledAgent.Get());
    if (Drone)
    {
        // Drone 支持垂直移动
        Drone->ApplyVerticalMovement(Input);
    }
    // 非 Drone Agent 忽略垂直输入
}

void UMAViewportManager::ApplyLookInput(FVector2D Input)
{
    if (!bIsInAgentViewMode || !ControlledAgent.IsValid())
    {
        return;
    }
    
    AMACharacter* Agent = ControlledAgent.Get();
    
    // 水平输入旋转 Agent Yaw（应用灵敏度）
    if (!FMath::IsNearlyZero(Input.X))
    {
        FRotator CurrentRotation = Agent->GetActorRotation();
        CurrentRotation.Yaw += Input.X * LookSensitivityYaw;
        Agent->SetActorRotation(CurrentRotation);
    }
    
    // 垂直输入调整相机 Pitch（限制范围，应用灵敏度）
    if (!FMath::IsNearlyZero(Input.Y))
    {
        CurrentCameraPitch = FMath::Clamp(
            CurrentCameraPitch + Input.Y * LookSensitivityPitch,
            MinCameraPitch,
            MaxCameraPitch
        );
        
        // 更新相机组件的俯仰角
        UMACameraSensorComponent* CameraSensor = Agent->GetCameraSensor();
        if (CameraSensor)
        {
            // 获取相机组件并调整俯仰角
            UCameraComponent* CameraComp = CameraSensor->GetCameraComponent();
            if (CameraComp)
            {
                FRotator CameraRotation = CameraComp->GetRelativeRotation();
                CameraRotation.Pitch = CurrentCameraPitch;
                CameraComp->SetRelativeRotation(CameraRotation);
            }
        }
    }
}
