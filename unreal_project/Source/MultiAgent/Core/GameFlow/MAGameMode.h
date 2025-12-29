// MAGameMode.h
// 游戏模式 - 负责初始化游戏流程

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MAGameMode.generated.h"

class UMAAgentManager;
class AMAMiniMapManager;

UCLASS()
class MULTIAGENT_API AMAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMAGameMode();

    UFUNCTION(BlueprintCallable, Category = "Subsystem")
    UMAAgentManager* GetAgentManager() const;

    UPROPERTY(EditDefaultsOnly, Category = "MiniMap")
    TSubclassOf<AMAMiniMapManager> MiniMapManagerClass;

protected:
    virtual void BeginPlay() override;

private:
    void SetupSpectatorStart();
    void LoadAndSpawnAgents();
    void SpawnAgentsFromSetupConfig(class UMAGameInstance* GameInstance);
    void SpawnMiniMapManager();
    FVector FindSafeSpectatorLocation(FVector DesiredLocation);

    UPROPERTY()
    AMAMiniMapManager* MiniMapManager;
};
