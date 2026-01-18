// MACommandManager.cpp
// ========== 指令调度层实现 ==========

#include "MACommandManager.h"
#include "MAAgentManager.h"
#include "MATempDataManager.h"
#include "../Config/MAConfigManager.h"
#include "../Comm/MACommSubsystem.h"
#include "../Comm/MACommTypes.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Skill/MASkillComponent.h"
#include "../../Agent/Skill/Utils/MASkillParamsProcessor.h"
#include "../../Agent/Skill/Utils/MAFeedbackGenerator.h"
#include "../../Agent/Skill/Utils/MASceneGraphUpdater.h"
#include "../../Agent/StateTree/MAStateTreeComponent.h"
#include "Components/StateTreeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommandManager, Log, All);

void UMACommandManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    InitializeCommandTags();
    
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
            {
                bUseStateTree = ConfigMgr->bUseStateTree;
            }
        }
    }
    
    UE_LOG(LogMACommandManager, Log, TEXT("========================================"));
    UE_LOG(LogMACommandManager, Log, TEXT("MACommandManager initialized"));
    UE_LOG(LogMACommandManager, Log, TEXT("  Execution Mode: %s"), 
        bUseStateTree ? TEXT("StateTree-driven") : TEXT("Direct Skill Activation"));
    UE_LOG(LogMACommandManager, Log, TEXT("========================================"));
}

