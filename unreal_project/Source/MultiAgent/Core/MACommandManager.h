// MACommandManager.h
// 命令管理器 - RTS 风格统一命令分发
// 参考星际争霸的命令系统：选择单位 → 下达命令 → 单位执行
// 所有命令通过此 Manager 分发，支持批量操作和外部控制 (Python/RPC)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "MATypes.h"
#include "MACommandManager.generated.h"

class AMACharacter;
class AMAPatrolPath;
class AMACoverageArea;
class UMAAbilitySystemComponent;

// ========== 命令类型 ==========
UENUM(BlueprintType)
enum class EMACommand : uint8
{
    None        UMETA(DisplayName = "None"),
    Idle        UMETA(DisplayName = "Idle"),        // 停止，进入空闲
    Patrol      UMETA(DisplayName = "Patrol"),      // 巡逻
    Charge      UMETA(DisplayName = "Charge"),      // 充电
    Follow      UMETA(DisplayName = "Follow"),      // 跟随
    Coverage    UMETA(DisplayName = "Coverage"),    // 区域覆盖
    Avoid       UMETA(DisplayName = "Avoid"),       // 避障导航
    Navigate    UMETA(DisplayName = "Navigate")     // 普通导航
};

// ========== 命令参数 ==========
USTRUCT(BlueprintType)
struct FMACommandParams
{
    GENERATED_BODY()

    // 目标位置 (Navigate, Avoid)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    FVector TargetLocation = FVector::ZeroVector;

    // 跟随目标 (Follow)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    TWeakObjectPtr<AMACharacter> FollowTarget;

    // 巡逻路径 (Patrol)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;

    // 覆盖区域 (Coverage)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    TWeakObjectPtr<AActor> CoverageArea;

    // 是否排除 Tracker 类型的 Agent
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    bool bExcludeTracker = true;
    
    // 是否显示屏幕消息
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    bool bShowMessage = true;
};

// ========== 命令结果 ==========
USTRUCT(BlueprintType)
struct FMACommandResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Command")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "Command")
    int32 AffectedCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Command")
    FString Message;
};

// ========== 委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCommandSent, EMACommand, Command, int32, AffectedCount, const FMACommandParams&, Params);

// ========== 命令管理器 ==========
UCLASS()
class MULTIAGENT_API UMACommandManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== 单个 Agent 命令 ==========
    
    // 发送命令给单个 Agent
    UFUNCTION(BlueprintCallable, Category = "Command")
    bool SendCommandToAgent(AMACharacter* Agent, EMACommand Command, const FMACommandParams& Params = FMACommandParams());

    // ========== 批量命令 (RTS 风格) ==========
    
    // 发送命令给所有可控 Agent (RobotDog + Drone)
    UFUNCTION(BlueprintCallable, Category = "Command")
    FMACommandResult SendCommand(EMACommand Command, const FMACommandParams& Params = FMACommandParams());

    // 发送命令给指定类型的 Agent
    UFUNCTION(BlueprintCallable, Category = "Command")
    FMACommandResult SendCommandToType(EMAAgentType Type, EMACommand Command, const FMACommandParams& Params = FMACommandParams());

    // 发送命令给指定列表的 Agent
    UFUNCTION(BlueprintCallable, Category = "Command")
    FMACommandResult SendCommandToAgents(const TArray<AMACharacter*>& Agents, EMACommand Command, const FMACommandParams& Params = FMACommandParams());

    // ========== 查询方法 ==========
    
    // 获取所有可控 Agent (RobotDog + Drone, 可选排除 Tracker)
    UFUNCTION(BlueprintCallable, Category = "Command")
    TArray<AMACharacter*> GetControllableAgents(bool bExcludeTracker = true) const;

    // 获取 Agent 当前命令
    UFUNCTION(BlueprintCallable, Category = "Command")
    EMACommand GetAgentCurrentCommand(AMACharacter* Agent) const;

    // ========== 辅助方法 ==========
    
    // 命令转字符串 (用于日志/UI)
    UFUNCTION(BlueprintCallable, Category = "Command")
    static FString CommandToString(EMACommand Command);

    // 字符串转命令 (用于外部控制)
    UFUNCTION(BlueprintCallable, Category = "Command")
    static EMACommand StringToCommand(const FString& CommandString);

    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Command")
    FOnCommandSent OnCommandSent;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
    // 应用命令到单个 Agent (内部实现)
    bool ApplyCommand(AMACharacter* Agent, EMACommand Command, const FMACommandParams& Params);

    // 清除其他命令 Tag
    void ClearOtherCommandTags(UMAAbilitySystemComponent* ASC, EMACommand ExceptCommand);

    // 设置命令相关的 Agent 属性 (PatrolPath, FollowTarget 等)
    void SetAgentCommandProperties(AMACharacter* Agent, EMACommand Command, const FMACommandParams& Params);

    // 命令转 GameplayTag
    FGameplayTag CommandToTag(EMACommand Command) const;

    // 检查 Agent 是否应该被排除
    bool ShouldExcludeAgent(AMACharacter* Agent, const FMACommandParams& Params) const;

    // 所有命令 Tag (缓存，避免重复创建)
    TMap<EMACommand, FGameplayTag> CommandTagCache;
    
    void InitializeCommandTags();
};
