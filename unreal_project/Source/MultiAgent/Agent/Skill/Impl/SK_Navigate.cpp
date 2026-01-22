// SK_Navigate.cpp

#include "SK_Navigate.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Types/MATypes.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

USK_Navigate::USK_Navigate()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    TargetLocation = FVector::ZeroVector;
    bNavigationSucceeded = false;
    NavigationResultMessage = TEXT("");
    bHasActiveRequest = false;
}

void USK_Navigate::SetTargetLocation(FVector InTargetLocation)
{
    TargetLocation = InTargetLocation;
}

bool USK_Navigate::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
    
    AMACharacter* Character = const_cast<USK_Navigate*>(this)->GetOwningCharacter();
    if (!Character) return false;
    
    return Cast<AAIController>(Character->GetController()) != nullptr;
}

void USK_Navigate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置结果状态
    bNavigationSucceeded = false;
    NavigationResultMessage = TEXT("");
    bHasActiveRequest = false;
    LastFlightCheckLogTime = 0.f;
    
    AMACharacter* Character = GetOwningCharacter();
    
    // 确保清理旧的定时器（技能实例复用时可能残留）
    if (Character && Character->GetWorld())
    {
        if (FlightCheckTimerHandle.IsValid())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
        }
        if (ManualNavTimerHandle.IsValid())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(ManualNavTimerHandle);
        }
    }
    
    StartNavigation();
}

