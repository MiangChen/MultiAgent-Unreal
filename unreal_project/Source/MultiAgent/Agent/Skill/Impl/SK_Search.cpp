// SK_Search.cpp

#include "SK_Search.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../Utils/MASkillGeometryUtils.h"
#include "../../Character/MACharacter.h"
#include "../../Component/MANavigationService.h"
#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "../../../Core/Camera/Runtime/MAPIPCameraManager.h"
#include "../../../Core/Camera/Domain/MAPIPCameraTypes.h"
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
    PatrolCycleCount = 0;
    ConsecutiveNavFailures = 0;
    
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
    
    // 获取 NavigationService
    NavigationService = Character->GetNavigationService();
    if (!NavigationService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Search] %s: NavigationService not found"), *Character->AgentLabel);
        bSearchSucceeded = false;
        SearchResultMessage = TEXT("Search failed: NavigationService not available");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 获取搜索参数
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    ScanWidth = Params.SearchScanWidth > 0.f ? Params.SearchScanWidth : 800.f;
    CurrentSearchMode = Params.SearchMode;
    PatrolWaitTime = Params.PatrolWaitTime > 0.f ? Params.PatrolWaitTime : 2.0f;
    PatrolCycleLimit = Params.PatrolCycleLimit > 0 ? Params.PatrolCycleLimit : 1;
    
    // 获取已找到的目标位置（用于提前结束）
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    FoundTargetLocations = Context.FoundLocations;
    bHasFoundTarget = FoundTargetLocations.Num() > 0;
    
    if (bHasFoundTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Found %d targets, will end early when reaching nearest target"),
            *Character->AgentLabel, FoundTargetLocations.Num());
    }
    
    // 获取画中画相机管理器
    PIPCameraManager = Character->GetWorld()->GetSubsystem<UMAPIPCameraManager>();
    
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
    
    // 获取机器人类型信息
    FString RobotTypeStr = NavigationService->bIsFlying ? TEXT("Flying") : TEXT("Ground");
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s): Starting %s search with %d waypoints%s"),
        *Character->AgentLabel,
        *RobotTypeStr,
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        SearchPath.Num(),
        CurrentSearchMode == ESearchMode::Patrol ? *FString::Printf(TEXT(", %d cycles"), PatrolCycleLimit) : TEXT(""));
    
    Character->ShowAbilityStatus(TEXT("Search"), FString::Printf(TEXT("0/%d waypoints"), SearchPath.Num()));
    Character->bIsMoving = true;
    
    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Search::OnNavigationCompleted);
    
    // Coverage 模式：创建持续跟随相机
    if (CurrentSearchMode == ESearchMode::Coverage)
    {
        CreateCoverageCamera();
    }
    
    // 启动第一个航点导航
    NavigateToNextWaypoint();
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
    
    // 根据搜索模式生成路径
    if (CurrentSearchMode == ESearchMode::Coverage)
    {
        // Coverage 模式：使用割草机式路径
        SearchPath = FMASkillGeometryUtils::GenerateLawnmowerPath(Boundary, ScanWidth);
    }
    else // ESearchMode::Patrol
    {
        // Patrol 模式：生成巡逻航点 (顶点 + 中心)
        SearchPath = FMASkillGeometryUtils::GeneratePatrolWaypoints(Boundary);
        
        // 使用最近邻排序优化路径
        FVector StartPos = Character->GetActorLocation();
        SearchPath = FMASkillGeometryUtils::SortWaypointsByNearestNeighbor(SearchPath, StartPos);
    }
    
    // 地面机器人需要过滤障碍物内的航点
    UMANavigationService* NavService = Character->GetNavigationService();
    if (NavService && !NavService->bIsFlying)
    {
        // 查询场景图获取建筑物多边形作为障碍物
        TArray<TArray<FVector>> ObstaclePolygons;
        
        if (UGameInstance* GameInstance = Character->GetGameInstance())
        {
            if (UMASceneGraphManager* SceneGraphManager = FMASceneGraphBootstrap::Resolve(GameInstance))
            {
                TArray<FMASceneGraphNode> Buildings = SceneGraphManager->GetAllBuildings();
                for (const FMASceneGraphNode& Building : Buildings)
                {
                    // 只处理有顶点数据的建筑物 (polygon/prism 类型)
                    if (Building.GuidArray.Num() >= 3)
                    {
                        // 从场景图节点获取顶点数据
                        // 注意：GuidArray 存储的是 GUID，需要从 JSON 中获取实际顶点
                        // 这里简化处理：使用 Center 和估算的边界
                        // TODO: 从 WorkingCopy 中获取实际 vertices
                    }
                }
            }
        }
        
        // 过滤障碍物内的航点
        if (ObstaclePolygons.Num() > 0)
        {
            SearchPath = FMASkillGeometryUtils::FilterGroundSafeWaypoints(SearchPath, ObstaclePolygons);
        }
        
        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Ground robot, filtered path has %d waypoints"),
            *Character->AgentLabel, SearchPath.Num());
    }
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Generated %s path with %d waypoints"),
        *Character->AgentLabel,
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        SearchPath.Num());
    
    // 保存到搜索结果缓存
    SkillComp->GetSearchResultsMutable().SearchPath = SearchPath;
}

