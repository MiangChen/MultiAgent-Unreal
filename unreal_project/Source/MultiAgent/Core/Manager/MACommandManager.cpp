// MACommandManager.cpp
// ========== 指令调度层实现 ==========

#include "MACommandManager.h"
#include "MAAgentManager.h"
#include "MATempDataManager.h"
#include "MASceneGraphManager.h"
#include "../Config/MAConfigManager.h"
#include "../Comm/MACommSubsystem.h"
#include "../Comm/MACommTypes.h"
#include "../Comm/Infrastructure/Codec/MACommJsonCodec.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Skill/MASkillComponent.h"
#include "../../Agent/Skill/Utils/MASkillParamsProcessor.h"
#include "../../Agent/Skill/Utils/MAFeedbackGenerator.h"
#include "../../Agent/Skill/Utils/MASceneGraphUpdater.h"
#include "../../Agent/Skill/Utils/MAConditionChecker.h"
#include "../../Agent/Skill/Utils/MASkillTemplateRegistry.h"
#include "../../Agent/Skill/Utils/MAEventTemplateRegistry.h"
#include "../../Agent/StateTree/MAStateTreeComponent.h"
#include "../../Agent/Component/MANavigationService.h"
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
    
    // 隐藏所有 Agent 的头顶气泡（新技能列表下发时清除旧消息）
    if (UMAAgentManager* AgentMgr = GetWorld()->GetSubsystem<UMAAgentManager>())
    {
        for (AMACharacter* Agent : AgentMgr->GetAllAgents())
        {
            if (Agent) Agent->HideSpeechBubble();
        }
    }
    
    UE_LOG(LogMACommandManager, Log, TEXT("ExecuteSkillList: %d time steps"), SkillList.TotalTimeSteps);
    
    bIsExecuting = true;
    bIsPaused = false;
    CurrentTimeStep = 0;
    CurrentSkillList = SkillList;
    AllFeedbacks.Empty();
    
    StartSceneGraphSyncTimer();
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
    
    // 停止所有运行时检查定时器
    if (UWorld* World = GetWorld())
    {
        for (auto& Pair : RuntimeCheckTimers)
        {
            World->GetTimerManager().ClearTimer(Pair.Value);
        }
    }
    RuntimeCheckTimers.Empty();
    
    // 取消所有 Agent 的技能并解绑委托（低电量返航中的 Agent 保留其返航技能）
    for (const auto& Pair : CurrentTimeStepCommands)
    {
        AMACharacter* Agent = Pair.Key;
        if (Agent)
        {
            if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
            {
                SkillComp->OnSkillCompleted.RemoveDynamic(this, &UMACommandManager::OnSkillCompleted);
                if (!Agent->IsLowEnergyReturning())
                {
                    SkillComp->CancelAllSkills();
                }
            }
        }
    }
    
    // 发送技能列表中断反馈
    SendSkillListCompletedFeedbackToPython(false, true, InterruptedAtTimeStep, TotalSteps);
    
    // 重置执行状态
    CurrentTimeStepCommands.Empty();
    PendingSkillCount = 0;
    bIsPaused = false;
    bIsExecuting = false;
    StopSceneGraphSyncTimer();
}