void USK_Navigate::StartNavigation()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: Character not found");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 检查是否为飞行器类型
    bool bIsFlying = (Character->AgentType == EMAAgentType::UAV || Character->AgentType == EMAAgentType::FixedWingUAV);
    
    if (bIsFlying)
    {
        // 飞行器使用 Character 自己的导航方法
        Character->ShowAbilityStatus(TEXT("Navigate"), FString::Printf(TEXT("-> (%.0f, %.0f, %.0f)"), TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
        Character->bIsMoving = true;
        
        bool bNavStarted = Character->TryNavigateTo(TargetLocation);
        
        if (bNavStarted)
        {
            // 飞行器导航启动成功，设置定时检查到达
            if (UWorld* World = Character->GetWorld())
            {
                // 确保旧定时器被清理（防止残留）
                if (FlightCheckTimerHandle.IsValid())
                {
                    World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
                }
                
                // 使用 Lambda 捕获 this 指针，确保回调安全
                FTimerDelegate TimerDelegate;
                TimerDelegate.BindWeakLambda(this, [this]()
                {
                    CheckFlightArrival();
                });
                
                World->GetTimerManager().SetTimer(
                    FlightCheckTimerHandle,
                    TimerDelegate,
                    0.1f,
                    true
                );
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: TryNavigateTo failed"), *Character->AgentName);
            bNavigationSucceeded = false;
            NavigationResultMessage = TEXT("Navigate failed: Flight navigation failed to start");
            CleanupNavigation();
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        }
        return;
    }
    
    // 地面机器人使用 AIController 导航
    AAIController* AICtrl = Cast<AAIController>(Character->GetController());
    if (!AICtrl)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: No AIController"), *Character->AgentName);
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: No AIController");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 调试：打印角色信息
    UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: Starting ground navigation. AgentType=%d, Location=(%.0f, %.0f, %.0f), Target=(%.0f, %.0f, %.0f)"),
        *Character->AgentName, (int32)Character->AgentType,
        Character->GetActorLocation().X, Character->GetActorLocation().Y, Character->GetActorLocation().Z,
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    
    if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
    {
        PathComp->OnRequestFinished.AddUObject(this, &USK_Navigate::OnMoveCompleted);
        
        UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: PathFollowingComponent found - Status=%d, HasValidPath=%s"),
            *Character->AgentName,
            (int32)PathComp->GetStatus(),
            PathComp->HasValidPath() ? TEXT("true") : TEXT("false"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Navigate] %s: No PathFollowingComponent!"), *Character->AgentName);
    }
    
    Character->ShowAbilityStatus(TEXT("Navigate"), FString::Printf(TEXT("-> (%.0f, %.0f)"), TargetLocation.X, TargetLocation.Y));
    Character->bIsMoving = true;
    
    // 将目标点投影到 NavMesh 上，确保目标可达
    FNavLocation ProjectedLocation;
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Character->GetWorld());
    if (NavSys)
    {
        // 使用较大的查询范围来处理高度差
        FVector QueryExtent(500.f, 500.f, 500.f);
        if (NavSys->ProjectPointToNavigation(TargetLocation, ProjectedLocation, QueryExtent))
        {
            if (!ProjectedLocation.Location.Equals(TargetLocation, 1.f))
            {
                UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: Target projected from (%.0f, %.0f, %.0f) to (%.0f, %.0f, %.0f)"),
                    *Character->AgentName,
                    TargetLocation.X, TargetLocation.Y, TargetLocation.Z,
                    ProjectedLocation.Location.X, ProjectedLocation.Location.Y, ProjectedLocation.Location.Z);
                TargetLocation = ProjectedLocation.Location;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: Failed to project target (%.0f, %.0f, %.0f) to NavMesh"),
                *Character->AgentName, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
        }
    }
    
    // 在调用 MoveToLocation 之前，先检查距离
    float DistanceToTarget = FVector::Dist(Character->GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: Distance to target BEFORE move: %.1f cm"), 
        *Character->AgentName, DistanceToTarget);
    
    EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(TargetLocation, AcceptanceRadius, true, true, false, true, nullptr);
    
    // 调试：打印导航请求结果和当前状态
    UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: MoveToLocation result = %d (0=Failed, 1=AlreadyAtGoal, 2=RequestSuccessful)"),
        *Character->AgentName, (int32)Result);
    
    // 打印 CharacterMovement 状态
    if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: MovementMode=%d, Velocity=(%.1f, %.1f, %.1f), MaxSpeed=%.1f"),
            *Character->AgentName,
            (int32)MoveComp->MovementMode,
            MoveComp->Velocity.X, MoveComp->Velocity.Y, MoveComp->Velocity.Z,
            MoveComp->MaxWalkSpeed);
    }
    
    // 打印 PathFollowingComponent 状态
    if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Navigate] %s: PathStatus=%d, CurrentRequestID=%d, HasValidPath=%s"),
            *Character->AgentName,
            (int32)PathComp->GetStatus(),
            PathComp->GetCurrentRequestId().GetID(),
            PathComp->HasValidPath() ? TEXT("true") : TEXT("false"));
    }
    
    // 保存当前请求 ID
    if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
    {
        CurrentRequestID = PathComp->GetCurrentRequestId();
        bHasActiveRequest = true;
    }
    
    // 如果距离很远但 NavMesh 说成功，立即切换到手动导航
    if (Result == EPathFollowingRequestResult::RequestSuccessful && DistanceToTarget > AcceptanceRadius * 3.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: Distance %.1f too far for immediate success, will monitor for false success"),
            *Character->AgentName, DistanceToTarget);
        
        // 设置一个短延迟检查，如果 0.5 秒后还没移动就切换手动导航
        if (UWorld* World = Character->GetWorld())
        {
            FVector StartLocation = Character->GetActorLocation();
            FTimerHandle CheckHandle;
            
            World->GetTimerManager().SetTimer(
                CheckHandle,
                [this, StartLocation, DistanceToTarget]()
                {
                    AMACharacter* Char = GetOwningCharacter();
                    if (!Char || !bHasActiveRequest) return;
                    
                    float MovedDistance = FVector::Dist(Char->GetActorLocation(), StartLocation);
                    if (MovedDistance < 50.f)  // 0.5秒内移动不到50cm
                    {
                        UE_LOG(LogTemp, Error, TEXT("[SK_Navigate] %s: NavMesh FALSE SUCCESS detected! Only moved %.1f cm in 0.5s, distance to target %.1f cm"),
                            *Char->AgentName, MovedDistance, DistanceToTarget);
                        
                        // 取消 NavMesh 导航
                        if (AAIController* AI = Cast<AAIController>(Char->GetController()))
                        {
                            AI->StopMovement();
                        }
                        
                        // 切换到手动导航
                        bHasActiveRequest = false;
                        StartManualNavigation();
                    }
                },
                0.5f,
                false
            );
        }
    }
    
    if (Result == EPathFollowingRequestResult::Failed)
    {
        // NavMesh 路径规划失败，尝试手动导航
        UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: NavMesh path failed, trying manual navigation"), *Character->AgentName);
        StartManualNavigation();
    }
    else if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        // 验证是否真的到达目标（避免误判）
        float ActualDistance = FVector::Dist2D(Character->GetActorLocation(), TargetLocation);
        if (ActualDistance < AcceptanceRadius * 2.f)
        {
            bNavigationSucceeded = true;
            NavigationResultMessage = FString::Printf(TEXT("Navigate succeeded: Already at destination (%.0f, %.0f, %.0f)"), 
                TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
            CleanupNavigation();
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        }
        else
        {
            // 距离较远但返回 AlreadyAtGoal，可能是 NavMesh 配置问题，尝试手动导航
            UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: AlreadyAtGoal but distance=%.0f > threshold, trying manual navigation"), 
                *Character->AgentName, ActualDistance);
            StartManualNavigation();
        }
    }
    // RequestSuccessful: 等待 OnMoveCompleted 回调
}