void USK_Search::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    AMACharacter* Character = GetOwningCharacter();
    FString RobotTypeStr = NavigationService.IsValid() ? (NavigationService->bIsFlying ? TEXT("Flying") : TEXT("Ground")) : TEXT("Unknown");
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s, %s): OnNavigationCompleted - Success=%s, Waypoint=%d/%d, Message=%s"),
        Character ? *Character->AgentLabel : TEXT("NULL"),
        *RobotTypeStr,
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        bSuccess ? TEXT("true") : TEXT("false"),
        CurrentWaypointIndex, SearchPath.Num(),
        *Message);
    
    if (bSuccess)
    {
        // 导航成功，重置失败计数
        ConsecutiveNavFailures = 0;
        
        // 处理航点到达
        HandleWaypointArrival();
    }
    else
    {
        // 导航失败，增加失败计数
        ConsecutiveNavFailures++;
        
        UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s (%s): Navigation failed (%d/%d): %s"),
            Character ? *Character->AgentLabel : TEXT("NULL"),
            *RobotTypeStr,
            ConsecutiveNavFailures, MaxConsecutiveNavFailures,
            *Message);
        
        if (ConsecutiveNavFailures >= MaxConsecutiveNavFailures)
        {
            // 连续失败次数过多，结束搜索
            bSearchSucceeded = false;
            SearchResultMessage = FString::Printf(TEXT("Search failed: Navigation failed %d times consecutively at waypoint %d/%d"),
                ConsecutiveNavFailures, CurrentWaypointIndex, SearchPath.Num());
            
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        }
        else
        {
            // 尝试跳过当前航点，继续下一个
            CurrentWaypointIndex++;
            NavigateToNextWaypoint();
        }
    }
}

