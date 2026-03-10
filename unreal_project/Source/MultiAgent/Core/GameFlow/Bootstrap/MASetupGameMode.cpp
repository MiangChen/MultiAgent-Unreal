// MASetupGameMode.cpp

#include "Core/GameFlow/Bootstrap/MASetupGameMode.h"
#include "Core/Config/MAConfigManager.h"
#include "UI/Setup/MASetupHUD.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AMASetupGameMode::AMASetupGameMode()
{
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    PlayerControllerClass = APlayerController::StaticClass();
    HUDClass = AMASetupHUD::StaticClass();
}

void AMASetupGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("[MASetupGameMode] BeginPlay"));
    
    // 从 ConfigManager 获取配置
    UMAConfigManager* ConfigMgr = GetGameInstance()->GetSubsystem<UMAConfigManager>();
    if (ConfigMgr && !ConfigMgr->bUseSetupUI)
    {
        // 不使用 Setup UI，直接加载默认地图
        UE_LOG(LogTemp, Log, TEXT("[MASetupGameMode] bUseSetupUI=false, loading: %s"), *ConfigMgr->DefaultMapPath);
        
        if (!ConfigMgr->DefaultMapPath.IsEmpty())
        {
            GetWorld()->GetTimerManager().SetTimerForNextTick([this, ConfigMgr]()
            {
                FString Options = TEXT("?game=/Script/MultiAgent.MAGameMode");
                UGameplayStatics::OpenLevel(this, FName(*ConfigMgr->DefaultMapPath), true, Options);
            });
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[MASetupGameMode] DefaultMapPath is empty!"));
        }
    }
    // 如果 bUseSetupUI=true，HUD 会显示 Setup UI
}