void USK_Navigate::StartManualNavigation()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: Character lost");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    bUsingManualNavigation = true;
    ManualNavStuckTime = 0.f;
    LastManualNavLocation = Character->GetActorLocation();
    
    Character->ShowAbilityStatus(TEXT("Navigate"), TEXT("Manual mode"));
    
    // 启动手动导航更新定时器
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ManualNavTimerHandle,
            FTimerDelegate::CreateUObject(this, &USK_Navigate::UpdateManualNavigation),
            0.05f,
            true
        );
    }
}

void USK_Navigate::UpdateManualNavigation()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: Character lost during manual navigation");
        CleanupNavigation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    FVector CurrentLocation = Character->GetActorLocation();
    float Distance2D = FVector::Dist2D(CurrentLocation, TargetLocation);
    
    // 检查是否到达目标
    if (Distance2D < AcceptanceRadius)
    {
        bNavigationSucceeded = true;
        NavigationResultMessage = FString::Printf(TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f) via manual navigation"), 
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
        CleanupNavigation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        return;
    }
    
    // 检查是否卡住（10秒内移动距离小于50cm）
    float MovedDistance = FVector::Dist(CurrentLocation, LastManualNavLocation);
    if (MovedDistance < 50.f)
    {
        ManualNavStuckTime += 0.05f;
        if (ManualNavStuckTime > 10.f)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: Manual navigation stuck for 10s, giving up"), *Character->AgentName);
            bNavigationSucceeded = false;
            NavigationResultMessage = TEXT("Navigate failed: Path blocked, unable to reach destination");
            CleanupNavigation();
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
            return;
        }
    }
    else
    {
        ManualNavStuckTime = 0.f;
        LastManualNavLocation = CurrentLocation;
    }
    
    // 计算移动方向（使用势场法）
    FVector MoveDirection = CalculatePotentialFieldDirection(Character);
    
    // 使用 AddMovementInput 移动
    Character->AddMovementInput(MoveDirection, 1.0f);
}

FVector USK_Navigate::CalculatePotentialFieldDirection(AMACharacter* Character)
{
    if (!Character) return FVector::ZeroVector;
    
    FVector CurrentLocation = Character->GetActorLocation();
    
    // 吸引力：指向目标
    FVector AttractiveForce = (TargetLocation - CurrentLocation).GetSafeNormal2D();
    
    // 排斥力：来自障碍物
    FVector RepulsiveForce = FVector::ZeroVector;
    float DetectionDistance = 300.f;
    float RepulsiveStrength = 1.5f;
    
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Character);
    
    // 多方向检测障碍物
    TArray<FVector> CheckDirections;
    CheckDirections.Add(AttractiveForce);  // 前方
    CheckDirections.Add(FVector::CrossProduct(FVector::UpVector, AttractiveForce).GetSafeNormal());  // 右
    CheckDirections.Add(-FVector::CrossProduct(FVector::UpVector, AttractiveForce).GetSafeNormal()); // 左
    CheckDirections.Add((AttractiveForce + CheckDirections[1]).GetSafeNormal());  // 右前
    CheckDirections.Add((AttractiveForce + CheckDirections[2]).GetSafeNormal());  // 左前
    
    FVector TraceStart = CurrentLocation + FVector(0.f, 0.f, 50.f);
    
    for (const FVector& Dir : CheckDirections)
    {
        FHitResult HitResult;
        FVector TraceEnd = TraceStart + Dir * DetectionDistance;
        
        bool bHit = Character->GetWorld()->SweepSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            FQuat::Identity,
            ECC_WorldStatic,
            FCollisionShape::MakeSphere(40.f),
            Params
        );
        
        if (bHit && HitResult.Distance > 0.f)
        {
            // 距离越近，排斥力越大
            float DistanceFactor = 1.f - (HitResult.Distance / DetectionDistance);
            DistanceFactor = FMath::Square(DistanceFactor);  // 平方增强近距离效果
            RepulsiveForce -= Dir * DistanceFactor * RepulsiveStrength;
        }
    }
    
    // 合成最终方向
    FVector ResultDirection = AttractiveForce + RepulsiveForce;
    
    if (ResultDirection.IsNearlyZero())
    {
        // 如果合力为零，尝试侧向移动
        return FVector::CrossProduct(FVector::UpVector, AttractiveForce).GetSafeNormal();
    }
    
    return ResultDirection.GetSafeNormal2D();
}

