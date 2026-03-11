// SK_Broadcast.cpp
// 喊话技能 - 多阶段实现

#include "SK_Broadcast.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUAVCharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Config/MAConfigManager.h"
#include "../../../Environment/Effect/MAShockWave.h"
#include "../../../Environment/Effect/MATextDisplay.h"
#include "Components/TextRenderComponent.h"
#include "TimerManager.h"

USK_Broadcast::USK_Broadcast()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
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
    
    bBroadcastSucceeded = false;
    BroadcastResultMessage = TEXT("");
    BroadcastProgress = 0.f;
    CurrentPhase = EBroadcastPhase::MoveToDistance;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 从配置管理器加载参数
    if (UGameInstance* GameInstance = Character->GetGameInstance())
    {
        if (UMAConfigManager* ConfigManager = GameInstance->GetSubsystem<UMAConfigManager>())
        {
            const FMABroadcastConfig& Config = ConfigManager->GetBroadcastConfig();
            BroadcastDistance = Config.BroadcastDistance;
            BroadcastDuration = Config.BroadcastDuration;
            EffectSpeed = Config.EffectSpeed;
            EffectWidth = Config.EffectWidth;
            EffectRate = Config.ShockRate;
            
            UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] Loaded config: BroadcastDistance=%.0f, Duration=%.1f, ShockRate=%.1f"),
                BroadcastDistance, BroadcastDuration, EffectRate);
        }
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    
    // 获取目标 Actor 和位置
    TargetActor = SkillComp->GetSkillRuntimeTargets().BroadcastTargetActor;
    TargetLocation = Context.TargetLocation;
    TargetName = Context.BroadcastTargetName;
    BroadcastMessage = Params.BroadcastMessage;
    
    if (BroadcastMessage.IsEmpty())
    {
        BroadcastMessage = TEXT("Hello!");
    }
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: ActivateAbility - TargetActor=%s, TargetLocation=%s, Message='%s'"),
        *Character->AgentLabel,
        TargetActor.IsValid() ? *TargetActor->GetName() : TEXT("null"),
        *TargetLocation.ToString(),
        *BroadcastMessage);
    
    // 验证目标
    if (!TargetActor.IsValid() && TargetLocation.IsZero())
    {
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: No valid target");
        UE_LOG(LogTemp, Warning, TEXT("[SK_Broadcast] %s: No valid target found"), *Character->AgentLabel);
        Character->ShowStatus(TEXT("[Broadcast] Target not found"), 2.f);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 如果有目标 Actor，使用其位置
    if (TargetActor.IsValid())
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    NavigationService = Character->GetNavigationService();
    if (!NavigationService)
    {
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: NavigationService not found");
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] %s: NavigationService not found"), *Character->AgentLabel);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 判断是否是飞行机器人
    bIsAircraft = (Character->AgentType == EMAAgentType::UAV || 
                   Character->AgentType == EMAAgentType::FixedWingUAV);
    
    // 获取飞行机器人的最小高度限制
    if (bIsAircraft)
    {
        if (AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(Character))
        {
            MinFlightAltitude = UAV->MinFlightAltitude;
        }
    }
    
    // 检查当前距离
    float HorizontalDistance = FVector::Dist2D(Character->GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: HorizontalDistance=%.0f, BroadcastDistance=%.0f"),
        *Character->AgentLabel, HorizontalDistance, BroadcastDistance);
    
    // 如果已经在合适距离内，直接转向
    if (HorizontalDistance <= BroadcastDistance * 1.2f && HorizontalDistance >= BroadcastDistance * 0.5f)
    {
        CurrentPhase = EBroadcastPhase::TurnToTarget;
    }
    else
    {
        CurrentPhase = EBroadcastPhase::MoveToDistance;
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
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: Lost reference during navigation");
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] HandleMoveToDistance: Lost reference"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: Could not start navigation");
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] %s: NavigateTo failed"), *Character->AgentLabel);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        bBroadcastSucceeded = false;
        BroadcastResultMessage = FString::Printf(TEXT("Broadcast failed: %s"), *Message);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
}

