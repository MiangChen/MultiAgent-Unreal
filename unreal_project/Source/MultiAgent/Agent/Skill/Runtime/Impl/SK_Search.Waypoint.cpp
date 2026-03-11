#include "SK_Search.h"

#include "Agent/Skill/Infrastructure/MASkillSearchPathBuilder.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "TimerManager.h"

int32 USK_Search::GetFoundObjectCount(const UMASkillComponent* SkillComp) const
{
    return SkillComp ? SkillComp->GetFeedbackContext().FoundObjects.Num() : 0;
}

FString USK_Search::GetRobotTypeLabel() const
{
    return NavigationService.IsValid()
        ? (NavigationService->bIsFlying ? TEXT("Flying") : TEXT("Ground"))
        : TEXT("Unknown");
}

bool USK_Search::HasReachedFoundTargetArea(const AMACharacter& Character) const
{
    if (!bHasFoundTarget)
    {
        return false;
    }

    const FVector CurrentLocation = Character.GetActorLocation();
    for (const FVector& TargetLoc : FoundTargetLocations)
    {
        if (FVector::Dist2D(CurrentLocation, TargetLoc) < 1000.f)
        {
            return true;
        }
    }

    return false;
}

void USK_Search::HandleCoverageWaypointArrival(AMACharacter& Character, UMASkillComponent* SkillComp)
{
    if (HasReachedFoundTargetArea(Character))
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Reached target area, ending search"), *Character.AgentLabel);
        Character.ShowAbilityStatus(TEXT("Search"), TEXT("Target Found!"));
        CompleteSearch(
            true,
            FString::Printf(TEXT("Search succeeded: Found %d objects at waypoint %d/%d"),
                GetFoundObjectCount(SkillComp), CurrentWaypointIndex + 1, SearchPath.Num()),
            false);
        return;
    }

    CurrentWaypointIndex++;
    if (CurrentWaypointIndex >= SearchPath.Num())
    {
        const int32 FoundCount = GetFoundObjectCount(SkillComp);
        Character.ShowAbilityStatus(TEXT("Search"), TEXT("Complete!"));
        CompleteSearch(
            true,
            FoundCount > 0
                ? FString::Printf(TEXT("Search succeeded: Completed %d waypoints, found %d objects"), SearchPath.Num(), FoundCount)
                : FString::Printf(TEXT("Search succeeded: Completed %d waypoints, no objects found"), SearchPath.Num()),
            false);
        return;
    }

    NavigateToNextWaypoint();
}

void USK_Search::HandlePatrolWaypointArrival(AMACharacter& Character, UMASkillComponent* SkillComp)
{
    CurrentWaypointIndex++;
    if (CurrentWaypointIndex >= SearchPath.Num())
    {
        PatrolCycleCount++;

        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s): Patrol cycle %d/%d completed"),
            *Character.AgentLabel, *GetRobotTypeLabel(), PatrolCycleCount, PatrolCycleLimit);

        if (PatrolCycleCount >= PatrolCycleLimit)
        {
            Character.ShowAbilityStatus(TEXT("Search"), TEXT("Patrol Complete!"));
            CompleteSearch(
                true,
                FString::Printf(TEXT("Patrol completed: %d cycles, %d waypoints per cycle, found %d objects"),
                    PatrolCycleCount, SearchPath.Num(), GetFoundObjectCount(SkillComp)),
                false);
            return;
        }

        CurrentWaypointIndex = 0;
        NavigateToNextWaypoint();
        return;
    }

    StartPIPCapture();
}

void USK_Search::HandleNavigationFailure(const FString& Message)
{
    AMACharacter* Character = GetOwningCharacter();
    ConsecutiveNavFailures++;

    UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s (%s): Navigation failed (%d/%d): %s"),
        Character ? *Character->AgentLabel : TEXT("NULL"),
        *GetRobotTypeLabel(),
        ConsecutiveNavFailures, MaxConsecutiveNavFailures,
        *Message);

    if (ConsecutiveNavFailures >= MaxConsecutiveNavFailures)
    {
        CompleteSearch(
            false,
            FString::Printf(TEXT("Search failed: Navigation failed %d times consecutively at waypoint %d/%d"),
                ConsecutiveNavFailures, CurrentWaypointIndex, SearchPath.Num()),
            true);
        return;
    }

    CurrentWaypointIndex++;
    NavigateToNextWaypoint();
}