FVector USK_Navigate::CalculateGroundAvoidanceDirection(AMACharacter* Character, const FVector& DesiredDirection)
{
    if (!Character) return DesiredDirection;
    
    FVector CurrentLocation = Character->GetActorLocation();
    float DetectionDistance = 200.f;
    float AvoidanceRadius = 50.f;
    
    // 前方障碍物检测
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Character);
    
    FVector TraceStart = CurrentLocation + FVector(0.f, 0.f, 50.f);  // 稍微抬高避免地面干扰
    FVector TraceEnd = TraceStart + DesiredDirection * DetectionDistance;
    
    bool bHit = Character->GetWorld()->SweepSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeSphere(AvoidanceRadius),
        Params
    );
    
    if (!bHit)
    {
        return DesiredDirection;
    }
    
    // 尝试左右绕行
    FVector RightDir = FVector::CrossProduct(FVector::UpVector, DesiredDirection).GetSafeNormal();
    FVector LeftDir = -RightDir;
    
    // 测试右侧
    TraceEnd = TraceStart + RightDir * DetectionDistance;
    bool bRightBlocked = Character->GetWorld()->SweepSingleByChannel(
        HitResult, TraceStart, TraceEnd, FQuat::Identity,
        ECC_WorldStatic, FCollisionShape::MakeSphere(AvoidanceRadius), Params
    );
    
    // 测试左侧
    TraceEnd = TraceStart + LeftDir * DetectionDistance;
    bool bLeftBlocked = Character->GetWorld()->SweepSingleByChannel(
        HitResult, TraceStart, TraceEnd, FQuat::Identity,
        ECC_WorldStatic, FCollisionShape::MakeSphere(AvoidanceRadius), Params
    );
    
    if (!bRightBlocked)
    {
        return (DesiredDirection * 0.3f + RightDir * 0.7f).GetSafeNormal();
    }
    else if (!bLeftBlocked)
    {
        return (DesiredDirection * 0.3f + LeftDir * 0.7f).GetSafeNormal();
    }
    
    // 两侧都被阻挡，尝试后退
    return -DesiredDirection;
}

void USK_Navigate::CheckFlightArrival()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bNavigationSucceeded = false;
        NavigationResultMessage = TEXT("Navigate failed: Character lost during flight");
        CleanupNavigation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 使用 3D 距离检查（目标高度也需要到达）
    FVector CurrentLoc = Character->GetActorLocation();
    float Distance3D = FVector::Dist(CurrentLoc, TargetLocation);
    
    if (Distance3D < AcceptanceRadius)
    {
        // 到达目标
        bNavigationSucceeded = true;
        NavigationResultMessage = FString::Printf(TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"), 
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
        CleanupNavigation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
    }
}

