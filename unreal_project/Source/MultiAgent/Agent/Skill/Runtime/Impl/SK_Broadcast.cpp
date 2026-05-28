// SK_Broadcast.cpp
// 喊话技能 - 多阶段实现

#include "SK_Broadcast.h"
#include "MAObservationSkillRuntimeHelpers.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Infrastructure/MASkillConfigBridge.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "../../../Environment/Effect/MAShockWave.h"
#include "../../../Environment/Effect/MATextDisplay.h"
#include "Components/TextRenderComponent.h"
#include "TimerManager.h"

USK_Broadcast::USK_Broadcast()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
}

void USK_Broadcast::ResetBroadcastRuntimeState()
{
    bBroadcastSucceeded = false;
    BroadcastResultMessage.Reset();
    BroadcastProgress = 0.f;
    StartTime = 0.f;
    CurrentPhase = EBroadcastPhase::MoveToDistance;
    bIsAircraft = false;
    MinFlightAltitude = 800.f;
    BroadcastMessage.Reset();
    NavigationService = nullptr;
    BroadcastEffect = nullptr;
    TextDisplay = nullptr;
    TextBubble = nullptr;
    TargetActor.Reset();
    TargetLocation = FVector::ZeroVector;
    TargetName.Reset();
}

void USK_Broadcast::FailBroadcast(
    const FString& ResultMessage,
    const FString& ErrorReason,
    const FString& StatusMessage)
{
    bBroadcastSucceeded = false;
    BroadcastResultMessage = ResultMessage;

    UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] %s"), *ErrorReason);

    if (!StatusMessage.IsEmpty())
    {
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_Broadcast::InitializeBroadcastContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    FMASkillConfigBridge::ApplyBroadcastConfig(
        Character,
        BroadcastDistance,
        BroadcastDuration,
        EffectSpeed,
        EffectWidth,
        EffectRate);

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] Loaded config: BroadcastDistance=%.0f, Duration=%.1f, ShockRate=%.1f"),
        BroadcastDistance, BroadcastDuration, EffectRate);

    const FMASkillParams& Params = SkillComp.GetSkillParams();
    const FMAFeedbackContext& Context = SkillComp.GetFeedbackContext();

    TargetActor = SkillComp.GetSkillRuntimeTargets().BroadcastTargetActor;
    TargetLocation = Context.TargetLocation;
    TargetName = Context.BroadcastTargetName;
    BroadcastMessage = Params.BroadcastMessage;

    if (BroadcastMessage.IsEmpty())
    {
        BroadcastMessage = TEXT("Hello!");
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: ActivateAbility - TargetActor=%s, TargetLocation=%s, Message='%s'"),
        *Character.AgentLabel,
        TargetActor.IsValid() ? *TargetActor->GetName() : TEXT("null"),
        *TargetLocation.ToString(),
        *BroadcastMessage);

    if (!TargetActor.IsValid() && TargetLocation.IsZero())
    {
        FailBroadcast(TEXT("Broadcast failed: No valid target"), TEXT("No valid target found"), TEXT("[Broadcast] Target not found"));
        return false;
    }

    if (TargetActor.IsValid())
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailBroadcast(TEXT("Broadcast failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    bIsAircraft = MAObservationSkillRuntime::IsAircraft(Character);
    MinFlightAltitude = MAObservationSkillRuntime::ResolveMinFlightAltitude(Character, MinFlightAltitude);

    const float HorizontalDistance = FVector::Dist2D(Character.GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: HorizontalDistance=%.0f, BroadcastDistance=%.0f"),
        *Character.AgentLabel, HorizontalDistance, BroadcastDistance);

    CurrentPhase = (HorizontalDistance <= BroadcastDistance * 1.2f && HorizontalDistance >= BroadcastDistance * 0.5f)
        ? EBroadcastPhase::TurnToTarget
        : EBroadcastPhase::MoveToDistance;
    return true;
}

void USK_Broadcast::HandleNavigationFailure(const FString& Message)
{
    FailBroadcast(FString::Printf(TEXT("Broadcast failed: %s"), *Message), Message);
}

void USK_Broadcast::CompleteBroadcast()
{
    AMACharacter* Character = GetOwningCharacter();

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] HandleComplete called"));
    bBroadcastSucceeded = true;
    BroadcastResultMessage = TargetName.IsEmpty()
        ? FString::Printf(TEXT("Broadcast completed: '%s'"), *BroadcastMessage)
        : FString::Printf(TEXT("Broadcast to %s completed: '%s'"), *TargetName, *BroadcastMessage);

    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("Broadcast"), TEXT("Complete!"));

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
            Context.BroadcastDurationSeconds = BroadcastDuration;
        }

        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Skill completed successfully"),
            *Character->AgentLabel);
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void USK_Broadcast::CleanupBroadcastRuntime(
    AMACharacter* Character,
    const bool bWasCancelled,
    bool& bOutSuccessToNotify,
    FString& InOutMessageToNotify)
{
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(BroadcastTimerHandle);
            World->GetTimerManager().ClearTimer(TurnTimerHandle);
            World->GetTimerManager().ClearTimer(ProgressTimerHandle);
        }

        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Broadcast::OnNavigationCompleted);
            NavigationService->CancelNavigation();
        }

        CleanupBroadcastEffect();
        HideTextBubble();
        Character->ShowStatus(TEXT(""), 0.f);

        if (bWasCancelled && BroadcastResultMessage.IsEmpty())
        {
            bOutSuccessToNotify = false;
            InOutMessageToNotify = FString::Printf(TEXT("Broadcast cancelled: Stopped while %s"), *GetPhaseString());
        }
    }

    NavigationService = nullptr;
    BroadcastEffect = nullptr;
    TextDisplay = nullptr;
    TextBubble = nullptr;
    TargetActor.Reset();
    TargetLocation = FVector::ZeroVector;
    TargetName.Reset();
}

