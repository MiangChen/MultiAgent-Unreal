// SK_Transport.cpp
// 运输技能实现

#include "SK_Transport.h"
#include "MAObservationSkillRuntimeHelpers.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Infrastructure/MASkillConfigBridge.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Transport/Runtime/MATransportCoordinator.h"
#include "TimerManager.h"

namespace
{
/** WaitReady 阶段栅栏轮询周期 (秒) */
constexpr float ReadyPollInterval = 0.2f;
}

USK_Transport::USK_Transport()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_Transport::ResetTransportRuntimeState()
{
    CurrentPhase = ETransportPhase::Joining;
    bTransportSucceeded = false;
    TransportResultMessage.Reset();
    StartTime = 0.f;
    bIsAircraft = false;
    MinFlightAltitude = 800.f;
    SessionKey.Reset();
    Destination = FVector::ZeroVector;
    GraspPoint = FVector::ZeroVector;
    FormationOffset = FVector::ZeroVector;
    bIsLeader = false;
    ParticipantCount = 0;
    NavigationService = nullptr;
    TargetObject.Reset();
}UMATransportCoordinator* USK_Transport::GetCoordinator() const
{
    const AMACharacter* Character = GetOwningCharacter();
    const UWorld* World = Character ? Character->GetWorld() : nullptr;
    return World ? World->GetSubsystem<UMATransportCoordinator>() : nullptr;
}

void USK_Transport::FailTransport(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage)
{
    bTransportSucceeded = false;
    TransportResultMessage = ResultMessage;

    UE_LOG(LogTemp, Error, TEXT("[SK_Transport] %s"), *ErrorReason);

    if (!StatusMessage.IsEmpty())
    {
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_Transport::InitializeTransportContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    FMASkillConfigBridge::ApplyTransportConfig(
        Character,
        GraspHeightOffset,
        LiftAltitude,
        CarryAltitude,
        AcceptanceRadius,
        ReadyTimeout);

    UE_LOG(LogTemp, Log, TEXT("[SK_Transport] Loaded config: GraspHeightOffset=%.0f, LiftAltitude=%.0f, CarryAltitude=%.0f, AcceptRadius=%.0f, ReadyTimeout=%.0f"),
        GraspHeightOffset, LiftAltitude, CarryAltitude, AcceptanceRadius, ReadyTimeout);

    AActor* Object = SkillComp.GetSkillRuntimeTargets().TransportTargetActor.Get();
    if (!Object)
    {
        FailTransport(TEXT("Transport failed: No valid target object"), TEXT("TransportTargetActor not found"), TEXT("[Transport] Target not found"));
        return false;
    }
    TargetObject = Object;

    Destination = SkillComp.GetSkillParams().TransportDestination;
    if (Destination.IsZero())
    {
        FailTransport(TEXT("Transport failed: No destination"), TEXT("TransportDestination is zero"), TEXT("[Transport] No destination"));
        return false;
    }

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailTransport(TEXT("Transport failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    bIsAircraft = MAObservationSkillRuntime::IsAircraft(Character);
    MinFlightAltitude = MAObservationSkillRuntime::ResolveMinFlightAltitude(Character, MinFlightAltitude);

    UMATransportCoordinator* Coordinator = GetCoordinator();
    if (!Coordinator)
    {
        FailTransport(TEXT("Transport failed: Coordinator not found"), TEXT("MATransportCoordinator not available"));
        return false;
    }

    SessionKey = UMATransportCoordinator::MakeSessionKey(Object, Destination);
    Coordinator->JoinSession(SessionKey, Object, Destination, &Character);

    CurrentPhase = ETransportPhase::Joining;
    return true;
}

void USK_Transport::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetTransportRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailTransport(TEXT("Transport failed: Character not found"), TEXT("Character not found"));
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailTransport(TEXT("Transport failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializeTransportContext(*Character, *SkillComp))
    {
        return;
    }

    if (UWorld* World = Character->GetWorld())
    {
        StartTime = World->GetTimeSeconds();
        Character->ShowAbilityStatus(TEXT("Transport"), TEXT("Coordinating..."));

        // 延迟一帧定稿：让同一时间步同步派发的其它参与者先完成 JoinSession
        World->GetTimerManager().SetTimerForNextTick(this, &USK_Transport::OnFinalizeTick);
    }
}

void USK_Transport::OnFinalizeTick()
{
    AMACharacter* Character = GetOwningCharacter();
    UMATransportCoordinator* Coordinator = GetCoordinator();
    if (!Character || !Coordinator)
    {
        FailTransport(TEXT("Transport failed: Lost reference during finalize"), TEXT("OnFinalizeTick lost Character or Coordinator"));
        return;
    }

    const FMATransportAssignment Assignment = Coordinator->FinalizeAndGetAssignment(SessionKey, Character);
    if (!Assignment.bValid)
    {
        FailTransport(TEXT("Transport failed: No assignment"), TEXT("FinalizeAndGetAssignment returned invalid"));
        return;
    }

    GraspPoint = Assignment.GraspPoint;
    FormationOffset = Assignment.FormationOffset;
    bIsLeader = Assignment.bIsLeader;
    ParticipantCount = Assignment.ParticipantCount;

    if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
    {
        FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
        Context.TransportParticipantCount = ParticipantCount;
        Context.TransportRole = bIsLeader ? TEXT("leader") : TEXT("follower");
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_Transport] %s: role=%s, participants=%d, graspPoint=%s"),
        *Character->AgentLabel, bIsLeader ? TEXT("leader") : TEXT("follower"),
        ParticipantCount, *GraspPoint.ToString());

    EnterMoveToGrasp();
}

void USK_Transport::EnterMoveToGrasp()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        FailTransport(TEXT("Transport failed: Lost reference"), TEXT("EnterMoveToGrasp lost Character or NavigationService"));
        return;
    }

    CurrentPhase = ETransportPhase::MoveToGrasp;
    Character->ShowAbilityStatus(TEXT("Transport"), TEXT("Moving to grasp point..."));

    // 抓取点垂直偏移：允许负值（贴在物体下方/侧面）。
    // 抓取阶段是贴近作业，目标 Z 不再受 MinFlightAltitude 兜底，由 simulation.json 的
    // flight.min_altitude 在飞控层决定下限（设为 0 则可以贴地作业）。
    FVector MovePoint = GraspPoint;
    MovePoint.Z += GraspHeightOffset;

    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Transport::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(MovePoint, AcceptanceRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Transport::OnNavigationCompleted);
        FailTransport(TEXT("Transport failed: Could not start navigation to grasp point"), TEXT("NavigateTo (grasp) returned false"));
    }
}