void USK_Navigate::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 详细日志：记录回调信息
    UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: OnMoveCompleted called - RequestID=%d, CurrentRequestID=%d, bHasActiveRequest=%s, Result.Code=%d, IsSuccess=%s"),
        Character ? *Character->AgentName : TEXT("NULL"),
        RequestID.GetID(),
        CurrentRequestID.GetID(),
        bHasActiveRequest ? TEXT("true") : TEXT("false"),
        (int32)Result.Code,
        Result.IsSuccess() ? TEXT("true") : TEXT("false"));
    
    // 检查是否是当前请求的回调（忽略旧请求的回调）
    if (!bHasActiveRequest || RequestID != CurrentRequestID)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: Ignoring callback - not current request"), 
            Character ? *Character->AgentName : TEXT("NULL"));
        return;
    }
    
    bHasActiveRequest = false;
    
    if (Character) 
    {
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 检查是否真的到达了目标
        float ActualDistance = FVector::Dist(Character->GetActorLocation(), TargetLocation);
        UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: Actual distance to target: %.1f cm (AcceptanceRadius=%.1f)"),
            *Character->AgentName, ActualDistance, AcceptanceRadius);
        
        // 如果距离太远但报告成功，这是一个假成功 - 切换到手动导航
        if (Result.IsSuccess() && ActualDistance > AcceptanceRadius * 2.f)
        {
            UE_LOG(LogTemp, Error, TEXT("[SK_Navigate] %s: FALSE SUCCESS! Distance %.1f > threshold %.1f, switching to manual navigation"),
                *Character->AgentName, ActualDistance, AcceptanceRadius * 2.f);
            
            // 不结束技能，而是切换到手动导航
            StartManualNavigation();
            return;
        }
        
        // 如果 NavMesh 报告 Blocked/OffPath 但距离还很远，尝试手动导航
        if (!Result.IsSuccess() && ActualDistance > AcceptanceRadius * 2.f)
        {
            if (Result.Code == EPathFollowingResult::Blocked || Result.Code == EPathFollowingResult::OffPath)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: NavMesh %s (distance %.1f), trying manual navigation"),
                    *Character->AgentName, 
                    Result.Code == EPathFollowingResult::Blocked ? TEXT("BLOCKED") : TEXT("OFF_PATH"),
                    ActualDistance);
                
                // 切换到手动导航而不是放弃
                StartManualNavigation();
                return;
            }
        }
    }
    
    // 根据结果码设置成功状态和消息
    bNavigationSucceeded = Result.IsSuccess();
    
    switch (Result.Code)
    {
        case EPathFollowingResult::Success: 
            NavigationResultMessage = FString::Printf(TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"), 
                TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
            break;
        case EPathFollowingResult::Blocked: 
            NavigationResultMessage = TEXT("Navigate failed: Path blocked by obstacle"); 
            break;
        case EPathFollowingResult::OffPath: 
            NavigationResultMessage = TEXT("Navigate failed: Lost navigation path"); 
            break;
        case EPathFollowingResult::Aborted: 
            NavigationResultMessage = TEXT("Navigate failed: Navigation aborted"); 
            break;
        default: 
            NavigationResultMessage = TEXT("Navigate failed: Unknown error"); 
            break;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[SK_Navigate] %s: Navigation completed - %s"), 
        Character ? *Character->AgentName : TEXT("NULL"), *NavigationResultMessage);
    
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, !Result.IsSuccess());
}

void USK_Navigate::CleanupNavigation()
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 无条件清理定时器
    if (Character)
    {
        Character->bIsMoving = false;
        
        // 清理飞行检查定时器
        if (FlightCheckTimerHandle.IsValid())
        {
            if (UWorld* World = Character->GetWorld())
            {
                World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
            }
            FlightCheckTimerHandle.Invalidate();
        }
        
        // 清理手动导航定时器
        if (ManualNavTimerHandle.IsValid())
        {
            if (UWorld* World = Character->GetWorld())
            {
                World->GetTimerManager().ClearTimer(ManualNavTimerHandle);
            }
            ManualNavTimerHandle.Invalidate();
        }
        
        // 清理地面导航回调
        if (AAIController* AICtrl = Cast<AAIController>(Character->GetController()))
        {
            if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
            {
                PathComp->OnRequestFinished.RemoveAll(this);
            }
        }
    }
    
    // 重置状态
    bHasActiveRequest = false;
    bUsingManualNavigation = false;
}

void USK_Navigate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 先清理定时器和状态
    CleanupNavigation();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bNavigationSucceeded;
    FString MessageToNotify = NavigationResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && NavigationResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            MessageToNotify = TEXT("Navigate failed: Cancelled by user");
            
            // 停止 AI 移动
            if (AAIController* AICtrl = Cast<AAIController>(Character->GetController()))
            {
                AICtrl->StopMovement();
            }
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag NavigateTag = FGameplayTag::RequestGameplayTag(FName("Command.Navigate"));
            if (SkillComp->HasMatchingGameplayTag(NavigateTag))
            {
                SkillComp->RemoveLooseGameplayTag(NavigateTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    // 先调用父类 EndAbility，确保技能完全结束
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
