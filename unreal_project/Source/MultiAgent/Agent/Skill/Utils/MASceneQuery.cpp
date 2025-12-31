// MASceneQuery.cpp
// 
// 临时实现版本 - 用于测试 Place 技能
// TODO: 后续需要对接正式的场景图结构，届时替换 MatchesLabel_Temp 和 ExtractLabel_Temp

#include "MASceneQuery.h"
#include "MAGeometryUtils.h"
#include "../../../Environment/MAPickupItem.h"
#include "../../Character/MACharacter.h"
#include "Kismet/GameplayStatics.h"

//=============================================================================
// 临时匹配逻辑 - 基于 ItemName 的模糊匹配
// 
// 输入语义标签格式 (来自 Python 端):
// {
//     "class": "object",
//     "type": "box",
//     "features": {"color": "red", "name": "RedBox"}
// }
//
// 场景中 MAPickupItem 的 ItemName 格式: "RedBox", "BlueBox", "GreenBox"
//
// 匹配策略:
// 1. 如果 features 中有 "name"，直接用 name 匹配 ItemName
// 2. 如果 features 中有 "color"，检查 ItemName 是否包含该颜色
// 3. 如果有 type，检查 ItemName 是否包含该类型（如 "box" -> "Box"）
//=============================================================================

bool FMASceneQuery::MatchesLabel_Temp(AMAPickupItem* Item, const FMASemanticLabel& Label)
{
    if (!Item) return false;
    
    FString ItemName = Item->ItemName;
    
    // 策略 1: 如果 features 中有 "name"，直接匹配
    if (Label.Features.Contains(TEXT("name")))
    {
        FString TargetName = Label.Features[TEXT("name")];
        if (ItemName.Equals(TargetName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Log, TEXT("[MatchesLabel_Temp] Matched by name: %s == %s"), *ItemName, *TargetName);
            return true;
        }
    }
    
    // 策略 2: 如果 features 中有 "color"，检查 ItemName 是否包含该颜色
    bool bColorMatch = true;
    if (Label.Features.Contains(TEXT("color")))
    {
        FString TargetColor = Label.Features[TEXT("color")];
        bColorMatch = ItemName.Contains(TargetColor, ESearchCase::IgnoreCase);
        if (!bColorMatch)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[MatchesLabel_Temp] Color mismatch: %s does not contain %s"), *ItemName, *TargetColor);
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
            UE_LOG(LogTemp, Verbose, TEXT("[MatchesLabel_Temp] Type mismatch: %s does not contain %s"), *ItemName, *Label.Type);
            return false;
        }
    }
    
    // 如果没有指定任何匹配条件，返回 false（避免匹配所有物体）
    if (Label.Features.Num() == 0 && Label.Type.IsEmpty())
    {
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MatchesLabel_Temp] Matched: %s (color=%s, type=%s)"), 
        *ItemName, 
        bColorMatch ? TEXT("yes") : TEXT("no"),
        bTypeMatch ? TEXT("yes") : TEXT("no"));
    
    return bColorMatch && bTypeMatch;
}

FMASemanticLabel FMASceneQuery::ExtractLabel_Temp(AMAPickupItem* Item)
{
    FMASemanticLabel Label;
    if (!Item) return Label;
    
    Label.Class = TEXT("object");
    
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
    
    // 添加 name 到 features
    Label.Features.Add(TEXT("name"), ItemName);
    
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
        FString RobotName = Label.Features.Contains(TEXT("name")) ? Label.Features[TEXT("name")] : TEXT("");
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
        if (Item && MatchesLabel_Temp(Item, Label))
        {
            Result.Actor = Actor;
            Result.Name = Item->ItemName;
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractLabel_Temp(Item);
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
        
        if (!Label.IsEmpty() && !MatchesLabel_Temp(Item, Label)) continue;
        
        FVector Location = Actor->GetActorLocation();
        if (FMAGeometryUtils::IsPointInPolygon(Location, BoundaryVertices))
        {
            FMASceneQueryResult Result;
            Result.Actor = Actor;
            Result.Name = Item->ItemName;
            Result.Location = Location;
            Result.Label = ExtractLabel_Temp(Item);
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
    
    UE_LOG(LogTemp, Log, TEXT("[FindNearestObject] Searching for: Class=%s, Type=%s, Features=%d"), 
        *Label.Class, *Label.Type, Label.Features.Num());
    
    // 打印 features 内容
    for (const auto& Pair : Label.Features)
    {
        UE_LOG(LogTemp, Log, TEXT("[FindNearestObject]   Feature: %s = %s"), *Pair.Key, *Pair.Value);
    }
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    UE_LOG(LogTemp, Log, TEXT("[FindNearestObject] Found %d PickupItems in scene"), FoundActors.Num());
    
    float MinDistance = FLT_MAX;
    
    for (AActor* Actor : FoundActors)
    {
        AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
        if (!Item) continue;
        
        UE_LOG(LogTemp, Verbose, TEXT("[FindNearestObject] Checking item: %s"), *Item->ItemName);
        
        if (!Label.IsEmpty() && !MatchesLabel_Temp(Item, Label))
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
            Result.Label = ExtractLabel_Temp(Item);
            Result.bFound = true;
            
            UE_LOG(LogTemp, Log, TEXT("[FindNearestObject] Found matching item: %s at distance %.1f"), 
                *Item->ItemName, Distance);
        }
    }
    
    if (!Result.bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FindNearestObject] No matching object found for label"));
    }
    
    return Result;
}

