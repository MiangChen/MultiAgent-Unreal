// SK_HandleHazard.cpp
// 处理危险技能实现

#include "SK_HandleHazard.h"
#include "MAObservationSkillRuntimeHelpers.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Infrastructure/MASkillConfigBridge.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "../../../Environment/Effect/MAWaterSpray.h"
#include "../../../Environment/Effect/MAFire.h"
#include "../../../Environment/IMAEnvironmentObject.h"
#include "TimerManager.h"

USK_HandleHazard::USK_HandleHazard()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
}

void USK_HandleHazard::ResetHandleHazardRuntimeState()
{
    bHandleSucceeded = false;
    HandleResultMessage.Reset();
    HandleProgress = 0.f;
    StartTime = 0.f;
    CurrentPhase = EHandleHazardPhase::MoveToSafeDistance;
    bIsAircraft = false;
    MinFlightAltitude = 800.f;
    NavigationService = nullptr;
    WaterSpray = nullptr;
    TargetFire.Reset();
    TargetEnvironmentObject.Reset();
    TargetLocation = FVector::ZeroVector;
    SprayStartLocation = FVector::ZeroVector;
}

void USK_HandleHazard::FailHandleHazard(
    const FString& ResultMessage,
    const FString& ErrorReason,
    const FString& StatusMessage)
{
    bHandleSucceeded = false;
    HandleResultMessage = ResultMessage;

    UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] %s"), *ErrorReason);

    if (!StatusMessage.IsEmpty())
    {
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_HandleHazard::InitializeHandleHazardContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    FMASkillConfigBridge::ApplyHandleHazardConfig(
        Character,
        SafeDistance,
        HandleDuration,
        SpraySpeed,
        SprayWidth);

    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Loaded config: SafeDistance=%.0f, Duration=%.1f, SpraySpeed=%.0f, SprayWidth=%.0f"),
        SafeDistance, HandleDuration, SpraySpeed, SprayWidth);

    const FMAFeedbackContext& Context = SkillComp.GetFeedbackContext();
    AActor* HazardActor = SkillComp.GetSkillRuntimeTargets().HazardTargetActor.Get();

    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: ActivateAbility - HazardTargetActor=%s, TargetLocation=%s"),
        *Character.AgentLabel,
        HazardActor ? *HazardActor->GetName() : TEXT("null"),
        *Context.TargetLocation.ToString());

    TargetFire = Cast<AMAFire>(HazardActor);

    if (TargetFire.IsValid())
    {
        TargetLocation = TargetFire->GetActorLocation();
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Target is AMAFire at %s"),
            *Character.AgentLabel, *TargetLocation.ToString());
    }
    else if (HazardActor)
    {
        if (IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(HazardActor))
        {
            const FString IsFireValue = EnvObject->GetFeature(TEXT("is_fire"), TEXT("false"));
            const bool bIsOnFire = IsFireValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);

            UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Target is environment object '%s', is_fire=%s"),
                *Character.AgentLabel, *EnvObject->GetObjectLabel(), *IsFireValue);

            TargetEnvironmentObject = HazardActor;
            TargetLocation = HazardActor->GetActorLocation();

            if (bIsOnFire)
            {
                UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Environment object is on fire, targeting at %s"),
                    *Character.AgentLabel, *TargetLocation.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[SK_HandleHazard] %s: Environment object is NOT on fire, but proceeding anyway"),
                    *Character.AgentLabel);
            }
        }
        else
        {
            TargetLocation = HazardActor->GetActorLocation();
            UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Target is generic actor at %s"),
                *Character.AgentLabel, *TargetLocation.ToString());
        }
    }
    else if (!Context.TargetLocation.IsZero())
    {
        TargetLocation = Context.TargetLocation;
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Using TargetLocation from context: %s"),
            *Character.AgentLabel, *TargetLocation.ToString());
    }
    else
    {
        FailHandleHazard(TEXT("HandleHazard failed: No valid target"), TEXT("No valid target found"), TEXT("[HandleHazard] Target not found"));
        return false;
    }

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailHandleHazard(TEXT("HandleHazard failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    bIsAircraft = MAObservationSkillRuntime::IsAircraft(Character);
    MinFlightAltitude = MAObservationSkillRuntime::ResolveMinFlightAltitude(Character, MinFlightAltitude);

    const float HorizontalDistance = FVector::Dist2D(Character.GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: HorizontalDistance=%.0f, SafeDistance=%.0f, IsAircraft=%s"),
        *Character.AgentLabel, HorizontalDistance, SafeDistance, bIsAircraft ? TEXT("true") : TEXT("false"));

    CurrentPhase = EHandleHazardPhase::MoveToSafeDistance;
    return true;
}

void USK_HandleHazard::HandleNavigationFailure(const FString& Message)
{
    FailHandleHazard(FString::Printf(TEXT("HandleHazard failed: %s"), *Message), Message);
}

FString USK_HandleHazard::ExtinguishHazardTarget()
{
    FString TargetObjectLabel;

    if (TargetFire.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Extinguishing AMAFire"));
        TargetFire->Extinguish();
        return TargetObjectLabel;
    }

    if (TargetEnvironmentObject.IsValid())
    {
        if (IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(TargetEnvironmentObject.Get()))
        {
            if (AMAFire* AttachedFire = EnvObject->GetAttachedFire())
            {
                UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Extinguishing attached fire on environment object"));
                AttachedFire->Extinguish();
            }

            TargetObjectLabel = EnvObject->GetObjectLabel();
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] No specific target to extinguish"));
    }

    return TargetObjectLabel;
}

void USK_HandleHazard::CompleteHandleHazard()
{
    AMACharacter* Character = GetOwningCharacter();

    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] HandleComplete called"));

    if (Character)
    {
        const FVector EndLocation = Character->GetActorLocation();
        const float EndHorizontalDist = FVector::Dist2D(EndLocation, TargetLocation);
        const float MovedDistance = FVector::Dist(SprayStartLocation, EndLocation);
        const float MovedHorizontal = FVector::Dist2D(SprayStartLocation, EndLocation);

        UE_LOG(LogTemp, Warning, TEXT("[SK_HandleHazard] %s: SPRAY PHASE MOVEMENT ANALYSIS:"), *Character->AgentLabel);
        UE_LOG(LogTemp, Warning, TEXT("  StartPos=%s, EndPos=%s"), *SprayStartLocation.ToString(), *EndLocation.ToString());
        UE_LOG(LogTemp, Warning, TEXT("  StartHorizDist=%.0f, EndHorizDist=%.0f, SafeDistance=%.0f"),
            FVector::Dist2D(SprayStartLocation, TargetLocation), EndHorizontalDist, SafeDistance);
        UE_LOG(LogTemp, Warning, TEXT("  MovedDuring Spray: Total=%.0f, Horizontal=%.0f"), MovedDistance, MovedHorizontal);
    }

    const FString TargetObjectLabel = ExtinguishHazardTarget();
    bHandleSucceeded = true;
    HandleResultMessage = TEXT("Hazard handled successfully");

    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Complete!"));

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
            Context.HazardHandleDurationSeconds = HandleDuration;
            Context.HazardTargetId = TargetObjectLabel;
        }

        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Skill completed successfully"),
            *Character->AgentLabel);
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void USK_HandleHazard::CleanupHandleHazardRuntime(
    AMACharacter* Character,
    const bool bWasCancelled,
    bool& bOutSuccessToNotify,
    FString& InOutMessageToNotify)
{
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(ProgressTimerHandle);
            World->GetTimerManager().ClearTimer(TurnTimerHandle);
        }

        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_HandleHazard::OnNavigationCompleted);
            NavigationService->CancelNavigation();
        }

        CleanupWaterSpray();
        Character->ShowStatus(TEXT(""), 0.f);

        if (bWasCancelled && HandleResultMessage.IsEmpty())
        {
            bOutSuccessToNotify = false;
            InOutMessageToNotify = FString::Printf(TEXT("HandleHazard cancelled: Stopped while %s"), *GetPhaseString());
        }
    }

    NavigationService = nullptr;
    WaterSpray = nullptr;
    TargetFire.Reset();
    TargetEnvironmentObject.Reset();
    TargetLocation = FVector::ZeroVector;
    SprayStartLocation = FVector::ZeroVector;
}