void USK_Search::HandleWaypointArrival()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    
    // 更新进度反馈
    if (CurrentWaypointIndex - LastUIUpdateIndex >= 5 || CurrentWaypointIndex >= SearchPath.Num() - 1)
    {
        LastUIUpdateIndex = CurrentWaypointIndex;
        if (SkillComp)
        {
            SkillComp->UpdateFeedback(CurrentWaypointIndex + 1, SearchPath.Num());
        }
        Character->ShowAbilityStatus(TEXT("Search"), 
            FString::Printf(TEXT("%d/%d waypoints"), CurrentWaypointIndex + 1, SearchPath.Num()));
    }
    
    // Coverage 模式：检查提前结束条件
    if (CurrentSearchMode == ESearchMode::Coverage && bHasFoundTarget)
    {
        FVector CurrentLocation = Character->GetActorLocation();
        for (const FVector& TargetLoc : FoundTargetLocations)
        {
            float DistToTarget = FVector::Dist2D(CurrentLocation, TargetLoc);
            if (DistToTarget < 1000.f)  // 距离目标 10m 以内
            {
                int32 FoundCount = SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
                
                bSearchSucceeded = true;
                SearchResultMessage = FString::Printf(TEXT("Search succeeded: Found %d objects at waypoint %d/%d"), 
                    FoundCount, CurrentWaypointIndex + 1, SearchPath.Num());
                
                UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Reached target area, ending search"), 
                    *Character->AgentLabel);
                Character->ShowAbilityStatus(TEXT("Search"), TEXT("Target Found!"));
                EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
                return;
            }
        }
    }
    
    // 移动到下一个航点
    CurrentWaypointIndex++;
    
    // 检查是否完成所有航点
    if (CurrentWaypointIndex >= SearchPath.Num())
    {
        if (CurrentSearchMode == ESearchMode::Patrol)
        {
            // Patrol 模式：循环回到第一个航点
            PatrolCycleCount++;
            
            UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s): Patrol cycle %d/%d completed"),
                *Character->AgentLabel,
                NavigationService.IsValid() ? (NavigationService->bIsFlying ? TEXT("Flying") : TEXT("Ground")) : TEXT("Unknown"),
                PatrolCycleCount, PatrolCycleLimit);
            
            // 检查是否达到巡逻次数限制
            if (PatrolCycleCount >= PatrolCycleLimit)
            {
                // 达到限制，结束巡逻
                int32 FoundCount = SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
                
                bSearchSucceeded = true;
                SearchResultMessage = FString::Printf(TEXT("Patrol completed: %d cycles, %d waypoints per cycle, found %d objects"),
                    PatrolCycleCount, SearchPath.Num(), FoundCount);
                
                Character->ShowAbilityStatus(TEXT("Search"), TEXT("Patrol Complete!"));
                EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
                return;
            }
            
            // 继续下一轮巡逻
            CurrentWaypointIndex = 0;
            NavigateToNextWaypoint();
        }
        else
        {
            // Coverage 模式：搜索完成
            int32 FoundCount = SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
            
            bSearchSucceeded = true;
            SearchResultMessage = FoundCount > 0 ? 
                FString::Printf(TEXT("Search succeeded: Completed %d waypoints, found %d objects"), SearchPath.Num(), FoundCount) :
                FString::Printf(TEXT("Search succeeded: Completed %d waypoints, no objects found"), SearchPath.Num());
            
            Character->ShowAbilityStatus(TEXT("Search"), TEXT("Complete!"));
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        }
        return;
    }
    
    // Patrol 模式：在每个航点展示画中画
    if (CurrentSearchMode == ESearchMode::Patrol)
    {
        StartPIPCapture();
    }
    else
    {
        // Coverage 模式：直接导航到下一个航点
        NavigateToNextWaypoint();
    }
}

void USK_Search::NavigateToNextWaypoint()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService.IsValid())
    {
        bSearchSucceeded = false;
        SearchResultMessage = TEXT("Search failed: Character or NavigationService lost");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    if (CurrentWaypointIndex >= SearchPath.Num())
    {
        // 不应该到达这里，但作为安全检查
        UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s: NavigateToNextWaypoint called with invalid index %d/%d"),
            *Character->AgentLabel, CurrentWaypointIndex, SearchPath.Num());
        return;
    }
    
    FVector TargetLocation = SearchPath[CurrentWaypointIndex];
    
    // 对于飞行机器人，保持当前高度
    if (NavigationService->bIsFlying)
    {
        TargetLocation.Z = Character->GetActorLocation().Z;
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("[SK_Search] %s: Navigating to waypoint %d/%d at (%.0f, %.0f, %.0f)"),
        *Character->AgentLabel, CurrentWaypointIndex + 1, SearchPath.Num(),
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    
    // Coverage 模式禁用平滑到达（保持匀速），Patrol 模式启用（每个航点减速停靠）
    bool bSmoothArrival = (CurrentSearchMode == ESearchMode::Patrol);
    
    // 使用较小的到达半径以确保精确到达
    bool bNavStarted = NavigationService->NavigateTo(TargetLocation, 100.f, bSmoothArrival);
    
    if (!bNavStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s: Failed to start navigation to waypoint %d"),
            *Character->AgentLabel, CurrentWaypointIndex + 1);
        
        // 导航启动失败，尝试下一个航点
        ConsecutiveNavFailures++;
        if (ConsecutiveNavFailures >= MaxConsecutiveNavFailures)
        {
            bSearchSucceeded = false;
            SearchResultMessage = FString::Printf(TEXT("Search failed: Could not start navigation after %d attempts"),
                ConsecutiveNavFailures);
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        }
        else
        {
            CurrentWaypointIndex++;
            NavigateToNextWaypoint();
        }
    }
}