void USK_Transport::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Transport::OnNavigationCompleted);
    }

    if (!bSuccess)
    {
        FailTransport(FString::Printf(TEXT("Transport failed: %s"), *Message), Message);
        return;
    }

    if (CurrentPhase == ETransportPhase::MoveToGrasp)
    {
        // 到达抓取点，上报就位，进入栅栏等待
        AMACharacter* Character = GetOwningCharacter();
        UMATransportCoordinator* Coordinator = GetCoordinator();
        UWorld* World = Character ? Character->GetWorld() : nullptr;
        if (!Character || !Coordinator || !World)
        {
            FailTransport(TEXT("Transport failed: Lost reference at grasp"), TEXT("OnNavigationCompleted(grasp) lost references"));
            return;
        }

        CurrentPhase = ETransportPhase::WaitReady;
        Coordinator->ReportReady(SessionKey, Character);
        Character->ShowAbilityStatus(TEXT("Transport"), TEXT("Waiting for team..."));

        World->GetTimerManager().SetTimer(PhaseTimerHandle, this, &USK_Transport::OnReadyTick, ReadyPollInterval, true);
    }
    else if (CurrentPhase == ETransportPhase::Ascend)
    {
        EnterCarry();
    }
    else if (CurrentPhase == ETransportPhase::Carry)
    {
        CompleteTransport();
    }
}

void USK_Transport::OnReadyTick()
{
    AMACharacter* Character = GetOwningCharacter();
    UMATransportCoordinator* Coordinator = GetCoordinator();
    UWorld* World = Character ? Character->GetWorld() : nullptr;
    if (!Character || !Coordinator || !World)
    {
        FailTransport(TEXT("Transport failed: Lost reference while waiting"), TEXT("OnReadyTick lost references"));
        return;
    }

    // 超时保护
    if (World->GetTimeSeconds() - StartTime > ReadyTimeout)
    {
        World->GetTimerManager().ClearTimer(PhaseTimerHandle);
        FailTransport(TEXT("Transport failed: Ready timeout"), TEXT("Not all participants ready within timeout"), TEXT("[Transport] Team timeout"));
        return;
    }

    // leader 在全体就位后发起抬起；follower 等待抬起完成
    if (bIsLeader)
    {
        Coordinator->TryBeginLift(SessionKey, Character);
    }

    if (Coordinator->IsLiftStarted(SessionKey))
    {
        World->GetTimerManager().ClearTimer(PhaseTimerHandle);
        EnterAscend();
    }
}