void USK_HandleHazard::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetHandleHazardRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailHandleHazard(TEXT("HandleHazard failed: Character not found"), TEXT("Character not found"));
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailHandleHazard(TEXT("HandleHazard failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializeHandleHazardContext(*Character, *SkillComp))
    {
        return;
    }

    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Starting..."));
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Starting phase %s"), *Character->AgentLabel, *GetPhaseString());
    UpdatePhase();
}

void USK_HandleHazard::UpdatePhase()
{
    switch (CurrentPhase)
    {
        case EHandleHazardPhase::MoveToSafeDistance:
            HandleMoveToSafeDistance();
            break;
        case EHandleHazardPhase::TurnToTarget:
            HandleTurnToTarget();
            break;
        case EHandleHazardPhase::StartSpray:
            HandleStartSpray();
            break;
        case EHandleHazardPhase::WaitForExtinguish:
            HandleWaitForExtinguish();
            break;
        case EHandleHazardPhase::Complete:
            HandleComplete();
            break;
    }
}

void USK_HandleHazard::HandleMoveToSafeDistance()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        FailHandleHazard(TEXT("HandleHazard failed: Lost reference during navigation"), TEXT("HandleMoveToSafeDistance lost Character or NavigationService"));
        return;
    }

    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Moving to safe distance..."));

    FVector SafePosition = CalculateSafePosition();
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Navigating to safe position %s"),
        *Character->AgentLabel, *SafePosition.ToString());
    
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_HandleHazard::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(SafePosition, 50.f))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_HandleHazard::OnNavigationCompleted);
        HandleNavigationFailure(TEXT("Could not start navigation"));
    }
}

