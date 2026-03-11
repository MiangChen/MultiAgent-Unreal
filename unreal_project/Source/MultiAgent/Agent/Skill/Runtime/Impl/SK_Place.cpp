// SK_Place.cpp
// Place 技能实现 - 支持三种模式的完整搬运动作序列
// 使用 NavigationService 进行统一导航

#include "SK_Place.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAHumanoidCharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUGVCharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "../../../Environment/IMAPickupItem.h"
#include "TimerManager.h"

USK_Place::USK_Place()
{
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
}

void USK_Place::ResetPlaceRuntimeState()
{
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
    CurrentPhase = EPlacePhase::MoveToSource;
}

void USK_Place::FailPlace(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage)
{
    bPlaceSucceeded = false;
    PlaceResultMessage = ResultMessage;

    if (AMACharacter* Character = GetOwningCharacter())
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = ErrorReason;
        }
        if (!StatusMessage.IsEmpty())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_Place::ConfigureLoadToUGV(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    const FMASearchRuntimeResults& Results = SkillComp.GetSearchResults();
    CurrentMode = EPlaceMode::LoadToUGV;

    if (!Results.Object1Actor.IsValid())
    {
        FailPlace(TEXT("Place failed: Source object not found"), TEXT("Source object not found"), TEXT("[Place] Source object not found"));
        return false;
    }

    SourceObject = Results.Object1Actor;
    SourceLocation = Results.Object1Location;

    TargetUGV = Cast<AMAUGVCharacter>(Results.Object2Actor.Get());
    if (!TargetUGV.IsValid())
    {
        FailPlace(TEXT("Place failed: Target UGV not found"), TEXT("Target UGV not found"), TEXT("[Place] UGV not found"));
        return false;
    }

    TargetLocation = TargetUGV->GetActorLocation();
    return true;
}

bool USK_Place::ConfigureUnloadToGround(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    const FMASearchRuntimeResults& Results = SkillComp.GetSearchResults();
    CurrentMode = EPlaceMode::UnloadToGround;

    if (!Results.Object1Actor.IsValid())
    {
        FailPlace(TEXT("Place failed: Source object not found"), TEXT("Source object not found"), TEXT("[Place] Source object not found"));
        return false;
    }

    SourceObject = Results.Object1Actor;
    AActor* ParentActor = SourceObject->GetAttachParentActor();
    TargetUGV = Cast<AMAUGVCharacter>(ParentActor);
    SourceLocation = TargetUGV.IsValid() ? TargetUGV->GetActorLocation() : SourceObject->GetActorLocation();

    DropLocation = Results.Object2Location;
    if (DropLocation.IsZero())
    {
        DropLocation = Character.GetActorLocation() + Character.GetActorForwardVector() * 100.f;
    }
    TargetLocation = DropLocation;
    return true;
}

bool USK_Place::ConfigureStackOnObject(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    const FMASearchRuntimeResults& Results = SkillComp.GetSearchResults();
    CurrentMode = EPlaceMode::StackOnObject;

    if (!Results.Object1Actor.IsValid())
    {
        FailPlace(TEXT("Place failed: Source object not found"), TEXT("Source object (target) not found"), TEXT("[Place] Source object not found"));
        return false;
    }

    if (!Results.Object2Actor.IsValid())
    {
        FailPlace(TEXT("Place failed: Target object not found"), TEXT("Target object (surface_target) not found"), TEXT("[Place] Target object not found"));
        return false;
    }

    SourceObject = Results.Object1Actor;
    SourceLocation = Results.Object1Location;
    TargetObject = Results.Object2Actor;
    TargetLocation = Results.Object2Location;
    return true;
}