void USK_Broadcast::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetBroadcastRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailBroadcast(TEXT("Broadcast failed: Character not found"), TEXT("Character not found"));
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailBroadcast(TEXT("Broadcast failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializeBroadcastContext(*Character, *SkillComp))
    {
        return;
    }

    Character->ShowAbilityStatus(TEXT("Broadcast"), TEXT("Starting..."));
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Starting phase %s"), *Character->AgentLabel, *GetPhaseString());
    UpdatePhase();
}

void USK_Broadcast::UpdatePhase()
{
    switch (CurrentPhase)
    {
        case EBroadcastPhase::MoveToDistance:
            HandleMoveToDistance();
            break;
        case EBroadcastPhase::TurnToTarget:
            HandleTurnToTarget();
            break;
        case EBroadcastPhase::Broadcasting:
            HandleBroadcasting();
            break;
        case EBroadcastPhase::Complete:
            HandleComplete();
            break;
    }
}

void USK_Broadcast::HandleMoveToDistance()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        FailBroadcast(TEXT("Broadcast failed: Lost reference during navigation"), TEXT("HandleMoveToDistance lost Character or NavigationService"));
        return;
    }

    Character->ShowAbilityStatus(TEXT("Broadcast"), TEXT("Moving to broadcast position..."));

    FVector BroadcastPosition = CalculateBroadcastPosition();
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Navigating to broadcast position %s"),
        *Character->AgentLabel, *BroadcastPosition.ToString());
    
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Broadcast::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(BroadcastPosition, 100.f))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Broadcast::OnNavigationCompleted);
        HandleNavigationFailure(TEXT("Could not start navigation"));
    }
}

void USK_Broadcast::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] OnNavigationCompleted: bSuccess=%s, Message=%s"),
        bSuccess ? TEXT("true") : TEXT("false"), *Message);
    
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Broadcast::OnNavigationCompleted);
    }

    if (bSuccess)
    {
        CurrentPhase = EBroadcastPhase::TurnToTarget;
        UpdatePhase();
    }
    else
    {
        HandleNavigationFailure(Message);
    }
}

