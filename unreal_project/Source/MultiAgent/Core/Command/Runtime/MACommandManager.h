// MACommandManager.h
// ========== 指令调度层 ==========
// 负责指令的分发调度，支持异步时间步执行

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Core/Shared/Types/MATypes.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "Core/Comm/Domain/MACommTypes.h"
#include "Agent/Skill/Feedback/MASkillFeedbackTypes.h"
#include "Agent/Skill/Domain/MAConditionCheckTypes.h"
#include "MACommandManager.generated.h"

class AMACharacter;
struct FMAAgentSkillCommand;
class UMATempDataManager;

// ========== 时间步执行反馈 ==========
USTRUCT(BlueprintType)
struct FMATimeStepFeedback
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TimeStep = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FMASkillExecutionFeedback> SkillFeedbacks;
};

// ========== 时间步完成委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeStepCompleted, const FMATimeStepFeedback&, Feedback);

// ========== 技能列表执行完成委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillListCompleted, const TArray<FMATimeStepFeedback>&, AllFeedbacks);

// ========== 执行暂停状态变化委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExecutionPauseStateChanged, bool, bPaused);

// ========== 指令调度层 ==========
UCLASS()
class MULTIAGENT_API UMACommandManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** 执行完整的技能列表（异步，按时间步顺序执行） */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void ExecuteSkillList(const FMASkillListMessage& SkillList);
    
    /** 中断当前技能列表执行 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void InterruptCurrentExecution();

    /** 发送指令到单个 Agent（蓝图接口，立即返回） */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void SendCommand(AMACharacter* Agent, EMACommand Command);

    /** 暂停当前技能列表执行 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void PauseExecution();

    /** 恢复当前技能列表执行 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void ResumeExecution();

    /** 切换暂停/恢复状态 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    void TogglePauseExecution();

    /** 是否处于暂停状态 */
    UFUNCTION(BlueprintPure, Category = "Command")
    bool IsPaused() const { return bIsPaused; }

    /** 技能列表执行完成委托 */
    UPROPERTY(BlueprintAssignable, Category = "Command")
    FOnSkillListCompleted OnSkillListCompleted;

    /** 时间步完成委托 */
    UPROPERTY(BlueprintAssignable, Category = "Command")
    FOnTimeStepCompleted OnTimeStepCompleted;

    /** 执行暂停状态变化委托 */
    UPROPERTY(BlueprintAssignable, Category = "Command")
    FOnExecutionPauseStateChanged OnExecutionPauseStateChanged;

    /** 辅助方法 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    static FString CommandToString(EMACommand Command);
    
    UFUNCTION(BlueprintCallable, Category = "Command")
    static EMACommand StringToCommand(const FString& CommandString);

    /** 获取 TempDataManager 用于广播技能状态更新 */
    class UMATempDataManager* GetTempDataManager() const;

    /** 是否正在执行技能列表 */
    UFUNCTION(BlueprintPure, Category = "Command")
    bool IsExecuting() const { return bIsExecuting; }

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
    void InitializeCommandTags();
    FGameplayTag CommandToTag(EMACommand Command) const;
    
    /** 执行当前时间步 */
    void ExecuteCurrentTimeStep();
    
    /** 单个技能完成回调 */
    UFUNCTION()
    void OnSkillCompleted(AMACharacter* Agent, bool bSuccess, const FString& Message);
    
    /** 发送指令到单个 Agent（内部使用，触发技能模拟） */
    void SendCommandToAgent(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd);
    
    /** 检查 Agent 是否有活跃的 StateTree */
    bool HasActiveStateTree(AMACharacter* Agent) const;
    
    /** 直接激活技能（兼容模式，无 StateTree 时使用），返回是否激活成功 */
    bool ActivateSkillDirectly(AMACharacter* Agent, class UMASkillComponent* SkillComp, EMACommand Command);

    /** 通过 Command Tag 将命令下发给 StateTree，返回是否成功派发 */
    bool DispatchCommandToStateTree(class UMASkillComponent* SkillComp, EMACommand Command) const;
    
    /** 发送时间步反馈到 Python 端 */
    void SendTimeStepFeedbackToPython(const FMATimeStepFeedback& Feedback);
    
    /** 发送技能列表执行完成/中断反馈到 Python 端 */
    void SendSkillListCompletedFeedbackToPython(bool bCompleted, bool bInterrupted, int32 CompletedSteps, int32 TotalSteps);

    /** 处理预检查失败：生成突发事件反馈，终止技能列表 */
    void HandlePrecheckFailure(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd, const FMAPrecheckResult& Result);

    /** 处理运行时检查失败：取消技能、生成突发事件反馈、终止技能列表 */
    void HandleRuntimeCheckFailure(AMACharacter* Agent, EMACommand Command, const FMAPrecheckResult& Result);

    /** 启动运行时检查定时器 */
    void StartRuntimeCheckTimer(AMACharacter* Agent, EMACommand Command);

    /** 停止运行时检查定时器 */
    void StopRuntimeCheckTimer(AMACharacter* Agent);

    /** 运行时检查定时器回调 */
    void OnRuntimeCheckTick(AMACharacter* Agent, EMACommand Command);

    TMap<EMACommand, FGameplayTag> CommandTagCache;
    
    // 配置
    bool bUseStateTree = true;
    
    // 执行状态
    bool bIsExecuting = false;
    bool bIsPaused = false;
    int32 CurrentTimeStep = 0;
    int32 PendingSkillCount = 0;
    
    // 当前技能列表
    FMASkillListMessage CurrentSkillList;
    
    // 反馈收集
    FMATimeStepFeedback CurrentTimeStepFeedback;
    TArray<FMATimeStepFeedback> AllFeedbacks;
    
    // 当前时间步的 Agent 和对应指令（用于反馈生成）
    TMap<AMACharacter*, EMACommand> CurrentTimeStepCommands;
    
    // 预检查产生的 info 事件（技能完成后附加到反馈）
    TMap<AMACharacter*, TArray<FMARenderedEvent>> PendingInfoEvents;
    
    // 运行时检查定时器（技能执行期间周期性检查）
    TMap<AMACharacter*, FTimerHandle> RuntimeCheckTimers;

    // 运行时检查定时器间隔（秒）
    static constexpr float RuntimeCheckIntervalSec = 1.0f;

    //=========================================================================
    // 气泡最小显示时长
    //=========================================================================

    void RequestHideSpeechBubble(AMACharacter* Agent);

    TMap<AMACharacter*, double> SpeechBubbleShowTimes;
    TMap<AMACharacter*, FTimerHandle> SpeechBubbleHideTimers;

    static constexpr float SpeechBubbleMinDisplaySec = 10.0f;

    //=========================================================================
    // 场景图周期性同步
    //=========================================================================

    /** 启动场景图同步定时器 */
    void StartSceneGraphSyncTimer();

    /** 停止场景图同步定时器 */
    void StopSceneGraphSyncTimer();

    /** 场景图同步定时器回调 */
    void OnSceneGraphSyncTick();

    FTimerHandle SceneGraphSyncTimerHandle;

    /** 场景图同步间隔（秒） */
    static constexpr float SceneGraphSyncIntervalSec = 2.0f;
};
