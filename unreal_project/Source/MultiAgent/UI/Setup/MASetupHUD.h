// MASetupHUD.h
// Setup 阶段的 HUD - 显示配置界面

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Application/MASetupHUDCoordinator.h"
#include "MASetupHUD.generated.h"

class UMASetupWidget;

/**
 * Setup 阶段的 HUD
 * 
 * 职责:
 * - 创建和显示 SetupWidget
 * - 处理开始仿真事件
 */
UCLASS()
class MULTIAGENT_API AMASetupHUD : public AHUD
{
    GENERATED_BODY()

public:
    AMASetupHUD();

    /** SetupWidget 实例 */
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UMASetupWidget* SetupWidget;

protected:
    virtual void BeginPlay() override;

private:
    /** 创建 SetupWidget */
    void CreateSetupWidget();

    /** 开始仿真回调 */
    UFUNCTION()
    void OnStartSimulation();

    /** Setup HUD 协调器 */
    FMASetupHUDCoordinator Coordinator;
};
