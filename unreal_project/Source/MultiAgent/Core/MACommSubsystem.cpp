// MACommSubsystem.cpp
// 通信子系统实现
// Requirements: 4.1, 4.2, 4.3, 4.4

#include "MACommSubsystem.h"
#include "MAGameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommSubsystem, Log, All);

//=============================================================================
// 生命周期
//=============================================================================

void UMACommSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 从 GameInstance 获取配置
    // Requirements: 4.1 - 作为 GameInstanceSubsystem 在游戏生命周期内存在
    if (UMAGameInstance* GameInstance = Cast<UMAGameInstance>(GetGameInstance()))
    {
        ServerURL = GameInstance->PlannerServerURL;
        bUseMockData = GameInstance->bUseMockData;
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("MACommSubsystem initialized"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  ServerURL: %s"), *ServerURL);
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
}

void UMACommSubsystem::Deinitialize()
{
    UE_LOG(LogMACommSubsystem, Log, TEXT("MACommSubsystem deinitializing"));
    
    // 清理状态
    bWaitingForResponse = false;
    LastCommand.Empty();
    
    Super::Deinitialize();
}

//=============================================================================
// 命令发送
//=============================================================================

void UMACommSubsystem::SendNaturalLanguageCommand(const FString& Command)
{
    // Requirements: 4.2 - 收到自然语言指令时发送给规划器或生成模拟响应
    
    if (Command.IsEmpty())
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("SendNaturalLanguageCommand: Empty command ignored"));
        
        // 广播失败响应
        BroadcastResponse(FMAPlannerResponse::Failure(TEXT("指令不能为空")));
        return;
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("SendNaturalLanguageCommand: %s"), *Command);
    
    // 保存指令并设置等待状态
    LastCommand = Command;
    bWaitingForResponse = true;

    if (bUseMockData)
    {
        // Requirements: 4.4 - bUseMockData 为 true 时生成模拟数据
        GenerateMockPlanResponse(Command);
    }
    else
    {
        // TODO: 实现真实的 HTTP 请求到规划器服务器
        // 当前回退到模拟数据
        UE_LOG(LogMACommSubsystem, Warning, TEXT("Real HTTP communication not yet implemented, using mock data"));
        GenerateMockPlanResponse(Command);
    }
}

//=============================================================================
// 内部方法
//=============================================================================

void UMACommSubsystem::GenerateMockPlanResponse(const FString& UserCommand)
{
    // Requirements: 4.4 - 生成模拟数据用于开发测试
    
    FMAPlannerResponse Response;
    Response.bSuccess = true;
    
    // 根据指令内容生成不同的模拟响应
    FString LowerCommand = UserCommand.ToLower();
    
    if (LowerCommand.Contains(TEXT("move")) || LowerCommand.Contains(TEXT("移动")) || LowerCommand.Contains(TEXT("go")))
    {
        Response.Message = TEXT("移动指令已接收");
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("1. 解析目标位置\n")
            TEXT("2. 计算最优路径\n")
            TEXT("3. 执行移动任务\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("patrol")) || LowerCommand.Contains(TEXT("巡逻")))
    {
        Response.Message = TEXT("巡逻指令已接收");
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("1. 设置巡逻路径点\n")
            TEXT("2. 配置巡逻参数\n")
            TEXT("3. 开始循环巡逻\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("stop")) || LowerCommand.Contains(TEXT("停止")))
    {
        Response.Message = TEXT("停止指令已接收");
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("1. 中断当前任务\n")
            TEXT("2. 停止移动\n")
            TEXT("3. 进入待命状态\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("status")) || LowerCommand.Contains(TEXT("状态")))
    {
        Response.Message = TEXT("状态查询已处理");
        Response.PlanText = FString::Printf(
            TEXT("系统状态:\n")
            TEXT("- 通信状态: 正常\n")
            TEXT("- 模拟模式: 已启用\n")
            TEXT("- 服务器: %s\n")
            TEXT("\n原始指令: %s"), *ServerURL, *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("help")) || LowerCommand.Contains(TEXT("帮助")))
    {
        Response.Message = TEXT("帮助信息");
        Response.PlanText = TEXT(
            "可用指令:\n"
            "- move/移动 [目标]: 移动到指定位置\n"
            "- patrol/巡逻: 开始巡逻任务\n"
            "- stop/停止: 停止当前任务\n"
            "- status/状态: 查询系统状态\n"
            "- help/帮助: 显示帮助信息");
    }
    else
    {
        // 默认响应
        Response.Message = FString::Printf(TEXT("收到指令: %s"), *UserCommand);
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("指令已记录，等待进一步处理。\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }

    // 广播响应
    BroadcastResponse(Response);

    UE_LOG(LogMACommSubsystem, Log, TEXT("Mock response generated - Success: %s, Message: %s"), 
        Response.bSuccess ? TEXT("true") : TEXT("false"), *Response.Message);
}

void UMACommSubsystem::BroadcastResponse(const FMAPlannerResponse& Response)
{
    // Requirements: 4.3 - 通过委托广播响应给订阅者
    
    bWaitingForResponse = false;
    
    // 广播给所有订阅者
    OnPlannerResponse.Broadcast(Response);
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("Response broadcasted to subscribers"));
}
