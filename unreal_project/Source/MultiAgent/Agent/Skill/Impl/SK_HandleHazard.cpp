// SK_HandleHazard.cpp
// 处理危险技能实现

#include "SK_HandleHazard.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAUAVCharacter.h"
#include "../../Component/MANavigationService.h"
#include "../../../Environment/Effect/MAWaterSpray.h"
#include "../../../Environment/Effect/MAFire.h"
#include "../../../Environment/IMAEnvironmentObject.h"
#include "../../../Core/Config/MAConfigManager.h"
#include "TimerManager.h"

USK_HandleHazard::USK_HandleHazard()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
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
    
    bHandleSucceeded = false;
    HandleResultMessage = TEXT("");
    HandleProgress = 0.f;
    CurrentPhase = EHandleHazardPhase::MoveToSafeDistance;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 从配置管理器加载参数
    if (UGameInstance* GameInstance = Character->GetGameInstance())
    {
        if (UMAConfigManager* ConfigManager = GameInstance->GetSubsystem<UMAConfigManager>())
        {
            const FMAHandleHazardConfig& Config = ConfigManager->GetHandleHazardConfig();
            SafeDistance = Config.SafeDistance;
            HandleDuration = Config.Duration;
            SpraySpeed = Config.SpraySpeed;
            SprayWidth = Config.SprayWidth;
            
            UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Loaded config: SafeDistance=%.0f, Duration=%.1f, SpraySpeed=%.0f, SprayWidth=%.0f"),
                SafeDistance, HandleDuration, SpraySpeed, SprayWidth);
        }
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    
    AActor* HazardActor = Params.HazardTargetActor.Get();
    
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: ActivateAbility - HazardTargetActor=%s, TargetLocation=%s"),
        *Character->AgentLabel,
        HazardActor ? *HazardActor->GetName() : TEXT("null"),
        *Context.TargetLocation.ToString());
    
    //=========================================================================
    // 确定目标类型
    // 情况1: 目标直接是 AMAFire
    // 情况2: 目标是环境实体（有 is_fire=true 属性）
    //=========================================================================
    
    TargetFire = Cast<AMAFire>(HazardActor);
    
    if (TargetFire.IsValid())
    {
        // 情况1: 直接是火焰对象
        TargetLocation = TargetFire->GetActorLocation();
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Target is AMAFire at %s"),
            *Character->AgentLabel, *TargetLocation.ToString());
    }
    else if (HazardActor)
    {
        // 情况2: 检查是否是着火的环境实体
        IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(HazardActor);
        if (EnvObject)
        {
            FString IsFireValue = EnvObject->GetFeature(TEXT("is_fire"), TEXT("false"));
            bool bIsOnFire = IsFireValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
            
            UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Target is environment object '%s', is_fire=%s"),
                *Character->AgentLabel, *EnvObject->GetObjectLabel(), *IsFireValue);
            
            if (bIsOnFire)
            {
                TargetEnvironmentObject = HazardActor;
                TargetLocation = HazardActor->GetActorLocation();
                UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Environment object is on fire, targeting at %s"),
                    *Character->AgentLabel, *TargetLocation.ToString());
            }
            else
            {
                // 环境对象没有着火，但仍然可以作为目标处理
                TargetEnvironmentObject = HazardActor;
                TargetLocation = HazardActor->GetActorLocation();
                UE_LOG(LogTemp, Warning, TEXT("[SK_HandleHazard] %s: Environment object is NOT on fire, but proceeding anyway"),
                    *Character->AgentLabel);
            }
        }
        else
        {
            // 不是环境对象接口，使用位置
            TargetLocation = HazardActor->GetActorLocation();
            UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Target is generic actor at %s"),
                *Character->AgentLabel, *TargetLocation.ToString());
        }
    }
    else if (!Context.TargetLocation.IsZero())
    {
        // 回退：使用 TargetLocation
        TargetLocation = Context.TargetLocation;
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Using TargetLocation from context: %s"),
            *Character->AgentLabel, *TargetLocation.ToString());
    }
    else
    {
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: No valid target");
        UE_LOG(LogTemp, Warning, TEXT("[SK_HandleHazard] %s: No valid target found"),
            *Character->AgentLabel);
        Character->ShowStatus(TEXT("[HandleHazard] Target not found"), 2.f);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    NavigationService = Character->GetNavigationService();
    if (!NavigationService)
    {
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: NavigationService not found");
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] %s: NavigationService not found"), *Character->AgentLabel);
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
    
    // 使用水平距离判断（对飞行机器人更合理）
    float HorizontalDistance = FVector::Dist2D(Character->GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: HorizontalDistance=%.0f, SafeDistance=%.0f, IsAircraft=%s"),
        *Character->AgentLabel, HorizontalDistance, SafeDistance, bIsAircraft ? TEXT("true") : TEXT("false"));
    
    // 总是移动到 SafeDistance 位置，确保喷水效果最佳
    CurrentPhase = EHandleHazardPhase::MoveToSafeDistance;

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
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: Lost reference during navigation");
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] HandleMoveToSafeDistance: Lost reference"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: Could not start navigation");
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] %s: NavigateTo failed"), *Character->AgentLabel);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        bHandleSucceeded = false;
        HandleResultMessage = FString::Printf(TEXT("HandleHazard failed: %s"), *Message);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
}

