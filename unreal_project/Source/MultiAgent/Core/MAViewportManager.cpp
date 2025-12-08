// MAViewportManager.cpp

#include "MAViewportManager.h"
#include "MAAgentManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "Engine/Engine.h"

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
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Camera: %s (%d/%d)"), 
                *Cameras[CurrentCameraIndex]->SensorName, 
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
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
        FString::Printf(TEXT("Camera: %s"), *Camera->SensorName));
}

void UMAViewportManager::ReturnToSpectator(APlayerController* PC)
{
    if (!PC) return;

    if (OriginalPawn.IsValid())
    {
        PC->SetViewTargetWithBlend(OriginalPawn.Get(), 0.3f);
        CurrentCameraIndex = -1;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Spectator view"));
    }
}
