// MASetupHUD.cpp

#include "MASetupHUD.h"
#include "MASetupWidget.h"
#include "../Core/Config/MAConfigManager.h"
#include "../Core/GameFlow/MAGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

AMASetupHUD::AMASetupHUD()
{
}

void AMASetupHUD::BeginPlay()
{
    Super::BeginPlay();

    // 检查是否需要显示 Setup UI
    UMAConfigManager* ConfigMgr = GetGameInstance()->GetSubsystem<UMAConfigManager>();
    if (!ConfigMgr || !ConfigMgr->bUseSetupUI)
    {
        UE_LOG(LogTemp, Log, TEXT("[MASetupHUD] bUseSetupUI=false, skipping Setup UI"));
        return;
    }
    
    // 延迟一帧创建 Widget
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        CreateSetupWidget();
    });
}

void AMASetupHUD::CreateSetupWidget()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupHUD] No PlayerController found"));
        return;
    }

    // 创建 SetupWidget
    SetupWidget = CreateWidget<UMASetupWidget>(PC, UMASetupWidget::StaticClass());
    if (SetupWidget)
    {
        SetupWidget->AddToViewport(100);  // 高优先级显示
        
        // 绑定委托
        SetupWidget->OnStartSimulation.AddDynamic(this, &AMASetupHUD::OnStartSimulation);
        UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] OnStartSimulation delegate bound, IsBound=%d"), 
            SetupWidget->OnStartSimulation.IsBound() ? 1 : 0);

        // 设置输入模式为 Game and UI，允许点击
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(SetupWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] SetupWidget created and displayed"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupHUD] Failed to create SetupWidget"));
    }
}

void AMASetupHUD::OnStartSimulation()
{
    UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] ========== OnStartSimulation called =========="));

    if (!SetupWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupHUD] SetupWidget is null!"));
        return;
    }

    // 获取 GameInstance 并保存配置
    UMAGameInstance* GameInstance = Cast<UMAGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // 将 SetupWidget 的配置转换为 TMap
        TMap<FString, int32> AgentConfigs;
        for (const FMAAgentSetupConfig& Config : SetupWidget->GetAgentConfigs())
        {
            if (AgentConfigs.Contains(Config.AgentType))
            {
                AgentConfigs[Config.AgentType] += Config.Count;
            }
            else
            {
                AgentConfigs.Add(Config.AgentType, Config.Count);
            }
        }

        FString SelectedScene = SetupWidget->GetSelectedScene();
        GameInstance->SaveSetupConfig(AgentConfigs, SelectedScene);

        // 场景名称到地图路径的映射
        // 根据你的实际地图路径修改
        static TMap<FString, FString> SceneToMapPath = {
            {TEXT("CyberCity"), TEXT("/Game/Map/LS_Scifi_ModernCity/Maps/Scifi_ModernCity")},
            {TEXT("DesertLab"), TEXT("/Game/Map/DesertLab/Maps/DesertLab")},
            {TEXT("SpruceForest"), TEXT("/Game/Map/Spruce_Forest/Demo/Maps/SpruceForest")},
            {TEXT("Town"), TEXT("/Game/Map/Town/DreamscapeFarmlands/Maps/Demo_Village_New")},
            {TEXT("Warehouse"), TEXT("/Game/Map/Warehouse/Maps/Demonstration_Map")}
        };

        FString MapPath = TEXT("/Game/Maps/") + SelectedScene;  // 默认路径
        if (FString* FoundPath = SceneToMapPath.Find(SelectedScene))
        {
            MapPath = *FoundPath;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] Selected scene: %s"), *SelectedScene);
        UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] Loading map: %s"), *MapPath);
        UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] Total agents: %d"), SetupWidget->GetTotalAgentCount());

        // 切换到仿真地图，强制使用 MAGameMode（覆盖地图自带的 GameMode 设置）
        FString Options = TEXT("?game=/Script/MultiAgent.MAGameMode");
        UGameplayStatics::OpenLevel(this, FName(*MapPath), true, Options);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupHUD] GameInstance not found"));
    }
}