void USK_HandleHazard::HandleTurnToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: Character lost during turn");
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] HandleTurnToTarget: Character lost"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Turning to target..."));
    
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Starting turn to target at %s"),
        *Character->AgentLabel, *TargetLocation.ToString());

    // 启动转向计时器，平滑转向目标
    Character->GetWorld()->GetTimerManager().SetTimer(
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
    if (!Character || !Character->GetWorld())
    {
        if (Character && Character->GetWorld())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        }
        CurrentPhase = EHandleHazardPhase::StartSpray;
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
        // 已经在目标位置，直接开始喷水
        Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EHandleHazardPhase::StartSpray;
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
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Turn complete, YawDiff=%.1f"),
            *Character->AgentLabel, YawDiff);
        
        Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = EHandleHazardPhase::StartSpray;
        UpdatePhase();
    }
}

void USK_HandleHazard::HandleStartSpray()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bHandleSucceeded = false;
        HandleResultMessage = TEXT("HandleHazard failed: Character lost");
        UE_LOG(LogTemp, Error, TEXT("[SK_HandleHazard] HandleStartSpray: Character lost"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    // 记录开始喷水时的位置和距离
    SprayStartLocation = Character->GetActorLocation();
    float StartHorizontalDist = FVector::Dist2D(SprayStartLocation, TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: HandleStartSpray - Position=%s, HorizontalDistToTarget=%.0f, SafeDistance=%.0f"),
        *Character->AgentLabel, *SprayStartLocation.ToString(), StartHorizontalDist, SafeDistance);

    Character->ShowAbilityStatus(TEXT("HandleHazard"), TEXT("Starting water spray..."));

    SpawnWaterSpray();

    StartTime = Character->GetWorld()->GetTimeSeconds();
    HandleProgress = 0.f;

    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] %s: Starting progress timer, duration=%.1fs"),
        *Character->AgentLabel, HandleDuration);

    Character->GetWorld()->GetTimerManager().SetTimer(
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
        HandleComplete();
        return;
    }

    float CurrentTime = Character->GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - StartTime;
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
    AMACharacter* Character = GetOwningCharacter();
    
    UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] HandleComplete called"));
    
    // 记录结束时的位置和距离，用于诊断 UAV 移动问题
    if (Character)
    {
        FVector EndLocation = Character->GetActorLocation();
        float EndHorizontalDist = FVector::Dist2D(EndLocation, TargetLocation);
        float MovedDistance = FVector::Dist(SprayStartLocation, EndLocation);
        float MovedHorizontal = FVector::Dist2D(SprayStartLocation, EndLocation);
        
        UE_LOG(LogTemp, Warning, TEXT("[SK_HandleHazard] %s: SPRAY PHASE MOVEMENT ANALYSIS:"), *Character->AgentLabel);
        UE_LOG(LogTemp, Warning, TEXT("  StartPos=%s, EndPos=%s"), *SprayStartLocation.ToString(), *EndLocation.ToString());
        UE_LOG(LogTemp, Warning, TEXT("  StartHorizDist=%.0f, EndHorizDist=%.0f, SafeDistance=%.0f"),
            FVector::Dist2D(SprayStartLocation, TargetLocation), EndHorizontalDist, SafeDistance);
        UE_LOG(LogTemp, Warning, TEXT("  MovedDuring Spray: Total=%.0f, Horizontal=%.0f"), MovedDistance, MovedHorizontal);
    }
    
    if (Character && Character->GetWorld())
    {
        Character->GetWorld()->GetTimerManager().ClearTimer(ProgressTimerHandle);
    }

    CleanupWaterSpray();

    //=========================================================================
    // 处理灭火逻辑
    // 情况1: 目标是 AMAFire - 直接调用 Extinguish()
    // 情况2: 目标是环境实体 - 熄灭附着火焰
    //=========================================================================
    FString TargetObjectLabel;
    
    if (TargetFire.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Extinguishing AMAFire"));
        TargetFire->Extinguish();
    }
    else if (TargetEnvironmentObject.IsValid())
    {
        IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(TargetEnvironmentObject.Get());
        if (EnvObject)
        {
            // 熄灭附着的火焰
            AMAFire* AttachedFire = EnvObject->GetAttachedFire();
            if (AttachedFire)
            {
                UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Extinguishing attached fire on environment object"));
                AttachedFire->Extinguish();
            }
            
            // 保存目标对象标签，供 MASceneGraphUpdater 使用
            TargetObjectLabel = EnvObject->GetObjectLabel();
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] No specific target to extinguish"));
    }

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

