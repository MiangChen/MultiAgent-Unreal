// SK_Guide.cpp
// 引导技能 - 引导目标对象到达指定位置
// 机器人使用 NavigationService 导航到目的地
// 被引导对象支持两种移动模式: Direct (直接移动) 或 NavMesh (导航服务)

#include "SK_Guide.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Config/MAConfigManager.h"
#include "../../../Environment/Entity/MAPerson.h"
#include "Containers/Ticker.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

USK_Guide::USK_Guide()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_Guide::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    bAgentArrived = false;
    bAgentWaiting = false;
    bGuideSucceeded = false;
    GuideResultMessage = TEXT("");
    LastTargetNavPos = FVector::ZeroVector;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bGuideSucceeded = false;
        GuideResultMessage = TEXT("Guide failed: Owner character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 从 ConfigManager 加载配置
    if (UWorld* World = Character->GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
            {
                MinFlightAltitude = ConfigMgr->GetFlightConfig().MinAltitude;
                
                const FMAFollowConfig& FollowCfg = ConfigMgr->GetFollowConfig();
                FollowDistance = FollowCfg.Distance * 0.4f;
                
                const FMAGroundNavigationConfig& GroundCfg = ConfigMgr->GetGroundNavigationConfig();
                AcceptanceRadius = GroundCfg.AcceptanceRadius;

                // 加载 Guide 配置
                const FMAGuideConfig& GuideCfg = ConfigMgr->GetGuideConfig();
                WaitDistanceThreshold = GuideCfg.WaitDistanceThreshold;
                if (GuideCfg.TargetMoveMode.Equals(TEXT("direct"), ESearchCase::IgnoreCase))
                {
                    TargetMoveMode = EMAGuideTargetMoveMode::Direct;
                }
                else
                {
                    TargetMoveMode = EMAGuideTargetMoveMode::NavMesh;
                }
            }
        }
    }


    // 获取引导参数
    if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        GuideTargetActor = SkillComp->GetSkillRuntimeTargets().GuideTargetActor;
        GuideDestination = Params.GuideDestination;
    }

    // 验证参数
    if (!GuideTargetActor.IsValid())
    {
        bGuideSucceeded = false;
        GuideResultMessage = TEXT("Guide failed: Target not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (GuideDestination.IsZero())
    {
        bGuideSucceeded = false;
        GuideResultMessage = TEXT("Guide failed: No valid destination");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 获取机器人 NavigationService
    NavigationService = Character->GetNavigationService();
    if (!NavigationService)
    {
        bGuideSucceeded = false;
        GuideResultMessage = TEXT("Guide failed: NavigationService not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // NavMesh 模式：尝试获取被引导对象的导航服务
    if (TargetMoveMode == EMAGuideTargetMoveMode::NavMesh)
    {
        TargetNavigationService = GetTargetNavigationService();
        if (!TargetNavigationService)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SK_Guide] Target has no NavigationService, falling back to Direct mode"));
            TargetMoveMode = EMAGuideTargetMoveMode::Direct;
        }
    }

    // 初始化状态
    StartTime = Character->GetWorld()->GetTimeSeconds();
    StartLocation = Character->GetActorLocation();
    Character->bIsMoving = true;

    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Guide::OnAgentNavigationCompleted);

    // 获取被引导对象的移动速度，让机器人匹配
    TargetMoveSpeed = GetTargetMoveSpeed();
    if (TargetMoveSpeed > 0.f && !IsFlying())
    {
        NavigationService->SetMoveSpeed(TargetMoveSpeed * 1.05f);
    }

    // 启动机器人导航到目的地
    NavigationService->NavigateTo(GuideDestination, AcceptanceRadius);

    // 启动 Ticker 更新引导逻辑
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &USK_Guide::TickGuide), 0.0f);

    UE_LOG(LogTemp, Log, TEXT("[SK_Guide] Started guiding %s to (%.0f,%.0f,%.0f), mode=%s, wait_threshold=%.0f"),
        *GuideTargetActor->GetName(), GuideDestination.X, GuideDestination.Y, GuideDestination.Z,
        TargetMoveMode == EMAGuideTargetMoveMode::NavMesh ? TEXT("NavMesh") : TEXT("Direct"),
        WaitDistanceThreshold);
}

void USK_Guide::OnAgentNavigationCompleted(bool bSuccess, const FString& Message)
{
    if (bSuccess)
    {
        bAgentArrived = true;
        UE_LOG(LogTemp, Log, TEXT("[SK_Guide] Agent arrived at destination"));
    }
    else if (!bAgentWaiting)
    {
        // 只有非等待状态下的导航失败才算真正失败
        OnGuideComplete(false, FString::Printf(TEXT("Guide failed: %s"), *Message));
    }
}

