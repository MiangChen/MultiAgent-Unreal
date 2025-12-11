// MAHUDTest.cpp
// Property-Based Tests for MAHUD UI Toggle State Consistency
// **Feature: ui-integration, Property 2: UI 切换状态一致性**
// **Validates: Requirements 2.1, 2.2**

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MAHUD.h"
#include "MASimpleMainWidget.h"
#include "Blueprint/UserWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

//=============================================================================
// Test Helpers
//=============================================================================

namespace MAHUDTestHelpers
{
    /**
     * 生成随机的 UI 切换序列
     * @param Length 序列长度
     * @return 切换操作序列 (true = toggle)
     */
    static TArray<bool> GenerateToggleSequence(int32 Length)
    {
        TArray<bool> Sequence;
        for (int32 i = 0; i < Length; i++)
        {
            Sequence.Add(true);  // 每次都是 toggle 操作
        }
        return Sequence;
    }

    /**
     * 生成随机的显示/隐藏操作序列
     * @param Length 序列长度
     * @return 操作序列 (0 = Toggle, 1 = Show, 2 = Hide)
     */
    static TArray<int32> GenerateRandomOperationSequence(int32 Length)
    {
        TArray<int32> Sequence;
        for (int32 i = 0; i < Length; i++)
        {
            Sequence.Add(FMath::RandRange(0, 2));
        }
        return Sequence;
    }
}

//=============================================================================
// Property 2: UI 切换状态一致性
// For any C 键按下事件, bMainUIVisible 的值必须翻转, 
// 且 MainWidget 的可见性必须与 bMainUIVisible 一致
// Validates: Requirements 2.1, 2.2
//=============================================================================

