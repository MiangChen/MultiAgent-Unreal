// MASetupGameMode.cpp

#include "MASetupGameMode.h"
#include "../UI/MASetupHUD.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerController.h"

AMASetupGameMode::AMASetupGameMode()
{
    // Setup 阶段使用简单的 SpectatorPawn
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    
    // 使用默认的 PlayerController
    PlayerControllerClass = APlayerController::StaticClass();
    
    // 使用 SetupHUD 显示配置界面
    HUDClass = AMASetupHUD::StaticClass();
}

void AMASetupGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("[MASetupGameMode] BeginPlay - Setup phase started"));
}
