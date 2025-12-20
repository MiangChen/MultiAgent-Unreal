// MAGameMode.cpp

#include "MAGameMode.h"
#include "MAGameInstance.h"
#include "../Input/MAPlayerController.h"
#include "../UI/MAHUD.h"
#include "MAAgentManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"

AMAGameMode::AMAGameMode()
{
    // 玩家使用 SpectatorPawn 作为上帝视角（原生飞行控制）
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    PlayerControllerClass = AMAPlayerController::StaticClass();
    
    // 设置 HUD 类 - MAHUD 继承自 MASelectionHUD，同时支持框选绘制和 UI Widget 管理
    HUDClass = AMAHUD::StaticClass();
}

void AMAGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("========== [GameMode] BeginPlay START =========="));
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] GameMode Class: %s"), *GetClass()->GetName());
    
    // 延迟一帧执行初始化，确保玩家和子系统已经初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Timer tick - starting initialization"));
        
        // 设置 Spectator 初始位置
        SetupSpectatorStart();
        
        // 生成 Agent
        LoadAndSpawnAgents();
        
        UE_LOG(LogTemp, Warning, TEXT("========== [GameMode] Initialization COMPLETE =========="));
    });
}

void AMAGameMode::SetupSpectatorStart()
{
    UMAGameInstance* GameInstance = Cast<UMAGameInstance>(GetGameInstance());
    if (!GameInstance)
    {
        return;
    }
    
    // 获取玩家控制器和 Pawn
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }
    
    APawn* SpectatorPawn = PC->GetPawn();
    if (!SpectatorPawn)
    {
        return;
    }
    
    FVector TargetLocation = GameInstance->SpectatorStartPosition;
    
    // 检测目标位置是否在建筑内部，如果是则自动调整
    TargetLocation = FindSafeSpectatorLocation(TargetLocation);
    
    // 设置 Spectator 位置和旋转
    SpectatorPawn->SetActorLocation(TargetLocation);
    SpectatorPawn->SetActorRotation(GameInstance->SpectatorStartRotation);
    PC->SetControlRotation(GameInstance->SpectatorStartRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Spectator set to (%.0f, %.0f, %.0f) Rot: (%.0f, %.0f, %.0f)"),
        TargetLocation.X,
        TargetLocation.Y,
        TargetLocation.Z,
        GameInstance->SpectatorStartRotation.Pitch,
        GameInstance->SpectatorStartRotation.Yaw,
        GameInstance->SpectatorStartRotation.Roll);
}

UMAAgentManager* AMAGameMode::GetAgentManager() const
{
    return GetWorld()->GetSubsystem<UMAAgentManager>();
}

void AMAGameMode::LoadAndSpawnAgents()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] LoadAndSpawnAgents() called"));
    
    UMAAgentManager* AgentManager = GetAgentManager();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] AgentManager not found!"));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] AgentManager found"));

    // 检查是否有 Setup 配置
    UMAGameInstance* GameInstance = Cast<UMAGameInstance>(GetGameInstance());
    if (GameInstance && GameInstance->bSetupCompleted && GameInstance->SetupAgentConfigs.Num() > 0)
    {
        // 从 Setup 配置生成 Agent
        SpawnAgentsFromSetupConfig(GameInstance);
        return;
    }

    // 否则从 JSON 文件加载（兼容旧流程）
    FString ProjectDir = FPaths::ProjectDir();
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ProjectDir: %s"), *ProjectDir);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] AgentConfigPath: %s"), *AgentConfigPath);
    
    FString ConfigFullPath = FPaths::Combine(ProjectDir, AgentConfigPath);
    ConfigFullPath = FPaths::ConvertRelativePathToFull(ConfigFullPath);
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Loading agent config from: %s"), *ConfigFullPath);
    
    if (!FPaths::FileExists(ConfigFullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Config file does NOT exist: %s"), *ConfigFullPath);
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Config file exists"));
    
    if (!AgentManager->LoadAndSpawnFromConfig(ConfigFullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to load agent config from: %s"), *ConfigFullPath);
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Successfully spawned %d agents from config"), AgentManager->GetAgentCount());
}

void AMAGameMode::SpawnAgentsFromSetupConfig(UMAGameInstance* GameInstance)
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Entering Deployment Mode from Setup config"));
    
    // 构建待部署列表
    TArray<FMAPendingDeployment> Deployments;
    for (const auto& Pair : GameInstance->SetupAgentConfigs)
    {
        Deployments.Add(FMAPendingDeployment(Pair.Key, Pair.Value));
    }
    
    // 清除 Setup 配置
    GameInstance->ClearSetupConfig();
    
    // 延迟进入部署模式，确保 PlayerController 已初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this, Deployments]()
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (AMAPlayerController* MAPC = Cast<AMAPlayerController>(PC))
        {
            // 使用新的 API：添加到背包并进入部署模式
            MAPC->EnterDeploymentModeWithUnits(Deployments);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[GameMode] MAPlayerController not found!"));
        }
    });
}

