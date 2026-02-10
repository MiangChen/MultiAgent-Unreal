// FMAUESceneQuery.cpp
// 场景查询辅助工具实现
//
// 本模块提供基于 UE5 场景的对象查询功能，作为场景图查询的回退方案

#include "MAUESceneQuery.h"
#include "../../../Utils/MAGeometryUtils.h"
#include "../../../Environment/IMAEnvironmentObject.h"
#include "../../Character/MACharacter.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogFMAUESceneQuery, Log, All);

//=============================================================================
// GUID 查找接口实现
//=============================================================================

AActor* FMAUESceneQuery::FindActorByGuid(UWorld* World, const FString& Guid)
{
    if (!World || Guid.IsEmpty())
    {
        UE_LOG(LogFMAUESceneQuery, Warning, TEXT("[FindActorByGuid] Invalid parameters: World=%s, Guid=%s"),
            World ? TEXT("valid") : TEXT("null"), *Guid);
        return nullptr;
    }
    
    // 遍历所有 Actor 查找匹配的 GUID
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetActorGuid().ToString() == Guid)
        {
            // UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindActorByGuid] Found Actor '%s' with GUID %s"),
            //     *Actor->GetName(), *Guid);
            return Actor;
        }
    }
    
    UE_LOG(LogFMAUESceneQuery, Warning, TEXT("[FindActorByGuid] Actor not found for GUID: %s"), *Guid);
    return nullptr;
}

TArray<AActor*> FMAUESceneQuery::FindActorsByGuidArray(UWorld* World, const TArray<FString>& GuidArray)
{
    TArray<AActor*> Result;
    
    if (!World || GuidArray.Num() == 0)
    {
        return Result;
    }
    
    // 将 GuidArray 转换为 Set 以加速查找
    TSet<FString> GuidSet;
    for (const FString& Guid : GuidArray)
    {
        GuidSet.Add(Guid);
    }
    
    // 遍历所有 Actor
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor)
        {
            FString ActorGuid = Actor->GetActorGuid().ToString();
            if (GuidSet.Contains(ActorGuid))
            {
                Result.Add(Actor);
                UE_LOG(LogFMAUESceneQuery, Verbose, TEXT("[FindActorsByGuidArray] Found Actor '%s' with GUID %s"),
                    *Actor->GetName(), *ActorGuid);
            }
        }
    }
    
    UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindActorsByGuidArray] Found %d/%d Actors"),
        Result.Num(), GuidArray.Num());
    
    return Result;
}

//=============================================================================
// 主要查询接口
//=============================================================================

FMAUESceneQueryResult FMAUESceneQuery::FindObjectByLabel(UWorld* World, const FMASemanticLabel& Label)
{
    FMAUESceneQueryResult Result;
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
        FString RobotLabel = Label.GetLabel();
        if (!RobotLabel.IsEmpty())
        {
            AMACharacter* Robot = FindRobotByLabel(World, RobotLabel);
            if (Robot)
            {
                Result.Actor = Robot;
                Result.Name = Robot->AgentLabel;
                Result.Location = Robot->GetActorLocation();
                Result.Label = Label;
                Result.bFound = true;
                return Result;
            }
        }
        // 如果没有指定名称，返回第一个匹配的机器人
        else
        {
            for (TActorIterator<AMACharacter> It(World); It; ++It)
            {
                AMACharacter* Character = *It;
                if (Character && MatchesLabel(Character, Label))
                {
                    Result.Actor = Character;
                    Result.Name = Character->AgentLabel;
                    Result.Location = Character->GetActorLocation();
                    Result.Label = ExtractLabel(Character);
                    Result.bFound = true;
                    
                    UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindObjectByLabel] Found robot '%s' matching label"),
                        *Result.Name);
                    return Result;
                }
            }
        }
    }
    
    // 遍历所有 Actor，查找匹配的对象
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        
        // 检查是否实现 IMAEnvironmentObject 接口
        IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(Actor);
        if (EnvObject && EnvObject->MatchesSemanticLabel(Label))
        {
            Result.Actor = Actor;
            Result.Name = EnvObject->GetObjectLabel();
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractLabel(Actor);
            Result.bFound = true;
            
            UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindObjectByLabel] Found environment object '%s' matching label"),
                *Result.Name);
            return Result;
        }
    }
    
    return Result;
}

TArray<FMAUESceneQueryResult> FMAUESceneQuery::FindObjectsInBoundary(UWorld* World, const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices)
{
    TArray<FMAUESceneQueryResult> Results;
    if (!World || BoundaryVertices.Num() < 3) return Results;
    
    // 遍历所有 Actor
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        
        FVector Location = Actor->GetActorLocation();
        
        // 检查位置是否在边界内
        if (!FMAGeometryUtils::IsPointInPolygon2D(Location, BoundaryVertices))
        {
            continue;
        }
        
        // 检查是否实现 IMAEnvironmentObject 接口
        IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(Actor);
        if (EnvObject)
        {
            // 如果指定了标签，检查是否匹配
            if (!Label.IsEmpty() && !EnvObject->MatchesSemanticLabel(Label)) continue;
            
            FMAUESceneQueryResult Result;
            Result.Actor = Actor;
            Result.Name = EnvObject->GetObjectLabel();
            Result.Location = Location;
            Result.Label = ExtractLabel(Actor);
            Result.bFound = true;
            Results.Add(Result);
            continue;
        }
        
        // 检查是否为机器人
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (Character)
        {
            // 如果指定了标签，检查是否匹配
            if (!Label.IsEmpty() && !MatchesLabel(Character, Label)) continue;
            
            FMAUESceneQueryResult Result;
            Result.Actor = Character;
            Result.Name = Character->AgentLabel;
            Result.Location = Location;
            Result.Label = ExtractLabel(Character);
            Result.bFound = true;
            Results.Add(Result);
        }
    }
    
    return Results;
}

