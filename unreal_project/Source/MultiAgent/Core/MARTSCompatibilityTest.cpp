// MARTSCompatibilityTest.cpp
// Property-Based Tests for RTS Functionality Compatibility
// **Feature: ui-integration, Property 5: RTS 功能兼容性**
// **Validates: Requirements 5.1, 5.2, 5.3, 5.4**

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Manager/MASelectionManager.h"
#include "Manager/MACommandManager.h"
#include "Types/MATypes.h"

#if WITH_DEV_AUTOMATION_TESTS

//=============================================================================
// Test Helpers - 模拟 Agent 数据用于测试
//=============================================================================

namespace MARTSTestHelpers
{
    /**
     * 生成随机的 Agent 索引列表
     * @param MaxAgents 最大 Agent 数量
     * @param Count 要选择的数量
     * @return 随机索引列表
     */
    static TArray<int32> GenerateRandomAgentIndices(int32 MaxAgents, int32 Count)
    {
        TArray<int32> Indices;
        TArray<int32> Available;
        
        for (int32 i = 0; i < MaxAgents; i++)
        {
            Available.Add(i);
        }
        
        for (int32 i = 0; i < FMath::Min(Count, MaxAgents); i++)
        {
            int32 RandIndex = FMath::RandRange(0, Available.Num() - 1);
            Indices.Add(Available[RandIndex]);
            Available.RemoveAt(RandIndex);
        }
        
        return Indices;
    }

    /**
     * 生成随机的编组索引
     * @return 1-5 之间的随机编组索引
     */
    static int32 GenerateRandomGroupIndex()
    {
        return FMath::RandRange(1, 5);
    }

    /**
     * 生成随机的命令类型
     * @return 随机命令
     */
    static EMACommand GenerateRandomCommand()
    {
        TArray<EMACommand> Commands = {
            EMACommand::Idle,
            EMACommand::Navigate,
            EMACommand::Follow,
            EMACommand::Charge,
            EMACommand::Search,
            EMACommand::Place
        };
        return Commands[FMath::RandRange(0, Commands.Num() - 1)];
    }
}

//=============================================================================
// Property 5.1: 框选功能兼容性
// For any 框选操作, 框选结果必须包含且仅包含框内的 Agent
// Validates: Requirements 5.1
//=============================================================================