bool USK_Place::InitializePlaceContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    const FMASkillParams& Params = SkillComp.GetSkillParams();

    const bool bConfigured = Params.PlaceObject2.IsRobot()
        ? ConfigureLoadToUGV(Character, SkillComp)
        : (Params.PlaceObject2.IsGround()
            ? ConfigureUnloadToGround(Character, SkillComp)
            : ConfigureStackOnObject(Character, SkillComp));
    if (!bConfigured)
    {
        return false;
    }

    if (FVector::Dist(Character.GetActorLocation(), SourceLocation) <= InteractionRadius)
    {
        CurrentPhase = EPlacePhase::BendDownPickup;
    }

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailPlace(TEXT("Place failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    return true;
}

void USK_Place::AdvanceToPhase(const EPlacePhase NextPhase)
{
    CurrentPhase = NextPhase;
    UpdatePhase();
}

void USK_Place::HandleNavigationFailure(const FString& Message)
{
    FailPlace(FString::Printf(TEXT("Place failed: %s"), *Message), Message);
}

FVector USK_Place::ResolveCurrentTargetLocation() const
{
    if (CurrentMode == EPlaceMode::LoadToUGV && TargetUGV.IsValid())
    {
        return TargetUGV->GetActorLocation();
    }
    if (CurrentMode == EPlaceMode::StackOnObject && TargetObject.IsValid())
    {
        return TargetObject->GetActorLocation();
    }
    return TargetLocation;
}

void USK_Place::CleanupPlaceRuntime(
    AMACharacter* Character,
    const bool bWasCancelled,
    bool& bOutSuccessToNotify,
    FString& InOutMessageToNotify)
{
    if (!Character)
    {
        NavigationService = nullptr;
        SourceObject.Reset();
        TargetObject.Reset();
        HeldObject.Reset();
        TargetUGV.Reset();
        return;
    }

    if (AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter())
    {
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnBendDownComplete);
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnStandUpComplete);
    }

    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
        NavigationService->CancelNavigation();
    }

    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseTimerHandle);
    }
    PhaseTimerHandle.Invalidate();
    Character->ShowStatus(TEXT(""), 0.f);

    if (bWasCancelled && HeldObject.IsValid())
    {
        if (IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get()))
        {
            Item->PlaceOnGround(Character->GetActorLocation());
        }
        HeldObject.Reset();
    }

    if (bWasCancelled && PlaceResultMessage.IsEmpty())
    {
        bOutSuccessToNotify = false;
        const FString PhaseStr = GetPhaseString();
        InOutMessageToNotify = FString::Printf(TEXT("Place cancelled: Stopped while %s"), *PhaseStr);

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceCancelledPhase = PhaseStr;
        }
    }

    NavigationService = nullptr;
    SourceObject.Reset();
    TargetObject.Reset();
    HeldObject.Reset();
    TargetUGV.Reset();
}

void USK_Place::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetPlaceRuntimeState();
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailPlace(TEXT("Place failed: Character not found"), TEXT("Character not found"));
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailPlace(TEXT("Place failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializePlaceContext(*Character, *SkillComp))
    {
        return;
    }

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Starting..."));
    UpdatePhase();
}

void USK_Place::UpdatePhase()
{
    switch (CurrentPhase)
    {
        case EPlacePhase::MoveToSource:
            HandleMoveToSource();
            break;
        case EPlacePhase::BendDownPickup:
            HandleBendDownPickup();
            break;
        case EPlacePhase::StandUpWithItem:
            HandleStandUpWithItem();
            break;
        case EPlacePhase::MoveToTarget:
            HandleMoveToTarget();
            break;
        case EPlacePhase::BendDownPlace:
            HandleBendDownPlace();
            break;
        case EPlacePhase::StandUpEmpty:
            HandleStandUpEmpty();
            break;
        case EPlacePhase::Complete:
            HandleComplete();
            break;
    }
}

void USK_Place::HandleMoveToSource()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailPlace(TEXT("Place failed: Character lost during operation"), TEXT("Character lost during operation"));
        return;
    }
    
    if (!NavigationService)
    {
        FailPlace(TEXT("Place failed: NavigationService lost during operation"), TEXT("NavigationService lost during operation"));
        return;
    }
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Moving to source..."));
    
    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
    
    // 使用 NavigationService 导航到源对象
    if (!NavigationService->NavigateTo(SourceLocation, InteractionRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
        FailPlace(TEXT("Place failed: Could not start navigation to source object"), TEXT("Could not navigate to source object"));
    }
}

void USK_Place::OnNavigationToSourceCompleted(bool bSuccess, const FString& Message)
{
    // 解绑回调
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
    }
    
    if (bSuccess)
    {
        AdvanceToPhase(EPlacePhase::BendDownPickup);
    }
    else
    {
        HandleNavigationFailure(Message);
    }
}

void USK_Place::HandleBendDownPickup()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Picking up..."));
    
    // 卸货模式（UnloadToGround）：物品在 UGV 上，不需要俯身动画
    // 直接拾取后进入移动阶段
    if (CurrentMode == EPlaceMode::UnloadToGround)
    {
        PerformPickupFromUGV();
        AdvanceToPhase(EPlacePhase::MoveToTarget);
        return;
    }
    
    // 装货模式（LoadToUGV）和堆叠模式（StackOnObject）：
    // 物品在地面上，需要俯身动画
    AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
    if (Humanoid)
    {
        // 绑定动画完成回调
        Humanoid->OnBendAnimationComplete.AddDynamic(this, &USK_Place::OnBendDownComplete);
        Humanoid->PlayBendDownAnimation();
    }
    else
    {
        PerformPickup();
        AdvanceToPhase(EPlacePhase::StandUpWithItem);
    }
}