void UMACommandManager::ExecuteCurrentTimeStep()
{
    const FMATimeStepCommands* TimeStep = MACommJsonCodec::FindSkillListTimeStep(CurrentSkillList, CurrentTimeStep);
    if (!TimeStep)
    {
        UE_LOG(LogMACommandManager, Log, TEXT("All time steps completed"));
        bIsExecuting = false;
        StopSceneGraphSyncTimer();
        
        // 发送技能列表完成反馈
        SendSkillListCompletedFeedbackToPython(true, false, CurrentSkillList.TotalTimeSteps, CurrentSkillList.TotalTimeSteps);
        
        OnSkillListCompleted.Broadcast(AllFeedbacks);
        return;
    }
    
    UE_LOG(LogMACommandManager, Log, TEXT("  TimeStep %d: %d commands"), CurrentTimeStep, TimeStep->Commands.Num());
    
    CurrentTimeStepFeedback = FMATimeStepFeedback();
    CurrentTimeStepFeedback.TimeStep = CurrentTimeStep;
    CurrentTimeStepCommands.Empty();
    PendingInfoEvents.Empty();
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
        AMACharacter* Agent = AgentManager->GetAgentByIDOrLabel(Cmd.AgentId);
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
    
    // 如果没有有效命令，进入下一时间步
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
    
    // 停止运行时检查定时器
    StopRuntimeCheckTimer(Agent);
    
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
    
    // 附加 PendingInfoEvents 到反馈 Data（预检查和运行时产生的 info 事件）
    if (TArray<FMARenderedEvent>* InfoEvents = PendingInfoEvents.Find(Agent))
    {
        if (InfoEvents->Num() > 0)
        {
            // 将 info 事件序列化为 JSON 数组字符串附加到 Data
            FString InfoEventsJson = TEXT("[");
            for (int32 i = 0; i < InfoEvents->Num(); ++i)
            {
                const FMARenderedEvent& Evt = (*InfoEvents)[i];
                if (i > 0) InfoEventsJson += TEXT(",");
                InfoEventsJson += FString::Printf(TEXT("{\"category\":\"%s\",\"type\":\"%s\",\"severity\":\"%s\",\"message\":\"%s\"}"),
                    *Evt.Category, *Evt.Type, *FMARenderedEvent::SeverityToString(Evt.Severity), *Evt.Message);
            }
            InfoEventsJson += TEXT("]");
            Feedback.Data.Add(TEXT("info_events"), InfoEventsJson);
        }
        PendingInfoEvents.Remove(Agent);
    }
    
    CurrentTimeStepFeedback.SkillFeedbacks.Add(Feedback);
    
    // 广播技能完成状态到 TempDataManager，让预览组件实时更新
    // 注意：使用 AgentLabel 而不是 AgentID，因为技能分配数据中使用的是标签名
    UMATempDataManager* TempDataMgr = GetTempDataManager();
    if (TempDataMgr)
    {
        ESkillExecutionStatus NewStatus = bSuccess ? ESkillExecutionStatus::Completed : ESkillExecutionStatus::Failed;
        TempDataMgr->BroadcastSkillStatusUpdate(CurrentTimeStep, Agent->AgentLabel, NewStatus);
    }
    
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
        // 暂停期间不推进时间步，等 Resume 时处理
        if (bIsPaused)
        {
            return;
        }

        SendTimeStepFeedbackToPython(CurrentTimeStepFeedback);
        AllFeedbacks.Add(CurrentTimeStepFeedback);
        
        // 广播时间步完成事件
        OnTimeStepCompleted.Broadcast(CurrentTimeStepFeedback);
        
        CurrentTimeStep++;
        ExecuteCurrentTimeStep();
    }
}

// ========== 暂停/恢复 ==========

void UMACommandManager::PauseExecution()
{
    if (!bIsExecuting || bIsPaused) return;

    bIsPaused = true;

    // 暂停所有运行时检查定时器
    if (UWorld* World = GetWorld())
    {
        for (auto& Pair : RuntimeCheckTimers)
        {
            World->GetTimerManager().PauseTimer(Pair.Value);
        }
    }

    // 暂停所有 Agent 的导航
    for (const auto& Pair : CurrentTimeStepCommands)
    {
        AMACharacter* Agent = Pair.Key;
        if (Agent)
        {
            if (UMANavigationService* NavService = Agent->GetNavigationService())
            {
                NavService->PauseNavigation();
            }
        }
    }

    OnExecutionPauseStateChanged.Broadcast(true);
    UE_LOG(LogMACommandManager, Log, TEXT("Execution paused at TimeStep %d"), CurrentTimeStep);

    // 暂停场景图同步定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().PauseTimer(SceneGraphSyncTimerHandle);
    }
}