void USK_Search::StartWaypointWait()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    FString RobotTypeStr = NavigationService.IsValid() ? (NavigationService->bIsFlying ? TEXT("Flying") : TEXT("Ground")) : TEXT("Unknown");
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s): Patrol mode - waiting %.1fs at waypoint %d/%d"),
        *Character->AgentLabel, *RobotTypeStr, PatrolWaitTime, CurrentWaypointIndex, SearchPath.Num());
    
    Character->ShowAbilityStatus(TEXT("Search"), 
        FString::Printf(TEXT("Patrol %d/%d - Waiting..."), CurrentWaypointIndex, SearchPath.Num()));
    
    // 设置等待定时器
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            WaitTimerHandle,
            this,
            &USK_Search::OnWaitCompleted,
            PatrolWaitTime,
            false  // 不循环
        );
    }
}

void USK_Search::OnWaitCompleted()
{
    AMACharacter* Character = GetOwningCharacter();
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Patrol wait completed, navigating to next waypoint"),
        Character ? *Character->AgentLabel : TEXT("NULL"));
    
    // 等待完成，导航到下一个航点
    NavigateToNextWaypoint();
}

//=============================================================================
// 画中画相机展示
//=============================================================================

void USK_Search::StartPIPCapture()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    // 如果是 Patrol 模式，生成三个方向的相机
    if (CurrentSearchMode == ESearchMode::Patrol)
    {
        PendingCameraDirections = GenerateCameraDirections();
    }
    
    CurrentPIPIndex = 0;
    
    if (PendingCameraDirections.Num() > 0)
    {
        ShowNextPIPView();
    }
    else
    {
        // 没有需要展示的，继续导航
        NavigateToNextWaypoint();
    }
}

TArray<FRotator> USK_Search::GenerateCameraDirections() const
{
    TArray<FRotator> Directions;
    
    AMACharacter* Character = const_cast<USK_Search*>(this)->GetOwningCharacter();
    if (!Character) return Directions;
    
    bool bIsFlying = NavigationService.IsValid() && NavigationService->bIsFlying;
    FVector CurrentLoc = Character->GetActorLocation();
    
    if (bHasFoundTarget && FoundTargetLocations.Num() > 0)
    {
        // 有目标：相机直接朝向目标
        FVector NearestTarget = FoundTargetLocations[0];
        float MinDist = FVector::DistSquared(CurrentLoc, NearestTarget);
        for (int32 i = 1; i < FoundTargetLocations.Num(); ++i)
        {
            float Dist = FVector::DistSquared(CurrentLoc, FoundTargetLocations[i]);
            if (Dist < MinDist)
            {
                MinDist = Dist;
                NearestTarget = FoundTargetLocations[i];
            }
        }
        
        // 第一个方向：直接朝向目标
        FVector DirToTarget = (NearestTarget - CurrentLoc).GetSafeNormal();
        FRotator TargetRotation = DirToTarget.Rotation();
        Directions.Add(TargetRotation);
        
        // 另外两个方向：保持相同 Pitch，Yaw 平分剩余角度
        Directions.Add(FRotator(TargetRotation.Pitch, TargetRotation.Yaw + 120.f, 0.f));
        Directions.Add(FRotator(TargetRotation.Pitch, TargetRotation.Yaw + 240.f, 0.f));
    }
    else
    {
        // 无目标：飞行机器人向下45°，地面机器人水平，三个方向平分360°
        float BasePitch = bIsFlying ? -45.f : 0.f;
        float BaseYaw = Character->GetActorRotation().Yaw;
        
        Directions.Add(FRotator(BasePitch, BaseYaw, 0.f));
        Directions.Add(FRotator(BasePitch, BaseYaw + 120.f, 0.f));
        Directions.Add(FRotator(BasePitch, BaseYaw + 240.f, 0.f));
    }
    
    return Directions;
}