void USK_Place::OnBendDownComplete()
{
    AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
    if (Humanoid)
    {
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnBendDownComplete);
    }
    
    // 俯身完成，执行拾取或放置
    if (CurrentPhase == EPlacePhase::BendDownPickup)
    {
        PerformPickup();
        AdvanceToPhase(EPlacePhase::StandUpWithItem);
    }
    else if (CurrentPhase == EPlacePhase::BendDownPlace)
    {
        PerformPlaceOnGround();
        AdvanceToPhase(EPlacePhase::StandUpEmpty);
    }
}

void USK_Place::HandleStandUpWithItem()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Standing up..."));
    
    // 播放起身动画（动画后半段）
    AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
    if (Humanoid)
    {
        Humanoid->OnBendAnimationComplete.AddDynamic(this, &USK_Place::OnStandUpComplete);
        Humanoid->PlayStandUpAnimation();
    }
    else
    {
        AdvanceToPhase(EPlacePhase::MoveToTarget);
    }
}

void USK_Place::OnStandUpComplete()
{
    AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
    if (Humanoid)
    {
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnStandUpComplete);
    }
    
    if (CurrentPhase == EPlacePhase::StandUpWithItem)
    {
        AdvanceToPhase(EPlacePhase::MoveToTarget);
    }
    else if (CurrentPhase == EPlacePhase::StandUpEmpty)
    {
        AdvanceToPhase(EPlacePhase::Complete);
    }
}

void USK_Place::HandleMoveToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailPlace(TEXT("Place failed: Character lost during operation"), TEXT("Character lost during operation"));
        return;
    }
    
    if (!NavigationService)
    {
        FailPlace(TEXT("Place failed: NavigationService lost during operation"), TEXT("NavigationService lost during operation"));
        return;
    }
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Moving to target..."));
    
    // 更新目标位置（UGV 可能已移动）
    const FVector CurrentTargetLocation = ResolveCurrentTargetLocation();
    
    // 检查是否已经在目标附近
    float DistanceToTarget = FVector::Dist(Character->GetActorLocation(), CurrentTargetLocation);
    if (DistanceToTarget <= InteractionRadius)
    {
        AdvanceToPhase(EPlacePhase::BendDownPlace);
        return;
    }
    
    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
    
    // 使用 NavigationService 导航到目标
    if (!NavigationService->NavigateTo(CurrentTargetLocation, InteractionRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
        FailPlace(TEXT("Place failed: Could not start navigation to target location"), TEXT("Could not navigate to target location"));
    }
}

void USK_Place::OnNavigationToTargetCompleted(bool bSuccess, const FString& Message)
{
    // 解绑回调
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
    }
    
    if (bSuccess)
    {
        AdvanceToPhase(EPlacePhase::BendDownPlace);
    }
    else
    {
        HandleNavigationFailure(Message);
    }
}

void USK_Place::HandleBendDownPlace()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Placing..."));
    
    // 装货模式（LoadToUGV）和堆叠模式（StackOnObject）：
    // 放置到高处（UGV 或另一个物体上），不需要俯身动画
    if (CurrentMode == EPlaceMode::LoadToUGV)
    {
        PerformPlaceOnUGV();
        AdvanceToPhase(EPlacePhase::Complete);
        return;
    }
    else if (CurrentMode == EPlaceMode::StackOnObject)
    {
        PerformPlaceOnObject();
        AdvanceToPhase(EPlacePhase::Complete);
        return;
    }
    
    // 卸货模式（UnloadToGround）：放置到地面，需要俯身动画
    AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
    if (Humanoid)
    {
        Humanoid->OnBendAnimationComplete.AddDynamic(this, &USK_Place::OnBendDownComplete);
        Humanoid->PlayBendDownAnimation();
    }
    else
    {
        PerformPlaceOnGround();
        AdvanceToPhase(EPlacePhase::Complete);
    }
}

void USK_Place::HandleStandUpEmpty()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Standing up..."));
    
    // 播放起身动画（动画后半段）
    AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
    if (Humanoid)
    {
        Humanoid->OnBendAnimationComplete.AddDynamic(this, &USK_Place::OnStandUpComplete);
        Humanoid->PlayStandUpAnimation();
    }
    else
    {
        AdvanceToPhase(EPlacePhase::Complete);
    }
}

