// MASceneQuery.cpp
// 场景查询辅助工具实现
//
// 本模块提供基于 UE5 场景的对象查询功能，作为场景图查询的回退方案

#include "MASceneQuery.h"
#include "MASkillGeometryUtils.h"
#include "../../../Environment/MAPickupItem.h"
#include "../../Character/MACharacter.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneQuery, Log, All);

//=============================================================================
// PickupItem 匹配逻辑
// 
// 输入语义标签格式 (来自 Python 端):
// {
//     "class": "object",
//     "type": "cargo",
//     "features": {"color": "red", "label": "RedBox", "subtype": "box"}
// }
//
// 场景中 MAPickupItem 的 ItemName 格式: "RedBox", "BlueBox", "GreenBox"
//
// 匹配策略:
// 1. 如果 features 中有 "label"，直接用 label 匹配 ItemName
// 2. 如果 features 中有 "color"，检查 ItemName 是否包含该颜色
// 3. 如果有 type，检查 ItemName 是否包含该类型（如 "box" -> "Box"）
//=============================================================================

bool FMASceneQuery::MatchesPickupItemLabel(AMAPickupItem* Item, const FMASemanticLabel& Label)
{
    if (!Item) return false;
    
    FString ItemName = Item->ItemName;
    
    // 策略 1: 优先检查 features 中的 "label"，然后检查 "name" (向后兼容)
    if (Label.HasFeature(TEXT("label")))
    {
        FString TargetLabel = Label.Features.FindRef(TEXT("label"));
        if (ItemName.Equals(TargetLabel, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneQuery, Log, TEXT("[MatchesPickupItemLabel] Matched by label: %s == %s"), *ItemName, *TargetLabel);
            return true;
        }
    }
    else if (Label.HasFeature(TEXT("name")))
    {
        // 向后兼容: 如果没有 label，检查 name
        FString TargetName = Label.GetName();
        if (ItemName.Equals(TargetName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogMASceneQuery, Log, TEXT("[MatchesPickupItemLabel] Matched by name (legacy): %s == %s"), *ItemName, *TargetName);
            return true;
        }
    }
    
    // 策略 2: 如果 features 中有 "color"，检查 ItemName 是否包含该颜色
    bool bColorMatch = true;
    if (Label.HasFeature(TEXT("color")))
    {
        FString TargetColor = Label.GetColor();
        bColorMatch = ItemName.Contains(TargetColor, ESearchCase::IgnoreCase);
        if (!bColorMatch)
        {
            UE_LOG(LogMASceneQuery, Verbose, TEXT("[MatchesPickupItemLabel] Color mismatch: %s does not contain %s"), *ItemName, *TargetColor);
            return false;
        }
    }
    
    // 策略 3: 如果有 type，检查 ItemName 是否包含该类型
    bool bTypeMatch = true;
    if (!Label.Type.IsEmpty())
    {
        bTypeMatch = ItemName.Contains(Label.Type, ESearchCase::IgnoreCase);
        if (!bTypeMatch)
        {
            UE_LOG(LogMASceneQuery, Verbose, TEXT("[MatchesPickupItemLabel] Type mismatch: %s does not contain %s"), *ItemName, *Label.Type);
            return false;
        }
    }
    
    // 如果没有指定任何匹配条件，返回 false（避免匹配所有物体）
    if (Label.Features.Num() == 0 && Label.Type.IsEmpty())
    {
        return false;
    }
    
    UE_LOG(LogMASceneQuery, Log, TEXT("[MatchesPickupItemLabel] Matched: %s (color=%s, type=%s)"), 
        *ItemName, 
        bColorMatch ? TEXT("yes") : TEXT("no"),
        bTypeMatch ? TEXT("yes") : TEXT("no"));
    
    return bColorMatch && bTypeMatch;
}

