// SK_TakePhoto.cpp
// 拍照技能 - 多阶段实现

#include "SK_TakePhoto.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
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

void USK_TakePhoto::ResetTakePhotoRuntimeState()
{
    bPhotoSucceeded = false;
    PhotoResultMessage.Reset();
    CurrentPhase = ETakePhotoPhase::MoveToDistance;
    bIsAircraft = false;
    MinFlightAltitude = 800.f;
    NavigationService = nullptr;
    PIPCameraManager = nullptr;
    PIPCameraId.Invalidate();
    TargetActor.Reset();
    TargetLocation = FVector::ZeroVector;
    TargetName.Reset();
}

void USK_TakePhoto::FailTakePhoto(
    const FString& ResultMessage,
    const FString& ErrorReason,
    const FString& StatusMessage)
{
    bPhotoSucceeded = false;
    PhotoResultMessage = ResultMessage;

    UE_LOG(LogTemp, Error, TEXT("[SK_TakePhoto] %s"), *ErrorReason);

    if (!StatusMessage.IsEmpty())
    {
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_TakePhoto::InitializeTakePhotoContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    if (UGameInstance* GameInstance = Character.GetGameInstance())
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

    const FMASkillParams& Params = SkillComp.GetSkillParams();
    const FMAFeedbackContext& Context = SkillComp.GetFeedbackContext();

    if (Params.PhotoFOVOverride > 0.f)
    {
        CameraFOV = Params.PhotoFOVOverride;
        UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Using wide FOV=%.0f for area-type target"),
            *Character.AgentLabel, CameraFOV);
    }

    TargetActor = SkillComp.GetSkillRuntimeTargets().PhotoTargetActor;
    TargetLocation = Context.TargetLocation;
    TargetName = Context.PhotoTargetName;

    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: ActivateAbility - TargetActor=%s, TargetLocation=%s, TargetName=%s"),
        *Character.AgentLabel,
        TargetActor.IsValid() ? *TargetActor->GetName() : TEXT("null"),
        *TargetLocation.ToString(),
        *TargetName);

    if (!TargetActor.IsValid() && TargetLocation.IsZero())
    {
        FailTakePhoto(TEXT("TakePhoto failed: No valid target"), TEXT("No valid target found"), TEXT("[TakePhoto] Target not found"));
        return false;
    }

    if (TargetActor.IsValid())
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailTakePhoto(TEXT("TakePhoto failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    bIsAircraft = (Character.AgentType == EMAAgentType::UAV ||
                   Character.AgentType == EMAAgentType::FixedWingUAV);

    if (bIsAircraft)
    {
        if (AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(&Character))
        {
            MinFlightAltitude = UAV->MinFlightAltitude;
        }
    }

    if (UWorld* World = Character.GetWorld())
    {
        PIPCameraManager = World->GetSubsystem<UMAPIPCameraManager>();
    }

    if (!PIPCameraManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_TakePhoto] %s: PIPCameraManager not found, will create camera on demand"),
            *Character.AgentLabel);
    }

    const float HorizontalDistance = FVector::Dist2D(Character.GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: HorizontalDistance=%.0f, PhotoDistance=%.0f"),
        *Character.AgentLabel, HorizontalDistance, PhotoDistance);

    CurrentPhase = (HorizontalDistance <= PhotoDistance * 1.2f && HorizontalDistance >= PhotoDistance * 0.5f)
        ? ETakePhotoPhase::TurnToTarget
        : ETakePhotoPhase::MoveToDistance;
    return true;
}

void USK_TakePhoto::HandleNavigationFailure(const FString& Message)
{
    FailTakePhoto(FString::Printf(TEXT("TakePhoto failed: %s"), *Message), Message);
}