bool USK_Guide::TickGuide(float DeltaTime)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        OnGuideComplete(false, TEXT("Guide failed: Owner character lost"));
        return false;
    }

    if (!GuideTargetActor.IsValid())
    {
        OnGuideComplete(false, TEXT("Guide failed: Target lost"));
        return false;
    }

    // 检查是否需要等待被引导对象
    CheckAndHandleWaiting();

    // 更新被引导对象的位置
    UpdateTargetPosition(DeltaTime);

    FString TargetName = GuideTargetActor->GetName();
    
    if (bAgentArrived)
    {
        // 机器人已到达，检查被引导对象是否也到达
        FVector TargetFollowPos = CalculateTargetFollowPosition();
        float DistanceToFollowPos = FVector::Dist2D(GuideTargetActor->GetActorLocation(), TargetFollowPos);

        if (DistanceToFollowPos < TargetAcceptanceRadius)
        {
            OnGuideComplete(true, FString::Printf(
                TEXT("Guide succeeded: Target guided to (%.0f, %.0f, %.0f)"),
                GuideDestination.X, GuideDestination.Y, GuideDestination.Z));
            return false;
        }
        
        Character->ShowStatus(FString::Printf(TEXT("Waiting for %s..."), *TargetName), 0.2f);
    }
    else if (bAgentWaiting)
    {
        Character->ShowStatus(FString::Printf(TEXT("Waiting for %s to catch up..."), *TargetName), 0.2f);
    }
    else
    {
        Character->ShowStatus(FString::Printf(TEXT("Guiding %s..."), *TargetName), 0.2f);
    }

    return true;
}

void USK_Guide::CheckAndHandleWaiting()
{
    if (!GuideTargetActor.IsValid() || !NavigationService) return;
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || bAgentArrived) return;  // 已到达就不需要等待逻辑了

    float DistanceToTarget = FVector::Dist2D(Character->GetActorLocation(), GuideTargetActor->GetActorLocation());
    
    if (!bAgentWaiting && DistanceToTarget > WaitDistanceThreshold)
    {
        // 距离过远，暂停导航（设置速度为0）
        bAgentWaiting = true;
        NavigationService->SetMoveSpeed(0.01f);  // 设置极低速度来"暂停"
        UE_LOG(LogTemp, Log, TEXT("[SK_Guide] Agent waiting for target (distance=%.0f > threshold=%.0f)"),
            DistanceToTarget, WaitDistanceThreshold);
    }
    else if (bAgentWaiting && DistanceToTarget < WaitDistanceThreshold * 0.6f)
    {
        // 被引导对象追上来了，恢复导航速度
        bAgentWaiting = false;
        if (TargetMoveSpeed > 0.f)
        {
            NavigationService->SetMoveSpeed(TargetMoveSpeed * 1.05f);
        }
        else
        {
            NavigationService->RestoreDefaultSpeed();
        }
        UE_LOG(LogTemp, Log, TEXT("[SK_Guide] Agent resuming navigation (distance=%.0f)"), DistanceToTarget);
    }
}

FVector USK_Guide::CalculateTargetFollowPosition() const
{
    AMACharacter* Character = const_cast<USK_Guide*>(this)->GetOwningCharacter();
    if (!Character) return FVector::ZeroVector;

    FVector AgentLocation = Character->GetActorLocation();
    FVector AgentForward = Character->GetActorForwardVector();
    FVector TargetFollowPosition = AgentLocation - AgentForward * FollowDistance;

    if (IsFlying() && GuideTargetActor.IsValid())
    {
        TargetFollowPosition.Z = GuideTargetActor->GetActorLocation().Z;
    }

    return TargetFollowPosition;
}

void USK_Guide::UpdateTargetPosition(float DeltaTime)
{
    if (!GuideTargetActor.IsValid()) return;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    // 计算被引导对象应该移动到的位置
    FVector MoveToPos;
    if (bAgentWaiting)
    {
        // 机器人等待中：被引导对象直接向机器人位置移动
        MoveToPos = Character->GetActorLocation();
        MoveToPos.Z = GuideTargetActor->GetActorLocation().Z;  // 保持高度
    }
    else
    {
        // 正常引导：被引导对象向跟随位置移动
        MoveToPos = CalculateTargetFollowPosition();
    }

    float DistanceToPos = FVector::Dist2D(GuideTargetActor->GetActorLocation(), MoveToPos);
    if (DistanceToPos < 50.f) return;

    if (TargetMoveMode == EMAGuideTargetMoveMode::NavMesh && TargetNavigationService)
    {
        UpdateTargetPositionNavMesh(MoveToPos);
    }
    else
    {
        UpdateTargetPositionDirect(DeltaTime, MoveToPos);
    }
}