void USK_Search::CleanupSearchRuntime(
    AMACharacter* Character,
    const bool bWasCancelled,
    bool& bOutSuccessToNotify,
    FString& InOutMessageToNotify)
{
    if (NavigationService.IsValid())
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Search::OnNavigationCompleted);
        if (bWasCancelled && NavigationService->IsNavigating())
        {
            NavigationService->CancelNavigation();
        }
        NavigationService.Reset();
    }

    if (!Character)
    {
        return;
    }

    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().ClearTimer(WaitTimerHandle);
        World->GetTimerManager().ClearTimer(TurnTimerHandle);
        World->GetTimerManager().ClearTimer(PIPDisplayTimerHandle);
    }
    WaitTimerHandle.Invalidate();
    TurnTimerHandle.Invalidate();
    PIPDisplayTimerHandle.Invalidate();

    CleanupPIP();

    Character->bIsMoving = false;
    Character->ShowStatus(TEXT(""), 0.f);

    if (bWasCancelled && SearchResultMessage.IsEmpty())
    {
        bOutSuccessToNotify = false;
        InOutMessageToNotify = CurrentSearchMode == ESearchMode::Patrol
            ? FString::Printf(TEXT("Patrol cancelled: Completed %d cycles, stopped at waypoint %d/%d"),
                PatrolCycleCount, CurrentWaypointIndex, SearchPath.Num())
            : FString::Printf(TEXT("Search cancelled: Stopped at waypoint %d/%d"),
                CurrentWaypointIndex, SearchPath.Num());
    }
}

void USK_Search::GenerateSearchPath()
{
    SearchPath.Empty();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return;

    SearchPath = FMASkillSearchPathBuilder::BuildPath(*Character, *SkillComp, CurrentSearchMode, ScanWidth);
    SkillComp->GetSearchResultsMutable().SearchPath = SearchPath;
}

void USK_Search::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    AMACharacter* Character = GetOwningCharacter();

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s, %s): OnNavigationCompleted - Success=%s, Waypoint=%d/%d, Message=%s"),
        Character ? *Character->AgentLabel : TEXT("NULL"),
        *GetRobotTypeLabel(),
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        bSuccess ? TEXT("true") : TEXT("false"),
        CurrentWaypointIndex, SearchPath.Num(),
        *Message);

    if (bSuccess)
    {
        ConsecutiveNavFailures = 0;
        HandleWaypointArrival();
    }
    else
    {
        HandleNavigationFailure(Message);
    }
}

void USK_Search::HandleWaypointArrival()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();

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

    if (CurrentSearchMode == ESearchMode::Patrol)
    {
        HandlePatrolWaypointArrival(*Character, SkillComp);
        return;
    }

    HandleCoverageWaypointArrival(*Character, SkillComp);
}

void USK_Search::NavigateToNextWaypoint()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService.IsValid())
    {
        CompleteSearch(false, TEXT("Search failed: Character or NavigationService lost"), true);
        return;
    }

    if (CurrentWaypointIndex >= SearchPath.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s: NavigateToNextWaypoint called with invalid index %d/%d"),
            *Character->AgentLabel, CurrentWaypointIndex, SearchPath.Num());
        return;
    }

    FVector TargetLocation = SearchPath[CurrentWaypointIndex];
    if (NavigationService->bIsFlying)
    {
        TargetLocation.Z = Character->GetActorLocation().Z;
    }

    UE_LOG(LogTemp, Verbose, TEXT("[SK_Search] %s: Navigating to waypoint %d/%d at (%.0f, %.0f, %.0f)"),
        *Character->AgentLabel, CurrentWaypointIndex + 1, SearchPath.Num(),
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z);

    const bool bSmoothArrival = (CurrentSearchMode == ESearchMode::Patrol);
    const bool bNavStarted = NavigationService->NavigateTo(TargetLocation, 100.f, bSmoothArrival);

    if (!bNavStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s: Failed to start navigation to waypoint %d"),
            *Character->AgentLabel, CurrentWaypointIndex + 1);

        ConsecutiveNavFailures++;
        if (ConsecutiveNavFailures >= MaxConsecutiveNavFailures)
        {
            CompleteSearch(
                false,
                FString::Printf(TEXT("Search failed: Could not start navigation after %d attempts"),
                    ConsecutiveNavFailures),
                true);
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

    const FString RobotTypeStr = NavigationService.IsValid()
        ? (NavigationService->bIsFlying ? TEXT("Flying") : TEXT("Ground"))
        : TEXT("Unknown");

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s): Patrol mode - waiting %.1fs at waypoint %d/%d"),
        *Character->AgentLabel, *RobotTypeStr, PatrolWaitTime, CurrentWaypointIndex, SearchPath.Num());

    Character->ShowAbilityStatus(TEXT("Search"),
        FString::Printf(TEXT("Patrol %d/%d - Waiting..."), CurrentWaypointIndex, SearchPath.Num()));

    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            WaitTimerHandle,
            this,
            &USK_Search::OnWaitCompleted,
            PatrolWaitTime,
            false);
    }
}

void USK_Search::OnWaitCompleted()
{
    AMACharacter* Character = GetOwningCharacter();

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Patrol wait completed, navigating to next waypoint"),
        Character ? *Character->AgentLabel : TEXT("NULL"));

    NavigateToNextWaypoint();
}