void USK_Search::ShowNextPIPView()
{
    if (CurrentPIPIndex >= PendingCameraDirections.Num())
    {
        // 所有方向展示完毕
        CleanupPIP();
        
        // Coverage 模式发现目标后结束，Patrol 模式继续导航
        if (CurrentSearchMode == ESearchMode::Coverage && bHasFoundTarget)
        {
            AMACharacter* Character = GetOwningCharacter();
            UMASkillComponent* SkillComp = Character ? Character->GetSkillComponent() : nullptr;
            int32 FoundCount = SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
            
            bSearchSucceeded = true;
            SearchResultMessage = FString::Printf(TEXT("Search succeeded: Found %d objects at waypoint %d/%d"), 
                FoundCount, CurrentWaypointIndex + 1, SearchPath.Num());
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        }
        else
        {
            NavigateToNextWaypoint();
        }
        return;
    }
    
    // 先转向，再拍照
    TargetTurnRotation = PendingCameraDirections[CurrentPIPIndex];
    TargetTurnRotation.Pitch = 0.f;  // 机器人只转 Yaw
    TurnToDirection(TargetTurnRotation);
}

void USK_Search::TurnToDirection(const FRotator& TargetRotation)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld())
    {
        OnTurnComplete();
        return;
    }
    
    TargetTurnRotation = TargetRotation;
    
    // 启动转向定时器
    Character->GetWorld()->GetTimerManager().SetTimer(
        TurnTimerHandle,
        [this]()
        {
            AMACharacter* Char = GetOwningCharacter();
            if (!Char || !Char->GetWorld())
            {
                OnTurnComplete();
                return;
            }
            
            float DeltaTime = Char->GetWorld()->GetDeltaSeconds();
            FRotator CurrentRot = Char->GetActorRotation();
            FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetTurnRotation, DeltaTime, 5.f);
            Char->SetActorRotation(NewRot);
            
            // 检查是否转向完成
            float YawDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRot.Yaw, TargetTurnRotation.Yaw));
            if (YawDiff < 5.f)
            {
                Char->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
                OnTurnComplete();
            }
        },
        0.016f,
        true
    );
}

void USK_Search::OnTurnComplete()
{
    // 转向完成，创建并展示画中画
    FRotator CameraRotation = PendingCameraDirections[CurrentPIPIndex];
    CreateAndShowPIP(CameraRotation);
}

void USK_Search::CreateAndShowPIP(const FRotator& CameraRotation)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !PIPCameraManager) 
    {
        OnPIPDisplayComplete();
        return;
    }
    
    // 清理之前的相机
    CleanupPIP();
    
    // 计算相机位置
    FVector CameraLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * CameraForwardOffset;
    
    // 创建相机配置
    FMAPIPCameraConfig CameraConfig;
    CameraConfig.Location = CameraLocation;
    CameraConfig.Rotation = CameraRotation;
    CameraConfig.FOV = CameraFOV;
    CameraConfig.Resolution = FIntPoint(640, 480);
    
    PIPCameraId = PIPCameraManager->CreatePIPCamera(CameraConfig);
    
    if (!PIPCameraId.IsValid())
    {
        OnPIPDisplayComplete();
        return;
    }
    
    // 显示配置：使用自动分配位置
    FMAPIPDisplayConfig DisplayConfig;
    DisplayConfig.Size = FVector2D(800.f, 450.f);
    DisplayConfig.ScreenPosition = PIPCameraManager->AllocateScreenPosition(DisplayConfig.Size);
    DisplayConfig.bShowBorder = true;
    DisplayConfig.bShowShadow = true;
    DisplayConfig.BorderColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);
    DisplayConfig.BorderThickness = 3.f;
    DisplayConfig.Title = FString::Printf(TEXT("[Search] %s - View %d/%d"), 
        *Character->AgentLabel, CurrentPIPIndex + 1, PendingCameraDirections.Num());
    
    PIPCameraManager->ShowPIPCamera(PIPCameraId, DisplayConfig);
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Showing PIP view %d/%d, Rotation=(%.0f, %.0f, %.0f)"),
        *Character->AgentLabel, CurrentPIPIndex + 1, PendingCameraDirections.Num(),
        CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
    
    // 设置展示时长定时器
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            PIPDisplayTimerHandle,
            this,
            &USK_Search::OnPIPDisplayComplete,
            PIPDisplayDuration,
            false
        );
    }
}

void USK_Search::OnPIPDisplayComplete()
{
    CleanupPIP();
    CurrentPIPIndex++;
    ShowNextPIPView();
}