void USK_Place::HandleComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return;
    
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    bPlaceSucceeded = true;
    
    // 设置目标名称和最终位置
    FString TargetName;
    FVector FinalLocation;
    
    switch (CurrentMode)
    {
        case EPlaceMode::LoadToUGV:
            TargetName = TargetUGV.IsValid() ? TargetUGV->AgentLabel : TEXT("UGV");
            FinalLocation = TargetUGV.IsValid() ? TargetUGV->GetActorLocation() : TargetLocation;
            break;
        case EPlaceMode::UnloadToGround:
            TargetName = TEXT("ground");
            FinalLocation = DropLocation;
            break;
        case EPlaceMode::StackOnObject:
            TargetName = Context.PlaceTargetName.IsEmpty() ? TEXT("object") : Context.PlaceTargetName;
            FinalLocation = TargetObject.IsValid() ? TargetObject->GetActorLocation() : TargetLocation;
            break;
    }
    
    // 更新反馈上下文
    Context.PlaceTargetName = TargetName;
    Context.PlaceFinalLocation = FinalLocation;
    
    PlaceResultMessage = FString::Printf(TEXT("Place succeeded: Moved %s to %s"), 
        *Context.PlacedObjectName, *TargetName);
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Complete!"));
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

// ========== 物体操作 ==========

void USK_Place::PerformPickup()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !SourceObject.IsValid()) return;
    
    IMAPickupItem* Item = Cast<IMAPickupItem>(SourceObject.Get());
    if (Item)
    {
        Item->AttachToHand(Character);
        HeldObject = SourceObject;
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlacedObjectName = Item->GetItemName();
        }
    }
}

void USK_Place::PerformPickupFromUGV()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !SourceObject.IsValid()) return;
    
    IMAPickupItem* Item = Cast<IMAPickupItem>(SourceObject.Get());
    if (!Item) return;
    
    Item->DetachFromCarrier();
    Item->AttachToHand(Character);
    HeldObject = SourceObject;
    
    if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
    {
        SkillComp->GetFeedbackContextMutable().PlacedObjectName = Item->GetItemName();
    }
}

void USK_Place::PerformPlaceOnGround()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid()) return;
    
    IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
    if (Item)
    {
        // 放置在角色前方，避免物体困住角色
        FVector PlaceLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 120.f;
        PlaceLocation.Z = Character->GetActorLocation().Z;
        Item->PlaceOnGround(PlaceLocation);
        HeldObject.Reset();
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceTargetName = TEXT("ground");
        }
    }
}

void USK_Place::PerformPlaceOnUGV()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid() || !TargetUGV.IsValid()) return;
    
    IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
    if (Item)
    {
        Item->AttachToUGV(TargetUGV.Get());
        HeldObject.Reset();
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceTargetName = TargetUGV->AgentLabel;
        }
    }
}

void USK_Place::PerformPlaceOnObject()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid() || !TargetObject.IsValid()) return;
    
    IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
    IMAPickupItem* Target = Cast<IMAPickupItem>(TargetObject.Get());
    
    if (Item && Target)
    {
        Item->PlaceOnObject(TargetObject.Get());
        HeldObject.Reset();
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceTargetName = Target->GetItemName();
        }
    }
}

// ========== 辅助方法 ==========

AMAHumanoidCharacter* USK_Place::GetHumanoidCharacter() const
{
    return Cast<AMAHumanoidCharacter>(GetOwningCharacter());
}

FString USK_Place::GetPhaseString() const
{
    switch (CurrentPhase)
    {
        case EPlacePhase::MoveToSource: return TEXT("moving to source");
        case EPlacePhase::BendDownPickup: return TEXT("bending down to pickup");
        case EPlacePhase::StandUpWithItem: return TEXT("standing up with item");
        case EPlacePhase::MoveToTarget: return TEXT("moving to target");
        case EPlacePhase::BendDownPlace: return TEXT("bending down to place");
        case EPlacePhase::StandUpEmpty: return TEXT("standing up");
        case EPlacePhase::Complete: return TEXT("complete");
        default: return TEXT("unknown phase");
    }
}

void USK_Place::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    bool bSuccessToNotify = bPlaceSucceeded;
    FString MessageToNotify = PlaceResultMessage;
    
    CleanupPlaceRuntime(Character, bWasCancelled, bSuccessToNotify, MessageToNotify);
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Place, bSuccessToNotify, MessageToNotify);
        }
    }
}
