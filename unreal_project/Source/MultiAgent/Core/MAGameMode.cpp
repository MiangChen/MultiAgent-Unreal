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
    
    // 设置 Spectator 位置和旋转
    SpectatorPawn->SetActorLocation(GameInstance->SpectatorStartPosition);
    SpectatorPawn->SetActorRotation(GameInstance->SpectatorStartRotation);
    PC->SetControlRotation(GameInstance->SpectatorStartRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Spectator set to (%.0f, %.0f, %.0f) Rot: (%.0f, %.0f, %.0f)"),
        GameInstance->SpectatorStartPosition.X,
        GameInstance->SpectatorStartPosition.Y,
        GameInstance->SpectatorStartPosition.Z,
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

    // 构建配置文件的绝对路径
    // AgentConfigPath 是相对于 unreal_project 目录的路径
    FString ProjectDir = FPaths::ProjectDir();
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ProjectDir: %s"), *ProjectDir);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] AgentConfigPath: %s"), *AgentConfigPath);
    
    FString ConfigFullPath = FPaths::Combine(ProjectDir, AgentConfigPath);
    ConfigFullPath = FPaths::ConvertRelativePathToFull(ConfigFullPath);
    
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Loading agent config from: %s"), *ConfigFullPath);
    
    // 检查文件是否存在
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