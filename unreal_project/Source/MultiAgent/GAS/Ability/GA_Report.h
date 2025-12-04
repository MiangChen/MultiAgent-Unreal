// GA_Report.h
// 报告技能 - 显示对话框报告信息

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Report.generated.h"

UCLASS()
class MULTIAGENT_API UGA_Report : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Report();

    // 设置报告内容
    UFUNCTION(BlueprintCallable, Category = "Report")
    void SetReportMessage(const FString& InMessage);

    // 显示时长
    UPROPERTY(EditDefaultsOnly, Category = "Report")
    float DisplayDuration = 5.f;

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

private:
    // 报告内容
    FString ReportMessage;
    
    // 定时器句柄
    FTimerHandle DisplayTimerHandle;
    
    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    // 显示报告对话框
    void ShowReportDialog();
    
    // 隐藏报告对话框
    void HideReportDialog();

    // 静态消息队列 (用于多个报告排队)
    static TArray<FString> MessageQueue;
    static bool bIsDisplayingMessage;
    static void ProcessMessageQueue();
};
