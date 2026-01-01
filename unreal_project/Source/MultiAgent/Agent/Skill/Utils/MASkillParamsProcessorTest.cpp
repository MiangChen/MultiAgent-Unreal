// MASkillParamsProcessorTest.cpp
// Property-Based Tests for MASkillParamsProcessor
// **Feature: place-skill, Property 1: Semantic Label Parsing Completeness**
// **Validates: Requirements 1.1, 1.2**

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MASkillParamsProcessor.h"
#include "../MASkillComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"

#if WITH_DEV_AUTOMATION_TESTS

//=============================================================================
// Test Helpers - 随机数据生成器
//=============================================================================

namespace MASkillParamsProcessorTestHelpers
{
    // 随机字符串生成
    static FString GenerateRandomString(int32 MinLen = 1, int32 MaxLen = 20)
    {
        static const TCHAR* Chars = TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
        static const int32 CharsLen = FCString::Strlen(Chars);
        
        int32 Len = FMath::RandRange(MinLen, MaxLen);
        FString Result;
        Result.Reserve(Len);
        
        for (int32 i = 0; i < Len; i++)
        {
            Result.AppendChar(Chars[FMath::RandRange(0, CharsLen - 1)]);
        }
        
        return Result;
    }
    
    // 生成随机 class 值
    static FString GenerateRandomClass()
    {
        TArray<FString> Classes = { TEXT("object"), TEXT("robot"), TEXT("ground"), TEXT("item"), TEXT("container") };
        return Classes[FMath::RandRange(0, Classes.Num() - 1)];
    }
    
    // 生成随机 type 值
    static FString GenerateRandomType()
    {
        TArray<FString> Types = { TEXT("box"), TEXT("UGV"), TEXT("crate"), TEXT("barrel"), TEXT("pallet"), TEXT("ugv"), TEXT("drone") };
        return Types[FMath::RandRange(0, Types.Num() - 1)];
    }
    
    // 生成随机 features map
    static TMap<FString, FString> GenerateRandomFeatures(int32 MinCount = 0, int32 MaxCount = 5)
    {
        TMap<FString, FString> Features;
        int32 Count = FMath::RandRange(MinCount, MaxCount);
        
        TArray<FString> FeatureKeys = { TEXT("color"), TEXT("size"), TEXT("name"), TEXT("weight"), TEXT("material"), TEXT("id") };
        TArray<FString> FeatureValues = { TEXT("red"), TEXT("blue"), TEXT("large"), TEXT("small"), TEXT("UGV_01"), TEXT("heavy"), TEXT("light") };
        
        for (int32 i = 0; i < Count && i < FeatureKeys.Num(); i++)
        {
            Features.Add(FeatureKeys[i], FeatureValues[FMath::RandRange(0, FeatureValues.Num() - 1)]);
        }
        
        return Features;
    }
    
    // 将 FMASemanticTarget 转换为 JSON 字符串
    static FString SemanticTargetToJson(const FString& Class, const FString& Type, const TMap<FString, FString>& Features)
    {
        TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        
        if (!Class.IsEmpty())
        {
            JsonObject->SetStringField(TEXT("class"), Class);
        }
        
        if (!Type.IsEmpty())
        {
            JsonObject->SetStringField(TEXT("type"), Type);
        }
        
        if (Features.Num() > 0)
        {
            TSharedRef<FJsonObject> FeaturesObject = MakeShared<FJsonObject>();
            for (const auto& Pair : Features)
            {
                FeaturesObject->SetStringField(Pair.Key, Pair.Value);
            }
            JsonObject->SetObjectField(TEXT("features"), FeaturesObject);
        }
        
        FString OutputString;
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject, Writer);
        
        return OutputString;
    }
    
    // 测试数据结构
    struct FSemanticLabelTestCase
    {
        FString Class;
        FString Type;
        TMap<FString, FString> Features;
        FString JsonString;
        
        FSemanticLabelTestCase() = default;
        
        FSemanticLabelTestCase(const FString& InClass, const FString& InType, const TMap<FString, FString>& InFeatures)
            : Class(InClass), Type(InType), Features(InFeatures)
        {
            JsonString = SemanticTargetToJson(Class, Type, Features);
        }
    };
    
    // 生成随机测试用例
    static FSemanticLabelTestCase GenerateRandomTestCase()
    {
        return FSemanticLabelTestCase(
            GenerateRandomClass(),
            GenerateRandomType(),
            GenerateRandomFeatures()
        );
    }
}

//=============================================================================
// Property 1: Semantic Label Parsing Completeness
// For any valid place command containing object1 and object2 semantic labels,
// parsing the command SHALL extract all class, type, and features fields correctly,
// and the extracted values SHALL match the original input.
// **Validates: Requirements 1.1, 1.2**
//=============================================================================