FVector AMAGameMode::FindSafeSpectatorLocation(FVector DesiredLocation)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return DesiredLocation;
    }
    
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    
    // 使用射线检测：从目标位置向上发射，如果立即碰到东西说明在建筑内
    FHitResult HitResult;
    FVector TraceStart = DesiredLocation;
    FVector TraceEnd = DesiredLocation + FVector(0.f, 0.f, 50.f);
    
    bool bHitAbove = World->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_WorldStatic,
        QueryParams
    );
    
    if (!bHitAbove)
    {
        // 上方没有碰撞，位置可能安全，再检查下方
        TraceEnd = DesiredLocation - FVector(0.f, 0.f, 50.f);
        bool bHitBelow = World->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );
        
        if (!bHitBelow)
        {
            // 上下都没碰撞，位置安全
            UE_LOG(LogTemp, Log, TEXT("[GameMode] Spectator location is safe"));
            return DesiredLocation;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Spectator location blocked, searching for safe position..."));
    
    // 尝试向上移动找到安全位置
    FVector TestLocation = DesiredLocation;
    for (int32 i = 0; i < 20; ++i)
    {
        TestLocation.Z += 200.f;
        
        // 检测上方
        TraceStart = TestLocation;
        TraceEnd = TestLocation + FVector(0.f, 0.f, 100.f);
        
        bool bBlocked = World->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );
        
        if (!bBlocked)
        {
            UE_LOG(LogTemp, Log, TEXT("[GameMode] Found safe location at Z=%.0f (moved up %.0f)"), 
                TestLocation.Z, TestLocation.Z - DesiredLocation.Z);
            return TestLocation;
        }
    }
    
    // 向上找不到，尝试在 XY 平面搜索
    const float SearchRadius = 500.f;
    const int32 NumDirections = 8;
    
    for (int32 Ring = 1; Ring <= 5; ++Ring)
    {
        float CurrentRadius = SearchRadius * Ring;
        
        for (int32 Dir = 0; Dir < NumDirections; ++Dir)
        {
            float Angle = (2.f * PI / NumDirections) * Dir;
            TestLocation = DesiredLocation + FVector(
                FMath::Cos(Angle) * CurrentRadius,
                FMath::Sin(Angle) * CurrentRadius,
                500.f  // 稍微抬高
            );
            
            // 检测上方
            TraceStart = TestLocation;
            TraceEnd = TestLocation + FVector(0.f, 0.f, 100.f);
            
            bool bBlocked = World->LineTraceSingleByChannel(
                HitResult,
                TraceStart,
                TraceEnd,
                ECC_WorldStatic,
                QueryParams
            );
            
            if (!bBlocked)
            {
                UE_LOG(LogTemp, Log, TEXT("[GameMode] Found safe location at (%.0f, %.0f, %.0f)"), 
                    TestLocation.X, TestLocation.Y, TestLocation.Z);
                return TestLocation;
            }
        }
    }
    
    // 实在找不到，返回一个很高的位置
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Could not find safe location, using high altitude"));
    return FVector(DesiredLocation.X, DesiredLocation.Y, 3000.f);
}