FMAUESceneQueryResult FMAUESceneQuery::FindNearestObject(UWorld* World, const FMASemanticLabel& Label, const FVector& FromLocation)
{
    FMAUESceneQueryResult Result;
    if (!World) return Result;
    
    UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindNearestObject] Searching for: %s"), *Label.ToString());
    
    float MinDistance = FLT_MAX;
    int32 MatchCount = 0;
    
    // 遍历所有 Actor
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        
        FString ObjectName;
        bool bMatches = false;
        
        // 检查是否实现 IMAEnvironmentObject 接口
        IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(Actor);
        if (EnvObject)
        {
            // 如果指定了标签，检查是否匹配
            if (!Label.IsEmpty() && !EnvObject->MatchesSemanticLabel(Label))
            {
                continue;
            }
            ObjectName = EnvObject->GetObjectLabel();
            bMatches = true;
        }
        
        // 检查是否为机器人
        if (!bMatches)
        {
            AMACharacter* Character = Cast<AMACharacter>(Actor);
            if (Character)
            {
                // 如果指定了标签，检查是否匹配
                if (!Label.IsEmpty() && !MatchesLabel(Character, Label))
                {
                    continue;
                }
                ObjectName = Character->AgentLabel;
                bMatches = true;
            }
        }
        
        if (!bMatches) continue;
        
        MatchCount++;
        
        UE_LOG(LogFMAUESceneQuery, Verbose, TEXT("[FindNearestObject] Checking object: %s"), *ObjectName);
        
        float Distance = FVector::Dist(FromLocation, Actor->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            Result.Actor = Actor;
            Result.Name = ObjectName;
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractLabel(Actor);
            Result.bFound = true;
            
            UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindNearestObject] Found matching object: %s at distance %.1f"), 
                *Result.Name, Distance);
        }
    }
    
    UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindNearestObject] Found %d matching objects in scene"), MatchCount);
    
    if (!Result.bFound)
    {
        UE_LOG(LogFMAUESceneQuery, Warning, TEXT("[FindNearestObject] No matching object found for label"));
    }
    
    return Result;
}

//=============================================================================
// 通用匹配和提取接口
//=============================================================================

bool FMAUESceneQuery::MatchesLabel(AActor* Actor, const FMASemanticLabel& Label)
{
    if (!Actor) return false;
    
    // 优先检查是否实现 IMAEnvironmentObject 接口
    if (IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(Actor))
    {
        return EnvObject->MatchesSemanticLabel(Label);
    }
    
    // 检查是否为机器人
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        if (Label.IsRobot())
        {
            FString TargetLabel = Label.GetLabel();
            if (TargetLabel.IsEmpty())
            {
                return true; // 没有指定标签，匹配所有机器人
            }
            return Character->AgentLabel.Equals(TargetLabel, ESearchCase::IgnoreCase);
        }
        return false;
    }
    
    // 对于未实现接口的 Actor，返回 false
    return false;
}

FMASemanticLabel FMAUESceneQuery::ExtractLabel(AActor* Actor)
{
    FMASemanticLabel Label;
    if (!Actor) return Label;
    
    // 优先检查是否实现 IMAEnvironmentObject 接口
    if (IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(Actor))
    {
        Label.Class = TEXT("object");
        Label.Type = EnvObject->GetObjectType();
        Label.Features = EnvObject->GetObjectFeatures();
        Label.Features.Add(TEXT("label"), EnvObject->GetObjectLabel());
        return Label;
    }
    
    // 检查是否为机器人
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        Label.Class = TEXT("robot");
        // 将 EMAAgentType 枚举转换为字符串
        Label.Type = UEnum::GetValueAsString(Character->AgentType);
        // 移除枚举前缀 "EMAAgentType::"
        Label.Type.RemoveFromStart(TEXT("EMAAgentType::"));
        Label.Features.Add(TEXT("label"), Character->AgentLabel);
        return Label;
    }
    
    return Label;
}

//=============================================================================
// 机器人查找
//=============================================================================

AMACharacter* FMAUESceneQuery::FindRobotByLabel(UWorld* World, const FString& RobotLabel)
{
    if (!World || RobotLabel.IsEmpty()) 
    {
        UE_LOG(LogFMAUESceneQuery, Warning, TEXT("[FindRobotByLabel] Invalid parameters: World=%s, RobotLabel=%s"), 
            World ? TEXT("valid") : TEXT("null"), *RobotLabel);
        return nullptr;
    }
    
    UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindRobotByLabel] Searching for robot: %s"), *RobotLabel);
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMACharacter::StaticClass(), FoundActors);
    
    UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindRobotByLabel] Found %d characters in scene"), FoundActors.Num());
    
    for (AActor* Actor : FoundActors)
    {
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (Character)
        {
            UE_LOG(LogFMAUESceneQuery, Verbose, TEXT("[FindRobotByLabel] Checking character: %s"), *Character->AgentLabel);
            
            if (Character->AgentLabel.Equals(RobotLabel, ESearchCase::IgnoreCase))
            {
                UE_LOG(LogFMAUESceneQuery, Log, TEXT("[FindRobotByLabel] Found robot: %s"), *Character->AgentLabel);
                return Character;
            }
        }
    }
    
    UE_LOG(LogFMAUESceneQuery, Warning, TEXT("[FindRobotByLabel] Robot not found: %s"), *RobotLabel);
    return nullptr;
}
