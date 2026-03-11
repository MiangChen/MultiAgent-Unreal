// MASetupHUD.cpp

#include "MASetupHUD.h"
#include "MASetupWidget.h"
#include "Bootstrap/MASetupBootstrap.h"
#include "Blueprint/UserWidget.h"

AMASetupHUD::AMASetupHUD()
{
}

void AMASetupHUD::BeginPlay()
{
    Super::BeginPlay();
    static const FMASetupBootstrap Bootstrap;
    Bootstrap.InitializeSetupHUD(this);
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

        Coordinator.ApplyWidgetInputMode(*this, SetupWidget);

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

    const FMASetupLaunchRequest LaunchRequest = SetupWidget->BuildLaunchRequest();
    UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] Selected scene: %s"), *LaunchRequest.SelectedScene);
    UE_LOG(LogTemp, Warning, TEXT("[MASetupHUD] Total agents: %d"), LaunchRequest.TotalAgentCount);

    if (!Coordinator.StartSimulation(*this, LaunchRequest))
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupHUD] Failed to start simulation"));
    }
}