void USK_Broadcast::HandleTurnToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: Character lost during turn");
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] HandleTurnToTarget: Character lost"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    Character->ShowAbilityStatus(TEXT("Broadcast"), TEXT("Turning to target..."));
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Starting turn to target at %s"),
        *Character->AgentLabel, *TargetLocation.ToString());

    // 启动转向计时器
    Character->GetWorld()->GetTimerManager().SetTimer(
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
    if (!Character || !Character->GetWorld())
    {
        if (Character && Character->GetWorld())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        }
        CurrentPhase = EBroadcastPhase::Broadcasting;
        UpdatePhase();
        return;
    }

    float DeltaTime = Character->GetWorld()->GetDeltaSeconds();
    
    // 计算目标朝向（只考虑水平方向）
    FVector DirectionToTarget = TargetLocation - Character->GetActorLocation();
    DirectionToTarget.Z = 0.f;
    DirectionToTarget = DirectionToTarget.GetSafeNormal();
    
    if (DirectionToTarget.IsNearlyZero())
    {
        Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EBroadcastPhase::Broadcasting;
        UpdatePhase();
        return;
    }
    
    FRotator CurrentRotation = Character->GetActorRotation();
    FRotator TargetRotation = DirectionToTarget.Rotation();
    TargetRotation.Pitch = 0.f;
    TargetRotation.Roll = 0.f;
    
    // 平滑插值转向
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);
    Character->SetActorRotation(NewRotation);
    
    // 检查是否已经对准目标（角度差小于5度）
    float YawDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRotation.Yaw, TargetRotation.Yaw));
    if (YawDiff < 5.f)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Turn complete, YawDiff=%.1f"),
            *Character->AgentLabel, YawDiff);
        
        Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EBroadcastPhase::Broadcasting;
        UpdatePhase();
    }
}

void USK_Broadcast::HandleBroadcasting()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bBroadcastSucceeded = false;
        BroadcastResultMessage = TEXT("Broadcast failed: Character lost");
        UE_LOG(LogTemp, Error, TEXT("[SK_Broadcast] HandleBroadcasting: Character lost"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    Character->ShowAbilityStatus(TEXT("Broadcast"), BroadcastMessage);

    // 显示文本气泡
    ShowTextBubble(BroadcastMessage);
    
    // 生成喊话特效（暂用喷水特效占位）
    SpawnBroadcastEffect();

    StartTime = Character->GetWorld()->GetTimeSeconds();
    BroadcastProgress = 0.f;

    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] %s: Starting broadcast, duration=%.1fs, message='%s'"),
        *Character->AgentLabel, BroadcastDuration, *BroadcastMessage);

    // 启动进度计时器
    Character->GetWorld()->GetTimerManager().SetTimer(
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
        HandleComplete();
        return;
    }

    float CurrentTime = Character->GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - StartTime;
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
    AMACharacter* Character = GetOwningCharacter();
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] HandleComplete called"));
    
    if (Character && Character->GetWorld())
    {
        Character->GetWorld()->GetTimerManager().ClearTimer(ProgressTimerHandle);
    }

    // 清理特效
    CleanupBroadcastEffect();
    HideTextBubble();

    bBroadcastSucceeded = true;
    
    if (TargetName.IsEmpty())
    {
        BroadcastResultMessage = FString::Printf(TEXT("Broadcast completed: '%s'"), *BroadcastMessage);
    }
    else
    {
        BroadcastResultMessage = FString::Printf(TEXT("Broadcast to %s completed: '%s'"), *TargetName, *BroadcastMessage);
    }

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

FVector USK_Broadcast::CalculateBroadcastPosition() const
{
    AMACharacter* Character = const_cast<USK_Broadcast*>(this)->GetOwningCharacter();
    if (!Character) return TargetLocation;

    FVector RobotLocation = Character->GetActorLocation();
    
    if (bIsAircraft)
    {
        // 飞行机器人：计算水平方向的喊话位置，保持当前高度或最小飞行高度
        FVector HorizontalDirection = (TargetLocation - RobotLocation);
        HorizontalDirection.Z = 0.f;
        HorizontalDirection = HorizontalDirection.GetSafeNormal();
        
        FVector BroadcastPos = TargetLocation - HorizontalDirection * BroadcastDistance;
        
        // 确保高度不低于最小飞行高度
        BroadcastPos.Z = FMath::Max(RobotLocation.Z, MinFlightAltitude);
        
        UE_LOG(LogTemp, Log, TEXT("[SK_Broadcast] Aircraft broadcast position: %s (MinAltitude=%.0f)"),
            *BroadcastPos.ToString(), MinFlightAltitude);
        
        return BroadcastPos;
    }
    else
    {
        // 地面机器人：沿水平方向后退到喊话距离
        FVector Direction = (TargetLocation - RobotLocation).GetSafeNormal();
        return TargetLocation - Direction * BroadcastDistance;
    }
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
    
    bool bShouldNotify = false;
    bool bSuccessToNotify = bBroadcastSucceeded;
    FString MessageToNotify = BroadcastResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (Character->GetWorld())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(BroadcastTimerHandle);
            Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
            Character->GetWorld()->GetTimerManager().ClearTimer(ProgressTimerHandle);
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
            bSuccessToNotify = false;
            MessageToNotify = FString::Printf(TEXT("Broadcast cancelled: Stopped while %s"), *GetPhaseString());
        }

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag BroadcastTag = FGameplayTag::RequestGameplayTag(FName("Command.Broadcast"));
            if (SkillComp->HasMatchingGameplayTag(BroadcastTag))
            {
                SkillComp->RemoveLooseGameplayTag(BroadcastTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }

    NavigationService = nullptr;
    TargetActor.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
