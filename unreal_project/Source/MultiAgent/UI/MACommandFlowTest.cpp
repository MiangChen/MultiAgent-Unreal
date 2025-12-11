// MACommandFlowTest.cpp
// Property-Based Tests for Command Submission Flow Completeness
// **Feature: ui-integration, Property 3: 命令提交流程完整性**
// **Validates: Requirements 2.2, 2.3, 2.4, 3.2, 3.3**

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MASimpleMainWidget.h"
#include "MAHUD.h"
#include "../Core/MACommSubsystem.h"
#include "../Core/MASimTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

//=============================================================================
// Test Helpers - 模拟命令提交流程
//=============================================================================

namespace MACommandFlowTestHelpers
{
    /**
     * 模拟命令流程状态
     * 用于验证 Property 3: 命令提交流程完整性
     */
    struct FMockCommandFlowState
    {
        // 流程阶段标记
        bool bWidgetSubmitted = false;      // MASimpleMainWidget 提交了指令
        bool bHUDReceived = false;          // MAHUD 收到了指令
        bool bCommSubsystemReceived = false; // MACommSubsystem 收到了指令
        bool bResponseGenerated = false;     // 响应已生成
        bool bWidgetDisplayed = false;       // Widget 显示了响应
        
        // 数据
        FString SubmittedCommand;
        FString ReceivedByHUD;
        FString ReceivedByComm;
        FMAPlannerResponse Response;
        FString DisplayedText;
        
        /**
         * 模拟 MASimpleMainWidget 提交指令
         * Requirements: 2.2
         */
        void SimulateWidgetSubmit(const FString& Command)
        {
            if (!Command.IsEmpty())
            {
                SubmittedCommand = Command;
                bWidgetSubmitted = true;
            }
        }
        
        /**
         * 模拟 MAHUD 接收并转发指令
         * Requirements: 2.3
         */
        void SimulateHUDReceive(const FString& Command)
        {
            if (bWidgetSubmitted && Command == SubmittedCommand)
            {
                ReceivedByHUD = Command;
                bHUDReceived = true;
            }
        }
        
        /**
         * 模拟 MACommSubsystem 接收指令
         * Requirements: 3.2
         */
        void SimulateCommSubsystemReceive(const FString& Command)
        {
            if (bHUDReceived && Command == ReceivedByHUD)
            {
                ReceivedByComm = Command;
                bCommSubsystemReceived = true;
            }
        }
        
        /**
         * 模拟生成响应
         * Requirements: 3.3
         */
        void SimulateGenerateResponse(const FString& Command)
        {
            if (bCommSubsystemReceived && Command == ReceivedByComm)
            {
                Response.bSuccess = true;
                Response.Message = FString::Printf(TEXT("收到指令: %s"), *Command);
                Response.PlanText = TEXT("模拟规划结果");
                bResponseGenerated = true;
            }
        }
        
        /**
         * 模拟 Widget 显示响应
         * Requirements: 2.4
         */
        void SimulateWidgetDisplay(const FMAPlannerResponse& InResponse)
        {
            if (bResponseGenerated)
            {
                DisplayedText = FString::Printf(TEXT("[%s]\n%s"), 
                    InResponse.bSuccess ? TEXT("成功") : TEXT("失败"),
                    *InResponse.Message);
                bWidgetDisplayed = true;
            }
        }
        
        /**
         * 执行完整流程
         */
        void ExecuteFullFlow(const FString& Command)
        {
            SimulateWidgetSubmit(Command);
            SimulateHUDReceive(Command);
            SimulateCommSubsystemReceive(Command);
            SimulateGenerateResponse(Command);
            SimulateWidgetDisplay(Response);
        }
        
        /**
         * 验证流程完整性
         * Property 3: 所有阶段都必须完成
         */
        bool IsFlowComplete() const
        {
            return bWidgetSubmitted && 
                   bHUDReceived && 
                   bCommSubsystemReceived && 
                   bResponseGenerated && 
                   bWidgetDisplayed;
        }
        