void USK_Guide::UpdateTargetPositionDirect(float DeltaTime, const FVector& TargetFollowPos)
{
    ACharacter* TargetChar = Cast<ACharacter>(GuideTargetActor.Get());
    if (!TargetChar) return;

    FVector TargetLocation = TargetChar->GetActorLocation();
    FVector Direction = (TargetFollowPos - TargetLocation).GetSafeNormal2D();
    
    if (Direction.IsNearlyZero()) return;

    TargetChar->AddMovementInput(Direction, 1.0f);
    
    FRotator CurrentRot = TargetChar->GetActorRotation();
    FRotator TargetRot = Direction.Rotation();
    TargetRot.Pitch = 0.f;
    TargetRot.Roll = 0.f;
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 5.f);
    TargetChar->SetActorRotation(NewRot);
}

void USK_Guide::UpdateTargetPositionNavMesh(const FVector& TargetFollowPos)
{
    if (!TargetNavigationService) return;

    float PosChange = FVector::Dist2D(TargetFollowPos, LastTargetNavPos);
    if (LastTargetNavPos.IsZero() || PosChange > NavUpdateThreshold)
    {
        LastTargetNavPos = TargetFollowPos;
        TargetNavigationService->NavigateTo(TargetFollowPos, TargetAcceptanceRadius * 0.5f);
    }
}

float USK_Guide::GetGuideProgress() const
{
    AMACharacter* Character = const_cast<USK_Guide*>(this)->GetOwningCharacter();
    if (!Character) return 0.f;

    float TotalDistance = FVector::Dist2D(StartLocation, GuideDestination);
    if (TotalDistance < KINDA_SMALL_NUMBER) return 1.f;

    float RemainingDistance = FVector::Dist2D(Character->GetActorLocation(), GuideDestination);
    return FMath::Clamp(1.f - (RemainingDistance / TotalDistance), 0.f, 1.f);
}

bool USK_Guide::IsFlying() const
{
    AMACharacter* Character = const_cast<USK_Guide*>(this)->GetOwningCharacter();
    if (!Character) return false;
    
    return Character->AgentType == EMAAgentType::UAV || 
           Character->AgentType == EMAAgentType::FixedWingUAV;
}

float USK_Guide::GetTargetMoveSpeed() const
{
    if (!GuideTargetActor.IsValid()) return 0.f;
    
    if (ACharacter* TargetChar = Cast<ACharacter>(GuideTargetActor.Get()))
    {
        if (UCharacterMovementComponent* MovementComp = TargetChar->GetCharacterMovement())
        {
            return MovementComp->MaxWalkSpeed;
        }
    }
    
    return 0.f;
}

UMANavigationService* USK_Guide::GetTargetNavigationService() const
{
    if (!GuideTargetActor.IsValid()) return nullptr;

    if (AMAPerson* Person = Cast<AMAPerson>(GuideTargetActor.Get()))
    {
        return Person->GetNavigationService();
    }

    if (AActor* Target = GuideTargetActor.Get())
    {
        return Target->FindComponentByClass<UMANavigationService>();
    }

    return nullptr;
}

void USK_Guide::OnGuideComplete(bool bSuccess, const FString& Message)
{
    bGuideSucceeded = bSuccess;
    GuideResultMessage = Message;

    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
    }

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            float Duration = Character->GetWorld()->GetTimeSeconds() - StartTime;
            FMAFeedbackContext& FeedbackCtx = SkillComp->GetFeedbackContextMutable();
            FeedbackCtx.GuideDurationSeconds = Duration;
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, !bSuccess);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Guide, bGuideSucceeded, GuideResultMessage);
        }
    }
}

void USK_Guide::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }

    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->bIsMoving = false;

        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Guide::OnAgentNavigationCompleted);
            NavigationService->CancelNavigation();
            NavigationService->RestoreDefaultSpeed();
        }
    }

    if (TargetNavigationService)
    {
        TargetNavigationService->CancelNavigation();
    }

    NavigationService = nullptr;
    TargetNavigationService = nullptr;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
