// MAHumanoidCharacter.h
// 人形机器人 - 可执行精细操作
// 技能: Navigate, Place

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAHumanoidCharacter.generated.h"

class UMAStateTreeComponent;
class UAnimSequence;

UCLASS()
class MULTIAGENT_API AMAHumanoidCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAHumanoidCharacter();

    // StateTree 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnNavigationTick() override;
    virtual void InitializeSkillSet() override;

private:
    UPROPERTY()
    UAnimSequence* IdleAnim = nullptr;
    
    UPROPERTY()
    UAnimSequence* WalkAnim = nullptr;
    
    bool bIsPlayingWalk = false;
};
