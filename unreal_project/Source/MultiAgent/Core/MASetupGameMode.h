// MASetupGameMode.h
// Setup 阶段的 GameMode - 用于配置小队

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MASetupGameMode.generated.h"

/**
 * Setup 阶段的 GameMode
 * 
 * 职责:
 * - 显示配置界面
 * - 不生成任何 Agent
 * - 等待用户完成配置后切换到仿真地图
 */
UCLASS()
class MULTIAGENT_API AMASetupGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMASetupGameMode();

protected:
    virtual void BeginPlay() override;
};