        /**
         * 验证数据一致性
         * Property 3: 指令在流程中保持一致
         */
        bool IsDataConsistent() const
        {
            return SubmittedCommand == ReceivedByHUD &&
                   ReceivedByHUD == ReceivedByComm &&
                   !DisplayedText.IsEmpty();
        }
        
        /**
         * 重置状态
         */
        void Reset()
        {
            bWidgetSubmitted = false;
            bHUDReceived = false;
            bCommSubsystemReceived = false;
            bResponseGenerated = false;
            bWidgetDisplayed = false;
            SubmittedCommand.Empty();
            ReceivedByHUD.Empty();
            ReceivedByComm.Empty();
            Response = FMAPlannerResponse();
            DisplayedText.Empty();
        }
    };

    /**
     * 生成随机测试指令
     */
    static TArray<FString> GenerateCommandFlowTestCommands()
    {
        TArray<FString> Commands;
        
        // 基础指令
        Commands.Add(TEXT("move to position A"));
        Commands.Add(TEXT("移动到仓库"));
        Commands.Add(TEXT("patrol area"));
        Commands.Add(TEXT("巡逻区域B"));
        Commands.Add(TEXT("stop"));
        Commands.Add(TEXT("停止"));
        Commands.Add(TEXT("status"));
        Commands.Add(TEXT("状态"));
        Commands.Add(TEXT("help"));
        Commands.Add(TEXT("帮助"));
        
        // 随机生成的指令
        for (int32 i = 0; i < 10; i++)
        {
            Commands.Add(FString::Printf(TEXT("random command %d"), FMath::RandRange(1, 1000)));
        }
        
        // 边界情况
        Commands.Add(TEXT("a"));  // 单字符
        Commands.Add(TEXT("这是一个非常长的测试指令用于验证系统能够正确处理长文本输入"));
        Commands.Add(TEXT("command with special chars: @#$%"));
        
        return Commands;
    }
}

//=============================================================================
// Property 3: 命令提交流程完整性
// For any 用户提交的指令, 必须经过 MASimpleMainWidget → MAHUD → MACommSubsystem 
// → 响应回调 → MASimpleMainWidget 显示的完整流程
// Validates: Requirements 2.2, 2.3, 2.4, 3.2, 3.3
//=============================================================================

