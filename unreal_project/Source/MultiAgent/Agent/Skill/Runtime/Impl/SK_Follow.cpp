// SK_Follow.cpp
// 跟随技能 - 支持跟随任意场景对象
// 使用 NavigationService 进行统一导航

#include "SK_Follow.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Infrastructure/MASkillConfigBridge.h"
#include "Agent/Skill/Infrastructure/MASkillPIPCameraBridge.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Shared/Types/MATypes.h"
#include "AIController.h"
#include "TimerManager.h"

USK_Follow::USK_Follow()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    bFollowSucceeded = false;
    FollowResultMessage = TEXT("");
}

void USK_Follow::SetTargetActor(AActor* InTargetActor)
{
    TargetActor = InTargetActor;
}

void USK_Follow::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置结果状态
    bFollowSucceeded = false;
    FollowResultMessage = TEXT("");
    ContinuousFollowTime = 0.f;
    TargetActor.Reset();
    
    AMACharacter* Character = GetOwningCharacter();
    UMASkillComponent* SkillComp = Character ? Character->GetSkillComponent() : nullptr;
    if (!SkillComp)
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: SkillComponent not available");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    TargetActor = SkillComp->GetSkillRuntimeTargets().FollowTarget;
    FollowDistance = SkillComp->GetSkillParams().FollowDistance;
    
    // 加载 skill 配置
    if (Character)
    {
        FMASkillConfigBridge::ApplyFollowConfig(
            *Character,
            FollowDistance,
            FollowPositionTolerance,
            ContinuousFollowTimeThreshold);
    }
    
    if (!TargetActor.IsValid())
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: Target not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    if (!Character)
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: Owner character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 获取 NavigationService
    NavigationService = Character->GetNavigationService();
    if (!NavigationService)
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: NavigationService not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    FString TargetName = TargetActor->GetName();
    Character->ShowAbilityStatus(TEXT("Following"), FString::Printf(TEXT("-> %s"), *TargetName));
    
    // 使用跟随模式 API
    // NavigationService 的 AcceptanceRadius 比技能层的 FollowPositionTolerance 收紧，
    // 让机器人实际停靠位置更贴近跟随点，避免"懒惰跟踪"——
    // 即机器人刚到容差边缘就停下，目标稍一移动又超出容差的问题
    float NavAcceptance = FMath::Max(FollowPositionTolerance - 200.f, 50.f);
    NavigationService->FollowActor(TargetActor.Get(), FollowDistance, NavAcceptance);
    
    // 创建画中画相机展示目标
    CreateFollowPIPCamera();
    
    // 启动状态检查定时器
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USK_Follow::UpdateFollow, UpdateInterval, true);
    }
}

void USK_Follow::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    // 跟随模式下，此回调仅在目标丢失时触发
    if (!bSuccess)
    {
        bFollowSucceeded = false;
        FollowResultMessage = Message;
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
    }
}

void USK_Follow::UpdateFollow()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: Owner character lost");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    if (!TargetActor.IsValid())
    {
        bFollowSucceeded = false;
        FollowResultMessage = TEXT("Follow failed: Target lost");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 使用 NavigationService 统一计算跟随位置
    FVector FollowLocation = NavigationService ? NavigationService->GetCurrentFollowPosition() : FVector::ZeroVector;
    // 飞行机器人使用 2D 距离（高度固定），地面机器人也使用 2D 距离（更稳定）
    float DistanceToFollow = FVector::Dist2D(Character->GetActorLocation(), FollowLocation);
    bool bInFollowPosition = (DistanceToFollow < FollowPositionTolerance);
    
    FString TargetName = TargetActor->GetName();
    
    // 调试日志：每次 tick 打印关键状态
    float DistanceToTarget = FVector::Dist(Character->GetActorLocation(), TargetActor->GetActorLocation());
    float Distance2DToTarget = FVector::Dist2D(Character->GetActorLocation(), TargetActor->GetActorLocation());
    UE_LOG(LogTemp, Log, TEXT("[SK_Follow] %s: Dist2DToFollow=%.0f, Tolerance=%.0f, InPos=%s, ContTime=%.1f/%.1f, Dist3DToTarget=%.0f, Dist2DToTarget=%.0f, RobotPos=%s, FollowPos=%s, TargetPos=%s"),
        *Character->AgentLabel,
        DistanceToFollow, FollowPositionTolerance,
        bInFollowPosition ? TEXT("Y") : TEXT("N"),
        ContinuousFollowTime, ContinuousFollowTimeThreshold,
        DistanceToTarget, Distance2DToTarget,
        *Character->GetActorLocation().ToCompactString(),
        *FollowLocation.ToCompactString(),
        *TargetActor->GetActorLocation().ToCompactString());
    
    // 只要满足跟随位置条件就累计时长（不重置）
    if (bInFollowPosition)
    {
        ContinuousFollowTime += UpdateInterval;
        Character->bIsMoving = false;
        
        // 持续跟随达到阈值时间，技能成功
        if (ContinuousFollowTime >= ContinuousFollowTimeThreshold)
        {
            bFollowSucceeded = true;
            FollowResultMessage = FString::Printf(TEXT("Follow succeeded: Continuously followed %s for %.1fs"), 
                *TargetName, ContinuousFollowTime);
            
            if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
            {
                FMAFeedbackContext& FeedbackCtx = SkillComp->GetFeedbackContextMutable();
                FeedbackCtx.FollowDurationSeconds = ContinuousFollowTime;
                FeedbackCtx.FollowTargetDistance = DistanceToFollow;
            }
            
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        }
    }
    
    // 状态显示：第一次满足条件前显示 "Moving to track"，满足后显示 "Tracking"
    if (ContinuousFollowTime > 0.f)
    {
        Character->ShowStatus(FString::Printf(TEXT("Tracking %s..."), *TargetName), UpdateInterval + 0.1f);
    }
    else
    {
        Character->ShowStatus(FString::Printf(TEXT("Moving to track %s..."), *TargetName), UpdateInterval + 0.1f);
    }
}

