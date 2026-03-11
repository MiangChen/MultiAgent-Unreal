// MAHumanoidCharacter.h
// 人形机器人 - 可执行精细操作
// 技能: Navigate, Place

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAHumanoidCharacter.generated.h"

class UMAStateTreeComponent;
class UAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBendAnimationComplete);

UCLASS()
class MULTIAGENT_API AMAHumanoidCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAHumanoidCharacter();

    // StateTree 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

    // ========== 动画播放接口 ==========
    /** 播放俯身动画（动画前半段：站立→弯腰）
     * 动画播放到中点后停止，角色保持弯腰姿势
     */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayBendDownAnimation();
    
    /** 播放起身动画（动画后半段：弯腰→站立）
     * 从中点开始播放到结束，然后切换到 Idle 动画
     */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayStandUpAnimation();
    
    /** 是否正在播放俯身/起身动画 */
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsBending() const { return bIsBending; }
    
    /** 俯身/起身动画完成回调 */
    UPROPERTY(BlueprintAssignable, Category = "Animation")
    FOnBendAnimationComplete OnBendAnimationComplete;
    
    /** 手部附着偏移（用于持有物体时的位置）
     * X: 前方距离（正值=身体前方）
     * Y: 左右偏移（0=中间）
     * Z: 高度偏移（相对于角色原点，负值=向下）
     * 
     * Humanoid 角色高度约 180cm，手部位置大约在腰部高度（约 90-100cm）
     * 角色原点在脚底，所以手部高度约 -90cm 到 0cm 之间
     */
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    FVector HandAttachOffset = FVector(60.f, 0.f, -20.f);
    
    /** 拾取动画总时长（从动画资产获取） */
    UPROPERTY(VisibleAnywhere, Category = "Animation")
    float PickupAnimDuration = 2.0f;
    
    /** 拾取动画中点时间（弯腰最低点） */
    UPROPERTY(VisibleAnywhere, Category = "Animation")
    float PickupAnimMidPoint = 1.0f;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnNavigationTick() override;
    virtual void InitializeSkillSet() override;

private:
    // 基础动画
    UPROPERTY()
    UAnimSequence* IdleAnim = nullptr;
    
    UPROPERTY()
    UAnimSequence* WalkAnim = nullptr;
    
    // 拾取动画（Item Pickup Set）
    // 动画结构: 站立(0%) → 弯腰(~50%) → 站立(100%)
    UPROPERTY()
    UAnimSequence* PickupAnim = nullptr;
    
    // 动画状态
    bool bIsPlayingWalk = false;
    
    /** 是否正在播放俯身/起身动画 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    bool bIsBending = false;
    
    /** 动画完成定时器 */
    FTimerHandle BendAnimTimerHandle;
    
    /** 俯身动画完成回调（内部） */
    void OnBendDownAnimFinished();
    
    /** 起身动画完成回调（内部） */
    void OnStandUpAnimFinished();
};
