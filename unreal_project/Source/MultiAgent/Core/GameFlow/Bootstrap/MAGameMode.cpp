// MAGameMode.cpp

#include "Core/GameFlow/Bootstrap/MAGameMode.h"
#include "Core/GameFlow/Bootstrap/MAGameInstance.h"
#include "Core/Config/MAConfigManager.h"
#include "Input/MAPlayerController.h"
#include "UI/HUD/MAHUD.h"
#include "UI/Components/MAMiniMapManager.h"
#include "Core/AgentRuntime/Runtime/MAAgentManager.h"
#include "Core/EnvironmentCore/Runtime/MAEnvironmentManager.h"
#include "Core/Camera/Runtime/MAExternalCameraManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"

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
    
    // 禁用低质量背景模糊模式，确保毛玻璃效果正常显示
    static IConsoleVariable* LowQualityBlurCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Slate.EnableLowQualityBackgroundBlur"));
    if (LowQualityBlurCVar)
    {
        LowQualityBlurCVar->Set(0);
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Disabled low quality background blur"));
    }
    
    // 设置后处理以显示描边
    SetupOutlinePostProcess();

    // 延迟一帧执行初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        SetupSpectatorStart();
        LoadAndSpawnAgents();
        SpawnMiniMapManager();
        
        // 延迟初始化外部摄像头，确保 Agent 已生成
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            InitializeExternalCameras();
        });
        
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Initialization complete"));
    });
}

UMAAgentManager* AMAGameMode::GetAgentManager() const
{
    return GetWorld()->GetSubsystem<UMAAgentManager>();
}

UMAEnvironmentManager* AMAGameMode::GetEnvironmentManager() const
{
    return GetWorld()->GetSubsystem<UMAEnvironmentManager>();
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

    UMAEnvironmentManager* EnvironmentManager = GetEnvironmentManager();
    if (!EnvironmentManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] EnvironmentManager not found!"));
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

    // 先生成环境对象
    EnvironmentManager->SpawnEnvironmentObjectsFromConfig();
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


void AMAGameMode::InitializeExternalCameras()
{
    UMAExternalCameraManager* ExternalCameraManager = GetWorld()->GetSubsystem<UMAExternalCameraManager>();
    if (!ExternalCameraManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] ExternalCameraManager not found!"));
        return;
    }

    // 初始化预配置的外部摄像头
    ExternalCameraManager->InitializeDefaultCameras();

    UE_LOG(LogTemp, Log, TEXT("[GameMode] External cameras initialized: %d cameras"),
        ExternalCameraManager->GetExternalCameraCount());
}

void AMAGameMode::SetupOutlinePostProcess()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        // 启用引擎底层的 Selection Outline 系统
        // 这些命令会启用编辑器风格的描边效果
        PC->ConsoleCommand(TEXT("r.CustomDepth 3"));

        // 启用 Selection Outline 渲染 (编辑器描边效果)
        PC->ConsoleCommand(TEXT("ShowFlag.SelectionOutline 1"));

        // 设置描边颜色为橙色 (可选)
        // PC->ConsoleCommand(TEXT("r.Editor.SelectionOutlineColor 1.0 0.5 0.0"));

        UE_LOG(LogTemp, Log, TEXT("[GameMode] Engine Selection Outline system enabled"));
    }
}
