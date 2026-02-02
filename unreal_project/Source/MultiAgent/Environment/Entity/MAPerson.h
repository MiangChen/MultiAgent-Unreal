// MAPerson.h
// 行人环境对象实现
// 使用 Population_System 资产包

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../IMAEnvironmentObject.h"
#include "MAPerson.generated.h"

class UMANavigationService;
class AMAFire;
class UAnimSequence;
struct FMAEnvironmentObjectConfig;

/** 行人动画类型 */
UENUM(BlueprintType)
enum class EMAPersonAnimation : uint8
{
    Idle,
    Walk,
    Run,
    Talk
};

/**
 * 行人环境对象
 * 
 * 使用 Population_System 资产包，支持:
 * - 多种角色类型 (business, casual, sportive)
 * - 丰富的动画 (idle, walk, run, talk, listen, clap 等)
 * - 男女老幼多种角色
 * 
 * 配置示例:
 * {
 *     "label": "Person_1",
 *     "type": "person",
 *     "position": [0, 0, 0],
 *     "features": {
 *         "subtype": "casual",
 *         "variant": "01",
 *         "gender": "male",
 *         "animation": "idle"
 *     }
 * }
 */
UCLASS()
class MULTIAGENT_API AMAPerson : public ACharacter, public IMAEnvironmentObject
{
    GENERATED_BODY()

public:
    AMAPerson();

    // IMAEnvironmentObject 接口
    virtual FString GetObjectLabel() const override;
    virtual FString GetObjectType() const override;
    virtual const TMap<FString, FString>& GetObjectFeatures() const override;
    virtual void SetAttachedFire(AMAFire* Fire) override;
    virtual AMAFire* GetAttachedFire() const override;

    /** 根据配置初始化 */
    UFUNCTION(BlueprintCallable, Category = "Person")
    void Configure(const FMAEnvironmentObjectConfig& Config);

    /** 设置动画 */
    UFUNCTION(BlueprintCallable, Category = "Person")
    void SetAnimation(EMAPersonAnimation Animation);

    // 导航接口
    UFUNCTION(BlueprintCallable, Category = "Person")
    bool NavigateTo(FVector Destination, float AcceptanceRadius = 200.f);

    UFUNCTION(BlueprintCallable, Category = "Person")
    void CancelNavigation();

    UFUNCTION(BlueprintPure, Category = "Person")
    bool IsNavigating() const;

    /** 获取导航服务组件 */
    UFUNCTION(BlueprintPure, Category = "Person")
    UMANavigationService* GetNavigationService() const { return NavigationService; }

    // 巡逻接口
    UFUNCTION(BlueprintCallable, Category = "Person")
    void StartPatrol();

    UFUNCTION(BlueprintCallable, Category = "Person")
    void StopPatrol();

    UFUNCTION(BlueprintPure, Category = "Person")
    bool IsPatrolling() const { return bIsPatrolling; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    // 角色模型选择
    void SelectCharacterMesh(const FString& Subtype, const FString& Variant, const FString& Gender);
    
    // 动画加载
    void LoadAnimations();

    // 导航回调
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);
    
    // 巡逻
    void MoveToNextWaypoint();

    // 环境对象属性
    UPROPERTY()
    FString ObjectLabel;

    UPROPERTY()
    FString ObjectType = TEXT("person");

    UPROPERTY()
    TMap<FString, FString> Features;

    UPROPERTY()
    TWeakObjectPtr<AMAFire> AttachedFire;

    // 组件
    UPROPERTY()
    TObjectPtr<UMANavigationService> NavigationService;

    // 动画资源
    UPROPERTY()
    TObjectPtr<UAnimSequence> IdleAnim;

    UPROPERTY()
    TObjectPtr<UAnimSequence> WalkAnim;

    UPROPERTY()
    TObjectPtr<UAnimSequence> RunAnim;

    UPROPERTY()
    TObjectPtr<UAnimSequence> TalkAnim;

    // 状态
    EMAPersonAnimation CurrentAnimation = EMAPersonAnimation::Idle;
    bool bIsPlayingWalk = false;

    // 巡逻状态
    TArray<FVector> PatrolWaypoints;
    FTimerHandle PatrolWaitTimerHandle;
    int32 CurrentWaypointIndex = 0;
    float PatrolWaitTime = 2.0f;
    bool bPatrolLoop = true;
    bool bIsPatrolling = false;
};
