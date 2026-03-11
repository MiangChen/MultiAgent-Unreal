// SK_TakePhoto.cpp
// 拍照技能 - 多阶段实现

#include "SK_TakePhoto.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUAVCharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Config/MAConfigManager.h"
#include "Core/Camera/Runtime/MAPIPCameraManager.h"
#include "Core/Camera/Domain/MAPIPCameraTypes.h"
#include "TimerManager.h"

USK_TakePhoto::USK_TakePhoto()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
}

void USK_TakePhoto::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    bPhotoSucceeded = false;
    PhotoResultMessage = TEXT("");
    CurrentPhase = ETakePhotoPhase::MoveToDistance;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 从配置管理器加载参数
    if (UGameInstance* GameInstance = Character->GetGameInstance())
    {
        if (UMAConfigManager* ConfigManager = GameInstance->GetSubsystem<UMAConfigManager>())
        {
            const FMATakePhotoConfig& Config = ConfigManager->GetTakePhotoConfig();
            PhotoDistance = Config.PhotoDistance;
            PhotoDuration = Config.PhotoDuration;
            CameraFOV = Config.CameraFOV;
            CameraForwardOffset = Config.CameraForwardOffset;
            
            UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] Loaded config: PhotoDistance=%.0f, Duration=%.1f, FOV=%.0f"),
                PhotoDistance, PhotoDuration, CameraFOV);
        }
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FMASkillParams& Params = SkillComp->GetSkillParams();
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    
    // 大范围目标 FOV 覆盖
    if (Params.PhotoFOVOverride > 0.f)
    {
        CameraFOV = Params.PhotoFOVOverride;
        UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Using wide FOV=%.0f for area-type target"),
            *Character->AgentLabel, CameraFOV);
    }
    
    // 获取目标 Actor 和位置
    TargetActor = SkillComp->GetSkillRuntimeTargets().PhotoTargetActor;
    TargetLocation = Context.TargetLocation;
    TargetName = Context.PhotoTargetName;
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: ActivateAbility - TargetActor=%s, TargetLocation=%s, TargetName=%s"),
        *Character->AgentLabel,
        TargetActor.IsValid() ? *TargetActor->GetName() : TEXT("null"),
        *TargetLocation.ToString(),
        *TargetName);
    
    // 验证目标
    if (!TargetActor.IsValid() && TargetLocation.IsZero())
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: No valid target");
        UE_LOG(LogTemp, Warning, TEXT("[SK_TakePhoto] %s: No valid target found"), *Character->AgentLabel);
        Character->ShowStatus(TEXT("[TakePhoto] Target not found"), 2.f);
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
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: NavigationService not found");
        UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] %s: NavigationService not found"), *Character->AgentLabel);
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
    
    // 获取画中画相机管理器
    PIPCameraManager = Character->GetWorld()->GetSubsystem<UMAPIPCameraManager>();
    if (!PIPCameraManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_TakePhoto] %s: PIPCameraManager not found, will create camera on demand"),
            *Character->AgentLabel);
    }
    
    // 检查当前距离
    float HorizontalDistance = FVector::Dist2D(Character->GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: HorizontalDistance=%.0f, PhotoDistance=%.0f"),
        *Character->AgentLabel, HorizontalDistance, PhotoDistance);
    
    // 如果已经在合适距离内，直接转向
    if (HorizontalDistance <= PhotoDistance * 1.2f && HorizontalDistance >= PhotoDistance * 0.5f)
    {
        CurrentPhase = ETakePhotoPhase::TurnToTarget;
    }
    else
    {
        CurrentPhase = ETakePhotoPhase::MoveToDistance;
    }

    Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Starting..."));
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Starting phase %s"), *Character->AgentLabel, *GetPhaseString());
    UpdatePhase();
}

void USK_TakePhoto::UpdatePhase()
{
    switch (CurrentPhase)
    {
        case ETakePhotoPhase::MoveToDistance:
            HandleMoveToDistance();
            break;
        case ETakePhotoPhase::TurnToTarget:
            HandleTurnToTarget();
            break;
        case ETakePhotoPhase::TakePhoto:
            HandleTakePhoto();
            break;
        case ETakePhotoPhase::Complete:
            HandleComplete();
            break;
    }
}

void USK_TakePhoto::HandleMoveToDistance()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: Lost reference during navigation");
        UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] HandleMoveToDistance: Lost reference"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Moving to photo position..."));

    FVector PhotoPosition = CalculatePhotoPosition();
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Navigating to photo position %s"),
        *Character->AgentLabel, *PhotoPosition.ToString());
    
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_TakePhoto::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(PhotoPosition, 100.f))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_TakePhoto::OnNavigationCompleted);
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: Could not start navigation");
        UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] %s: NavigateTo failed"), *Character->AgentLabel);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
}