/**
 * Property Test: Semantic Label Parsing Completeness
 * 
 * 属性定义:
 * - 对于任意有效的语义标签 JSON，解析后的 FMASemanticTarget 必须包含所有原始字段
 * - class 字段必须完全匹配
 * - type 字段必须完全匹配
 * - features 中的所有键值对必须完全匹配
 * 
 * **Feature: place-skill, Property 1: Semantic Label Parsing Completeness**
 * **Validates: Requirements 1.1, 1.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMASkillParamsProcessorSemanticLabelParsingPropertyTest,
    "MultiAgent.Skills.Place.Property1_SemanticLabelParsingCompleteness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMASkillParamsProcessorSemanticLabelParsingPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MASkillParamsProcessorTestHelpers;
    
    // 属性测试迭代次数 (至少 100 次)
    const int32 NumIterations = 100;
    int32 PassedCount = 0;
    
    for (int32 i = 0; i < NumIterations; i++)
    {
        // 生成随机测试用例
        FSemanticLabelTestCase TestCase = GenerateRandomTestCase();
        
        // 解析 JSON 到 FMASemanticTarget
        FMASemanticTarget ParsedTarget;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(TestCase.JsonString, ParsedTarget);
        
        // 验证 Property 1.1: class 字段必须完全匹配
        if (ParsedTarget.Class != TestCase.Class)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Class mismatch - Expected '%s', Got '%s' (JSON: %s)"), 
                i, *TestCase.Class, *ParsedTarget.Class, *TestCase.JsonString));
            continue;
        }
        
        // 验证 Property 1.2: type 字段必须完全匹配
        if (ParsedTarget.Type != TestCase.Type)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Type mismatch - Expected '%s', Got '%s' (JSON: %s)"), 
                i, *TestCase.Type, *ParsedTarget.Type, *TestCase.JsonString));
            continue;
        }
        
        // 验证 Property 1.3: features 数量必须匹配
        if (ParsedTarget.Features.Num() != TestCase.Features.Num())
        {
            AddError(FString::Printf(TEXT("Iteration %d: Features count mismatch - Expected %d, Got %d (JSON: %s)"), 
                i, TestCase.Features.Num(), ParsedTarget.Features.Num(), *TestCase.JsonString));
            continue;
        }
        
        // 验证 Property 1.4: features 中的所有键值对必须完全匹配
        bool bFeaturesMatch = true;
        for (const auto& Pair : TestCase.Features)
        {
            const FString* ParsedValue = ParsedTarget.Features.Find(Pair.Key);
            if (!ParsedValue || *ParsedValue != Pair.Value)
            {
                AddError(FString::Printf(TEXT("Iteration %d: Feature mismatch for key '%s' - Expected '%s', Got '%s' (JSON: %s)"), 
                    i, *Pair.Key, *Pair.Value, ParsedValue ? **ParsedValue : TEXT("(null)"), *TestCase.JsonString));
                bFeaturesMatch = false;
                break;
            }
        }
        
        if (!bFeaturesMatch)
        {
            continue;
        }
        
        PassedCount++;
    }
    
    // 输出测试结果
    AddInfo(FString::Printf(TEXT("Property 1 Test: %d/%d iterations passed"), PassedCount, NumIterations));
    
    return PassedCount == NumIterations;
}

//=============================================================================
// Property 1 补充测试: 空 JSON 处理
// 验证空 JSON 字符串被正确处理
//=============================================================================

/**
 * Property Test: Empty JSON Handling
 * 
 * **Feature: place-skill, Property 1: Semantic Label Parsing Completeness**
 * **Validates: Requirements 1.1, 1.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMASkillParamsProcessorEmptyJsonPropertyTest,
    "MultiAgent.Skills.Place.Property1_EmptyJsonHandling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMASkillParamsProcessorEmptyJsonPropertyTest::RunTest(const FString& Parameters)
{
    // 测试空字符串
    FMASemanticTarget EmptyTarget;
    FMASkillParamsProcessor::ParseSemanticTargetFromJson(TEXT(""), EmptyTarget);
    
    TestTrue(TEXT("Empty JSON should result in empty Class"), EmptyTarget.Class.IsEmpty());
    TestTrue(TEXT("Empty JSON should result in empty Type"), EmptyTarget.Type.IsEmpty());
    TestEqual(TEXT("Empty JSON should result in empty Features"), EmptyTarget.Features.Num(), 0);
    TestTrue(TEXT("Empty JSON should result in IsEmpty() == true"), EmptyTarget.IsEmpty());
    
    return true;
}

//=============================================================================
// Property 1 补充测试: 部分字段缺失处理
// 验证缺少某些字段的 JSON 被正确处理
//=============================================================================

/**
 * Property Test: Partial Fields Handling
 * 
 * **Feature: place-skill, Property 1: Semantic Label Parsing Completeness**
 * **Validates: Requirements 1.1, 1.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMASkillParamsProcessorPartialFieldsPropertyTest,
    "MultiAgent.Skills.Place.Property1_PartialFieldsHandling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMASkillParamsProcessorPartialFieldsPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MASkillParamsProcessorTestHelpers;
    
    // 测试只有 class 字段
    {
        FString JsonOnlyClass = TEXT("{\"class\":\"object\"}");
        FMASemanticTarget Target;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(JsonOnlyClass, Target);
        
        TestEqual(TEXT("Only class: Class should be 'object'"), Target.Class, FString(TEXT("object")));
        TestTrue(TEXT("Only class: Type should be empty"), Target.Type.IsEmpty());
        TestEqual(TEXT("Only class: Features should be empty"), Target.Features.Num(), 0);
    }
    
    // 测试只有 type 字段
    {
        FString JsonOnlyType = TEXT("{\"type\":\"box\"}");
        FMASemanticTarget Target;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(JsonOnlyType, Target);
        
        TestTrue(TEXT("Only type: Class should be empty"), Target.Class.IsEmpty());
        TestEqual(TEXT("Only type: Type should be 'box'"), Target.Type, FString(TEXT("box")));
        TestEqual(TEXT("Only type: Features should be empty"), Target.Features.Num(), 0);
    }
    
    // 测试只有 features 字段
    {
        FString JsonOnlyFeatures = TEXT("{\"features\":{\"color\":\"red\",\"name\":\"item1\"}}");
        FMASemanticTarget Target;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(JsonOnlyFeatures, Target);
        
        TestTrue(TEXT("Only features: Class should be empty"), Target.Class.IsEmpty());
        TestTrue(TEXT("Only features: Type should be empty"), Target.Type.IsEmpty());
        TestEqual(TEXT("Only features: Features count should be 2"), Target.Features.Num(), 2);
        TestEqual(TEXT("Only features: color should be 'red'"), Target.Features.FindRef(TEXT("color")), FString(TEXT("red")));
        TestEqual(TEXT("Only features: name should be 'item1'"), Target.Features.FindRef(TEXT("name")), FString(TEXT("item1")));
    }
    
    // 测试 class + type (无 features)
    {
        FString JsonClassType = TEXT("{\"class\":\"robot\",\"type\":\"UGV\"}");
        FMASemanticTarget Target;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(JsonClassType, Target);
        
        TestEqual(TEXT("Class+Type: Class should be 'robot'"), Target.Class, FString(TEXT("robot")));
        TestEqual(TEXT("Class+Type: Type should be 'UGV'"), Target.Type, FString(TEXT("UGV")));
        TestEqual(TEXT("Class+Type: Features should be empty"), Target.Features.Num(), 0);
    }
    
    return true;
}

//=============================================================================
// Property 1 补充测试: Round-Trip 一致性
// 验证解析后的数据可以重新序列化并再次解析得到相同结果
//=============================================================================

/**
 * Property Test: Round-Trip Consistency
 * 
 * **Feature: place-skill, Property 1: Semantic Label Parsing Completeness**
 * **Validates: Requirements 1.1, 1.2**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMASkillParamsProcessorRoundTripPropertyTest,
    "MultiAgent.Skills.Place.Property1_RoundTripConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMASkillParamsProcessorRoundTripPropertyTest::RunTest(const FString& Parameters)
{
    using namespace MASkillParamsProcessorTestHelpers;
    
    const int32 NumIterations = 100;
    int32 PassedCount = 0;
    
    for (int32 i = 0; i < NumIterations; i++)
    {
        // 生成随机测试用例
        FSemanticLabelTestCase TestCase = GenerateRandomTestCase();
        
        // 第一次解析
        FMASemanticTarget FirstParse;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(TestCase.JsonString, FirstParse);
        
        // 重新序列化
        FString ReserializedJson = SemanticTargetToJson(FirstParse.Class, FirstParse.Type, FirstParse.Features);
        
        // 第二次解析
        FMASemanticTarget SecondParse;
        FMASkillParamsProcessor::ParseSemanticTargetFromJson(ReserializedJson, SecondParse);
        
        // 验证两次解析结果一致
        bool bMatch = true;
        
        if (FirstParse.Class != SecondParse.Class)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Round-trip Class mismatch - First '%s', Second '%s'"), 
                i, *FirstParse.Class, *SecondParse.Class));
            bMatch = false;
        }
        
        if (FirstParse.Type != SecondParse.Type)
        {
            AddError(FString::Printf(TEXT("Iteration %d: Round-trip Type mismatch - First '%s', Second '%s'"), 
                i, *FirstParse.Type, *SecondParse.Type));
            bMatch = false;
        }
        
        if (FirstParse.Features.Num() != SecondParse.Features.Num())
        {
            AddError(FString::Printf(TEXT("Iteration %d: Round-trip Features count mismatch - First %d, Second %d"), 
                i, FirstParse.Features.Num(), SecondParse.Features.Num()));
            bMatch = false;
        }
        else
        {
            for (const auto& Pair : FirstParse.Features)
            {
                const FString* SecondValue = SecondParse.Features.Find(Pair.Key);
                if (!SecondValue || *SecondValue != Pair.Value)
                {
                    AddError(FString::Printf(TEXT("Iteration %d: Round-trip Feature mismatch for key '%s'"), i, *Pair.Key));
                    bMatch = false;
                    break;
                }
            }
        }
        
        if (bMatch)
        {
            PassedCount++;
        }
    }
    
    AddInfo(FString::Printf(TEXT("Property 1 Round-Trip Test: %d/%d iterations passed"), PassedCount, NumIterations));
    
    return PassedCount == NumIterations;
}

#endif // WITH_DEV_AUTOMATION_TESTS