void USK_Follow::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    bool bSuccessToNotify = bFollowSucceeded;
    FString MessageToNotify = FollowResultMessage;
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
        
        Character->bIsMoving = false;
        
        // 停止跟随
        if (NavigationService)
        {
            NavigationService->StopFollowing();
        }
        
        // 清理画中画
        CleanupPIPCamera();
        
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (bWasCancelled && FollowResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FString TargetName = TargetActor.IsValid() ? TargetActor->GetName() : TEXT("unknown");
            MessageToNotify = FString::Printf(TEXT("Follow cancelled: Stopped following %s"), *TargetName);
        }
        
    }
    
    NavigationService = nullptr;
    TargetActor.Reset();
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Follow, bSuccessToNotify, MessageToNotify);
        }
    }
}

void USK_Follow::CreateFollowPIPCamera()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld() || !TargetActor.IsValid()) return;

    bool bIsFlying = NavigationService && NavigationService->bIsFlying;
    
    // 相机配置：位置跟随机器人，朝向目标对象
    FMAPIPCameraConfig CameraConfig;
    CameraConfig.FollowTarget = Character;  // 相机位置跟随机器人
    CameraConfig.FollowOffset = bIsFlying ? FVector(150.f, 0.f, -80.f) : FVector(150.f, 0.f, 0.f);
    CameraConfig.LookAtTarget = TargetActor;  // 相机朝向目标对象
    CameraConfig.FOV = 60.f;
    CameraConfig.Resolution = FIntPoint(640, 480);
    CameraConfig.SmoothSpeed = 3.f;  // 平滑插值，避免画面抖动
    
    PIPCameraId = FMASkillPIPCameraBridge::CreatePIPCamera(Character->GetWorld(), CameraConfig);
    if (!PIPCameraId.IsValid()) return;
    
    // 显示配置
    FMAPIPDisplayConfig DisplayConfig;
    DisplayConfig.Size = FVector2D(640.f, 360.f);
    DisplayConfig.ScreenPosition = FMASkillPIPCameraBridge::AllocateScreenPosition(Character->GetWorld(), DisplayConfig.Size);  // 右上角
    DisplayConfig.bShowBorder = true;
    DisplayConfig.bShowShadow = true;
    DisplayConfig.BorderColor = FLinearColor(0.2f, 0.6f, 0.2f, 1.f);  // 绿色边框
    DisplayConfig.BorderThickness = 3.f;
    DisplayConfig.Title = FString::Printf(TEXT("[Follow] %s"), *TargetActor->GetName());
    
    FMASkillPIPCameraBridge::ShowPIPCamera(Character->GetWorld(), PIPCameraId, DisplayConfig);
    
    UE_LOG(LogTemp, Log, TEXT("[SK_Follow] %s: Created PIP camera tracking %s"),
        *Character->AgentLabel, *TargetActor->GetName());
}

void USK_Follow::CleanupPIPCamera()
{
    if (AMACharacter* Character = GetOwningCharacter())
    {
        FMASkillPIPCameraBridge::HidePIPCamera(Character->GetWorld(), PIPCameraId);
        FMASkillPIPCameraBridge::DestroyPIPCamera(Character->GetWorld(), PIPCameraId);
    }
}
