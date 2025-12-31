// MASceneQuery.h
// 场景查询辅助工具 - 根据语义标签查找场景对象
//
// 临时实现版本 - 用于测试 Place 技能
// TODO: 后续需要对接正式的场景图结构

#pragma once

#include "CoreMinimal.h"

class AActor;
class AMAPickupItem;
class AMACharacter;

// 语义标签结构
struct FMASemanticLabel
{
    FString Class;      // object, robot, ground
    FString Type;       // boat, box, UGV
    TMap<FString, FString> Features;  // color, subtype, name 等
    
    bool IsEmpty() const { return Class.IsEmpty() && Type.IsEmpty() && Features.Num() == 0; }
    bool IsGround() const { return Class.Equals(TEXT("ground"), ESearchCase::IgnoreCase); }
    bool IsRobot() const { return Class.Equals(TEXT("robot"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("UGV"), ESearchCase::IgnoreCase); }
};

// 查询结果
struct FMASceneQueryResult
{
    AActor* Actor = nullptr;
    FString Name;
    FVector Location = FVector::ZeroVector;
    FMASemanticLabel Label;
    bool bFound = false;
};

class MULTIAGENT_API FMASceneQuery
{
public:
    // 根据语义标签查找场景中的对象
    static FMASceneQueryResult FindObjectByLabel(UWorld* World, const FMASemanticLabel& Label);
    
    // 查找边界内的所有匹配对象
    static TArray<FMASceneQueryResult> FindObjectsInBoundary(UWorld* World, const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices);
    
    // 查找最近的匹配对象
    static FMASceneQueryResult FindNearestObject(UWorld* World, const FMASemanticLabel& Label, const FVector& FromLocation);
    
    // 检查对象是否匹配语义标签
    static bool MatchesLabel(AActor* Actor, const FMASemanticLabel& Label);
    
    // 从 Actor 提取语义标签
    static FMASemanticLabel ExtractLabel(AActor* Actor);
    
    // 查找机器人
    static AMACharacter* FindRobotByName(UWorld* World, const FString& RobotName);

private:
    //=========================================================================
    // 临时匹配逻辑 - 基于 ItemName 的模糊匹配
    // TODO: 后续对接正式场景图时替换
    //=========================================================================
    
    /** 临时匹配逻辑：基于 ItemName 进行模糊匹配 */
    static bool MatchesLabel_Temp(AMAPickupItem* Item, const FMASemanticLabel& Label);
    
    /** 临时提取逻辑：从 ItemName 提取语义标签 */
    static FMASemanticLabel ExtractLabel_Temp(AMAPickupItem* Item);
};