void USK_HandleHazard::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] OnNavigationCompleted: bSuccess=%s, Message=%s"),
        bSuccess ? TEXT("true") : TEXT("false"), *Message);
    
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_HandleHazard::OnNavigationCompleted);
    }

    if (bSuccess)
    {
        // 导航完成后，先转向目标再开始喷水
        CurrentPhase = EHandleHazardPhase::TurnToTarget;
        UpdatePhase();
    }
    else
    {
        HandleNavigationFailure(Message);
    }
}

void USK_HandleHazard::HandleTurnToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailHandleHazard(TEXT("HandleHazard failed: Character lost during turn"), TEXT("HandleTurnToTarget lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailHandleHazard(TEXT("HandleHazard failed: World not found during turn"), TEXT("HandleTurnToTarget lost World"));
        return;
    }

    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Turning to target..."));
    
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Starting turn to target at %s"),
        *Character->AgentLabel, *TargetLocation.ToString());

    // 启动转向计时器，平滑转向目标
    World->GetTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &USK_HandleHazard::OnTurnTick,
        0.016f,  // ~60fps for smooth rotation
        true
    );
}

void USK_HandleHazard::OnTurnTick()
{
    AMACharacter* Character = GetOwningCharacter();
    UWorld* World = Character ? Character->GetWorld() : nullptr;
    if (!Character || !World)
    {
        FailHandleHazard(TEXT("HandleHazard failed: Character lost during turn"), TEXT("OnTurnTick lost Character or World"));
        return;
    }

    const float DeltaTime = World->GetDeltaSeconds();
    
    const MAObservationSkillRuntime::FTurnTowardTargetResult TurnResult =
        MAObservationSkillRuntime::StepTurnTowardTarget(*Character, TargetLocation, DeltaTime);

    if (TurnResult.bReachedTargetYaw && TurnResult.YawDiff <= KINDA_SMALL_NUMBER)
    {
        // 已经在目标位置，直接开始喷水
        World->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EHandleHazardPhase::StartSpray;
        UpdatePhase();
        return;
    }

    if (TurnResult.bReachedTargetYaw)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Turn complete, YawDiff=%.1f"),
            *Character->AgentLabel, TurnResult.YawDiff);

        World->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EHandleHazardPhase::StartSpray;
        UpdatePhase();
    }
}

void USK_HandleHazard::HandleStartSpray()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailHandleHazard(TEXT("HandleHazard failed: Character lost"), TEXT("HandleStartSpray lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailHandleHazard(TEXT("HandleHazard failed: World not found"), TEXT("HandleStartSpray lost World"));
        return;
    }

    // 记录开始喷水时的位置和距离
    SprayStartLocation = Character->GetActorLocation();
    float StartHorizontalDist = FVector::Dist2D(SprayStartLocation, TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: HandleStartSpray - Position=%s, HorizontalDistToTarget=%.0f, SafeDistance=%.0f"),
        *Character->AgentLabel, *SprayStartLocation.ToString(), StartHorizontalDist, SafeDistance);

    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Starting water spray..."));

    SpawnWaterSpray();

    StartTime = World->GetTimeSeconds();
    HandleProgress = 0.f;

    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Starting progress timer, duration=%.1fs"),
        *Character->AgentLabel, HandleDuration);

    World->GetTimerManager().SetTimer(
        ProgressTimerHandle,
        this,
        &USK_HandleHazard::OnProgressTick,
        0.1f,
        true
    );

    CurrentPhase = EHandleHazardPhase::WaitForExtinguish;
}