void USK_TakePhoto::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] OnNavigationCompleted: bSuccess=%s, Message=%s"),
        bSuccess ? TEXT("true") : TEXT("false"), *Message);
    
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_TakePhoto::OnNavigationCompleted);
    }

    if (bSuccess)
    {
        CurrentPhase = ETakePhotoPhase::TurnToTarget;
        UpdatePhase();
    }
    else
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = FString::Printf(TEXT("TakePhoto failed: %s"), *Message);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
}

void USK_TakePhoto::HandleTurnToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: Character lost during turn");
        UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] HandleTurnToTarget: Character lost"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Aiming at target..."));
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Starting turn to target at %s"),
        *Character->AgentLabel, *TargetLocation.ToString());

    // 启动转向计时器
    Character->GetWorld()->GetTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &USK_TakePhoto::OnTurnTick,
        0.016f,  // ~60fps
        true
    );
}

void USK_TakePhoto::OnTurnTick()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld())
    {
        if (Character && Character->GetWorld())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        }
        CurrentPhase = ETakePhotoPhase::TakePhoto;
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
        CurrentPhase = ETakePhotoPhase::TakePhoto;
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
        UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Turn complete, YawDiff=%.1f"),
            *Character->AgentLabel, YawDiff);
        
        Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = ETakePhotoPhase::TakePhoto;
        UpdatePhase();
    }
}

void USK_TakePhoto::HandleTakePhoto()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPhotoSucceeded = false;
        PhotoResultMessage = TEXT("TakePhoto failed: Character lost");
        UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] HandleTakePhoto: Character lost"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }

    Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Taking photo..."));
    Character->ShowStatus(TEXT("📷"), PhotoDuration);

    // 创建并显示画中画相机
    CreatePIPCamera();
    ShowPIPCamera();

    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Taking photo, duration=%.1fs"),
        *Character->AgentLabel, PhotoDuration);

    // 设置拍照完成定时器
    Character->GetWorld()->GetTimerManager().SetTimer(
        PhotoTimerHandle,
        this,
        &USK_TakePhoto::OnPhotoComplete,
        PhotoDuration,
        false
    );
}

void USK_TakePhoto::OnPhotoComplete()
{
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] OnPhotoComplete called"));
    
    // 隐藏并清理画中画相机
    HidePIPCamera();
    CleanupPIPCamera();
    
    CurrentPhase = ETakePhotoPhase::Complete;
    UpdatePhase();
}

void USK_TakePhoto::HandleComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    
    bPhotoSucceeded = true;
    
    if (TargetName.IsEmpty())
    {
        PhotoResultMessage = TEXT("Photo taken successfully");
    }
    else
    {
        PhotoResultMessage = FString::Printf(TEXT("Photo taken successfully: %s"), *TargetName);
    }

    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Complete!"));
        Character->ShowStatus(TEXT(""), 0.f);
        
        UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Skill completed successfully"),
            *Character->AgentLabel);
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

FVector USK_TakePhoto::CalculatePhotoPosition() const
{
    AMACharacter* Character = const_cast<USK_TakePhoto*>(this)->GetOwningCharacter();
    if (!Character) return TargetLocation;

    FVector RobotLocation = Character->GetActorLocation();
    
    if (bIsAircraft)
    {
        // 飞行机器人：计算水平方向的拍照位置，保持当前高度或最小飞行高度
        FVector HorizontalDirection = (TargetLocation - RobotLocation);
        HorizontalDirection.Z = 0.f;
        HorizontalDirection = HorizontalDirection.GetSafeNormal();
        
        FVector PhotoPos = TargetLocation - HorizontalDirection * PhotoDistance;
        
        // 确保高度不低于最小飞行高度
        PhotoPos.Z = FMath::Max(RobotLocation.Z, MinFlightAltitude);
        
        UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] Aircraft photo position: %s (MinAltitude=%.0f)"),
            *PhotoPos.ToString(), MinFlightAltitude);
        
        return PhotoPos;
    }
    else
    {
        // 地面机器人：沿水平方向后退到拍照距离
        FVector Direction = (TargetLocation - RobotLocation).GetSafeNormal();
        return TargetLocation - Direction * PhotoDistance;
    }
}

