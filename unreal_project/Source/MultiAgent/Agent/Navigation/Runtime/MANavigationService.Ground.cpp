// MANavigationService.Ground.cpp
// 地面导航、NavMesh 回调与手动导航相关实现

#include "MANavigationService.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

void UMANavigationService::BindNavMeshCompletionCallback(AAIController* AICtrl)
{
    if (UPathFollowingComponent* PathComp = AICtrl ? AICtrl->GetPathFollowingComponent() : nullptr)
    {
        PathComp->OnRequestFinished.AddUObject(this, &UMANavigationService::OnNavMeshMoveCompleted);
    }
}

void UMANavigationService::TryProjectTargetToNavMesh()
{
    if (!OwnerCharacter)
    {
        return;
    }

    FNavLocation ProjectedLocation;
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerCharacter->GetWorld());
    if (!NavSys)
    {
        return;
    }

    // 限制查询范围，避免投影到错误的 NavMesh 层（如地下）
    const FVector QueryExtent(500.f, 500.f, 200.f);
    if (!NavSys->ProjectPointToNavigation(TargetLocation, ProjectedLocation, QueryExtent))
    {
        return;
    }

    // 检查投影结果是否合理（Z 轴偏移不应超过 150）
    const float ZDiff = FMath::Abs(ProjectedLocation.Location.Z - TargetLocation.Z);
    if (ZDiff >= 150.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: Rejected projection with large Z diff (%.0f), keeping original target"),
            *OwnerCharacter->GetName(), ZDiff);
        return;
    }

    if (!ProjectedLocation.Location.Equals(TargetLocation, 1.f))
    {
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Target projected from (%.0f, %.0f, %.0f) to (%.0f, %.0f, %.0f)"),
            *OwnerCharacter->GetName(),
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z,
            ProjectedLocation.Location.X, ProjectedLocation.Location.Y, ProjectedLocation.Location.Z);
        TargetLocation = ProjectedLocation.Location;
    }
}

void UMANavigationService::ScheduleNavMeshFalseSuccessCheck(const FVector& CheckStartLocation)
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    FTimerHandle CheckHandle;
    World->GetTimerManager().SetTimer(
        CheckHandle,
        [this, CheckStartLocation]()
        {
            if (!OwnerCharacter || !bHasActiveNavMeshRequest)
            {
                return;
            }

            const float MovedDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), CheckStartLocation);
            if (MovedDistance >= 30.f)
            {
                return;
            }

            UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: NavMesh FALSE SUCCESS detected!"),
                *OwnerCharacter->GetName());

            if (AAIController* AI = Cast<AAIController>(OwnerCharacter->GetController()))
            {
                AI->StopMovement();
            }

            bHasActiveNavMeshRequest = false;
            StartManualNavigation();
        },
        0.5f,
        false
    );
}

void UMANavigationService::HandleGroundMoveRequestResult(EPathFollowingRequestResult::Type Result, float DistanceToTarget)
{
    if (Result == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: NavMesh path failed, trying manual navigation"),
            *OwnerCharacter->GetName());
        StartManualNavigation();
        return;
    }

    if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        const float ActualDistance = FVector::Dist2D(OwnerCharacter->GetActorLocation(), TargetLocation);
        if (ActualDistance < AcceptanceRadius * 2.f)
        {
            SetNavigationState(EMANavigationState::Arrived);
            CompleteNavigation(true, FString::Printf(
                TEXT("Navigate succeeded: Already at destination (%.0f, %.0f, %.0f)"),
                TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: AlreadyAtGoal but distance=%.0f, trying manual navigation"),
                *OwnerCharacter->GetName(), ActualDistance);
            StartManualNavigation();
        }
        return;
    }

    if (Result == EPathFollowingRequestResult::RequestSuccessful && DistanceToTarget > AcceptanceRadius * 3.f)
    {
        ScheduleNavMeshFalseSuccessCheck(OwnerCharacter->GetActorLocation());
    }
}

bool UMANavigationService::ShouldFallbackToManualNavigation(const FPathFollowingResult& Result, float ActualDistance) const
{
    const bool bTooFar = ActualDistance > AcceptanceRadius * 2.f;
    if (!bTooFar)
    {
        return false;
    }

    if (Result.IsSuccess())
    {
        return true;
    }

    return Result.Code == EPathFollowingResult::Blocked || Result.Code == EPathFollowingResult::OffPath;
}

FString UMANavigationService::BuildNavFailureMessage(EPathFollowingResult::Type ResultCode) const
{
    switch (ResultCode)
    {
        case EPathFollowingResult::Blocked:
            return TEXT("Navigate failed: Path blocked by obstacle");
        case EPathFollowingResult::OffPath:
            return TEXT("Navigate failed: Lost navigation path");
        case EPathFollowingResult::Aborted:
            return TEXT("Navigate failed: Navigation aborted");
        default:
            return TEXT("Navigate failed: Unknown error");
    }
}

bool UMANavigationService::StartGroundNavigation()
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character not found"));
        return false;
    }
    
    AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController());
    if (!AICtrl)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: No AIController, trying manual navigation"), 
            *OwnerCharacter->GetName());
        StartManualNavigation();
        return true;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Starting ground navigation. Location=(%.0f, %.0f, %.0f), Target=(%.0f, %.0f, %.0f)"),
        *OwnerCharacter->GetName(),
        OwnerCharacter->GetActorLocation().X, OwnerCharacter->GetActorLocation().Y, OwnerCharacter->GetActorLocation().Z,
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    
    BindNavMeshCompletionCallback(AICtrl);
    TryProjectTargetToNavMesh();
    
    // 检查距离
    float DistanceToTarget = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetLocation);
    
    // 调用 MoveToLocation
    EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(
        TargetLocation, AcceptanceRadius, true, true, false, true, nullptr);
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: MoveToLocation result = %d"),
        *OwnerCharacter->GetName(), (int32)Result);
    
    // 保存当前请求 ID
    if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
    {
        CurrentRequestID = PathComp->GetCurrentRequestId();
        bHasActiveNavMeshRequest = true;
    }
    
    HandleGroundMoveRequestResult(Result, DistanceToTarget);
    
    return true;
}
void UMANavigationService::OnNavMeshMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    FString OwnerName = OwnerCharacter ? OwnerCharacter->GetName() : TEXT("NULL");
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: OnNavMeshMoveCompleted - RequestID=%d, Result.Code=%d"),
        *OwnerName, RequestID.GetID(), (int32)Result.Code);
    
    // 检查是否是当前请求的回调
    if (!bHasActiveNavMeshRequest || RequestID != CurrentRequestID)
    {
        return;
    }
    
    bHasActiveNavMeshRequest = false;
    
    if (OwnerCharacter)
    {
        const float ActualDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetLocation);
        if (ShouldFallbackToManualNavigation(Result, ActualDistance))
        {
            if (Result.IsSuccess())
            {
                UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: FALSE SUCCESS! Distance %.1f, switching to manual navigation"),
                    *OwnerCharacter->GetName(), ActualDistance);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: NavMesh %s, trying manual navigation"),
                    *OwnerCharacter->GetName(),
                    Result.Code == EPathFollowingResult::Blocked ? TEXT("BLOCKED") : TEXT("OFF_PATH"));
            }
            StartManualNavigation();
            return;
        }
    }
    
    // 根据结果设置状态
    if (Result.IsSuccess())
    {
        CompleteNavigateSuccess();
    }
    else
    {
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, BuildNavFailureMessage(Result.Code));
    }
}
