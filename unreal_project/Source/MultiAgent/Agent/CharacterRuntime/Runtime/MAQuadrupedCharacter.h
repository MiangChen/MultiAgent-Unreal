// MAQuadrupedCharacter.h
// 四足机器人角色 - 支持 StateTree AI
// 技能: Navigate, Search, Follow

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAQuadrupedCharacter.generated.h"

class UMAStateTreeComponent;

UCLASS()
class MULTIAGENT_API AMAQuadrupedCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAQuadrupedCharacter();

    virtual void Tick(float DeltaTime) override;

    // StateTree 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void InitializeSkillSet() override;

    UPROPERTY()
    UAnimSequence* IdleAnim;
    
    UPROPERTY()
    UAnimSequence* WalkAnim;
    
    bool bIsPlayingWalk;

private:
    void PlayWalkAnimation();
    void PlayIdleAnimation();
    
    UFUNCTION()
    void OnEnergyDepleted();
};