FMASemanticLabel FMASceneQuery::ExtractPickupItemLabel(AMAPickupItem* Item)
{
    FMASemanticLabel Label;
    if (!Item) return Label;
    
    Label.Class = TEXT("object");
    Label.Type = TEXT("cargo");
    
    FString ItemName = Item->ItemName;
    
    // 从 ItemName 提取 color 和 type
    // 假设格式为 "ColorType"，如 "RedBox", "BlueBox"
    // 常见颜色列表
    TArray<FString> Colors = { TEXT("Red"), TEXT("Blue"), TEXT("Green"), TEXT("Yellow"), TEXT("Orange"), TEXT("Purple"), TEXT("Black"), TEXT("White") };
    
    for (const FString& Color : Colors)
    {
        if (ItemName.StartsWith(Color, ESearchCase::IgnoreCase))
        {
            Label.Features.Add(TEXT("color"), Color.ToLower());
            
            // 提取类型（颜色后面的部分）
            FString TypePart = ItemName.Mid(Color.Len());
            if (!TypePart.IsEmpty())
            {
                Label.Type = TypePart.ToLower();
            }
            break;
        }
    }
    
    // 如果没有匹配到颜色，整个名称作为类型
    if (Label.Type.IsEmpty())
    {
        Label.Type = ItemName.ToLower();
    }
    
    // 添加 label 到 features (统一使用 label 而非 name)
    Label.Features.Add(TEXT("label"), ItemName);
    
    return Label;
}

//=============================================================================
// 主要查询接口
//=============================================================================

FMASceneQueryResult FMASceneQuery::FindObjectByLabel(UWorld* World, const FMASemanticLabel& Label)
{
    FMASceneQueryResult Result;
    if (!World || Label.IsEmpty()) return Result;
    
    // 处理 ground 类型
    if (Label.IsGround())
    {
        Result.bFound = true;
        Result.Name = TEXT("Ground");
        Result.Label = Label;
        return Result;
    }
    
    // 处理 robot 类型
    if (Label.IsRobot())
    {
        FString RobotName = Label.GetName();
        if (!RobotName.IsEmpty())
        {
            AMACharacter* Robot = FindRobotByName(World, RobotName);
            if (Robot)
            {
                Result.Actor = Robot;
                Result.Name = Robot->AgentName;
                Result.Location = Robot->GetActorLocation();
                Result.Label = Label;
                Result.bFound = true;
                return Result;
            }
        }
    }
    
    // 查找 PickupItem
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (Item && MatchesPickupItemLabel(Item, Label))
        {
            Result.Actor = Actor;
            Result.Name = Item->ItemName;
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractPickupItemLabel(Item);
            Result.bFound = true;
            return Result;
        }
    }
    
    return Result;
}

TArray<FMASceneQueryResult> FMASceneQuery::FindObjectsInBoundary(UWorld* World, const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices)
{
    TArray<FMASceneQueryResult> Results;
    if (!World || BoundaryVertices.Num() < 3) return Results;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (!Item) continue;
        
        if (!Label.IsEmpty() && !MatchesPickupItemLabel(Item, Label)) continue;
        
        FVector Location = Actor->GetActorLocation();
        if (FMASkillGeometryUtils::IsPointInPolygon(Location, BoundaryVertices))
        {
            FMASceneQueryResult Result;
            Result.Actor = Actor;
            Result.Name = Item->ItemName;
            Result.Location = Location;
            Result.Label = ExtractPickupItemLabel(Item);
            Result.bFound = true;
            Results.Add(Result);
        }
    }
    
    return Results;
}