void UMACommandManager::InitializeCommandTags()
{
    CommandTagCache.Add(EMACommand::Idle, FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
    CommandTagCache.Add(EMACommand::Navigate, FGameplayTag::RequestGameplayTag(FName("Command.Navigate")));
    CommandTagCache.Add(EMACommand::Follow, FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
    CommandTagCache.Add(EMACommand::Charge, FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
    CommandTagCache.Add(EMACommand::Search, FGameplayTag::RequestGameplayTag(FName("Command.Search")));
    CommandTagCache.Add(EMACommand::Place, FGameplayTag::RequestGameplayTag(FName("Command.Place")));
    CommandTagCache.Add(EMACommand::TakeOff, FGameplayTag::RequestGameplayTag(FName("Command.TakeOff")));
    CommandTagCache.Add(EMACommand::Land, FGameplayTag::RequestGameplayTag(FName("Command.Land")));
    CommandTagCache.Add(EMACommand::ReturnHome, FGameplayTag::RequestGameplayTag(FName("Command.ReturnHome")));
    CommandTagCache.Add(EMACommand::TakePhoto, FGameplayTag::RequestGameplayTag(FName("Command.TakePhoto")));
    CommandTagCache.Add(EMACommand::Broadcast, FGameplayTag::RequestGameplayTag(FName("Command.Broadcast")));
    CommandTagCache.Add(EMACommand::HandleHazard, FGameplayTag::RequestGameplayTag(FName("Command.HandleHazard")));
    CommandTagCache.Add(EMACommand::Guide, FGameplayTag::RequestGameplayTag(FName("Command.Guide")));
}

// ========== 技能列表执行 ==========

void UMACommandManager::ExecuteSkillList(const FMASkillListMessage& SkillList)
{
    if (SkillList.TotalTimeSteps == 0)
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("Empty skill list"));
        return;
    }
    
    // 如果正在执行，先中断当前执行
    if (bIsExecuting)
    {
        UE_LOG(LogMACommandManager, Log, TEXT("Interrupting current skill list execution"));
        InterruptCurrentExecution();
    }
    
    UE_LOG(LogMACommandManager, Log, TEXT("ExecuteSkillList: %d time steps"), SkillList.TotalTimeSteps);
    
    bIsExecuting = true;
    CurrentTimeStep = 0;
    CurrentSkillList = SkillList;
    AllFeedbacks.Empty();
    
    ExecuteCurrentTimeStep();
}

void UMACommandManager::InterruptCurrentExecution()
{
    if (!bIsExecuting) return;
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
    // 保存中断前的状态用于反馈
    int32 InterruptedAtTimeStep = CurrentTimeStep;
    int32 TotalSteps = CurrentSkillList.TotalTimeSteps;
    
    // 取消所有 Agent 的技能并解绑委托
    for (const auto& Pair : CurrentTimeStepCommands)
    {
        AMACharacter* Agent = Pair.Key;
        if (Agent)
        {
            if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
            {
                SkillComp->OnSkillCompleted.RemoveDynamic(this, &UMACommandManager::OnSkillCompleted);
                SkillComp->CancelAllSkills();
            }
        }
    }
    
    // 发送技能列表中断反馈
    SendSkillListCompletedFeedbackToPython(false, true, InterruptedAtTimeStep, TotalSteps);
    
    // 重置执行状态
    CurrentTimeStepCommands.Empty();
    PendingSkillCount = 0;
    bIsExecuting = false;
}

void UMACommandManager::ExecuteCurrentTimeStep()
{
    const FMATimeStepCommands* TimeStep = CurrentSkillList.GetTimeStep(CurrentTimeStep);
    if (!TimeStep)
    {
        UE_LOG(LogMACommandManager, Log, TEXT("All time steps completed"));
        bIsExecuting = false;
        
        // 发送技能列表完成反馈
        SendSkillListCompletedFeedbackToPython(true, false, CurrentSkillList.TotalTimeSteps, CurrentSkillList.TotalTimeSteps);
        
        OnSkillListCompleted.Broadcast(AllFeedbacks);
        return;
    }
    
    UE_LOG(LogMACommandManager, Log, TEXT("  TimeStep %d: %d commands"), CurrentTimeStep, TimeStep->Commands.Num());
    
    CurrentTimeStepFeedback = FMATimeStepFeedback();
    CurrentTimeStepFeedback.TimeStep = CurrentTimeStep;
    CurrentTimeStepCommands.Empty();
    PendingSkillCount = 0;
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogMACommandManager, Error, TEXT("AgentManager not found"));
        bIsExecuting = false;
        return;
    }
    
    // 收集有效命令
    TArray<TPair<AMACharacter*, const FMAAgentSkillCommand*>> ValidCommands;
    
    for (const FMAAgentSkillCommand& Cmd : TimeStep->Commands)
    {
        AMACharacter* Agent = AgentManager->GetAgentByID(Cmd.AgentId);
        if (!Agent)
        {
            UE_LOG(LogMACommandManager, Warning, TEXT("    Agent '%s' not found"), *Cmd.AgentId);
            FMASkillExecutionFeedback Feedback;
            Feedback.AgentId = Cmd.AgentId;
            Feedback.SkillName = Cmd.SkillName;
            Feedback.bSuccess = false;
            Feedback.Message = TEXT("Agent not found");
            CurrentTimeStepFeedback.SkillFeedbacks.Add(Feedback);
            continue;
        }
        
        if (Agent->GetSkillComponent())
        {
            ValidCommands.Add(TPair<AMACharacter*, const FMAAgentSkillCommand*>(Agent, &Cmd));
        }
    }
    
    if (ValidCommands.Num() == 0)
    {
        AllFeedbacks.Add(CurrentTimeStepFeedback);
        CurrentTimeStep++;
        ExecuteCurrentTimeStep();
        return;
    }
    
    // Broadcast InProgress status for each skill before activation
    UMATempDataManager* TempDataMgr = GetTempDataManager();
    if (TempDataMgr)
    {
        for (const auto& Pair : ValidCommands)
        {
            const FMAAgentSkillCommand* Cmd = Pair.Value;
            TempDataMgr->BroadcastSkillStatusUpdate(CurrentTimeStep, Cmd->AgentId, ESkillExecutionStatus::InProgress);
        }
    }
    else
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("TempDataManager not available, cannot broadcast InProgress status"));
    }
    
    // 绑定委托并激活技能
    for (const auto& Pair : ValidCommands)
    {
        AMACharacter* Agent = Pair.Key;
        const FMAAgentSkillCommand* Cmd = Pair.Value;
        EMACommand Command = StringToCommand(Cmd->SkillName);
        
        UE_LOG(LogMACommandManager, Log, TEXT("    %s -> %s"), *Cmd->AgentId, *Cmd->SkillName);
        
        if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
        {
            SkillComp->OnSkillCompleted.AddDynamic(this, &UMACommandManager::OnSkillCompleted);
            CurrentTimeStepCommands.Add(Agent, Command);
            PendingSkillCount++;
        }
    }
    
    for (const auto& Pair : ValidCommands)
    {
        AMACharacter* Agent = Pair.Key;
        const FMAAgentSkillCommand* Cmd = Pair.Value;
        EMACommand Command = StringToCommand(Cmd->SkillName);
        SendCommandToAgent(Agent, Command, Cmd);
    }
}

