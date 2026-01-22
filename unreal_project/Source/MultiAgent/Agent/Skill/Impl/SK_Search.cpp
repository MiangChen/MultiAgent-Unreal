// SK_Search.cpp

#include "SK_Search.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../Utils/MASkillGeometryUtils.h"
#include "../../Character/MACharacter.h"
#include "TimerManager.h"

USK_Search::USK_Search()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
    bSearchSucceeded = false;
    SearchResultMessage = TEXT("");
}

void USK_Search::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    CurrentWaypointIndex = 0;
    LastUIUpdateIndex = -1;
    
    // 重置结果状态
    bSearchSucceeded = false;
    SearchResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bSearchSucceeded = false;
        SearchResultMessage = TEXT("Search failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bSearchSucceeded = false;
        SearchResultMessage = TEXT("Search failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 获取搜索参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    ScanWidth = Params.SearchScanWidth > 0.f ? Params.SearchScanWidth : 800.f;  // 默认扫描宽度改为 800cm (8m)
    
    // 获取已找到的目标位置（用于提前结束）
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    FoundTargetLocations = Context.FoundLocations;
    bHasFoundTarget = FoundTargetLocations.Num() > 0;
    
    if (bHasFoundTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Found %d targets, will end early when reaching nearest target"),
            *Character->AgentName, FoundTargetLocations.Num());
    }
    
    // 生成搜索航线
    GenerateSearchPath();
    
    if (SearchPath.Num() == 0)
    {
        bSearchSucceeded = false;
        SearchResultMessage = TEXT("Search failed: No valid search path generated");
        Character->ShowStatus(TEXT("[Search] No valid path"), 2.f);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    Character->ShowAbilityStatus(TEXT("Search"), FString::Printf(TEXT("0/%d waypoints"), SearchPath.Num()));
    Character->bIsMoving = true;
    
    // 使用 Tick 而不是定时器，更平滑且性能更好
    if (UWorld* World = Character->GetWorld())
    {
        // 绑定到 Tick
        TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &USK_Search::TickSearch), 0.0f);
    }
}

bool USK_Search::TickSearch(float DeltaTime)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bSearchSucceeded = false;
        SearchResultMessage = TEXT("Search failed: Character lost during search");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return false; // 停止 Tick
    }
    
    FVector CurrentLocation = Character->GetActorLocation();
    
    // 检查是否已经飞到目标附近（提前结束）
    if (bHasFoundTarget)
    {
        for (const FVector& TargetLoc : FoundTargetLocations)
        {
            float DistToTarget = FVector::Dist2D(CurrentLocation, TargetLoc);
            if (DistToTarget < 500.f)  // 距离目标 5m 以内就结束
            {
                UMASkillComponent* SkillComp = Character->GetSkillComponent();
                int32 FoundCount = SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
                
                bSearchSucceeded = true;
                SearchResultMessage = FString::Printf(TEXT("Search succeeded: Found %d objects, reached target area at waypoint %d/%d"), 
                    FoundCount, CurrentWaypointIndex, SearchPath.Num());
                
                UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Reached target area, ending search early"), *Character->AgentName);
                Character->ShowAbilityStatus(TEXT("Search"), TEXT("Target Found!"));
                EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
                return false; // 停止 Tick
            }
        }
    }
    
    if (CurrentWaypointIndex >= SearchPath.Num())
    {
        // 搜索完成
        UMASkillComponent* SkillComp = Character->GetSkillComponent();
        int32 FoundCount = SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
        
        bSearchSucceeded = true;
        SearchResultMessage = FoundCount > 0 ? 
            FString::Printf(TEXT("Search succeeded: Completed %d waypoints, found %d objects"), SearchPath.Num(), FoundCount) :
            FString::Printf(TEXT("Search succeeded: Completed %d waypoints, no objects found"), SearchPath.Num());
        
        Character->ShowAbilityStatus(TEXT("Search"), TEXT("Complete!"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        return false; // 停止 Tick
    }
    
    FVector TargetLocation = SearchPath[CurrentWaypointIndex];
    TargetLocation.Z = CurrentLocation.Z;  // 保持高度
    
    float Distance = FVector::Dist2D(CurrentLocation, TargetLocation);
    
    if (Distance < 100.f)
    {
        // 到达当前航点，切换到下一个（阈值放宽到 100cm 加快转弯）
        CurrentWaypointIndex++;
        
        // 每 5 个航点更新一次 UI，减少 UI 更新频率
        if (CurrentWaypointIndex - LastUIUpdateIndex >= 5 || CurrentWaypointIndex >= SearchPath.Num())
        {
            LastUIUpdateIndex = CurrentWaypointIndex;
            if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
            {
                SkillComp->UpdateFeedback(CurrentWaypointIndex, SearchPath.Num());
            }
            Character->ShowAbilityStatus(TEXT("Search"), 
                FString::Printf(TEXT("%d/%d waypoints"), CurrentWaypointIndex, SearchPath.Num()));
        }
        return true; // 继续 Tick
    }
    
    // 平滑移动到目标航点
    FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal2D();
    float MoveSpeed = 600.f;  // cm/s (10 m/s)，大幅提高搜索速度
    FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
    NewLocation.Z = CurrentLocation.Z;
    Character->SetActorLocation(NewLocation);
    
    // 平滑朝向移动方向
    if (!Direction.IsNearlyZero())
    {
        FRotator CurrentRotation = Character->GetActorRotation();
        FRotator TargetRotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f);
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 8.f);
        Character->SetActorRotation(NewRotation);
    }
    
    return true; // 继续 Tick
}

void USK_Search::GenerateSearchPath()
{
    SearchPath.Empty();
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return;
    
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    const TArray<FVector>& Boundary = Params.SearchBoundary;
    
    if (Boundary.Num() < 3) return;
    
    // 使用几何工具生成割草机航线
    SearchPath = FMASkillGeometryUtils::GenerateLawnmowerPath(Boundary, ScanWidth);
    
    // 保存到搜索结果缓存
    SkillComp->GetSearchResultsMutable().SearchPath = SearchPath;
}

void USK_Search::UpdateSearch()
{
    // 保留此方法以兼容，但不再使用（已改用 TickSearch）
}

void USK_Search::NavigateToNextWaypoint()
{
    // 保留此方法以兼容，但不再使用
}

void USK_Search::OnWaypointReached()
{
    // 保留此方法以兼容，但不再使用
}

void USK_Search::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bSearchSucceeded;
    FString MessageToNotify = SearchResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    // 移除 Ticker
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
            World->GetTimerManager().ClearTimer(CheckTimerHandle);
        }
        UpdateTimerHandle.Invalidate();
        CheckTimerHandle.Invalidate();
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && SearchResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            MessageToNotify = FString::Printf(TEXT("Search cancelled: Stopped at waypoint %d/%d"), 
                CurrentWaypointIndex, SearchPath.Num());
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag SearchTag = FGameplayTag::RequestGameplayTag(FName("Command.Search"));
            if (SkillComp->HasMatchingGameplayTag(SearchTag))
            {
                SkillComp->RemoveLooseGameplayTag(SearchTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    SearchPath.Empty();
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