void UMACommandManager::ResumeExecution()
{
    if (!bIsExecuting || !bIsPaused) return;

    bIsPaused = false;

    // 恢复所有运行时检查定时器
    if (UWorld* World = GetWorld())
    {
        for (auto& Pair : RuntimeCheckTimers)
        {
            World->GetTimerManager().UnPauseTimer(Pair.Value);
        }
    }

    // 恢复所有 Agent 的导航
    for (const auto& Pair : CurrentTimeStepCommands)
    {
        AMACharacter* Agent = Pair.Key;
        if (Agent)
        {
            if (UMANavigationService* NavService = Agent->GetNavigationService())
            {
                NavService->ResumeNavigation();
            }
        }
    }

    OnExecutionPauseStateChanged.Broadcast(false);
    UE_LOG(LogMACommandManager, Log, TEXT("Execution resumed at TimeStep %d"), CurrentTimeStep);

    // 恢复场景图同步定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().UnPauseTimer(SceneGraphSyncTimerHandle);
    }

    // 暂停期间所有技能已完成，立即推进时间步
    if (PendingSkillCount <= 0)
    {
        SendTimeStepFeedbackToPython(CurrentTimeStepFeedback);
        AllFeedbacks.Add(CurrentTimeStepFeedback);
        OnTimeStepCompleted.Broadcast(CurrentTimeStepFeedback);
        CurrentTimeStep++;
        ExecuteCurrentTimeStep();
    }
}

void UMACommandManager::TogglePauseExecution()
{
    if (bIsPaused)
    {
        ResumeExecution();
    }
    else
    {
        PauseExecution();
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
    
    // 2) 条件预检查
    UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager();
    FMAPrecheckResult PrecheckResult = FMAConditionChecker::RunPrecheck(Agent, Command, SkillComp, SceneGraphMgr);
    
    if (!PrecheckResult.bAllPassed)
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("    [PRECHECK FAILED] %s [%s]: %s"),
            *Agent->AgentID, *CommandToString(Command),
            PrecheckResult.FailedEvents.Num() > 0 ? *PrecheckResult.FailedEvents[0].Message : TEXT("Unknown"));
        HandlePrecheckFailure(Agent, Command, Cmd, PrecheckResult);
        return;
    }
    
    // 3) 存储 info 事件（技能完成后附加到反馈）
    if (PrecheckResult.InfoEvents.Num() > 0)
    {
        PendingInfoEvents.Add(Agent, PrecheckResult.InfoEvents);
    }
    
    // 4) 清除旧命令 Tag（旧技能的 EndAbility 检测不到 Tag，不会通知完成）
    SkillComp->ClearAllCommands();
    
    // 5) 激活技能（内部会先取消旧技能，此时 Tag 已清除，不会错误通知）
    bool bHasStateTree = HasActiveStateTree(Agent);
    bool bActivated = false;
    
    if (!bUseStateTree || !bHasStateTree)
    {
        bActivated = ActivateSkillDirectly(Agent, SkillComp, Command);
    }
    
    // 6) 激活成功后再设置命令 Tag（用于技能完成时的通知判断）
    FGameplayTag CommandTag = CommandToTag(Command);
    if (bActivated && CommandTag.IsValid())
    {
        SkillComp->AddLooseGameplayTag(CommandTag);
    }
    
    // 7) 启动运行时检查定时器（技能激活成功后）
    if (bActivated && Command != EMACommand::Idle && Command != EMACommand::None)
    {
        StartRuntimeCheckTimer(Agent, Command);
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
            *CommandToString(Command), *Agent->AgentLabel);
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

UMASceneGraphManager* UMACommandManager::GetSceneGraphManager() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<UMASceneGraphManager>();
        }
    }
    return nullptr;
}