void UMACommandManager::OnSkillCompleted(AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    if (!Agent || !bIsExecuting) return;
    
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
    {
        SkillComp->OnSkillCompleted.RemoveDynamic(this, &UMACommandManager::OnSkillCompleted);
    }
    
    EMACommand Command = EMACommand::None;
    if (EMACommand* FoundCommand = CurrentTimeStepCommands.Find(Agent))
    {
        Command = *FoundCommand;
    }
    
    // 更新场景图（在生成反馈之前）
    FMASceneGraphUpdater::UpdateAfterSkillCompletion(Agent, Command, bSuccess);
    
    // 生成反馈
    FMASkillExecutionFeedback Feedback = FMAFeedbackGenerator::Generate(Agent, Command, bSuccess, Message);
    CurrentTimeStepFeedback.SkillFeedbacks.Add(Feedback);
    
    if (bSuccess)
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("    [SUCCESS] %s [%s]: %s"), 
            *Agent->AgentID, *Feedback.SkillName, *Feedback.Message);
    }
    else
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("    [FAILED] %s [%s]: %s"), 
            *Agent->AgentID, *Feedback.SkillName, *Feedback.Message);
    }
    
    PendingSkillCount--;
    if (PendingSkillCount <= 0)
    {
        SendTimeStepFeedbackToPython(CurrentTimeStepFeedback);
        AllFeedbacks.Add(CurrentTimeStepFeedback);
        
        // 广播时间步完成事件
        OnTimeStepCompleted.Broadcast(CurrentTimeStepFeedback);
        
        CurrentTimeStep++;
        ExecuteCurrentTimeStep();
    }
}

// ========== 核心接口 ==========

void UMACommandManager::SendCommand(AMACharacter* Agent, EMACommand Command)
{
    SendCommandToAgent(Agent, Command, nullptr);
}

void UMACommandManager::SendCommandToAgent(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent) return;
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp) return;
    
    // 1) 参数预处理
    FMASkillParamsProcessor::Process(Agent, Command, Cmd);
    
    // 2) 清除旧命令 Tag（旧技能的 EndAbility 检测不到 Tag，不会通知完成）
    SkillComp->ClearAllCommands();
    
    // 3) 先设置新命令 Tag（确保技能激活后立即失败时也能正确通知）
    FGameplayTag CommandTag = CommandToTag(Command);
    if (CommandTag.IsValid())
    {
        SkillComp->AddLooseGameplayTag(CommandTag);
    }
    
    // 4) 激活技能（会先取消旧的同类型技能，此时 Tag 已清除，不会错误通知）
    bool bHasStateTree = HasActiveStateTree(Agent);
    bool bActivated = false;
    
    if (!bUseStateTree || !bHasStateTree)
    {
        bActivated = ActivateSkillDirectly(Agent, SkillComp, Command);
    }
    
    // 5) 如果激活失败，移除刚设置的 Tag（避免残留）
    if (!bActivated && CommandTag.IsValid())
    {
        SkillComp->RemoveLooseGameplayTag(CommandTag);
    }
}

bool UMACommandManager::HasActiveStateTree(AMACharacter* Agent) const
{
    if (!Agent) return false;
    
    UMAStateTreeComponent* MASTComp = Agent->FindComponentByClass<UMAStateTreeComponent>();
    if (MASTComp)
    {
        return MASTComp->IsStateTreeEnabled() && MASTComp->IsRunning();
    }
    
    UStateTreeComponent* STComp = Agent->FindComponentByClass<UStateTreeComponent>();
    return STComp && STComp->IsRunning();
}