void USK_Search::CleanupPIP()
{
    if (PIPCameraManager && PIPCameraId.IsValid())
    {
        PIPCameraManager->HidePIPCamera(PIPCameraId);
        PIPCameraManager->DestroyPIPCamera(PIPCameraId);
        PIPCameraId.Invalidate();
    }
}

void USK_Search::CreateCoverageCamera()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !PIPCameraManager) return;
    
    bool bIsFlying = NavigationService.IsValid() && NavigationService->bIsFlying;
    
    // 相机配置：跟随机器人
    // 飞行：前下方偏移 + 固定向下朝向（Yaw 无关紧要）
    // 地面：正前方偏移 + 跟随机器人朝向（相机始终看向机器人前方）
    FMAPIPCameraConfig CameraConfig;
    CameraConfig.FollowTarget = Character;
    CameraConfig.FOV = CameraFOV;
    CameraConfig.Resolution = FIntPoint(640, 480);
    CameraConfig.SmoothSpeed = 5.f;
    
    if (bIsFlying)
    {
        CameraConfig.FollowOffset = FVector(100.f, 0.f, -80.f);
        CameraConfig.Rotation = FRotator(-90.f, 0.f, 0.f);
    }
    else
    {
        CameraConfig.FollowOffset = FVector(100.f, 0.f, 0.f);
        CameraConfig.Rotation = FRotator::ZeroRotator;
        CameraConfig.bFollowTargetRotation = true;
    }
    
    PIPCameraId = PIPCameraManager->CreatePIPCamera(CameraConfig);
    if (!PIPCameraId.IsValid()) return;
    
    // 显示配置：使用自动分配的位置
    FMAPIPDisplayConfig DisplayConfig;
    DisplayConfig.Size = FVector2D(800.f, 450.f);
    DisplayConfig.ScreenPosition = PIPCameraManager->AllocateScreenPosition(DisplayConfig.Size);
    DisplayConfig.bShowBorder = true;
    DisplayConfig.bShowShadow = true;
    DisplayConfig.BorderColor = FLinearColor(0.3f, 0.5f, 0.8f, 1.f);  // 蓝色边框
    DisplayConfig.BorderThickness = 3.f;
    DisplayConfig.Title = FString::Printf(TEXT("[Search] %s"), *Character->AgentLabel);
    
    PIPCameraManager->ShowPIPCamera(PIPCameraId, DisplayConfig);
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Created coverage camera (%s)"),
        *Character->AgentLabel, bIsFlying ? TEXT("down") : TEXT("forward"));
}

void USK_Search::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bSearchSucceeded;
    FString MessageToNotify = SearchResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    // 解绑 NavigationService 委托并取消导航
    if (NavigationService.IsValid())
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Search::OnNavigationCompleted);
        
        // 如果被取消，取消导航
        if (bWasCancelled && NavigationService->IsNavigating())
        {
            NavigationService->CancelNavigation();
        }
        
        NavigationService.Reset();
    }
    
    if (Character)
    {
        // 清理等待定时器
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(WaitTimerHandle);
            World->GetTimerManager().ClearTimer(TurnTimerHandle);
            World->GetTimerManager().ClearTimer(PIPDisplayTimerHandle);
        }
        WaitTimerHandle.Invalidate();
        TurnTimerHandle.Invalidate();
        PIPDisplayTimerHandle.Invalidate();
        
        // 清理画中画
        CleanupPIP();
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && SearchResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            if (CurrentSearchMode == ESearchMode::Patrol)
            {
                MessageToNotify = FString::Printf(TEXT("Patrol cancelled: Completed %d cycles, stopped at waypoint %d/%d"), 
                    PatrolCycleCount, CurrentWaypointIndex, SearchPath.Num());
            }
            else
            {
                MessageToNotify = FString::Printf(TEXT("Search cancelled: Stopped at waypoint %d/%d"), 
                    CurrentWaypointIndex, SearchPath.Num());
            }
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
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: EndAbility - Mode=%s, Success=%s, Cycles=%d, Message=%s"),
        Character ? *Character->AgentLabel : TEXT("NULL"),
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        bSuccessToNotify ? TEXT("true") : TEXT("false"),
        PatrolCycleCount,
        *MessageToNotify);
    
    SearchPath.Empty();
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
