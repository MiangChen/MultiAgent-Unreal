# MANavigationService Refactor Notes

Updated: 2026-02-27

## Purpose

This note records hidden behavior guards ("tricks") that must not be lost in future refactors, and documents what was intentionally changed in this round.

## Hidden Tricks Preserved

1. Pause order for NavMesh stop:
   - In pause flow, set `bHasActiveNavMeshRequest = false` before `AAIController::StopMovement()`.
   - Reason: `StopMovement()` can synchronously trigger `OnNavMeshMoveCompleted`; if flag is still true, flow can be mis-detected as a normal completion.

2. NavMesh callback gating:
   - `OnNavMeshMoveCompleted` handles result only when both conditions pass:
   - `bHasActiveNavMeshRequest == true` and `RequestID == CurrentRequestID`.

3. NavMesh false-success guard:
   - If initial move request is successful but target is far (`DistanceToTarget > AcceptanceRadius * 3`), run delayed check (`0.5s`).
   - If actor moved less than `30` units, treat as false-success and fallback to manual navigation.

4. Nav projection safety check:
   - Keep query extent `FVector(500, 500, 200)`.
   - Reject projection when `ZDiff >= 150` to avoid wrong layer projection.

5. Manual navigation stuck detector:
   - If movement per update `< 50`, accumulate `ManualNavStuckTime`.
   - If `ManualNavStuckTime > StuckTimeout`, fail navigation.

6. Terminal-state reset safety:
   - `CompleteNavigation` uses delayed idle reset (`0.1s`) only when current state is still terminal (`Arrived/Failed/Cancelled`).

## Intentional Behavior Fixes Added

1. Pause/Resume state snapshot upgraded:
   - Replaced ad-hoc flags (`bWasFollowing`, `bWasUsingManualNav`, `PausedTargetLocation`) with `PauseSnapshot`.
   - Snapshot now stores mode, state, target, follow context, and return-home context.

2. Explicit return-home runtime flag:
   - Added `bIsReturnHomeActive`.
   - Avoid inferring return-home from altitude values.

3. Follow mode aligned with design intent (hybrid follow):
   - Far target (`DistanceToTarget > NavMeshDistanceThreshold`): NavMesh follow updates.
   - Near target: direct follow with local steering/path planner.

4. Flow split with helper boundaries (for safer maintenance):
   - Follow helpers: `StartFollowUpdateTimer`, `HandleFollowFailure`, `UpdateGroundFollowNavMesh`, `UpdateGroundFollowDirect`.
   - Pause/Resume helper: `ResumePausedFlyingOperation`.

## Troubleshooting Entry Points

- Pause/Resume issues:
  - `PauseNavigation`
  - `ResumeNavigation`
  - `CapturePauseSnapshot`
  - `ResumeFromPauseSnapshot`

- Ground navigation issues:
  - `StartGroundNavigation`
  - `HandleGroundMoveRequestResult`
  - `OnNavMeshMoveCompleted`
  - `StartManualNavigation`

- Follow issues:
  - `FollowActor`
  - `UpdateFollowMode`
  - `UpdateGroundFollowNavMesh`
  - `UpdateGroundFollowDirect`

- Return-home issues:
  - `ReturnHome`
  - `UpdateReturnHome`
  - `bIsReturnHomeActive`

## Validation in This Round

- Build command used:
  - `.../UE_5.7/Engine/Build/BatchFiles/Mac/Build.sh MultiAgentEditor Mac Development -Project=.../unreal_project/MultiAgent.uproject -WaitMutex`
- Result: `Succeeded`.