void USK_HandleHazard::HandleWaitForExtinguish()
{
    // Progress driven by OnProgressTick
}

void USK_HandleHazard::OnProgressTick()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailHandleHazard(TEXT("HandleHazard failed: Character lost during execution"), TEXT("OnProgressTick lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailHandleHazard(TEXT("HandleHazard failed: World not found during execution"), TEXT("OnProgressTick lost World"));
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();
    const float ElapsedTime = CurrentTime - StartTime;
    HandleProgress = FMath::Clamp(ElapsedTime / HandleDuration, 0.f, 1.f);

    int32 ProgressPercent = FMath::RoundToInt(HandleProgress * 100.f);
    FString StatusText = FString::Printf(TEXT("Extinguishing... %d%%"), ProgressPercent);
    Character->ShowAbilityStatus(TEXT("HandleHazard"), StatusText);

    if (HandleProgress >= 1.f)
    {
        CurrentPhase = EHandleHazardPhase::Complete;
        UpdatePhase();
    }
}

void USK_HandleHazard::HandleComplete()
{
    CompleteHandleHazard();
}

FVector USK_HandleHazard::CalculateSafePosition() const
{
    const AMACharacter* Character = GetOwningCharacter();
    if (!Character) return TargetLocation;

    const FVector SafePos = MAObservationSkillRuntime::CalculateStandOffPosition(
        *Character, TargetLocation, SafeDistance, bIsAircraft, MinFlightAltitude);

    if (bIsAircraft)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Aircraft safe position: %s (MinAltitude=%.0f)"),
            *SafePos.ToString(), MinFlightAltitude);
    }

    return SafePos;
}

void USK_HandleHazard::SpawnWaterSpray()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] SpawnWaterSpray: Invalid Character or World"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Spawning water spray at %s targeting %s (Speed=%.0f, Width=%.0f)"),
        *Character->AgentLabel, *Character->GetActorLocation().ToString(), *TargetLocation.ToString(),
        SpraySpeed, SprayWidth);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;
    
    WaterSpray = Character->GetWorld()->SpawnActor<AMAWaterSpray>(
        AMAWaterSpray::StaticClass(),
        Character->GetActorLocation(),
        Character->GetActorRotation(),
        SpawnParams
    );

    if (WaterSpray)
    {
        // 设置喷射参数
        WaterSpray->SetSprayParameters(SpraySpeed, SprayWidth);
        
        WaterSpray->AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
        WaterSpray->StartSpray(TargetLocation);
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Water spray spawned and started"),
            *Character->AgentLabel);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] %s: Failed to spawn water spray actor"),
            *Character->AgentLabel);
    }
}

void USK_HandleHazard::CleanupWaterSpray()
{
    if (WaterSpray)
    {
        WaterSpray->StopSpray();
        WaterSpray->Destroy();
        WaterSpray = nullptr;
    }
}

FString USK_HandleHazard::GetPhaseString() const
{
    switch (CurrentPhase)
    {
        case EHandleHazardPhase::MoveToSafeDistance: return TEXT("moving to safe distance");
        case EHandleHazardPhase::TurnToTarget: return TEXT("turning to target");
        case EHandleHazardPhase::StartSpray: return TEXT("starting spray");
        case EHandleHazardPhase::WaitForExtinguish: return TEXT("extinguishing");
        case EHandleHazardPhase::Complete: return TEXT("complete");
        default: return TEXT("unknown");
    }
}

void USK_HandleHazard::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();

    bool bSuccessToNotify = bHandleSucceeded;
    FString MessageToNotify = HandleResultMessage;
    CleanupHandleHazardRuntime(Character, bWasCancelled, bSuccessToNotify, MessageToNotify);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::HandleHazard, bSuccessToNotify, MessageToNotify);
        }
    }
}
