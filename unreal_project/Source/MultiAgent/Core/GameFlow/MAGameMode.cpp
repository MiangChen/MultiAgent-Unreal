// MAGameMode.cpp

#include "MAGameMode.h"
#include "MAGameInstance.h"
#include "../Config/MAConfigManager.h"
#include "../Input/MAPlayerController.h"
#include "../UI/MAHUD.h"
#include "../UI/MAMiniMapManager.h"
#include "../Manager/MAAgentManager.h"
#include "GameFramework/SpectatorPawn.h"

AMAGameMode::AMAGameMode()
{
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    PlayerControllerClass = AMAPlayerController::StaticClass();
    HUDClass = AMAHUD::StaticClass();
}

void AMAGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("[GameMode] BeginPlay"));
    
    // 延迟一帧执行初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        SetupSpectatorStart();
        LoadAndSpawnAgents();
        SpawnMiniMapManager();
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Initialization complete"));
    });
}

UMAAgentManager* AMAGameMode::GetAgentManager() const
{
    return GetWorld()->GetSubsystem<UMAAgentManager>();
}

void AMAGameMode::SetupSpectatorStart()
{
    UMAConfigManager* ConfigMgr = GetGameInstance()->GetSubsystem<UMAConfigManager>();
    if (!ConfigMgr) return;
    
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    
    APawn* SpectatorPawn = PC->GetPawn();
    if (!SpectatorPawn) return;
    
    FVector TargetLocation = FindSafeSpectatorLocation(ConfigMgr->SpectatorStartPosition);
    
    SpectatorPawn->SetActorLocation(TargetLocation);
    SpectatorPawn->SetActorRotation(ConfigMgr->SpectatorStartRotation);
    PC->SetControlRotation(ConfigMgr->SpectatorStartRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Spectator set to (%.0f, %.0f, %.0f)"),
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
}

void AMAGameMode::LoadAndSpawnAgents()
{
    UMAAgentManager* AgentManager = GetAgentManager();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] AgentManager not found!"));
        return;
    }

    UMAGameInstance* GameInstance = Cast<UMAGameInstance>(GetGameInstance());
    UMAConfigManager* ConfigMgr = GetGameInstance()->GetSubsystem<UMAConfigManager>();
    
    // 检查是否使用 Setup UI 配置
    if (GameInstance && ConfigMgr && ConfigMgr->bUseSetupUI && 
        GameInstance->bSetupCompleted && GameInstance->SetupAgentConfigs.Num() > 0)
    {
        SpawnAgentsFromSetupConfig(GameInstance);
    }
    else
    {
        AgentManager->SpawnAgentsFromConfig();
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Spawned %d agents from config"), AgentManager->GetAgentCount());
    }
}

void AMAGameMode::SpawnAgentsFromSetupConfig(UMAGameInstance* GameInstance)
{
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Entering Deployment Mode from Setup config"));
    
    TArray<FMAPendingDeployment> Deployments;
    for (const auto& Pair : GameInstance->SetupAgentConfigs)
    {
        Deployments.Add(FMAPendingDeployment(Pair.Key, Pair.Value));
    }
    
    GameInstance->ClearSetupConfig();
    
    GetWorld()->GetTimerManager().SetTimerForNextTick([this, Deployments]()
    {
        if (AMAPlayerController* MAPC = Cast<AMAPlayerController>(GetWorld()->GetFirstPlayerController()))
        {
            MAPC->EnterDeploymentModeWithUnits(Deployments);
        }
    });
}

void AMAGameMode::SpawnMiniMapManager()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UClass* ClassToSpawn = MiniMapManagerClass ? MiniMapManagerClass.Get() : AMAMiniMapManager::StaticClass();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    MiniMapManager = World->SpawnActor<AMAMiniMapManager>(ClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    
    if (MiniMapManager)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] MiniMapManager spawned"));
    }
}

FVector AMAGameMode::FindSafeSpectatorLocation(FVector DesiredLocation)
{
    UWorld* World = GetWorld();
    if (!World) return DesiredLocation;
    
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    
    FHitResult HitResult;
    FVector TraceStart = DesiredLocation;
    FVector TraceEnd = DesiredLocation + FVector(0.f, 0.f, 50.f);
    
    bool bHitAbove = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);
    
    if (!bHitAbove)
    {
        TraceEnd = DesiredLocation - FVector(0.f, 0.f, 50.f);
        if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
        {
            return DesiredLocation;
        }
    }
    
    // 尝试向上移动
    FVector TestLocation = DesiredLocation;
    for (int32 i = 0; i < 20; ++i)
    {
        TestLocation.Z += 200.f;
        TraceStart = TestLocation;
        TraceEnd = TestLocation + FVector(0.f, 0.f, 100.f);
        
        if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
        {
            return TestLocation;
        }
    }
    
    return FVector(DesiredLocation.X, DesiredLocation.Y, 3000.f);
}