void USK_TakePhoto::CompleteTakePhoto()
{
    AMACharacter* Character = GetOwningCharacter();

    bPhotoSucceeded = true;
    PhotoResultMessage = TargetName.IsEmpty()
        ? TEXT("Photo taken successfully")
        : FString::Printf(TEXT("Photo taken successfully: %s"), *TargetName);

    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Complete!"));
        Character->ShowStatus(TEXT(""), 0.f);

        UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Skill completed successfully"),
            *Character->AgentLabel);
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void USK_TakePhoto::CleanupTakePhotoRuntime(
    AMACharacter* Character,
    const bool bWasCancelled,
    bool& bOutSuccessToNotify,
    FString& InOutMessageToNotify)
{
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(PhotoTimerHandle);
            World->GetTimerManager().ClearTimer(TurnTimerHandle);
        }

        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_TakePhoto::OnNavigationCompleted);
            NavigationService->CancelNavigation();
        }

        HidePIPCamera();
        CleanupPIPCamera();
        Character->ShowStatus(TEXT(""), 0.f);

        if (bWasCancelled && PhotoResultMessage.IsEmpty())
        {
            bOutSuccessToNotify = false;
            InOutMessageToNotify = FString::Printf(TEXT("TakePhoto cancelled: Stopped while %s"), *GetPhaseString());
        }
    }

    NavigationService = nullptr;
    PIPCameraManager = nullptr;
    PIPCameraId.Invalidate();
    TargetActor.Reset();
    TargetLocation = FVector::ZeroVector;
    TargetName.Reset();
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
    ResetTakePhotoRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailTakePhoto(TEXT("TakePhoto failed: Character not found"), TEXT("Character not found"));
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailTakePhoto(TEXT("TakePhoto failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializeTakePhotoContext(*Character, *SkillComp))
    {
        return;
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
        FailTakePhoto(TEXT("TakePhoto failed: Lost reference during navigation"), TEXT("HandleMoveToDistance lost Character or NavigationService"));
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
        HandleNavigationFailure(TEXT("Could not start navigation"));
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
        HandleNavigationFailure(Message);
    }
}

void USK_TakePhoto::HandleTurnToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailTakePhoto(TEXT("TakePhoto failed: Character lost during turn"), TEXT("HandleTurnToTarget lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailTakePhoto(TEXT("TakePhoto failed: World not found during turn"), TEXT("HandleTurnToTarget lost World"));
        return;
    }

    Character->ShowAbilityStatus(TEXT("TakePhoto"), TEXT("Aiming at target..."));
    
    UE_LOG(LogTemp, Log, TEXT("[SK_TakePhoto] %s: Starting turn to target at %s"),
        *Character->AgentLabel, *TargetLocation.ToString());

    // 启动转向计时器
    World->GetTimerManager().SetTimer(
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
    UWorld* World = Character ? Character->GetWorld() : nullptr;
    if (!Character || !World)
    {
        FailTakePhoto(TEXT("TakePhoto failed: Character lost during turn"), TEXT("OnTurnTick lost Character or World"));
        return;
    }

    const float DeltaTime = World->GetDeltaSeconds();
    
    // 计算目标朝向（只考虑水平方向）
    FVector DirectionToTarget = TargetLocation - Character->GetActorLocation();
    DirectionToTarget.Z = 0.f;
    DirectionToTarget = DirectionToTarget.GetSafeNormal();
    
    if (DirectionToTarget.IsNearlyZero())
    {
        World->GetTimerManager().ClearTimer(TurnTimerHandle);
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

        World->GetTimerManager().ClearTimer(TurnTimerHandle);
        CurrentPhase = ETakePhotoPhase::TakePhoto;
        UpdatePhase();
    }
}

void USK_TakePhoto::HandleTakePhoto()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailTakePhoto(TEXT("TakePhoto failed: Character lost"), TEXT("HandleTakePhoto lost Character"));
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        FailTakePhoto(TEXT("TakePhoto failed: World not found"), TEXT("HandleTakePhoto lost World"));
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
    World->GetTimerManager().SetTimer(
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
    CompleteTakePhoto();
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

    bool bSuccessToNotify = bPhotoSucceeded;
    FString MessageToNotify = PhotoResultMessage;
    CleanupTakePhotoRuntime(Character, bWasCancelled, bSuccessToNotify, MessageToNotify);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::TakePhoto, bSuccessToNotify, MessageToNotify);
        }
    }
}