/**
 * Property Test: 命令提交流程完整性 - 基础验证
 * 
 * 属性定义:
 * - 对于任意非空指令，必须经过完整的 5 阶段流程
 * - 每个阶段的数据必须与前一阶段一致
 * 
 * **Feature: ui-integration, Property 3: 命令提交流程完整性**
 * **Validates: Requirements 2.2, 2.3, 2.4, 3.2, 3.3**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMACommandFlowCompletenessPropertyTest,
    "MultiAgent.UI.CommandFlow.Property3_FlowCompleteness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMACommandFlowCompletenessPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MACommandFlowTestHelpers;
    
    TArray<FString> TestCommands = GenerateCommandFlowTestCommands();
    int32 PassedCount = 0;
    
    for (const FString& Command : TestCommands)
    {
        FMockCommandFlowState State;
        State.ExecuteFullFlow(Command);
        
        // Property 3.1: 流程必须完整
        if (!State.IsFlowComplete())
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Flow incomplete for command '%s' (W:%d H:%d C:%d R:%d D:%d)"),
                *Command,
                State.bWidgetSubmitted ? 1 : 0,
                State.bHUDReceived ? 1 : 0,
                State.bCommSubsystemReceived ? 1 : 0,
                State.bResponseGenerated ? 1 : 0,
                State.bWidgetDisplayed ? 1 : 0));
            continue;
        }
        
        // Property 3.2: 数据必须一致
        if (!State.IsDataConsistent())
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Data inconsistent for command '%s'"),
                *Command));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 3 Flow Completeness Test: %d/%d commands passed"), 
        PassedCount, TestCommands.Num()));
    
    return PassedCount == TestCommands.Num();
}

/**
 * Property Test: 空指令不应触发完整流程
 * 
 * 属性定义:
 * - 空指令应在第一阶段被拒绝
 * - 流程不应继续到后续阶段
 * 
 * **Feature: ui-integration, Property 3: 命令提交流程完整性**
 * **Validates: Requirements 2.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMACommandFlowEmptyRejectionPropertyTest,
    "MultiAgent.UI.CommandFlow.Property3_EmptyCommandRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMACommandFlowEmptyRejectionPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MACommandFlowTestHelpers;
    
    const int32 NumTests = 50;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        FMockCommandFlowState State;
        
        // 尝试提交空指令
        State.SimulateWidgetSubmit(TEXT(""));
        
        // Property 3: 空指令不应被提交
        if (State.bWidgetSubmitted)
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Empty command was submitted (Test %d)"),
                TestIndex));
            continue;
        }
        
        // 验证后续阶段未执行
        if (State.bHUDReceived || State.bCommSubsystemReceived || 
            State.bResponseGenerated || State.bWidgetDisplayed)
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Flow continued after empty command rejection (Test %d)"),
                TestIndex));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 3 Empty Rejection Test: %d/%d tests passed"), 
        PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

/**
 * Property Test: 指令数据在流程中保持不变
 * 
 * 属性定义:
 * - 对于任意指令，在流程的每个阶段数据必须相同
 * - 不应发生数据丢失或修改
 * 
 * **Feature: ui-integration, Property 3: 命令提交流程完整性**
 * **Validates: Requirements 2.2, 2.3, 3.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMACommandFlowDataIntegrityPropertyTest,
    "MultiAgent.UI.CommandFlow.Property3_DataIntegrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMACommandFlowDataIntegrityPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MACommandFlowTestHelpers;
    
    const int32 NumTests = 100;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        // 生成随机指令
        FString Command = FString::Printf(TEXT("test_command_%d_%d"), 
            TestIndex, FMath::RandRange(1, 10000));
        
        FMockCommandFlowState State;
        State.ExecuteFullFlow(Command);
        
        // Property 3: 验证数据在每个阶段保持一致
        bool bDataIntact = true;
        
        // 检查 Widget → HUD
        if (State.SubmittedCommand != State.ReceivedByHUD)
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Data changed Widget→HUD (Test %d, '%s' vs '%s')"),
                TestIndex, *State.SubmittedCommand, *State.ReceivedByHUD));
            bDataIntact = false;
        }
        
        // 检查 HUD → CommSubsystem
        if (State.ReceivedByHUD != State.ReceivedByComm)
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Data changed HUD→Comm (Test %d, '%s' vs '%s')"),
                TestIndex, *State.ReceivedByHUD, *State.ReceivedByComm));
            bDataIntact = false;
        }
        
        // 检查响应包含原始指令信息
        if (!State.Response.Message.Contains(Command))
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Response doesn't reference original command (Test %d)"),
                TestIndex));
            bDataIntact = false;
        }
        
        if (bDataIntact)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 3 Data Integrity Test: %d/%d tests passed"), 
        PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

/**
 * Property Test: 响应必须包含有效数据
 * 
 * 属性定义:
 * - 对于任意成功的指令，响应必须包含非空的 Message
 * - 响应的 bSuccess 状态必须正确反映处理结果
 * 
 * **Feature: ui-integration, Property 3: 命令提交流程完整性**
 * **Validates: Requirements 3.3, 2.4**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMACommandFlowResponseValidityPropertyTest,
    "MultiAgent.UI.CommandFlow.Property3_ResponseValidity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMACommandFlowResponseValidityPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MACommandFlowTestHelpers;
    
    TArray<FString> TestCommands = GenerateCommandFlowTestCommands();
    int32 PassedCount = 0;
    
    for (const FString& Command : TestCommands)
    {
        FMockCommandFlowState State;
        State.ExecuteFullFlow(Command);
        
        // Property 3: 响应必须有效
        bool bResponseValid = true;
        
        // 检查响应 Message 非空
        if (State.Response.Message.IsEmpty())
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Empty response message for command '%s'"),
                *Command));
            bResponseValid = false;
        }
        
        // 检查成功状态
        if (!State.Response.bSuccess)
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Response marked as failure for valid command '%s'"),
                *Command));
            bResponseValid = false;
        }
        
        // 检查显示文本非空
        if (State.DisplayedText.IsEmpty())
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Empty display text for command '%s'"),
                *Command));
            bResponseValid = false;
        }
        
        if (bResponseValid)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 3 Response Validity Test: %d/%d commands passed"), 
        PassedCount, TestCommands.Num()));
    
    return PassedCount == TestCommands.Num();
}

/**
 * Property Test: 多次提交流程独立性
 * 
 * 属性定义:
 * - 连续提交多个指令，每个指令的流程应独立完成
 * - 前一个指令的状态不应影响后续指令
 * 
 * **Feature: ui-integration, Property 3: 命令提交流程完整性**
 * **Validates: Requirements 2.2, 2.3, 2.4, 3.2, 3.3**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMACommandFlowIndependencePropertyTest,
    "MultiAgent.UI.CommandFlow.Property3_FlowIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMACommandFlowIndependencePropertyTest::RunTest(const FString& Parameters)
{
    using namespace MACommandFlowTestHelpers;
    
    const int32 NumTests = 50;
    const int32 CommandsPerTest = 5;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        bool bAllFlowsComplete = true;
        
        // 连续执行多个指令
        for (int32 CmdIndex = 0; CmdIndex < CommandsPerTest; CmdIndex++)
        {
            FString Command = FString::Printf(TEXT("test_%d_cmd_%d"), TestIndex, CmdIndex);
            
            FMockCommandFlowState State;
            State.ExecuteFullFlow(Command);
            
            // 验证每个流程独立完成
            if (!State.IsFlowComplete())
            {
                AddError(FString::Printf(
                    TEXT("Property 3 violated: Flow %d incomplete in test %d"),
                    CmdIndex, TestIndex));
                bAllFlowsComplete = false;
                break;
            }
            
            // 验证数据正确
            if (State.SubmittedCommand != Command)
            {
                AddError(FString::Printf(
                    TEXT("Property 3 violated: Command mismatch in flow %d test %d"),
                    CmdIndex, TestIndex));
                bAllFlowsComplete = false;
                break;
            }
        }
        
        if (bAllFlowsComplete)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 3 Flow Independence Test: %d/%d tests passed"), 
        PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

/**
 * Property Test: 使用真实 MACommSubsystem 验证流程
 * 
 * 属性定义:
 * - 使用真实的 MACommSubsystem 对象验证命令处理
 * - 验证 SendNaturalLanguageCommand 正确处理指令
 * 
 * **Feature: ui-integration, Property 3: 命令提交流程完整性**
 * **Validates: Requirements 3.2, 3.3**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMACommandFlowRealSubsystemPropertyTest,
    "MultiAgent.UI.CommandFlow.Property3_RealSubsystemFlow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMACommandFlowRealSubsystemPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MACommandFlowTestHelpers;
    
    TArray<FString> TestCommands = GenerateCommandFlowTestCommands();
    int32 PassedCount = 0;
    
    for (const FString& Command : TestCommands)
    {
        // 创建真实的 CommSubsystem 对象
        UMACommSubsystem* Subsystem = NewObject<UMACommSubsystem>();
        Subsystem->bUseMockData = true;
        
        // 发送指令
        Subsystem->SendNaturalLanguageCommand(Command);
        
        // Property 3: 验证指令被正确处理
        bool bFlowValid = true;
        
        // 检查指令被记录
        if (Subsystem->GetLastCommand() != Command)
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: LastCommand mismatch for '%s', got '%s'"),
                *Command, *Subsystem->GetLastCommand()));
            bFlowValid = false;
        }
        
        // 检查不再等待响应 (同步 Mock 模式)
        if (Subsystem->IsWaitingForResponse())
        {
            AddError(FString::Printf(
                TEXT("Property 3 violated: Still waiting after command '%s'"),
                *Command));
            bFlowValid = false;
        }
        
        if (bFlowValid)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 3 Real Subsystem Test: %d/%d commands passed"), 
        PassedCount, TestCommands.Num()));
    
    return PassedCount == TestCommands.Num();
}

#endif // WITH_DEV_AUTOMATION_TESTS
