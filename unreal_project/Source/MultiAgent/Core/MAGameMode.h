// MAGameMode.h
// 游戏模式 - 负责初始化各个子系统，协调游戏流程

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MACharacter.h"
#include "MAGameMode.generated.h"

class UMAActorSubsystem;
class AMACameraSensor;

UCLASS()
class MULTIAGENT_API AMAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMAGameMode();

    // 获取 ActorSubsystem
    UFUNCTION(BlueprintCallable, Category = "Subsystem")
    UMAActorSubsystem* GetActorSubsystem() const;

    // 人类 Character C++ 类
    UPROPERTY(EditDefaultsOnly, Category = "Character")
    TSubclassOf<AMACharacter> HumanCharacterClass;

    // 机器狗 Character C++ 类
    UPROPERTY(EditDefaultsOnly, Category = "Character")
    TSubclassOf<AMACharacter> RobotDogCharacterClass;

    // 生成的人类数量
    UPROPERTY(EditDefaultsOnly, Category = "Character")
    int32 NumHumans = 2;

    // 生成的机器狗数量
    UPROPERTY(EditDefaultsOnly, Category = "Character")
    int32 NumRobotDogs = 3;

protected:
    virtual void BeginPlay() override;
    void SpawnInitialCharacters();
    
    // 生成并附着摄像头到 Character
    void SpawnAndAttachCamera(UMAActorSubsystem* ActorSubsystem, AMACharacter* ParentCharacter, FVector RelativeLocation, FRotator RelativeRotation);
    
    // 生成充电站
    void SpawnChargingStation();
};
