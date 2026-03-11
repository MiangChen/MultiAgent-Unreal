#include "SK_Search.h"

#include "Agent/Skill/Infrastructure/MASkillPIPCameraBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "TimerManager.h"

void USK_Search::StartPIPCapture()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

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
        NavigateToNextWaypoint();
    }
}

TArray<FRotator> USK_Search::GenerateCameraDirections() const
{
    TArray<FRotator> Directions;

    AMACharacter* Character = const_cast<USK_Search*>(this)->GetOwningCharacter();
    if (!Character) return Directions;

    const bool bIsFlying = NavigationService.IsValid() && NavigationService->bIsFlying;
    const FVector CurrentLoc = Character->GetActorLocation();

    if (bHasFoundTarget && FoundTargetLocations.Num() > 0)
    {
        FVector NearestTarget = FoundTargetLocations[0];
        float MinDist = FVector::DistSquared(CurrentLoc, NearestTarget);
        for (int32 Index = 1; Index < FoundTargetLocations.Num(); ++Index)
        {
            const float Dist = FVector::DistSquared(CurrentLoc, FoundTargetLocations[Index]);
            if (Dist < MinDist)
            {
                MinDist = Dist;
                NearestTarget = FoundTargetLocations[Index];
            }
        }

        const FVector DirToTarget = (NearestTarget - CurrentLoc).GetSafeNormal();
        const FRotator TargetRotation = DirToTarget.Rotation();
        Directions.Add(TargetRotation);
        Directions.Add(FRotator(TargetRotation.Pitch, TargetRotation.Yaw + 120.f, 0.f));
        Directions.Add(FRotator(TargetRotation.Pitch, TargetRotation.Yaw + 240.f, 0.f));
        return Directions;
    }

    const float BasePitch = bIsFlying ? -45.f : 0.f;
    const float BaseYaw = Character->GetActorRotation().Yaw;
    Directions.Add(FRotator(BasePitch, BaseYaw, 0.f));
    Directions.Add(FRotator(BasePitch, BaseYaw + 120.f, 0.f));
    Directions.Add(FRotator(BasePitch, BaseYaw + 240.f, 0.f));
    return Directions;
}

void USK_Search::ShowNextPIPView()
{
    if (CurrentPIPIndex >= PendingCameraDirections.Num())
    {
        CleanupPIP();

        if (CurrentSearchMode == ESearchMode::Coverage && bHasFoundTarget)
        {
            AMACharacter* Character = GetOwningCharacter();
            CompleteSearch(
                true,
                FString::Printf(TEXT("Search succeeded: Found %d objects at waypoint %d/%d"),
                    GetFoundObjectCount(Character ? Character->GetSkillComponent() : nullptr),
                    CurrentWaypointIndex + 1, SearchPath.Num()),
                false);
            return;
        }

        NavigateToNextWaypoint();
        return;
    }

    TargetTurnRotation = PendingCameraDirections[CurrentPIPIndex];
    TargetTurnRotation.Pitch = 0.f;
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

    Character->GetWorld()->GetTimerManager().SetTimer(
        TurnTimerHandle,
        [this]()
        {
            AMACharacter* Character = GetOwningCharacter();
            if (!Character || !Character->GetWorld())
            {
                OnTurnComplete();
                return;
            }

            const float DeltaTime = Character->GetWorld()->GetDeltaSeconds();
            const FRotator CurrentRot = Character->GetActorRotation();
            const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetTurnRotation, DeltaTime, 5.f);
            Character->SetActorRotation(NewRot);

            const float YawDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRot.Yaw, TargetTurnRotation.Yaw));
            if (YawDiff < 5.f)
            {
                Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
                OnTurnComplete();
            }
        },
        0.016f,
        true);
}

void USK_Search::OnTurnComplete()
{
    CreateAndShowPIP(PendingCameraDirections[CurrentPIPIndex]);
}

void USK_Search::CreateAndShowPIP(const FRotator& CameraRotation)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld())
    {
        OnPIPDisplayComplete();
        return;
    }

    CleanupPIP();

    const FVector CameraLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * CameraForwardOffset;

    FMAPIPCameraConfig CameraConfig;
    CameraConfig.Location = CameraLocation;
    CameraConfig.Rotation = CameraRotation;
    CameraConfig.FOV = CameraFOV;
    CameraConfig.Resolution = FIntPoint(640, 480);

    PIPCameraId = FMASkillPIPCameraBridge::CreatePIPCamera(Character->GetWorld(), CameraConfig);
    if (!PIPCameraId.IsValid())
    {
        OnPIPDisplayComplete();
        return;
    }

    FMAPIPDisplayConfig DisplayConfig;
    DisplayConfig.Size = FVector2D(800.f, 450.f);
    DisplayConfig.ScreenPosition = FMASkillPIPCameraBridge::AllocateScreenPosition(Character->GetWorld(), DisplayConfig.Size);
    DisplayConfig.bShowBorder = true;
    DisplayConfig.bShowShadow = true;
    DisplayConfig.BorderColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);
    DisplayConfig.BorderThickness = 3.f;
    DisplayConfig.Title = FString::Printf(TEXT("[Search] %s - View %d/%d"),
        *Character->AgentLabel, CurrentPIPIndex + 1, PendingCameraDirections.Num());

    FMASkillPIPCameraBridge::ShowPIPCamera(Character->GetWorld(), PIPCameraId, DisplayConfig);

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Showing PIP view %d/%d, Rotation=(%.0f, %.0f, %.0f)"),
        *Character->AgentLabel, CurrentPIPIndex + 1, PendingCameraDirections.Num(),
        CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);

    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            PIPDisplayTimerHandle,
            this,
            &USK_Search::OnPIPDisplayComplete,
            PIPDisplayDuration,
            false);
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
    if (AMACharacter* Character = GetOwningCharacter())
    {
        FMASkillPIPCameraBridge::HidePIPCamera(Character->GetWorld(), PIPCameraId);
        FMASkillPIPCameraBridge::DestroyPIPCamera(Character->GetWorld(), PIPCameraId);
    }
}

void USK_Search::CreateCoverageCamera()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld()) return;

    const bool bIsFlying = NavigationService.IsValid() && NavigationService->bIsFlying;

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

    PIPCameraId = FMASkillPIPCameraBridge::CreatePIPCamera(Character->GetWorld(), CameraConfig);
    if (!PIPCameraId.IsValid()) return;

    FMAPIPDisplayConfig DisplayConfig;
    DisplayConfig.Size = FVector2D(800.f, 450.f);
    DisplayConfig.ScreenPosition = FMASkillPIPCameraBridge::AllocateScreenPosition(Character->GetWorld(), DisplayConfig.Size);
    DisplayConfig.bShowBorder = true;
    DisplayConfig.bShowShadow = true;
    DisplayConfig.BorderColor = FLinearColor(0.3f, 0.5f, 0.8f, 1.f);
    DisplayConfig.BorderThickness = 3.f;
    DisplayConfig.Title = FString::Printf(TEXT("[Search] %s"), *Character->AgentLabel);

    FMASkillPIPCameraBridge::ShowPIPCamera(Character->GetWorld(), PIPCameraId, DisplayConfig);

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Created coverage camera (%s)"),
        *Character->AgentLabel, bIsFlying ? TEXT("down") : TEXT("forward"));
}