bool UMACommandManager::ActivateSkillDirectly(AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command)
{
    bool bActivated = false;
    
    switch (Command)
    {
        case EMACommand::Navigate:
            bActivated = SkillComp->TryActivateNavigate(SkillComp->GetSkillParams().TargetLocation);
            break;
        case EMACommand::Follow:
            bActivated = SkillComp->TryActivateFollow(SkillComp->GetSkillParams().FollowTarget.Get(), SkillComp->GetSkillParams().FollowDistance);
            break;
        case EMACommand::Charge:
            bActivated = SkillComp->TryActivateCharge();
            break;
        case EMACommand::Search:
            bActivated = SkillComp->TryActivateSearch();
            break;
        case EMACommand::Place:
            bActivated = SkillComp->TryActivatePlace();
            break;
        case EMACommand::TakeOff:
            bActivated = SkillComp->TryActivateTakeOff();
            break;
        case EMACommand::Land:
            bActivated = SkillComp->TryActivateLand();
            break;
        case EMACommand::ReturnHome:
            bActivated = SkillComp->TryActivateReturnHome();
            break;
        case EMACommand::TakePhoto:
            bActivated = SkillComp->TryActivateTakePhoto();
            break;
        case EMACommand::Broadcast:
            bActivated = SkillComp->TryActivateBroadcast();
            break;
        case EMACommand::HandleHazard:
            bActivated = SkillComp->TryActivateHandleHazard();
            break;
        case EMACommand::Guide:
            bActivated = SkillComp->TryActivateGuide();
            break;
        case EMACommand::Idle:
            SkillComp->NotifySkillCompleted(true, TEXT("Idle state entered"));
            bActivated = true;
            break;
        default:
            SkillComp->NotifySkillCompleted(false, TEXT("Unknown command type"));
            break;
    }
    
    if (!bActivated && Command != EMACommand::Idle && Command != EMACommand::None)
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("    Failed to activate skill: %s for %s"), 
            *CommandToString(Command), *Agent->AgentName);
        SkillComp->NotifySkillCompleted(false, FString::Printf(TEXT("%s failed to activate"), *CommandToString(Command)));
    }
    
    return bActivated;
}

// ========== 辅助方法 ==========

FString UMACommandManager::CommandToString(EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Idle: return TEXT("idle");
        case EMACommand::Navigate: return TEXT("navigate");
        case EMACommand::Follow: return TEXT("follow");
        case EMACommand::Charge: return TEXT("charge");
        case EMACommand::Search: return TEXT("search");
        case EMACommand::Place: return TEXT("place");
        case EMACommand::TakeOff: return TEXT("take_off");
        case EMACommand::Land: return TEXT("land");
        case EMACommand::ReturnHome: return TEXT("return_home");
        case EMACommand::TakePhoto: return TEXT("take_photo");
        case EMACommand::Broadcast: return TEXT("broadcast");
        case EMACommand::HandleHazard: return TEXT("handle_hazard");
        case EMACommand::Guide: return TEXT("guide");
        default: return TEXT("None");
    }
}

EMACommand UMACommandManager::StringToCommand(const FString& CommandString)
{
    if (CommandString.Equals(TEXT("idle"), ESearchCase::IgnoreCase)) return EMACommand::Idle;
    if (CommandString.Equals(TEXT("navigate"), ESearchCase::IgnoreCase)) return EMACommand::Navigate;
    if (CommandString.Equals(TEXT("follow"), ESearchCase::IgnoreCase)) return EMACommand::Follow;
    if (CommandString.Equals(TEXT("charge"), ESearchCase::IgnoreCase)) return EMACommand::Charge;
    if (CommandString.Equals(TEXT("search"), ESearchCase::IgnoreCase)) return EMACommand::Search;
    if (CommandString.Equals(TEXT("place"), ESearchCase::IgnoreCase)) return EMACommand::Place;
    if (CommandString.Equals(TEXT("take_off"), ESearchCase::IgnoreCase)) return EMACommand::TakeOff;
    if (CommandString.Equals(TEXT("land"), ESearchCase::IgnoreCase)) return EMACommand::Land;
    if (CommandString.Equals(TEXT("return_home"), ESearchCase::IgnoreCase)) return EMACommand::ReturnHome;
    if (CommandString.Equals(TEXT("take_photo"), ESearchCase::IgnoreCase)) return EMACommand::TakePhoto;
    if (CommandString.Equals(TEXT("broadcast"), ESearchCase::IgnoreCase)) return EMACommand::Broadcast;
    if (CommandString.Equals(TEXT("handle_hazard"), ESearchCase::IgnoreCase)) return EMACommand::HandleHazard;
    if (CommandString.Equals(TEXT("guide"), ESearchCase::IgnoreCase)) return EMACommand::Guide;
    return EMACommand::None;
}

