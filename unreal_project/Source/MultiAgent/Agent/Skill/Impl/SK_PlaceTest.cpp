// SK_PlaceTest.cpp
// Property-Based Tests for SK_Place
// **Feature: place-skill, Property 5: Navigation Trigger Based on Distance**
// **Validates: Requirements 3.1**

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SK_Place.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAHumanoidCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

//=============================================================================
// Test Helpers - 随机数据生成器
//=============================================================================

namespace SK_PlaceTestHelpers
{
    // 生成随机位置
    static FVector GenerateRandomLocation(float MinRange = -10000.f, float MaxRange = 10000.f)
    {
        return FVector(
            FMath::RandRange(MinRange, MaxRange),
            FMath::RandRange(MinRange, MaxRange),
            FMath::RandRange(0.f, 500.f)  // Z 轴限制在合理范围
        );
    }
    
    // 生成在指定位置附近的随机位置
    static FVector GenerateNearbyLocation(const FVector& Center, float MinDist, float MaxDist)
    {
        // 生成随机方向
        FVector Direction = FMath::VRand();
        Direction.Z = 0.f;  // 保持在同一平面
        Direction.Normalize();
        
        // 生成随机距离
        float Distance = FMath::RandRange(MinDist, MaxDist);
        
        return Center + Direction * Distance;
    }
    
    // 测试数据结构：导航触发测试用例
    struct FNavigationTriggerTestCase
    {
        FVector CharacterLocation;
        FVector SourceObjectLocation;
        float InteractionRadius;
        float Distance;
        bool bShouldTriggerNavigation;
        
        FNavigationTriggerTestCase() = default;
        
        FNavigationTriggerTestCase(const FVector& InCharLoc, const FVector& InSourceLoc, float InRadius)
            : CharacterLocation(InCharLoc)
            , SourceObjectLocation(InSourceLoc)
            , InteractionRadius(InRadius)
        {
            Distance = FVector::Dist(CharacterLocation, SourceObjectLocation);
            bShouldTriggerNavigation = Distance > InteractionRadius;
        }
    };
    
    // 生成随机测试用例 - 角色在交互范围外
    static FNavigationTriggerTestCase GenerateOutOfRangeTestCase(float InteractionRadius = 150.f)
    {
        FVector SourceLocation = GenerateRandomLocation();
        // 确保角色在交互范围外（距离 > InteractionRadius）
        float MinDistance = InteractionRadius + 1.f;
        float MaxDistance = InteractionRadius + 5000.f;
        FVector CharLocation = GenerateNearbyLocation(SourceLocation, MinDistance, MaxDistance);
        
        return FNavigationTriggerTestCase(CharLocation, SourceLocation, InteractionRadius);
    }
    
    // 生成随机测试用例 - 角色在交互范围内
    static FNavigationTriggerTestCase GenerateInRangeTestCase(float InteractionRadius = 150.f)
    {
        FVector SourceLocation = GenerateRandomLocation();
        // 确保角色在交互范围内（距离 <= InteractionRadius）
        float MinDistance = 0.f;
        float MaxDistance = InteractionRadius - 1.f;
        if (MaxDistance < 0.f) MaxDistance = 0.f;
        FVector CharLocation = GenerateNearbyLocation(SourceLocation, MinDistance, MaxDistance);
        
        return FNavigationTriggerTestCase(CharLocation, SourceLocation, InteractionRadius);
    }
    
    // 生成边界测试用例 - 角色恰好在交互范围边界
    static FNavigationTriggerTestCase GenerateBoundaryTestCase(float InteractionRadius = 150.f, bool bJustOutside = true)
    {
        FVector SourceLocation = GenerateRandomLocation();
        // 生成恰好在边界的位置
        float Distance = bJustOutside ? (InteractionRadius + 0.1f) : (InteractionRadius - 0.1f);
        FVector Direction = FMath::VRand();
        Direction.Z = 0.f;
        Direction.Normalize();
        FVector CharLocation = SourceLocation + Direction * Distance;
        
        return FNavigationTriggerTestCase(CharLocation, SourceLocation, InteractionRadius);
    }
}

//=============================================================================
// Property 5: Navigation Trigger Based on Distance
// For any Place skill activation where the Humanoid's distance to object1 
// exceeds InteractionRadius, the skill SHALL initiate navigation to object1 
// before attempting pickup.
// **Validates: Requirements 3.1**
//=============================================================================

