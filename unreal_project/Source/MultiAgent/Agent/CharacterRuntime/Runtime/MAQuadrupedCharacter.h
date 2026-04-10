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

    /** 正常行走时的动画播放速率（满速时使用） */
    float NormalWalkPlayRate = 2.5f;
    /** 最低动画播放速率（防止动画静止） */
    float MinWalkPlayRate = 0.5f;
    /** 最高动画播放速率（安全上限） */
    float MaxWalkPlayRate = 3.0f;

private:
    void PlayWalkAnimation();
    void PlayIdleAnimation();
    
    UFUNCTION()
    void OnEnergyDepleted();
};