FGameplayTag UMACommandManager::CommandToTag(EMACommand Command) const
{
    if (const FGameplayTag* Tag = CommandTagCache.Find(Command))
    {
        return *Tag;
    }
    return FGameplayTag();
}

UMATempDataManager* UMACommandManager::GetTempDataManager() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<UMATempDataManager>();
        }
    }
    return nullptr;
}

void UMACommandManager::SendTimeStepFeedbackToPython(const FMATimeStepFeedback& Feedback)
{
    FMATimeStepFeedbackMessage CommFeedback;
    CommFeedback.TimeStep = Feedback.TimeStep;
    
    for (const FMASkillExecutionFeedback& SkillFeedback : Feedback.SkillFeedbacks)
    {
        FMASkillFeedback_Comm CommSkillFeedback;
        CommSkillFeedback.AgentId = SkillFeedback.AgentId;
        CommSkillFeedback.SkillName = SkillFeedback.SkillName;
        CommSkillFeedback.bSuccess = SkillFeedback.bSuccess;
        CommSkillFeedback.Message = SkillFeedback.Message;
        CommSkillFeedback.Data = SkillFeedback.Data;
        CommFeedback.Feedbacks.Add(CommSkillFeedback);
    }
    
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>())
        {
            CommSubsystem->SendTimeStepFeedback(CommFeedback);
        }
    }
}

void UMACommandManager::SendSkillListCompletedFeedbackToPython(bool bCompleted, bool bInterrupted, int32 CompletedSteps, int32 TotalSteps)
{
    FMASkillListCompletedMessage Message;
    Message.bCompleted = bCompleted;
    Message.bInterrupted = bInterrupted;
    Message.CompletedTimeSteps = CompletedSteps;
    Message.TotalTimeSteps = TotalSteps;
    
    if (bCompleted)
    {
        Message.Message = FString::Printf(TEXT("Skill list completed: %d/%d time steps executed successfully"), CompletedSteps, TotalSteps);
    }
    else if (bInterrupted)
    {
        Message.Message = FString::Printf(TEXT("Skill list interrupted at time step %d/%d"), CompletedSteps, TotalSteps);
    }
    else
    {
        Message.Message = FString::Printf(TEXT("Skill list execution ended: %d/%d time steps"), CompletedSteps, TotalSteps);
    }
    
    // 转换 AllFeedbacks 到通信格式
    for (const FMATimeStepFeedback& TSFeedback : AllFeedbacks)
    {
        FMATimeStepFeedbackMessage CommTSFeedback;
        CommTSFeedback.TimeStep = TSFeedback.TimeStep;
        
        for (const FMASkillExecutionFeedback& SkillFeedback : TSFeedback.SkillFeedbacks)
        {
            FMASkillFeedback_Comm CommSkillFeedback;
            CommSkillFeedback.AgentId = SkillFeedback.AgentId;
            CommSkillFeedback.SkillName = SkillFeedback.SkillName;
            CommSkillFeedback.bSuccess = SkillFeedback.bSuccess;
            CommSkillFeedback.Message = SkillFeedback.Message;
            CommSkillFeedback.Data = SkillFeedback.Data;
            CommTSFeedback.Feedbacks.Add(CommSkillFeedback);
        }
        
        Message.AllTimeStepFeedbacks.Add(CommTSFeedback);
    }
    
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>())
        {
            CommSubsystem->SendSkillListCompletedFeedback(Message);
        }
    }
    
    UE_LOG(LogMACommandManager, Log, TEXT("Sent skill list completed feedback: %s"), *Message.Message);
}