FMASceneQueryResult FMASceneQuery::FindNearestObject(UWorld* World, const FMASemanticLabel& Label, const FVector& FromLocation)
{
    FMASceneQueryResult Result;
    if (!World) return Result;
    
    UE_LOG(LogMASceneQuery, Log, TEXT("[FindNearestObject] Searching for: %s"), *Label.ToString());
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    UE_LOG(LogMASceneQuery, Log, TEXT("[FindNearestObject] Found %d PickupItems in scene"), FoundActors.Num());
    
    float MinDistance = FLT_MAX;
    
    for (AActor* Actor : FoundActors)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (!Item) continue;
        
        UE_LOG(LogMASceneQuery, Verbose, TEXT("[FindNearestObject] Checking item: %s"), *Item->ItemName);
        
        if (!Label.IsEmpty() && !MatchesPickupItemLabel(Item, Label))
        {
            continue;
        }
        
        float Distance = FVector::Dist(FromLocation, Actor->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            Result.Actor = Actor;
            Result.Name = Item->ItemName;
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractPickupItemLabel(Item);
            Result.bFound = true;
            
            UE_LOG(LogMASceneQuery, Log, TEXT("[FindNearestObject] Found matching item: %s at distance %.1f"), 
                *Item->ItemName, Distance);
        }
    }
    
    if (!Result.bFound)
    {
        UE_LOG(LogMASceneQuery, Warning, TEXT("[FindNearestObject] No matching object found for label"));
    }
    
    return Result;
}

//=============================================================================
// 通用匹配和提取接口
//=============================================================================

bool FMASceneQuery::MatchesLabel(AActor* Actor, const FMASemanticLabel& Label)
{
    if (!Actor) return false;
    
    // 检查是否为 PickupItem
    if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
    {
        return MatchesPickupItemLabel(Item, Label);
    }
    
    // 检查是否为机器人
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        if (Label.IsRobot())
        {
            FString TargetName = Label.GetName();
            if (TargetName.IsEmpty())
            {
                return true; // 没有指定名称，匹配所有机器人
            }
            return Character->AgentName.Equals(TargetName, ESearchCase::IgnoreCase);
        }
        return false;
    }
    
    return false;
}

FMASemanticLabel FMASceneQuery::ExtractLabel(AActor* Actor)
{
    FMASemanticLabel Label;
    if (!Actor) return Label;
    
    // 检查是否为 PickupItem
    if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
    {
        return ExtractPickupItemLabel(Item);
    }
    
    // 检查是否为机器人
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        Label.Class = TEXT("robot");
        // 将 EMAAgentType 枚举转换为字符串
        Label.Type = UEnum::GetValueAsString(Character->AgentType);
        // 移除枚举前缀 "EMAAgentType::"
        Label.Type.RemoveFromStart(TEXT("EMAAgentType::"));
        Label.Features.Add(TEXT("label"), Character->AgentName);
        return Label;
    }
    
    return Label;
}

//=============================================================================
// 机器人查找 (回退方法)
//=============================================================================

AMACharacter* FMASceneQuery::FindRobotByName(UWorld* World, const FString& RobotName)
{
    if (!World || RobotName.IsEmpty()) 
    {
        UE_LOG(LogMASceneQuery, Warning, TEXT("[FindRobotByName] Invalid parameters: World=%s, RobotName=%s"), 
            World ? TEXT("valid") : TEXT("null"), *RobotName);
        return nullptr;
    }
    
    UE_LOG(LogMASceneQuery, Log, TEXT("[FindRobotByName] Searching for robot: %s"), *RobotName);
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMACharacter::StaticClass(), FoundActors);
    
    UE_LOG(LogMASceneQuery, Log, TEXT("[FindRobotByName] Found %d characters in scene"), FoundActors.Num());
    
    for (AActor* Actor : FoundActors)
    {
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (Character)
        {
            UE_LOG(LogMASceneQuery, Verbose, TEXT("[FindRobotByName] Checking character: %s"), *Character->AgentName);
            
            if (Character->AgentName.Equals(RobotName, ESearchCase::IgnoreCase))
            {
                UE_LOG(LogMASceneQuery, Log, TEXT("[FindRobotByName] Found robot: %s"), *Character->AgentName);
                return Character;
            }
        }
    }
    
    UE_LOG(LogMASceneQuery, Warning, TEXT("[FindRobotByName] Robot not found: %s"), *RobotName);
    return nullptr;
}