FVector USK_HandleHazard::CalculateSafePosition() const
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return TargetLocation;

    FVector RobotLocation = Character->GetActorLocation();
    
    if (bIsAircraft)
    {
        // 飞行机器人：计算水平方向的安全位置，保持当前高度或最小飞行高度
        FVector HorizontalDirection = (TargetLocation - RobotLocation);
        HorizontalDirection.Z = 0.f;
        HorizontalDirection = HorizontalDirection.GetSafeNormal();
        
        FVector SafePos = TargetLocation - HorizontalDirection * SafeDistance;
        
        // 确保高度不低于最小飞行高度
        SafePos.Z = FMath::Max(RobotLocation.Z, MinFlightAltitude);
        
        UE_LOG(LogTemp, Log, TEXT("[SK_HandleHazard] Aircraft safe position: %s (MinAltitude=%.0f)"),
            *SafePos.ToString(), MinFlightAltitude);
        
        return SafePos;
    }
    else
    {
        // 地面机器人：沿水平方向后退到安全距离
        FVector Direction = (TargetLocation - RobotLocation).GetSafeNormal();
        return TargetLocation - Direction * SafeDistance;
    }
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
    
    bool bShouldNotify = false;
    bool bSuccessToNotify = bHandleSucceeded;
    FString MessageToNotify = HandleResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (Character->GetWorld())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(ProgressTimerHandle);
            Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
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
            bSuccessToNotify = false;
            MessageToNotify = FString::Printf(TEXT("HandleHazard cancelled: Stopped while %s"), *GetPhaseString());
        }

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag HandleHazardTag = FGameplayTag::RequestGameplayTag(FName("Command.HandleHazard"));
            if (SkillComp->HasMatchingGameplayTag(HandleHazardTag))
            {
                SkillComp->RemoveLooseGameplayTag(HandleHazardTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }

    NavigationService = nullptr;
    TargetFire.Reset();
    TargetEnvironmentObject.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