void USK_Broadcast::HandleTurnToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailBroadcast(TEXT("Broadcast failed: Character lost during turn"), TEXT("HandleTurnToTarget lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailBroadcast(TEXT("Broadcast failed: World not found during turn"), TEXT("HandleTurnToTarget lost World"));
        return;
    }

    Character->ShowAbilityStatus(TEXT("Broadcast"), TEXT("Turning to target..."));
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Starting turn to target at %s"),
        *Character->AgentLabel, *TargetLocation.ToString());

    // 启动转向计时器
    World->GetTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &USK_Broadcast::OnTurnTick,
        0.016f,  // ~60fps
        true
    );
}

void USK_Broadcast::OnTurnTick()
{
    AMACharacter* Character = GetOwningCharacter();
    UWorld* World = Character ? Character->GetWorld() : nullptr;
    if (!Character || !World)
    {
        FailBroadcast(TEXT("Broadcast failed: Character lost during turn"), TEXT("OnTurnTick lost Character or World"));
        return;
    }

    const float DeltaTime = World->GetDeltaSeconds();
    
    const MAObservationSkillRuntime::FTurnTowardTargetResult TurnResult =
        MAObservationSkillRuntime::StepTurnTowardTarget(*Character, TargetLocation, DeltaTime);

    if (TurnResult.bReachedTargetYaw && TurnResult.YawDiff <= KINDA_SMALL_NUMBER)
    {
        World->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EBroadcastPhase::Broadcasting;
        UpdatePhase();
        return;
    }

    if (TurnResult.bReachedTargetYaw)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Turn complete, YawDiff=%.1f"),
            *Character->AgentLabel, TurnResult.YawDiff);

        World->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EBroadcastPhase::Broadcasting;
        UpdatePhase();
    }
}

void USK_Broadcast::HandleBroadcasting()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailBroadcast(TEXT("Broadcast failed: Character lost"), TEXT("HandleBroadcasting lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailBroadcast(TEXT("Broadcast failed: World not found"), TEXT("HandleBroadcasting lost World"));
        return;
    }

    Character->ShowAbilityStatus(TEXT("Broadcast"), BroadcastMessage);

    // 显示文本气泡
    ShowTextBubble(BroadcastMessage);
    
    // 生成喊话特效（暂用喷水特效占位）
    SpawnBroadcastEffect();

    StartTime = World->GetTimeSeconds();
    BroadcastProgress = 0.f;

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Starting broadcast, duration=%.1fs, message='%s'"),
        *Character->AgentLabel, BroadcastDuration, *BroadcastMessage);

    // 启动进度计时器
    World->GetTimerManager().SetTimer(
        ProgressTimerHandle,
        this,
        &USK_Broadcast::OnProgressTick,
        0.1f,
        true
    );
}

void USK_Broadcast::OnProgressTick()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailBroadcast(TEXT("Broadcast failed: Character lost during execution"), TEXT("OnProgressTick lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailBroadcast(TEXT("Broadcast failed: World not found during execution"), TEXT("OnProgressTick lost World"));
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();
    const float ElapsedTime = CurrentTime - StartTime;
    BroadcastProgress = FMath::Clamp(ElapsedTime / BroadcastDuration, 0.f, 1.f);

    int32 ProgressPercent = FMath::RoundToInt(BroadcastProgress * 100.f);
    FString StatusText = FString::Printf(TEXT("Broadcasting... %d%%"), ProgressPercent);
    Character->ShowAbilityStatus(TEXT("Broadcast"), StatusText);

    if (BroadcastProgress >= 1.f)
    {
        CurrentPhase = EBroadcastPhase::Complete;
        UpdatePhase();
    }
}

void USK_Broadcast::HandleComplete()
{
    CompleteBroadcast();
}

FVector USK_Broadcast::CalculateBroadcastPosition() const
{
    const AMACharacter* Character = GetOwningCharacter();
    if (!Character) return TargetLocation;

    const FVector BroadcastPos = MAObservationSkillRuntime::CalculateStandOffPosition(
        *Character, TargetLocation, BroadcastDistance, bIsAircraft, MinFlightAltitude);

    if (bIsAircraft)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] Aircraft broadcast position: %s (MinAltitude=%.0f)"),
            *BroadcastPos.ToString(), MinFlightAltitude);
    }

    return BroadcastPos;
}