void UMACommandManager::HandlePrecheckFailure(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd, const FMAPrecheckResult& Result)
{
    if (!Agent || Result.FailedEvents.Num() == 0) return;
    
    const FMARenderedEvent& FirstEvent = Result.FailedEvents[0];
    
    // 构建突发事件反馈
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = CommandToString(Command);
    Feedback.bSuccess = false;
    Feedback.Message = FirstEvent.Message;
    
    // 填充 Data TMap 中的突发事件字段
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        if (!Context.TaskId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("task_id"), Context.TaskId);
        }
    }
    Feedback.Data.Add(TEXT("robot_id"), Agent->AgentID);
    Feedback.Data.Add(TEXT("robot_label"), Agent->AgentLabel);
    Feedback.Data.Add(TEXT("is_emergency_event"), TEXT("true"));
    Feedback.Data.Add(TEXT("event_category"), FirstEvent.Category);
    Feedback.Data.Add(TEXT("event_type"), FirstEvent.Type);
    Feedback.Data.Add(TEXT("event_severity"), FMARenderedEvent::SeverityToString(FirstEvent.Severity));
    Feedback.Data.Add(TEXT("event_key"), FirstEvent.Payload.FindRef(TEXT("event_key")));
    
    // 显示头顶气泡消息
    Agent->ShowSpeechBubble(FirstEvent.Message);
    
    // 添加到当前时间步反馈
    CurrentTimeStepFeedback.SkillFeedbacks.Add(Feedback);
    
    // 广播失败状态
    UMATempDataManager* TempDataMgr = GetTempDataManager();
    if (TempDataMgr)
    {
        TempDataMgr->BroadcastSkillStatusUpdate(CurrentTimeStep, Agent->AgentID, ESkillExecutionStatus::Failed);
    }
    
    // 发送当前时间步反馈（包含突发事件）
    SendTimeStepFeedbackToPython(CurrentTimeStepFeedback);
    AllFeedbacks.Add(CurrentTimeStepFeedback);
    
    // 终止技能列表执行（内部会取消其他 Agent 的技能、发送中断反馈、重置状态）
    InterruptCurrentExecution();
}

// ========== 运行时检查 ==========

void UMACommandManager::StartRuntimeCheckTimer(AMACharacter* Agent, EMACommand Command)
{
    if (!Agent) return;
    
    // 先清除旧定时器（如果存在）
    StopRuntimeCheckTimer(Agent);
    
    UWorld* World = GetWorld();
    if (!World) return;
    
    FTimerHandle TimerHandle;
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindUObject(this, &UMACommandManager::OnRuntimeCheckTick, Agent, Command);
    
    World->GetTimerManager().SetTimer(
        TimerHandle,
        TimerDelegate,
        RuntimeCheckIntervalSec,
        true  // bLoop
    );
    
    RuntimeCheckTimers.Add(Agent, TimerHandle);
    
    UE_LOG(LogMACommandManager, Log, TEXT("    [RUNTIME CHECK] Started timer for %s [%s] (%.1fs interval)"),
        *Agent->AgentID, *CommandToString(Command), RuntimeCheckIntervalSec);
}

void UMACommandManager::StopRuntimeCheckTimer(AMACharacter* Agent)
{
    if (!Agent) return;
    
    FTimerHandle* TimerHandle = RuntimeCheckTimers.Find(Agent);
    if (!TimerHandle) return;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(*TimerHandle);
    }
    
    RuntimeCheckTimers.Remove(Agent);
    
    UE_LOG(LogMACommandManager, Log, TEXT("    [RUNTIME CHECK] Stopped timer for %s"), *Agent->AgentID);
}

void UMACommandManager::OnRuntimeCheckTick(AMACharacter* Agent, EMACommand Command)
{
    if (!Agent || !bIsExecuting) return;
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp) return;
    
    UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager();
    FMAPrecheckResult RuntimeResult = FMAConditionChecker::RunRuntimeCheck(Agent, Command, SkillComp, SceneGraphMgr);
    
    // 收集 info 事件（如 HighPriorityTargetDiscovery）到 PendingInfoEvents
    // 按 event_key 去重：运行时检查是周期性的，同一事件可能被多次检测到
    if (RuntimeResult.InfoEvents.Num() > 0)
    {
        TArray<FMARenderedEvent>& ExistingInfoEvents = PendingInfoEvents.FindOrAdd(Agent);
        
        // 构建已有事件的 event_key 集合
        TSet<FString> ExistingKeys;
        for (const FMARenderedEvent& Existing : ExistingInfoEvents)
        {
            const FString* Key = Existing.Payload.Find(TEXT("event_key"));
            if (Key && !Key->IsEmpty())
            {
                ExistingKeys.Add(*Key);
            }
        }
        
        // 仅添加尚未记录的事件
        for (const FMARenderedEvent& NewEvt : RuntimeResult.InfoEvents)
        {
            const FString* Key = NewEvt.Payload.Find(TEXT("event_key"));
            if (Key && !Key->IsEmpty())
            {
                if (!ExistingKeys.Contains(*Key))
                {
                    ExistingInfoEvents.Add(NewEvt);
                    ExistingKeys.Add(*Key);
                }
            }
            else
            {
                // 没有 event_key 的事件直接添加（兜底）
                ExistingInfoEvents.Add(NewEvt);
            }
        }
    }
    
    // 如果有 abort/soft_abort 事件，处理运行时检查失败
    if (!RuntimeResult.bAllPassed)
    {
        UE_LOG(LogMACommandManager, Warning, TEXT("    [RUNTIME CHECK FAILED] %s [%s]: %s"),
            *Agent->AgentID, *CommandToString(Command),
            RuntimeResult.FailedEvents.Num() > 0 ? *RuntimeResult.FailedEvents[0].Message : TEXT("Unknown"));
        HandleRuntimeCheckFailure(Agent, Command, RuntimeResult);
    }
}