void USK_Transport::EnterAscend()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        FailTransport(TEXT("Transport failed: Lost reference"), TEXT("EnterAscend lost Character or NavigationService"));
        return;
    }

    // 仅飞行器需要垂直爬升；地面机器人或当前已高于阈值则直接进入运输阶段。
    const FVector CurrentLocation = Character->GetActorLocation();
    if (!bIsAircraft || CurrentLocation.Z >= LiftAltitude)
    {
        EnterCarry();
        return;
    }

    CurrentPhase = ETransportPhase::Ascend;
    Character->ShowAbilityStatus(TEXT("Transport"), TEXT("Ascending..."));

    // 保持 XY 不变、Z 抬到 LiftAltitude；ApplyFlightAltitude 会再用 MinFlightAltitude 兜底，
    // 此处取两者较大值，确保在低于飞控最低限制时不会被压回去。
    const FVector AscendTarget = ApplyFlightAltitude(CurrentLocation, FMath::Max(LiftAltitude, MinFlightAltitude));

    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Transport::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(AscendTarget, AcceptanceRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Transport::OnNavigationCompleted);
        FailTransport(TEXT("Transport failed: Could not start ascend"), TEXT("NavigateTo (ascend) returned false"));
    }
}

void USK_Transport::EnterCarry()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        FailTransport(TEXT("Transport failed: Lost reference"), TEXT("EnterCarry lost Character or NavigationService"));
        return;
    }

    CurrentPhase = ETransportPhase::Carry;
    Character->ShowAbilityStatus(TEXT("Transport"), TEXT("Carrying to destination..."));

    // 各参与者保持相对物体中心的编队偏移飞往目的地；运输高度统一抬到 CarryAltitude
    FVector CarryTarget = Destination + FVector(FormationOffset.X, FormationOffset.Y, 0.f);
    CarryTarget = ApplyFlightAltitude(CarryTarget, FMath::Max(Destination.Z, CarryAltitude));

    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Transport::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(CarryTarget, AcceptanceRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Transport::OnNavigationCompleted);
        FailTransport(TEXT("Transport failed: Could not start navigation to destination"), TEXT("NavigateTo (carry) returned false"));
    }
}

void USK_Transport::CompleteTransport()
{
    AMACharacter* Character = GetOwningCharacter();

    bTransportSucceeded = true;
    TransportResultMessage = TEXT("Transport completed successfully");

    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("Transport"), TEXT("Complete!"));

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
            if (UWorld* World = Character->GetWorld())
            {
                Context.TransportDurationSeconds = World->GetTimeSeconds() - StartTime;
            }
        }
    }

    // 物体保留附着在 leader 上（很可能仍在空中），不解除。
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

FVector USK_Transport::ApplyFlightAltitude(const FVector& Point, float Altitude) const
{
    FVector Result = Point;
    Result.Z = Altitude;
    if (bIsAircraft && Result.Z < MinFlightAltitude)
    {
        Result.Z = MinFlightAltitude;
    }
    return Result;
}

void USK_Transport::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();

    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseTimerHandle);
        }

        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Transport::OnNavigationCompleted);
            NavigationService->CancelNavigation();
        }

        if (UMATransportCoordinator* Coordinator = GetCoordinator())
        {
            // 退出 Session（不解除物体附着）
            Coordinator->LeaveSession(SessionKey, Character);
        }

        Character->ShowStatus(TEXT(""), 0.f);
    }

    bool bSuccessToNotify = bTransportSucceeded;
    FString MessageToNotify = TransportResultMessage;

    if (bWasCancelled && TransportResultMessage.IsEmpty())
    {
        bSuccessToNotify = false;
        MessageToNotify = TEXT("Transport cancelled");
    }

    NavigationService = nullptr;
    TargetObject.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Transport, bSuccessToNotify, MessageToNotify);
        }
    }
}