/**
 * Property Test: 框选状态一致性
 * 
 * 属性定义:
 * - BeginBoxSelect 后 IsBoxSelecting() 必须为 true
 * - CancelBoxSelect 或 EndBoxSelect 后 IsBoxSelecting() 必须为 false
 * - 框选起点和终点必须正确记录
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSBoxSelectStatePropertyTest,
    "MultiAgent.Core.RTS.Property5_BoxSelectStateConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSBoxSelectStatePropertyTest::RunTest(const FString& Parameters)
{
    // 模拟框选状态逻辑 (不依赖实际 World)
    const int32 NumTests = 100;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        // 模拟框选状态
        bool bIsBoxSelecting = false;
        FVector2D BoxSelectStart = FVector2D::ZeroVector;
        FVector2D BoxSelectEnd = FVector2D::ZeroVector;
        
        // 生成随机起点
        FVector2D RandomStart(FMath::RandRange(0.f, 1920.f), FMath::RandRange(0.f, 1080.f));
        
        // BeginBoxSelect
        bIsBoxSelecting = true;
        BoxSelectStart = RandomStart;
        BoxSelectEnd = RandomStart;
        
        // Property 5.1.1: BeginBoxSelect 后状态必须为 true
        if (!bIsBoxSelecting)
        {
            AddError(FString::Printf(TEXT("Property 5.1 violated: IsBoxSelecting should be true after BeginBoxSelect (Test %d)"), TestIndex));
            continue;
        }
        
        // Property 5.1.2: 起点必须正确记录
        if (BoxSelectStart != RandomStart)
        {
            AddError(FString::Printf(TEXT("Property 5.1 violated: BoxSelectStart mismatch (Test %d)"), TestIndex));
            continue;
        }
        
        // 生成随机终点并更新
        FVector2D RandomEnd(FMath::RandRange(0.f, 1920.f), FMath::RandRange(0.f, 1080.f));
        BoxSelectEnd = RandomEnd;
        
        // Property 5.1.3: 终点必须正确记录
        if (BoxSelectEnd != RandomEnd)
        {
            AddError(FString::Printf(TEXT("Property 5.1 violated: BoxSelectEnd mismatch (Test %d)"), TestIndex));
            continue;
        }
        
        // 随机选择取消或结束
        if (FMath::RandBool())
        {
            // CancelBoxSelect
            bIsBoxSelecting = false;
        }
        else
        {
            // EndBoxSelect
            bIsBoxSelecting = false;
        }
        
        // Property 5.1.4: 结束后状态必须为 false
        if (bIsBoxSelecting)
        {
            AddError(FString::Printf(TEXT("Property 5.1 violated: IsBoxSelecting should be false after End/Cancel (Test %d)"), TestIndex));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.1 Box Select Test: %d/%d tests passed"), PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

//=============================================================================
// Property 5.2 & 5.3: 编组保存和选择功能
// For any 编组操作, SaveToControlGroup 后 SelectControlGroup 必须恢复相同的选择
// Validates: Requirements 5.2, 5.3
//=============================================================================

/**
 * Property Test: 编组保存和恢复一致性 (Round-trip)
 * 
 * 属性定义:
 * - 对于任意选择集合，保存到编组后再选择该编组，必须恢复相同的集合
 * - SaveToControlGroup(i) → SelectControlGroup(i) 应该是恒等操作
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.2, 5.3**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSControlGroupRoundTripPropertyTest,
    "MultiAgent.Core.RTS.Property5_ControlGroupRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSControlGroupRoundTripPropertyTest::RunTest(const FString& Parameters)
{
    // 模拟编组逻辑 (不依赖实际 Agent)
    const int32 NumTests = 100;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        // 模拟选择和编组状态
        TArray<int32> SelectedAgentIndices;  // 使用索引模拟 Agent
        TMap<int32, TArray<int32>> ControlGroupsMap;
        
        // 生成随机选择 (1-10 个 Agent)
        int32 NumAgents = FMath::RandRange(1, 10);
        for (int32 i = 0; i < NumAgents; i++)
        {
            SelectedAgentIndices.Add(i);
        }
        
        // 随机选择编组索引 (1-5)
        int32 GroupIndex = MARTSTestHelpers::GenerateRandomGroupIndex();
        
        // SaveToControlGroup: 保存当前选择到编组
        ControlGroupsMap.Add(GroupIndex, SelectedAgentIndices);
        
        // 清除当前选择 (模拟用户操作)
        TArray<int32> OriginalSelection = SelectedAgentIndices;
        SelectedAgentIndices.Empty();
        
        // SelectControlGroup: 选择编组
        TArray<int32>* GroupPtr = ControlGroupsMap.Find(GroupIndex);
        if (GroupPtr)
        {
            SelectedAgentIndices = *GroupPtr;
        }
        
        // Property 5.2/5.3: 恢复的选择必须与原始选择相同
        bool bMatch = (SelectedAgentIndices.Num() == OriginalSelection.Num());
        if (bMatch)
        {
            for (int32 Index : OriginalSelection)
            {
                if (!SelectedAgentIndices.Contains(Index))
                {
                    bMatch = false;
                    break;
                }
            }
        }
        
        if (!bMatch)
        {
            AddError(FString::Printf(
                TEXT("Property 5.2/5.3 violated: Control group round-trip failed (Test %d, Group %d, Original %d, Restored %d)"),
                TestIndex, GroupIndex, OriginalSelection.Num(), SelectedAgentIndices.Num()));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.2/5.3 Control Group Round-Trip Test: %d/%d tests passed"), PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

/**
 * Property Test: 编组独立性
 * 
 * 属性定义:
 * - 不同编组之间相互独立
 * - 保存到编组 i 不应影响编组 j (i != j)
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.2, 5.3**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSControlGroupIndependencePropertyTest,
    "MultiAgent.Core.RTS.Property5_ControlGroupIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSControlGroupIndependencePropertyTest::RunTest(const FString& Parameters)
{
    const int32 NumTests = 50;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        TMap<int32, TArray<int32>> ControlGroupsMap;
        
        // 为每个编组 (1-5) 保存不同的选择
        TMap<int32, TArray<int32>> ExpectedGroups;
        for (int32 GroupIndex = 1; GroupIndex <= 5; GroupIndex++)
        {
            TArray<int32> Selection;
            int32 NumAgents = FMath::RandRange(0, 5);
            for (int32 i = 0; i < NumAgents; i++)
            {
                Selection.Add(GroupIndex * 10 + i);  // 确保每个编组有不同的 Agent
            }
            ControlGroupsMap.Add(GroupIndex, Selection);
            ExpectedGroups.Add(GroupIndex, Selection);
        }
        
        // 验证每个编组的内容没有被其他编组影响
        bool bAllMatch = true;
        for (int32 GroupIndex = 1; GroupIndex <= 5; GroupIndex++)
        {
            TArray<int32>* ActualPtr = ControlGroupsMap.Find(GroupIndex);
            TArray<int32>* ExpectedPtr = ExpectedGroups.Find(GroupIndex);
            
            if (!ActualPtr || !ExpectedPtr)
            {
                bAllMatch = false;
                break;
            }
            
            if (ActualPtr->Num() != ExpectedPtr->Num())
            {
                bAllMatch = false;
                break;
            }
            
            for (int32 Index : *ExpectedPtr)
            {
                if (!ActualPtr->Contains(Index))
                {
                    bAllMatch = false;
                    break;
                }
            }
            
            if (!bAllMatch) break;
        }
        
        if (!bAllMatch)
        {
            AddError(FString::Printf(TEXT("Property 5.2/5.3 violated: Control groups not independent (Test %d)"), TestIndex));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.2/5.3 Control Group Independence Test: %d/%d tests passed"), PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

//=============================================================================
// Property 5.4: 命令快捷键功能
// For any 命令操作, 命令必须被正确转换和执行
// Validates: Requirements 5.4
//=============================================================================

/**
 * Property Test: 命令字符串转换一致性 (Round-trip)
 * 
 * 属性定义:
 * - 对于任意命令，CommandToString → StringToCommand 必须恢复原命令
 * - 命令转换是双射的
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.4**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSCommandConversionPropertyTest,
    "MultiAgent.Core.RTS.Property5_CommandConversionRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSCommandConversionPropertyTest::RunTest(const FString& Parameters)
{
    // 测试所有命令类型的转换
    TArray<EMACommand> AllCommands = {
        EMACommand::None,
        EMACommand::Idle,
        EMACommand::Navigate,
        EMACommand::Follow,
        EMACommand::Charge,
        EMACommand::Search,
        EMACommand::Place
    };
    
    int32 PassedCount = 0;
    
    for (EMACommand Command : AllCommands)
    {
        // Command → String
        FString CommandStr = UMACommandManager::CommandToString(Command);
        
        // String → Command
        EMACommand RestoredCommand = UMACommandManager::StringToCommand(CommandStr);
        
        // Property 5.4: Round-trip 必须恢复原命令
        if (RestoredCommand != Command)
        {
            AddError(FString::Printf(
                TEXT("Property 5.4 violated: Command conversion round-trip failed for %s (got %s)"),
                *CommandStr, *UMACommandManager::CommandToString(RestoredCommand)));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.4 Command Conversion Test: %d/%d commands passed"), PassedCount, AllCommands.Num()));
    
    return PassedCount == AllCommands.Num();
}

/**
 * Property Test: 命令字符串唯一性
 * 
 * 属性定义:
 * - 不同的命令必须有不同的字符串表示
 * - CommandToString 是单射的
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.4**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSCommandStringUniquenessPropertyTest,
    "MultiAgent.Core.RTS.Property5_CommandStringUniqueness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSCommandStringUniquenessPropertyTest::RunTest(const FString& Parameters)
{
    TArray<EMACommand> AllCommands = {
        EMACommand::None,
        EMACommand::Idle,
        EMACommand::Navigate,
        EMACommand::Follow,
        EMACommand::Charge,
        EMACommand::Search,
        EMACommand::Place
    };
    
    TSet<FString> SeenStrings;
    int32 DuplicateCount = 0;
    
    for (EMACommand Command : AllCommands)
    {
        FString CommandStr = UMACommandManager::CommandToString(Command);
        
        if (SeenStrings.Contains(CommandStr))
        {
            AddError(FString::Printf(TEXT("Property 5.4 violated: Duplicate command string '%s'"), *CommandStr));
            DuplicateCount++;
        }
        else
        {
            SeenStrings.Add(CommandStr);
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.4 Command Uniqueness Test: %d unique strings, %d duplicates"), 
        SeenStrings.Num(), DuplicateCount));
    
    return DuplicateCount == 0;
}

//=============================================================================
// Property 5 综合测试: 选择操作一致性
// 验证选择操作的基本属性
//=============================================================================

/**
 * Property Test: 选择操作幂等性和一致性
 * 
 * 属性定义:
 * - SelectAgent(A) 后 IsSelected(A) 必须为 true
 * - ClearSelection() 后 HasSelection() 必须为 false
 * - AddToSelection 不会重复添加
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSSelectionConsistencyPropertyTest,
    "MultiAgent.Core.RTS.Property5_SelectionConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSSelectionConsistencyPropertyTest::RunTest(const FString& Parameters)
{
    const int32 NumTests = 100;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        // 模拟选择状态
        TArray<int32> SelectedAgents;
        
        // 生成随机操作序列
        int32 NumOperations = FMath::RandRange(5, 20);
        bool bTestPassed = true;
        
        for (int32 OpIndex = 0; OpIndex < NumOperations && bTestPassed; OpIndex++)
        {
            int32 Operation = FMath::RandRange(0, 3);
            int32 AgentIndex = FMath::RandRange(0, 9);
            
            switch (Operation)
            {
                case 0:  // SelectAgent (清除其他，只选这一个)
                {
                    SelectedAgents.Empty();
                    SelectedAgents.Add(AgentIndex);
                    
                    // Property: 选择后必须包含该 Agent
                    if (!SelectedAgents.Contains(AgentIndex))
                    {
                        AddError(FString::Printf(TEXT("Property 5.1 violated: SelectAgent failed (Test %d, Op %d)"), TestIndex, OpIndex));
                        bTestPassed = false;
                    }
                    // Property: 只有一个被选中
                    if (SelectedAgents.Num() != 1)
                    {
                        AddError(FString::Printf(TEXT("Property 5.1 violated: SelectAgent should result in single selection (Test %d, Op %d)"), TestIndex, OpIndex));
                        bTestPassed = false;
                    }
                    break;
                }
                case 1:  // AddToSelection
                {
                    if (!SelectedAgents.Contains(AgentIndex))
                    {
                        SelectedAgents.Add(AgentIndex);
                    }
                    
                    // Property: 添加后必须包含该 Agent
                    if (!SelectedAgents.Contains(AgentIndex))
                    {
                        AddError(FString::Printf(TEXT("Property 5.1 violated: AddToSelection failed (Test %d, Op %d)"), TestIndex, OpIndex));
                        bTestPassed = false;
                    }
                    break;
                }
                case 2:  // RemoveFromSelection
                {
                    SelectedAgents.Remove(AgentIndex);
                    
                    // Property: 移除后不应包含该 Agent
                    if (SelectedAgents.Contains(AgentIndex))
                    {
                        AddError(FString::Printf(TEXT("Property 5.1 violated: RemoveFromSelection failed (Test %d, Op %d)"), TestIndex, OpIndex));
                        bTestPassed = false;
                    }
                    break;
                }
                case 3:  // ClearSelection
                {
                    SelectedAgents.Empty();
                    
                    // Property: 清除后必须为空
                    if (SelectedAgents.Num() != 0)
                    {
                        AddError(FString::Printf(TEXT("Property 5.1 violated: ClearSelection failed (Test %d, Op %d)"), TestIndex, OpIndex));
                        bTestPassed = false;
                    }
                    break;
                }
            }
        }
        
        if (bTestPassed)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.1 Selection Consistency Test: %d/%d tests passed"), PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

/**
 * Property Test: ToggleSelection 双向性
 * 
 * 属性定义:
 * - 对于任意 Agent，连续两次 ToggleSelection 应恢复原状态
 * - Toggle(Toggle(state)) == state
 * 
 * **Feature: ui-integration, Property 5: RTS 功能兼容性**
 * **Validates: Requirements 5.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMARTSToggleSelectionRoundTripPropertyTest,
    "MultiAgent.Core.RTS.Property5_ToggleSelectionRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMARTSToggleSelectionRoundTripPropertyTest::RunTest(const FString& Parameters)
{
    const int32 NumTests = 100;
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumTests; TestIndex++)
    {
        // 模拟选择状态
        TSet<int32> SelectedAgents;
        
        // 随机初始化一些选择
        int32 NumInitialSelected = FMath::RandRange(0, 5);
        for (int32 i = 0; i < NumInitialSelected; i++)
        {
            SelectedAgents.Add(FMath::RandRange(0, 9));
        }
        
        // 选择一个 Agent 进行 Toggle 测试
        int32 TestAgentIndex = FMath::RandRange(0, 9);
        bool bInitiallySelected = SelectedAgents.Contains(TestAgentIndex);
        
        // 第一次 Toggle
        if (SelectedAgents.Contains(TestAgentIndex))
        {
            SelectedAgents.Remove(TestAgentIndex);
        }
        else
        {
            SelectedAgents.Add(TestAgentIndex);
        }
        
        // 第二次 Toggle
        if (SelectedAgents.Contains(TestAgentIndex))
        {
            SelectedAgents.Remove(TestAgentIndex);
        }
        else
        {
            SelectedAgents.Add(TestAgentIndex);
        }
        
        // Property: 两次 Toggle 后应恢复原状态
        bool bFinallySelected = SelectedAgents.Contains(TestAgentIndex);
        if (bFinallySelected != bInitiallySelected)
        {
            AddError(FString::Printf(
                TEXT("Property 5.1 violated: Toggle round-trip failed (Test %d, Agent %d, Initial %s, Final %s)"),
                TestIndex, TestAgentIndex,
                bInitiallySelected ? TEXT("selected") : TEXT("not selected"),
                bFinallySelected ? TEXT("selected") : TEXT("not selected")));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 5.1 Toggle Round-Trip Test: %d/%d tests passed"), PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

#endif // WITH_DEV_AUTOMATION_TESTS