void USK_TakePhoto::CreatePIPCamera()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld()) return;
    
    // 获取管理器
    if (!PIPCameraManager)
    {
        PIPCameraManager = Character->GetWorld()->GetSubsystem<UMAPIPCameraManager>();
    }
    
    if (!PIPCameraManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] PIPCameraManager not available"));
        return;
    }
    
    // 如果已有相机，先销毁
    if (PIPCameraId.IsValid() && PIPCameraManager->DoesPIPCameraExist(PIPCameraId))
    {
        PIPCameraManager->DestroyPIPCamera(PIPCameraId);
    }

    // 计算相机位置：机器人位置 + 前方偏移
    FVector CameraLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * CameraForwardOffset;
    
    // 计算朝向目标的旋转
    FVector DirectionToTarget = (TargetLocation - CameraLocation).GetSafeNormal();
    FRotator CameraRotation = DirectionToTarget.Rotation();

    // 创建相机配置
    FMAPIPCameraConfig CameraConfig;
    CameraConfig.Location = CameraLocation;
    CameraConfig.Rotation = CameraRotation;
    CameraConfig.FOV = CameraFOV;
    CameraConfig.Resolution = FIntPoint(640, 480);
    
    // 创建画中画相机
    PIPCameraId = PIPCameraManager->CreatePIPCamera(CameraConfig);
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Created PIP camera at %s, looking at %s"),
        *Character->AgentLabel, *CameraLocation.ToString(), *TargetLocation.ToString());
}

void USK_TakePhoto::ShowPIPCamera()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !PIPCameraManager || !PIPCameraId.IsValid()) return;
    
    // 显示配置
    FMAPIPDisplayConfig DisplayConfig;
    DisplayConfig.ScreenPosition = FVector2D(0.7f, 0.3f);  // 右下角
    DisplayConfig.Size = FVector2D(800.f, 450.f);
    DisplayConfig.bShowBorder = true;
    DisplayConfig.bShowShadow = true;
    DisplayConfig.BorderColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);
    DisplayConfig.BorderThickness = 3.f;
    
    // 设置标题
    if (!TargetName.IsEmpty())
    {
        DisplayConfig.Title = FString::Printf(TEXT("[Photo] %s"), *TargetName);
    }
    else
    {
        DisplayConfig.Title = FString::Printf(TEXT("[Photo] %s"), *Character->AgentLabel);
    }
    
    PIPCameraManager->ShowPIPCamera(PIPCameraId, DisplayConfig);
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Showing PIP camera view"),
        *Character->AgentLabel);
}

void USK_TakePhoto::HidePIPCamera()
{
    if (!PIPCameraManager || !PIPCameraId.IsValid()) return;
    
    PIPCameraManager->HidePIPCamera(PIPCameraId);
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] Hiding PIP camera"));
}

void USK_TakePhoto::CleanupPIPCamera()
{
    if (!PIPCameraManager || !PIPCameraId.IsValid()) return;
    
    PIPCameraManager->DestroyPIPCamera(PIPCameraId);
    PIPCameraId.Invalidate();
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] Cleaned up PIP camera"));
}

FString USK_TakePhoto::GetPhaseString() const
{
    switch (CurrentPhase)
    {
        case ETakePhotoPhase::MoveToDistance: return TEXT("moving to distance");
        case ETakePhotoPhase::TurnToTarget: return TEXT("turning to target");
        case ETakePhotoPhase::TakePhoto: return TEXT("taking photo");
        case ETakePhotoPhase::Complete: return TEXT("complete");
        default: return TEXT("unknown");
    }
}

void USK_TakePhoto::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    bool bShouldNotify = false;
    bool bSuccessToNotify = bPhotoSucceeded;
    FString MessageToNotify = PhotoResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (Character->GetWorld())
        {
            Character->GetWorld()->GetTimerManager().ClearTimer(PhotoTimerHandle);
            Character->GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        }

        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_TakePhoto::OnNavigationCompleted);
            NavigationService->CancelNavigation();
        }

        CleanupPIPCamera();

        Character->ShowStatus(TEXT(""), 0.f);

        if (bWasCancelled && PhotoResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            MessageToNotify = FString::Printf(TEXT("TakePhoto cancelled: Stopped while %s"), *GetPhaseString());
        }

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag TakePhotoTag = FGameplayTag::RequestGameplayTag(FName("Command.TakePhoto"));
            if (SkillComp->HasMatchingGameplayTag(TakePhotoTag))
            {
                SkillComp->RemoveLooseGameplayTag(TakePhotoTag);
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
