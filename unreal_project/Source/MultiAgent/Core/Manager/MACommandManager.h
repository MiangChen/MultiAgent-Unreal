// MACommandManager.h
// ========== 指令调度层 ==========
// 负责指令的分发调度，支持异步时间步执行

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "../Types/MATypes.h"
#include "../Comm/MACommTypes.h"
#include "../../Agent/Skill/Utils/MAFeedbackGenerator.h"
#include "MACommandManager.generated.h"

class AMACharacter;
struct FMAAgentSkillCommand;

// ========== 指令类型 ==========
UENUM(BlueprintType)
enum class EMACommand : uint8
{
    None        UMETA(DisplayName = "None"),
    Idle        UMETA(DisplayName = "Idle"),
    Navigate    UMETA(DisplayName = "Navigate"),
    Follow      UMETA(DisplayName = "Follow"),
    Charge      UMETA(DisplayName = "Charge"),
    Search      UMETA(DisplayName = "Search"),
    Place       UMETA(DisplayName = "Place"),
    TakeOff     UMETA(DisplayName = "TakeOff"),
    Land        UMETA(DisplayName = "Land"),
    ReturnHome  UMETA(DisplayName = "ReturnHome")
};

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

    /** 技能列表执行完成委托 */
    UPROPERTY(BlueprintAssignable, Category = "Command")
    FOnSkillListCompleted OnSkillListCompleted;

    /** 时间步完成委托 */
    UPROPERTY(BlueprintAssignable, Category = "Command")
    FOnTimeStepCompleted OnTimeStepCompleted;

    /** 辅助方法 */
    UFUNCTION(BlueprintCallable, Category = "Command")
    static FString CommandToString(EMACommand Command);
    
    UFUNCTION(BlueprintCallable, Category = "Command")
    static EMACommand StringToCommand(const FString& CommandString);

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
    
    /** 发送时间步反馈到 Python 端 */
    void SendTimeStepFeedbackToPython(const FMATimeStepFeedback& Feedback);
    
    /** 发送技能列表执行完成/中断反馈到 Python 端 */
    void SendSkillListCompletedFeedbackToPython(bool bCompleted, bool bInterrupted, int32 CompletedSteps, int32 TotalSteps);

    TMap<EMACommand, FGameplayTag> CommandTagCache;
    
    // 配置
    bool bUseStateTree = true;
    
    // 执行状态
    bool bIsExecuting = false;
    int32 CurrentTimeStep = 0;
    int32 PendingSkillCount = 0;
    
    // 当前技能列表
    FMASkillListMessage CurrentSkillList;
    
    // 反馈收集
    FMATimeStepFeedback CurrentTimeStepFeedback;
    TArray<FMATimeStepFeedback> AllFeedbacks;
    
    // 当前时间步的 Agent 和对应指令（用于反馈生成）
    TMap<AMACharacter*, EMACommand> CurrentTimeStepCommands;
};