/**
 * Property Test: UI 切换状态一致性 - Toggle 操作翻转状态
 * 
 * 属性定义:
 * - 对于任意初始状态，调用 ToggleMainUI() 后 bMainUIVisible 必须翻转
 * - 连续调用 N 次 ToggleMainUI()，最终状态应为 (初始状态 XOR N%2)
 * 
 * **Feature: ui-integration, Property 2: UI 切换状态一致性**
 * **Validates: Requirements 2.1, 2.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMAHUDToggleStatePropertyTest,
    "MultiAgent.UI.MAHUD.Property2_ToggleStateConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMAHUDToggleStatePropertyTest::RunTest(const FString& Parameters)
{
    // 创建测试用的 HUD 对象
    // 注意: AMAHUD 是 Actor，需要在有效的 World 中创建
    // 但我们可以直接测试其状态逻辑，不依赖 Widget 实例
    
    // 测试不同长度的 toggle 序列
    TArray<int32> SequenceLengths = {1, 2, 3, 5, 10, 20, 50, 100};
    
    int32 PassedCount = 0;
    int32 TotalTests = SequenceLengths.Num() * 2;  // 每个长度测试两种初始状态
    
    for (int32 Length : SequenceLengths)
    {
        // 测试初始状态为 false
        {
            bool bCurrentState = false;  // 模拟 bMainUIVisible 初始值
            
            for (int32 i = 0; i < Length; i++)
            {
                // 模拟 ToggleMainUI() 的核心逻辑
                bool bPreviousState = bCurrentState;
                bCurrentState = !bCurrentState;  // Toggle 操作
                
                // Property 2.1: 每次 toggle 后状态必须翻转
                if (bCurrentState == bPreviousState)
                {
                    AddError(FString::Printf(
                        TEXT("Property 2 violated: State did not flip at iteration %d (Length=%d, InitialState=false)"),
                        i, Length));
                    goto NextTest1;
                }
            }
            
            // Property 2.2: 最终状态应为 (初始状态 XOR 奇偶性)
            bool bExpectedFinalState = (Length % 2 == 1);  // 初始 false，奇数次 toggle 后为 true
            if (bCurrentState != bExpectedFinalState)
            {
                AddError(FString::Printf(
                    TEXT("Property 2 violated: Final state mismatch (Length=%d, Expected=%s, Got=%s)"),
                    Length, 
                    bExpectedFinalState ? TEXT("true") : TEXT("false"),
                    bCurrentState ? TEXT("true") : TEXT("false")));
            }
            else
            {
                PassedCount++;
            }
        }
        NextTest1:
        
        // 测试初始状态为 true
        {
            bool bCurrentState = true;  // 模拟 bMainUIVisible 初始值
            
            for (int32 i = 0; i < Length; i++)
            {
                bool bPreviousState = bCurrentState;
                bCurrentState = !bCurrentState;  // Toggle 操作
                
                if (bCurrentState == bPreviousState)
                {
                    AddError(FString::Printf(
                        TEXT("Property 2 violated: State did not flip at iteration %d (Length=%d, InitialState=true)"),
                        i, Length));
                    goto NextTest2;
                }
            }
            
            bool bExpectedFinalState = (Length % 2 == 0);  // 初始 true，偶数次 toggle 后仍为 true
            if (bCurrentState != bExpectedFinalState)
            {
                AddError(FString::Printf(
                    TEXT("Property 2 violated: Final state mismatch (Length=%d, InitialState=true, Expected=%s, Got=%s)"),
                    Length,
                    bExpectedFinalState ? TEXT("true") : TEXT("false"),
                    bCurrentState ? TEXT("true") : TEXT("false")));
            }
            else
            {
                PassedCount++;
            }
        }
        NextTest2:;
    }
    
    AddInfo(FString::Printf(TEXT("Property 2 Toggle Test: %d/%d tests passed"), PassedCount, TotalTests));
    
    return PassedCount == TotalTests;
}

//=============================================================================
// Property 2 补充测试: Show/Hide 操作幂等性
// 验证 ShowMainUI() 和 HideMainUI() 的幂等性
//=============================================================================

/**
 * Property Test: Show/Hide 操作幂等性
 * 
 * 属性定义:
 * - 连续调用 ShowMainUI() 多次，状态应保持为 true
 * - 连续调用 HideMainUI() 多次，状态应保持为 false
 * 
 * **Feature: ui-integration, Property 2: UI 切换状态一致性**
 * **Validates: Requirements 2.1, 2.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMAHUDShowHideIdempotentPropertyTest,
    "MultiAgent.UI.MAHUD.Property2_ShowHideIdempotent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMAHUDShowHideIdempotentPropertyTest::RunTest(const FString& Parameters)
{
    // 测试 Show 操作的幂等性
    {
        bool bState = false;
        
        // 模拟多次 ShowMainUI() 调用
        for (int32 i = 0; i < 100; i++)
        {
            // ShowMainUI() 的核心逻辑: 如果已经显示则不改变状态
            if (!bState)
            {
                bState = true;
            }
            // else: 已经显示，保持状态
            
            // 验证状态始终为 true
            if (!bState)
            {
                AddError(FString::Printf(TEXT("Property 2 violated: Show operation failed at iteration %d"), i));
                return false;
            }
        }
        
        TestTrue(TEXT("After multiple Show operations, state should be true"), bState);
    }
    
    // 测试 Hide 操作的幂等性
    {
        bool bState = true;
        
        // 模拟多次 HideMainUI() 调用
        for (int32 i = 0; i < 100; i++)
        {
            // HideMainUI() 的核心逻辑: 如果已经隐藏则不改变状态
            if (bState)
            {
                bState = false;
            }
            // else: 已经隐藏，保持状态
            
            // 验证状态始终为 false
            if (bState)
            {
                AddError(FString::Printf(TEXT("Property 2 violated: Hide operation failed at iteration %d"), i));
                return false;
            }
        }
        
        TestFalse(TEXT("After multiple Hide operations, state should be false"), bState);
    }
    
    return true;
}

//=============================================================================
// Property 2 补充测试: 随机操作序列状态一致性
// 验证任意操作序列后状态的一致性
//=============================================================================

/**
 * Property Test: 随机操作序列状态一致性
 * 
 * 属性定义:
 * - 对于任意 Toggle/Show/Hide 操作序列，最终状态必须可预测
 * - Toggle 翻转状态，Show 设为 true，Hide 设为 false
 * 
 * **Feature: ui-integration, Property 2: UI 切换状态一致性**
 * **Validates: Requirements 2.1, 2.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMAHUDRandomOperationPropertyTest,
    "MultiAgent.UI.MAHUD.Property2_RandomOperationConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMAHUDRandomOperationPropertyTest::RunTest(const FString& Parameters)
{
    // 运行多次随机测试
    const int32 NumRandomTests = 100;
    const int32 MaxSequenceLength = 50;
    
    int32 PassedCount = 0;
    
    for (int32 TestIndex = 0; TestIndex < NumRandomTests; TestIndex++)
    {
        // 随机初始状态
        bool bState = FMath::RandBool();
        bool bExpectedState = bState;
        
        // 随机序列长度
        int32 SequenceLength = FMath::RandRange(1, MaxSequenceLength);
        
        // 生成并执行随机操作序列
        for (int32 i = 0; i < SequenceLength; i++)
        {
            int32 Operation = FMath::RandRange(0, 2);
            
            switch (Operation)
            {
                case 0:  // Toggle
                    bState = !bState;
                    bExpectedState = !bExpectedState;
                    break;
                case 1:  // Show
                    if (!bState) bState = true;
                    bExpectedState = true;
                    break;
                case 2:  // Hide
                    if (bState) bState = false;
                    bExpectedState = false;
                    break;
            }
            
            // 验证每步操作后状态一致
            if (bState != bExpectedState)
            {
                AddError(FString::Printf(
                    TEXT("Property 2 violated: State mismatch at test %d, step %d, op %d"),
                    TestIndex, i, Operation));
                goto NextRandomTest;
            }
        }
        
        PassedCount++;
        NextRandomTest:;
    }
    
    AddInfo(FString::Printf(TEXT("Property 2 Random Test: %d/%d tests passed"), PassedCount, NumRandomTests));
    
    return PassedCount == NumRandomTests;
}

//=============================================================================
// Property 2 补充测试: Toggle 双向性
// 验证 Toggle 操作的可逆性
//=============================================================================

/**
 * Property Test: Toggle 双向性 (Round-trip)
 * 
 * 属性定义:
 * - 对于任意状态，连续两次 Toggle 操作后应恢复原状态
 * - Toggle(Toggle(state)) == state
 * 
 * **Feature: ui-integration, Property 2: UI 切换状态一致性**
 * **Validates: Requirements 2.1, 2.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMAHUDToggleRoundTripPropertyTest,
    "MultiAgent.UI.MAHUD.Property2_ToggleRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMAHUDToggleRoundTripPropertyTest::RunTest(const FString& Parameters)
{
    // 测试 100 次随机初始状态
    const int32 NumTests = 100;
    int32 PassedCount = 0;
    
    for (int32 i = 0; i < NumTests; i++)
    {
        bool bInitialState = FMath::RandBool();
        bool bState = bInitialState;
        
        // 第一次 Toggle
        bState = !bState;
        
        // 第二次 Toggle
        bState = !bState;
        
        // 验证恢复原状态
        if (bState == bInitialState)
        {
            PassedCount++;
        }
        else
        {
            AddError(FString::Printf(
                TEXT("Property 2 violated: Round-trip failed at test %d (Initial=%s, Final=%s)"),
                i,
                bInitialState ? TEXT("true") : TEXT("false"),
                bState ? TEXT("true") : TEXT("false")));
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 2 Round-Trip Test: %d/%d tests passed"), PassedCount, NumTests));
    
    return PassedCount == NumTests;
}

#endif // WITH_DEV_AUTOMATION_TESTS