void USK_Broadcast::SpawnBroadcastEffect()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] SpawnBroadcastEffect: Invalid Character or World"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Spawning shock wave effect at %s targeting %s"),
        *Character->AgentLabel, *Character->GetActorLocation().ToString(), *TargetLocation.ToString());

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;
    
    // 使用声波特效
    BroadcastEffect = Character->GetWorld()->SpawnActor<AMAShockWave>(
        AMAShockWave::StaticClass(),
        Character->GetActorLocation(),
        Character->GetActorRotation(),
        SpawnParams
    );

    if (BroadcastEffect)
    {
        // 设置特效参数
        BroadcastEffect->SetShockWaveParameters(EffectSpeed, EffectWidth, EffectRate);
        
        BroadcastEffect->AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
        BroadcastEffect->StartShockWave(TargetLocation);
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Shock wave effect spawned and started"),
            *Character->AgentLabel);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] %s: Failed to spawn shock wave effect actor"),
            *Character->AgentLabel);
    }
}

void USK_Broadcast::CleanupBroadcastEffect()
{
    if (BroadcastEffect)
    {
        BroadcastEffect->StopShockWave();
        BroadcastEffect->Destroy();
        BroadcastEffect = nullptr;
    }
}

void USK_Broadcast::ShowTextBubble(const FString& Message)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld()) return;

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Creating text display for message: '%s'"),
        *Character->AgentLabel, *Message);

    // 创建滚动文本显示屏
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;
    
    TextDisplay = Character->GetWorld()->SpawnActor<AMATextDisplay>(
        AMATextDisplay::StaticClass(),
        Character->GetActorLocation() + FVector(0.f, 0.f, 200.f),
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (TextDisplay)
    {
        // 附加到角色 - 只跟随位置，不跟随旋转
        FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
        TextDisplay->AttachToActor(Character, AttachRules);
        
        // 设置显示屏尺寸和文本
        TextDisplay->SetDisplaySize(800.f, 50.f);
        TextDisplay->SetScrollSpeed(100.f);
        TextDisplay->SetText(Message);
        TextDisplay->StartScrolling();
        
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Text display created and started"),
            *Character->AgentLabel);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Broadcast] %s: Failed to create text display"),
            *Character->AgentLabel);
    }
}

void USK_Broadcast::HideTextBubble()
{
    if (TextDisplay && TextDisplay->IsValidLowLevel())
    {
        TextDisplay->StopScrolling();
        TextDisplay->Destroy();
        TextDisplay = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] Text display destroyed"));
    }
    
    // 清理旧版文本气泡（兼容）
    if (TextBubble && TextBubble->IsValidLowLevel())
    {
        TextBubble->DestroyComponent();
        TextBubble = nullptr;
    }
}

FString USK_Broadcast::GetPhaseString() const
{
    switch (CurrentPhase)
    {
        case EBroadcastPhase::MoveToDistance: return TEXT("moving to distance");
        case EBroadcastPhase::TurnToTarget: return TEXT("turning to target");
        case EBroadcastPhase::Broadcasting: return TEXT("broadcasting");
        case EBroadcastPhase::Complete: return TEXT("complete");
        default: return TEXT("unknown");
    }
}

void USK_Broadcast::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();

    bool bSuccessToNotify = bBroadcastSucceeded;
    FString MessageToNotify = BroadcastResultMessage;
    CleanupBroadcastRuntime(Character, bWasCancelled, bSuccessToNotify, MessageToNotify);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Broadcast, bSuccessToNotify, MessageToNotify);
        }
    }
}
