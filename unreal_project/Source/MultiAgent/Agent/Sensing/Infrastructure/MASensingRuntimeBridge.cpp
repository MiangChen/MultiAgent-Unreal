#include "Agent/Sensing/Infrastructure/MASensingRuntimeBridge.h"

#include "Agent/Sensing/Runtime/MACameraSensorComponent.h"
#include "Engine/World.h"
#include "SocketSubsystem.h"
#include "TimerManager.h"

bool FMASensingRuntimeBridge::StartStreamTimer(
    UMACameraSensorComponent& CameraSensor,
    FTimerHandle& TimerHandle,
    const float IntervalSeconds)
{
    UWorld* World = CameraSensor.GetWorld();
    if (!World)
    {
        return false;
    }

    FTimerManager& TimerManager = World->GetTimerManager();
    if (TimerHandle.IsValid())
    {
        TimerManager.ClearTimer(TimerHandle);
    }

    TimerManager.SetTimer(TimerHandle, &CameraSensor, &UMACameraSensorComponent::OnStreamTick, IntervalSeconds, true);
    return true;
}

void FMASensingRuntimeBridge::ClearStreamTimer(UMACameraSensorComponent& CameraSensor, FTimerHandle& TimerHandle)
{
    if (UWorld* World = CameraSensor.GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimerHandle);
    }
}

bool FMASensingRuntimeBridge::CreateListenSocket(UMACameraSensorComponent& CameraSensor, const int32 Port)
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return false;
    }

    CameraSensor.ListenSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("CameraStreamListener"), false);
    if (!CameraSensor.ListenSocket)
    {
        return false;
    }

    CameraSensor.ListenSocket->SetReuseAddr(true);
    CameraSensor.ListenSocket->SetNonBlocking(true);

    TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
    Addr->SetAnyAddress();
    Addr->SetPort(Port);

    if (!CameraSensor.ListenSocket->Bind(*Addr) || !CameraSensor.ListenSocket->Listen(8))
    {
        SocketSubsystem->DestroySocket(CameraSensor.ListenSocket);
        CameraSensor.ListenSocket = nullptr;
        return false;
    }

    return true;
}

void FMASensingRuntimeBridge::CleanupSockets(UMACameraSensorComponent& CameraSensor)
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        CameraSensor.ClientSockets.Empty();
        CameraSensor.ListenSocket = nullptr;
        return;
    }

    for (FSocket* Client : CameraSensor.ClientSockets)
    {
        if (Client)
        {
            SocketSubsystem->DestroySocket(Client);
        }
    }
    CameraSensor.ClientSockets.Empty();

    if (CameraSensor.ListenSocket)
    {
        SocketSubsystem->DestroySocket(CameraSensor.ListenSocket);
        CameraSensor.ListenSocket = nullptr;
    }
}
