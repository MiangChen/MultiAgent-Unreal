// SK_Place.cpp
// Place 技能实现 - 支持三种模式的完整搬运动作序列
// 使用 NavigationService 进行统一导航

#include "SK_Place.h"
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

void USK_Place::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 重置状态
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
    CurrentPhase = EPlacePhase::MoveToSource;
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    const FMASearchRuntimeResults& Results = SkillComp->GetSearchResults();
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    
    // 确定操作模式
    if (Params.PlaceObject2.IsRobot())
    {
        // 装货模式：将 target 放到 UGV 上
        CurrentMode = EPlaceMode::LoadToUGV;
        
        if (!Results.Object1Actor.IsValid())
        {
            bPlaceSucceeded = false;
            PlaceResultMessage = TEXT("Place failed: Source object not found");
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Source object not found");
            Character->ShowStatus(TEXT("[Place] Source object not found"), 2.f);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        
        SourceObject = Results.Object1Actor;
        SourceLocation = Results.Object1Location;
        
        // 目标 UGV
        TargetUGV = Cast<AMAUGVCharacter>(Results.Object2Actor.Get());
        if (!TargetUGV.IsValid())
        {
            bPlaceSucceeded = false;
            PlaceResultMessage = TEXT("Place failed: Target UGV not found");
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Target UGV not found");
            Character->ShowStatus(TEXT("[Place] UGV not found"), 2.f);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        TargetLocation = TargetUGV->GetActorLocation();
    }
    else if (Params.PlaceObject2.IsGround())
    {
        // 卸货模式：从 UGV 卸货到地面
        CurrentMode = EPlaceMode::UnloadToGround;
        
        // 源对象在 UGV 上
        if (!Results.Object1Actor.IsValid())
        {
            bPlaceSucceeded = false;
            PlaceResultMessage = TEXT("Place failed: Source object not found");
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Source object not found");
            Character->ShowStatus(TEXT("[Place] Source object not found"), 2.f);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        
        SourceObject = Results.Object1Actor;
        
        // 查找物体所在的 UGV
        AActor* ParentActor = SourceObject->GetAttachParentActor();
        TargetUGV = Cast<AMAUGVCharacter>(ParentActor);
        if (TargetUGV.IsValid())
        {
            SourceLocation = TargetUGV->GetActorLocation();
        }
        else
        {
            // 物体不在 UGV 上，直接从当前位置拾取
            SourceLocation = SourceObject->GetActorLocation();
        }
        
        // 放置位置：角色当前位置附近的地面
        DropLocation = Results.Object2Location;
        if (DropLocation.IsZero())
        {
            DropLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.f;
        }
        TargetLocation = DropLocation;
    }
    else
    {
        // 堆叠模式：将 target 放到 surface_target 上
        CurrentMode = EPlaceMode::StackOnObject;
        
        if (!Results.Object1Actor.IsValid())
        {
            bPlaceSucceeded = false;
            PlaceResultMessage = TEXT("Place failed: Source object not found");
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Source object (target) not found");
            Character->ShowStatus(TEXT("[Place] Source object not found"), 2.f);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        
        if (!Results.Object2Actor.IsValid())
        {
            bPlaceSucceeded = false;
            PlaceResultMessage = TEXT("Place failed: Target object not found");
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Target object (surface_target) not found");
            Character->ShowStatus(TEXT("[Place] Target object not found"), 2.f);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        
        SourceObject = Results.Object1Actor;
        SourceLocation = Results.Object1Location;
        TargetObject = Results.Object2Actor;
        TargetLocation = Results.Object2Location;
    }
    
    // 检查是否已经在源对象附近
    float DistanceToSource = FVector::Dist(Character->GetActorLocation(), SourceLocation);
    if (DistanceToSource <= InteractionRadius)
    {
        CurrentPhase = EPlacePhase::BendDownPickup;
    }
    
    // 获取 NavigationService
    NavigationService = Character->GetNavigationService();
    if (!NavigationService)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: NavigationService not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
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
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Character lost during operation");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    if (!NavigationService)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: NavigationService lost during operation");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Moving to source..."));
    
    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
    
    // 使用 NavigationService 导航到源对象
    if (!NavigationService->NavigateTo(SourceLocation, InteractionRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Could not start navigation to source object");
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Could not navigate to source object");
        }
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        // 导航成功，进入拾取阶段
        CurrentPhase = EPlacePhase::BendDownPickup;
        UpdatePhase();
    }
    else
    {
        // 导航失败
        AMACharacter* Character = GetOwningCharacter();
        bPlaceSucceeded = false;
        PlaceResultMessage = FString::Printf(TEXT("Place failed: %s"), *Message);
        
        if (Character)
        {
            if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
            {
                SkillComp->GetFeedbackContextMutable().PlaceErrorReason = Message;
            }
        }
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        CurrentPhase = EPlacePhase::MoveToTarget;
        UpdatePhase();
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
        // 非 Humanoid 角色，直接执行拾取并进入下一阶段
        PerformPickup();
        CurrentPhase = EPlacePhase::StandUpWithItem;
        UpdatePhase();
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
        // 装货模式和堆叠模式：从地面拾取
        PerformPickup();
        
        // 进入起身阶段
        CurrentPhase = EPlacePhase::StandUpWithItem;
        UpdatePhase();
    }
    else if (CurrentPhase == EPlacePhase::BendDownPlace)
    {
        // 卸货模式：放置到地面
        PerformPlaceOnGround();
        
        // 进入起身阶段
        CurrentPhase = EPlacePhase::StandUpEmpty;
        UpdatePhase();
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
        // 非 Humanoid 角色，直接进入下一阶段
        CurrentPhase = EPlacePhase::MoveToTarget;
        UpdatePhase();
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
        // 起身后移动到目标
        CurrentPhase = EPlacePhase::MoveToTarget;
        UpdatePhase();
    }
    else if (CurrentPhase == EPlacePhase::StandUpEmpty)
    {
        // 完成
        CurrentPhase = EPlacePhase::Complete;
        UpdatePhase();
    }
}

void USK_Place::HandleMoveToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Character lost during operation");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    if (!NavigationService)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: NavigationService lost during operation");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Moving to target..."));
    
    // 更新目标位置（UGV 可能已移动）
    FVector CurrentTargetLocation = TargetLocation;
    if (CurrentMode == EPlaceMode::LoadToUGV && TargetUGV.IsValid())
    {
        CurrentTargetLocation = TargetUGV->GetActorLocation();
    }
    else if (CurrentMode == EPlaceMode::StackOnObject && TargetObject.IsValid())
    {
        CurrentTargetLocation = TargetObject->GetActorLocation();
    }
    
    // 检查是否已经在目标附近
    float DistanceToTarget = FVector::Dist(Character->GetActorLocation(), CurrentTargetLocation);
    if (DistanceToTarget <= InteractionRadius)
    {
        CurrentPhase = EPlacePhase::BendDownPlace;
        UpdatePhase();
        return;
    }
    
    // 绑定导航完成回调
    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
    
    // 使用 NavigationService 导航到目标
    if (!NavigationService->NavigateTo(CurrentTargetLocation, InteractionRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Could not start navigation to target location");
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = TEXT("Could not navigate to target location");
        }
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        // 导航成功，进入放置阶段
        CurrentPhase = EPlacePhase::BendDownPlace;
        UpdatePhase();
    }
    else
    {
        // 导航失败
        AMACharacter* Character = GetOwningCharacter();
        bPlaceSucceeded = false;
        PlaceResultMessage = FString::Printf(TEXT("Place failed: %s"), *Message);
        
        if (Character)
        {
            if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
            {
                SkillComp->GetFeedbackContextMutable().PlaceErrorReason = Message;
            }
        }
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
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
        CurrentPhase = EPlacePhase::Complete;
        UpdatePhase();
        return;
    }
    else if (CurrentMode == EPlaceMode::StackOnObject)
    {
        PerformPlaceOnObject();
        CurrentPhase = EPlacePhase::Complete;
        UpdatePhase();
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
        // 非 Humanoid 角色，直接执行放置并完成
        PerformPlaceOnGround();
        CurrentPhase = EPlacePhase::Complete;
        UpdatePhase();
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
        // 非 Humanoid 角色，直接完成
        CurrentPhase = EPlacePhase::Complete;
        UpdatePhase();
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
    
    bool bShouldNotify = false;
    bool bSuccessToNotify = bPlaceSucceeded;
    FString MessageToNotify = PlaceResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        // 清理动画回调
        AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter();
        if (Humanoid)
        {
            Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnBendDownComplete);
            Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnStandUpComplete);
        }
        
        // 清理导航服务回调并取消导航
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
            IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
            if (Item)
            {
                FVector DropPos = Character->GetActorLocation();
                Item->PlaceOnGround(DropPos);
            }
            HeldObject.Reset();
        }
        
        if (bWasCancelled && PlaceResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FString PhaseStr = GetPhaseString();
            MessageToNotify = FString::Printf(TEXT("Place cancelled: Stopped while %s"), *PhaseStr);
            
            if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
            {
                SkillComp->GetFeedbackContextMutable().PlaceCancelledPhase = PhaseStr;
            }
        }
        
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag PlaceTag = FGameplayTag::RequestGameplayTag(FName("Command.Place"));
            if (SkillComp->HasMatchingGameplayTag(PlaceTag))
            {
                SkillComp->RemoveLooseGameplayTag(PlaceTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    // 清理引用
    NavigationService = nullptr;
    SourceObject.Reset();
    TargetObject.Reset();
    HeldObject.Reset();
    TargetUGV.Reset();
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