void UMACommandManager::HandleRuntimeCheckFailure(AMACharacter* Agent, EMACommand Command, const FMAPrecheckResult& Result)
{
    if (!Agent || Result.FailedEvents.Num() == 0) return;
    
    // 防止重入：如果已经不在执行状态，忽略
    if (!bIsExecuting) return;
    
    const FMARenderedEvent& FirstEvent = Result.FailedEvents[0];
    
    // 1) 停止运行时检查定时器
    StopRuntimeCheckTimer(Agent);
    
    // 2) 取消技能列表中的技能（低电量返航是系统级技能，不取消）
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
    {
        SkillComp->OnSkillCompleted.RemoveDynamic(this, &UMACommandManager::OnSkillCompleted);
        if (!Agent->IsLowEnergyReturning())
        {
            SkillComp->CancelAllSkills();
        }
    }
    
    // 3) 构建突发事件反馈（与 HandlePrecheckFailure 格式一致）
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = CommandToString(Command);
    Feedback.bSuccess = false;
    Feedback.Message = FirstEvent.Message;
    
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        if (!Context.TaskId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("task_id"), Context.TaskId);
        }
    }
    Feedback.Data.Add(TEXT("robot_id"), Agent->AgentID);
    Feedback.Data.Add(TEXT("robot_label"), Agent->AgentLabel);
    Feedback.Data.Add(TEXT("is_emergency_event"), TEXT("true"));
    Feedback.Data.Add(TEXT("event_category"), FirstEvent.Category);
    Feedback.Data.Add(TEXT("event_type"), FirstEvent.Type);
    Feedback.Data.Add(TEXT("event_severity"), FMARenderedEvent::SeverityToString(FirstEvent.Severity));
    Feedback.Data.Add(TEXT("event_key"), FirstEvent.Payload.FindRef(TEXT("event_key")));
    
    // 显示头顶气泡消息
    Agent->ShowSpeechBubble(FirstEvent.Message);
    
    // 4) 添加到当前时间步反馈
    CurrentTimeStepFeedback.SkillFeedbacks.Add(Feedback);
    
    // 5) 广播失败状态
    UMATempDataManager* TempDataMgr = GetTempDataManager();
    if (TempDataMgr)
    {
        TempDataMgr->BroadcastSkillStatusUpdate(CurrentTimeStep, Agent->AgentID, ESkillExecutionStatus::Failed);
    }
    
    // 6) 发送当前时间步反馈并终止技能列表
    SendTimeStepFeedbackToPython(CurrentTimeStepFeedback);
    AllFeedbacks.Add(CurrentTimeStepFeedback);
    
    InterruptCurrentExecution();
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

//=============================================================================
// 场景图周期性同步
//=============================================================================

void UMACommandManager::StartSceneGraphSyncTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            SceneGraphSyncTimerHandle,
            this, &UMACommandManager::OnSceneGraphSyncTick,
            SceneGraphSyncIntervalSec,
            true
        );
    }
}

void UMACommandManager::StopSceneGraphSyncTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SceneGraphSyncTimerHandle);
    }
}

void UMACommandManager::OnSceneGraphSyncTick()
{
    FMASceneGraphUpdater::SyncDynamicNodes(GetWorld());
}