//=============================================================================
// 原有的匹配逻辑 - 保留但暂不使用
// TODO: 后续对接正式场景图时启用
//=============================================================================

bool FMASceneQuery::MatchesLabel(AActor* Actor, const FMASemanticLabel& Label)
{
    // 临时实现：转发到 MatchesLabel_Temp
    AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
    return MatchesLabel_Temp(Item, Label);
    
    /* 原有实现 - 暂时注释
    if (!Actor) return false;
    
    AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
    if (!Item) return false;
    
    FMASemanticLabel ActorLabel = ExtractLabel(Actor);
    
    if (!Label.Type.IsEmpty() && !ActorLabel.Type.Equals(Label.Type, ESearchCase::IgnoreCase))
    {
        return false;
    }
    
    for (const auto& Pair : Label.Features)
    {
        if (ActorLabel.Features.Contains(Pair.Key))
        {
            if (!ActorLabel.Features[Pair.Key].Equals(Pair.Value, ESearchCase::IgnoreCase))
            {
                return false;
            }
        }
    }
    
    return true;
    */
}

FMASemanticLabel FMASceneQuery::ExtractLabel(AActor* Actor)
{
    // 临时实现：转发到 ExtractLabel_Temp
    AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
    return ExtractLabel_Temp(Item);
    
    /* 原有实现 - 暂时注释
    FMASemanticLabel Label;
    if (!Actor) return Label;
    
    if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
    {
        Label.Class = TEXT("object");
        
        FString Name = Item->ItemName;
        TArray<FString> Parts;
        Name.ParseIntoArray(Parts, TEXT("_"));
        
        if (Parts.Num() >= 2)
        {
            Label.Features.Add(TEXT("color"), Parts[0]);
            Label.Type = Parts[1];
        }
        else if (Parts.Num() == 1)
        {
            Label.Type = Parts[0];
        }
        
        for (const FName& Tag : Actor->Tags)
        {
            FString TagStr = Tag.ToString();
            int32 ColonIndex;
            if (TagStr.FindChar(':', ColonIndex))
            {
                FString Key = TagStr.Left(ColonIndex);
                FString Value = TagStr.Mid(ColonIndex + 1);
                Label.Features.Add(Key, Value);
            }
        }
    }
    
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        Label.Class = TEXT("robot");
        Label.Type = TEXT("UGV");
        Label.Features.Add(TEXT("name"), Character->AgentName);
    }
    
    return Label;
    */
}

AMACharacter* FMASceneQuery::FindRobotByName(UWorld* World, const FString& RobotName)
{
    if (!World || RobotName.IsEmpty()) 
    {
        UE_LOG(LogTemp, Warning, TEXT("[FindRobotByName] Invalid parameters: World=%s, RobotName=%s"), 
            World ? TEXT("valid") : TEXT("null"), *RobotName);
        return nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[FindRobotByName] Searching for robot: %s"), *RobotName);
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMACharacter::StaticClass(), FoundActors);
    
    UE_LOG(LogTemp, Log, TEXT("[FindRobotByName] Found %d characters in scene"), FoundActors.Num());
    
    for (AActor* Actor : FoundActors)
    {
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (Character)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[FindRobotByName] Checking character: %s"), *Character->AgentName);
            
            if (Character->AgentName.Equals(RobotName, ESearchCase::IgnoreCase))
            {
                UE_LOG(LogTemp, Log, TEXT("[FindRobotByName] Found robot: %s"), *Character->AgentName);
                return Character;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[FindRobotByName] Robot not found: %s"), *RobotName);
    return nullptr;
}