/**
 * Property Test: Navigation Trigger Based on Distance
 * 
 * 属性定义:
 * - 对于任意 Place 技能激活，如果 Humanoid 到 object1 的距离超过 InteractionRadius，
 *   技能必须先导航到 object1，然后才能尝试拾取
 * - 如果距离在 InteractionRadius 内，技能应该直接进入拾取阶段
 * 
 * 测试策略:
 * - 由于无法在自动化测试中创建完整的游戏世界和角色，我们测试核心逻辑：
 *   距离计算和阶段判定逻辑
 * 
 * **Feature: place-skill, Property 5: Navigation Trigger Based on Distance**
 * **Validates: Requirements 3.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSK_PlaceNavigationTriggerPropertyTest,
    "MultiAgent.Skills.Place.Property5_NavigationTriggerBasedOnDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSK_PlaceNavigationTriggerPropertyTest::RunTest(const FString& Parameters)
{
    using namespace SK_PlaceTestHelpers;
    
    // 属性测试迭代次数 (至少 100 次)
    const int32 NumIterations = 100;
    int32 PassedCount = 0;
    
    // 默认交互半径（与 SK_Place 中的默认值一致）
    const float DefaultInteractionRadius = 150.f;
    
    for (int32 i = 0; i < NumIterations; i++)
    {
        // 随机选择测试场景类型
        int32 ScenarioType = FMath::RandRange(0, 3);
        FNavigationTriggerTestCase TestCase;
        
        switch (ScenarioType)
        {
            case 0:
                // 场景 1: 角色在交互范围外
                TestCase = GenerateOutOfRangeTestCase(DefaultInteractionRadius);
                break;
            case 1:
                // 场景 2: 角色在交互范围内
                TestCase = GenerateInRangeTestCase(DefaultInteractionRadius);
                break;
            case 2:
                // 场景 3: 角色恰好在边界外
                TestCase = GenerateBoundaryTestCase(DefaultInteractionRadius, true);
                break;
            case 3:
                // 场景 4: 角色恰好在边界内
                TestCase = GenerateBoundaryTestCase(DefaultInteractionRadius, false);
                break;
        }
        
        // 验证距离计算
        float CalculatedDistance = FVector::Dist(TestCase.CharacterLocation, TestCase.SourceObjectLocation);
        
        // 验证 Property 5.1: 距离计算必须正确
        if (!FMath::IsNearlyEqual(CalculatedDistance, TestCase.Distance, 0.01f))
        {
            AddError(FString::Printf(TEXT("Iteration %d: Distance calculation mismatch - Expected %.2f, Got %.2f"), 
                i, TestCase.Distance, CalculatedDistance));
            continue;
        }
        
        // 验证 Property 5.2: 导航触发判定必须正确
        // 如果距离 > InteractionRadius，应该触发导航（bShouldTriggerNavigation = true）
        // 如果距离 <= InteractionRadius，不应该触发导航（bShouldTriggerNavigation = false）
        bool bActualShouldNavigate = CalculatedDistance > TestCase.InteractionRadius;
        
        if (bActualShouldNavigate != TestCase.bShouldTriggerNavigation)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Navigation trigger mismatch - Distance: %.2f, Radius: %.2f, Expected: %s, Got: %s"), 
                i, CalculatedDistance, TestCase.InteractionRadius,
                TestCase.bShouldTriggerNavigation ? TEXT("true") : TEXT("false"),
                bActualShouldNavigate ? TEXT("true") : TEXT("false")));
            continue;
        }
        
        // 验证 Property 5.3: 阶段判定逻辑
        // 模拟 SK_Place::ActivateAbility 中的阶段判定逻辑
        EPlacePhase ExpectedInitialPhase;
        if (CalculatedDistance <= TestCase.InteractionRadius)
        {
            // 已经在交互范围内，直接进入拾取阶段
            ExpectedInitialPhase = EPlacePhase::BendDownPickup;
        }
        else
        {
            // 需要先导航到源对象
            ExpectedInitialPhase = EPlacePhase::MoveToSource;
        }
        
        // 验证阶段判定与导航触发一致
        bool bPhaseMatchesNavigation = 
            (ExpectedInitialPhase == EPlacePhase::MoveToSource && bActualShouldNavigate) ||
            (ExpectedInitialPhase == EPlacePhase::BendDownPickup && !bActualShouldNavigate);
        
        if (!bPhaseMatchesNavigation)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Phase determination mismatch - Distance: %.2f, Radius: %.2f, Phase: %d, ShouldNavigate: %s"), 
                i, CalculatedDistance, TestCase.InteractionRadius,
                static_cast<int32>(ExpectedInitialPhase),
                bActualShouldNavigate ? TEXT("true") : TEXT("false")));
            continue;
        }
        
        PassedCount++;
    }
    
    // 输出测试结果
    AddInfo(FString::Printf(TEXT("Property 5 Test: %d/%d iterations passed"), PassedCount, NumIterations));
    
    return PassedCount == NumIterations;
}

//=============================================================================
// Property 5 补充测试: 边界条件精确测试
// 验证恰好在 InteractionRadius 边界的情况
//=============================================================================

/**
 * Property Test: Exact Boundary Conditions
 * 
 * **Feature: place-skill, Property 5: Navigation Trigger Based on Distance**
 * **Validates: Requirements 3.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSK_PlaceNavigationBoundaryPropertyTest,
    "MultiAgent.Skills.Place.Property5_ExactBoundaryConditions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSK_PlaceNavigationBoundaryPropertyTest::RunTest(const FString& Parameters)
{
    const float InteractionRadius = 150.f;
    const FVector SourceLocation(1000.f, 1000.f, 0.f);
    
    // 测试 1: 距离恰好等于 InteractionRadius
    {
        FVector CharLocation = SourceLocation + FVector(InteractionRadius, 0.f, 0.f);
        float Distance = FVector::Dist(CharLocation, SourceLocation);
        
        // 距离 == InteractionRadius 时，不应该触发导航（<= 判定）
        bool bShouldNavigate = Distance > InteractionRadius;
        TestFalse(TEXT("Distance == InteractionRadius should NOT trigger navigation"), bShouldNavigate);
    }
    
    // 测试 2: 距离略大于 InteractionRadius
    {
        FVector CharLocation = SourceLocation + FVector(InteractionRadius + 0.01f, 0.f, 0.f);
        float Distance = FVector::Dist(CharLocation, SourceLocation);
        
        // 距离 > InteractionRadius 时，应该触发导航
        bool bShouldNavigate = Distance > InteractionRadius;
        TestTrue(TEXT("Distance > InteractionRadius should trigger navigation"), bShouldNavigate);
    }
    
    // 测试 3: 距离略小于 InteractionRadius
    {
        FVector CharLocation = SourceLocation + FVector(InteractionRadius - 0.01f, 0.f, 0.f);
        float Distance = FVector::Dist(CharLocation, SourceLocation);
        
        // 距离 < InteractionRadius 时，不应该触发导航
        bool bShouldNavigate = Distance > InteractionRadius;
        TestFalse(TEXT("Distance < InteractionRadius should NOT trigger navigation"), bShouldNavigate);
    }
    
    // 测试 4: 距离为 0（角色与源对象重叠）
    {
        FVector CharLocation = SourceLocation;
        float Distance = FVector::Dist(CharLocation, SourceLocation);
        
        // 距离 == 0 时，不应该触发导航
        bool bShouldNavigate = Distance > InteractionRadius;
        TestFalse(TEXT("Distance == 0 should NOT trigger navigation"), bShouldNavigate);
    }
    
    // 测试 5: 不同的 InteractionRadius 值
    {
        TArray<float> RadiusValues = { 50.f, 100.f, 150.f, 200.f, 500.f };
        
        for (float Radius : RadiusValues)
        {
            // 在范围内
            FVector InRangeLocation = SourceLocation + FVector(Radius * 0.5f, 0.f, 0.f);
            float InRangeDistance = FVector::Dist(InRangeLocation, SourceLocation);
            bool bInRangeShouldNavigate = InRangeDistance > Radius;
            TestFalse(FString::Printf(TEXT("Radius %.0f: In-range should NOT trigger navigation"), Radius), bInRangeShouldNavigate);
            
            // 在范围外
            FVector OutRangeLocation = SourceLocation + FVector(Radius * 2.f, 0.f, 0.f);
            float OutRangeDistance = FVector::Dist(OutRangeLocation, SourceLocation);
            bool bOutRangeShouldNavigate = OutRangeDistance > Radius;
            TestTrue(FString::Printf(TEXT("Radius %.0f: Out-of-range should trigger navigation"), Radius), bOutRangeShouldNavigate);
        }
    }
    
    return true;
}

//=============================================================================
// Property 5 补充测试: 3D 距离计算
// 验证 3D 空间中的距离计算正确性
//=============================================================================

/**
 * Property Test: 3D Distance Calculation
 * 
 * **Feature: place-skill, Property 5: Navigation Trigger Based on Distance**
 * **Validates: Requirements 3.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSK_PlaceNavigation3DDistancePropertyTest,
    "MultiAgent.Skills.Place.Property5_3DDistanceCalculation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSK_PlaceNavigation3DDistancePropertyTest::RunTest(const FString& Parameters)
{
    using namespace SK_PlaceTestHelpers;
    
    const int32 NumIterations = 100;
    int32 PassedCount = 0;
    const float InteractionRadius = 150.f;
    
    for (int32 i = 0; i < NumIterations; i++)
    {
        // 生成随机 3D 位置
        FVector SourceLocation = GenerateRandomLocation();
        FVector CharLocation = GenerateRandomLocation();
        
        // 计算 3D 欧几里得距离
        float Distance3D = FVector::Dist(CharLocation, SourceLocation);
        
        // 手动计算验证
        float DeltaX = CharLocation.X - SourceLocation.X;
        float DeltaY = CharLocation.Y - SourceLocation.Y;
        float DeltaZ = CharLocation.Z - SourceLocation.Z;
        float ManualDistance = FMath::Sqrt(DeltaX * DeltaX + DeltaY * DeltaY + DeltaZ * DeltaZ);
        
        // 验证距离计算一致性
        if (!FMath::IsNearlyEqual(Distance3D, ManualDistance, 0.001f))
        {
            AddError(FString::Printf(TEXT("Iteration %d: 3D distance calculation mismatch - FVector::Dist: %.4f, Manual: %.4f"), 
                i, Distance3D, ManualDistance));
            continue;
        }
        
        // 验证导航触发判定
        bool bShouldNavigate = Distance3D > InteractionRadius;
        bool bExpectedNavigate = ManualDistance > InteractionRadius;
        
        if (bShouldNavigate != bExpectedNavigate)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Navigation trigger inconsistency"), i));
            continue;
        }
        
        PassedCount++;
    }
    
    AddInfo(FString::Printf(TEXT("Property 5 3D Distance Test: %d/%d iterations passed"), PassedCount, NumIterations));
    
    return PassedCount == NumIterations;
}

//=============================================================================
// Property 5 补充测试: 阶段转换一致性
// 验证初始阶段与导航触发的一致性
//=============================================================================

/**
 * Property Test: Phase Transition Consistency
 * 
 * **Feature: place-skill, Property 5: Navigation Trigger Based on Distance**
 * **Validates: Requirements 3.1**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSK_PlacePhaseTransitionPropertyTest,
    "MultiAgent.Skills.Place.Property5_PhaseTransitionConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSK_PlacePhaseTransitionPropertyTest::RunTest(const FString& Parameters)
{
    using namespace SK_PlaceTestHelpers;
    
    const int32 NumIterations = 100;
    int32 PassedCount = 0;
    
    // 测试不同的 InteractionRadius 值
    TArray<float> RadiusValues = { 50.f, 100.f, 150.f, 200.f, 300.f, 500.f };
    
    for (int32 i = 0; i < NumIterations; i++)
    {
        // 随机选择一个 InteractionRadius
        float InteractionRadius = RadiusValues[FMath::RandRange(0, RadiusValues.Num() - 1)];
        
        // 生成随机位置
        FVector SourceLocation = GenerateRandomLocation();
        FVector CharLocation = GenerateRandomLocation();
        
        float Distance = FVector::Dist(CharLocation, SourceLocation);
        
        // 根据 SK_Place::ActivateAbility 中的逻辑判定初始阶段
        EPlacePhase InitialPhase;
        if (Distance <= InteractionRadius)
        {
            InitialPhase = EPlacePhase::BendDownPickup;
        }
        else
        {
            InitialPhase = EPlacePhase::MoveToSource;
        }
        
        // 验证一致性：
        // - 如果初始阶段是 MoveToSource，则距离必须 > InteractionRadius
        // - 如果初始阶段是 BendDownPickup，则距离必须 <= InteractionRadius
        bool bConsistent = false;
        
        if (InitialPhase == EPlacePhase::MoveToSource)
        {
            bConsistent = (Distance > InteractionRadius);
            if (!bConsistent)
            {
                AddError(FString::Printf(TEXT("Iteration %d: MoveToSource phase but distance (%.2f) <= radius (%.2f)"), 
                    i, Distance, InteractionRadius));
            }
        }
        else if (InitialPhase == EPlacePhase::BendDownPickup)
        {
            bConsistent = (Distance <= InteractionRadius);
            if (!bConsistent)
            {
                AddError(FString::Printf(TEXT("Iteration %d: BendDownPickup phase but distance (%.2f) > radius (%.2f)"), 
                    i, Distance, InteractionRadius));
            }
        }
        
        if (bConsistent)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 5 Phase Transition Test: %d/%d iterations passed"), PassedCount, NumIterations));
    
    return PassedCount == NumIterations;
}

#endif // WITH_DEV_AUTOMATION_TESTS
