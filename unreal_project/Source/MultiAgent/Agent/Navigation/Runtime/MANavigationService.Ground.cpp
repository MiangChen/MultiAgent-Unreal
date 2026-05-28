// MANavigationService.Ground.cpp
// 地面导航、NavMesh 回调与手动导航相关实现

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
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
    const float ActualDistance = OwnerCharacter
        ? FVector::Dist2D(OwnerCharacter->GetActorLocation(), TargetLocation)
        : DistanceToTarget;
    const FMANavigationGroundRequestFeedback Feedback =
        FMANavigationUseCases::BuildGroundMoveRequestDecision(
            Result,
            DistanceToTarget,
            ActualDistance,
            AcceptanceRadius,
            TargetLocation);

    if (Feedback.Action == EMANavigationGroundRequestAction::FallbackToManual)
    {
        const TCHAR* Reason = Result == EPathFollowingRequestResult::AlreadyAtGoal
            ? TEXT("AlreadyAtGoal but target still far")
            : TEXT("NavMesh path failed");
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: %s, trying manual navigation"),
            *OwnerCharacter->GetName(), Reason);
        StartManualNavigation();
        return;
    }

    if (Feedback.Action == EMANavigationGroundRequestAction::CompleteSuccess)
    {
        SetNavigationState(EMANavigationState::Arrived);
        CompleteNavigation(true, Feedback.CompletionMessage);
        return;
    }

    if (Feedback.bShouldScheduleFalseSuccessCheck)
    {
        ScheduleNavMeshFalseSuccessCheck(OwnerCharacter->GetActorLocation());
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
        const FMANavigationCompletionFeedback Feedback =
            FMANavigationUseCases::BuildGroundCompletionDecision(
                Result,
                ActualDistance,
                AcceptanceRadius,
                TargetLocation);

        if (Feedback.Action == EMANavigationCompletionAction::FallbackToManual)
        {
            if (Feedback.bWasFalseSuccess)
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

        if (Feedback.Action == EMANavigationCompletionAction::CompleteSuccess)
        {
            CompleteNavigateSuccess();
        }
        else
        {
            SetNavigationState(EMANavigationState::Failed);
            CompleteNavigation(false, Feedback.CompletionMessage);
        }
        return;
    }

    if (Result.IsSuccess())
    {
        CompleteNavigateSuccess();
        return;
    }

    const FMANavigationCompletionFeedback Feedback =
        FMANavigationUseCases::BuildGroundCompletionDecision(
            Result,
            0.f,
            AcceptanceRadius,
            TargetLocation);
    SetNavigationState(EMANavigationState::Failed);
    CompleteNavigation(false, Feedback.CompletionMessage);
}
