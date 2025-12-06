// MAAbilitySystemComponent.cpp

#include "MAAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Ability/GA_Pickup.h"
#include "Ability/GA_Drop.h"
#include "Ability/GA_Navigate.h"
#include "Ability/GA_Follow.h"
#include "Ability/GA_TakePhoto.h"
// GA_Patrol 已移除，Patrol 改用 StateTree
#include "Ability/GA_Search.h"
#include "Ability/GA_Observe.h"
#include "Ability/GA_Report.h"
#include "Ability/GA_Charge.h"
#include "Ability/GA_Formation.h"
#include "Ability/GA_Avoid.h"
#include "../Actor/MAPatrolPath.h"
#include "MAGameplayTags.h"

UMAAbilitySystemComponent::UMAAbilitySystemComponent()
{
}

void UMAAbilitySystemComponent::InitializeAbilities(AActor* InOwnerActor)
{
    if (!InOwnerActor) return;

    // 授予默认 Abilities (从 DefaultAbilities 数组)
    for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, InOwnerActor));
        }
    }

    // 始终授予 Pickup、Drop、Navigate、TakePhoto Ability，并保存 Handle
    PickupAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Pickup::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    DropAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Drop::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    NavigateAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Navigate::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    FollowAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Follow::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    TakePhotoAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_TakePhoto::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    
    // 新增 Robot Abilities
    // 注意: Patrol 已移至 StateTree，不再使用 GAS Ability
    SearchAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Search::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    ObserveAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Observe::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    ReportAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Report::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    ChargeAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Charge::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    FormationAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Formation::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    AvoidAbilityHandle = GiveAbility(FGameplayAbilitySpec(UGA_Avoid::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    
    UE_LOG(LogTemp, Log, TEXT("Initialized GAS abilities for %s (Pickup=%d, Drop=%d, Navigate=%d, Follow=%d, TakePhoto=%d, Search=%d, Charge=%d)"), 
        *InOwnerActor->GetName(), PickupAbilityHandle.IsValid(), DropAbilityHandle.IsValid(), NavigateAbilityHandle.IsValid(), 
        FollowAbilityHandle.IsValid(), TakePhotoAbilityHandle.IsValid(), SearchAbilityHandle.IsValid(), ChargeAbilityHandle.IsValid());
}

bool UMAAbilitySystemComponent::TryActivatePickup()
{
    if (PickupAbilityHandle.IsValid())
    {
        return TryActivateAbility(PickupAbilityHandle);
    }
    
    // Fallback: 尝试通过 Class 激活
    return TryActivateAbilityByClass(UGA_Pickup::StaticClass());
}

bool UMAAbilitySystemComponent::TryActivateDrop()
{
    if (DropAbilityHandle.IsValid())
    {
        return TryActivateAbility(DropAbilityHandle);
    }
    
    // Fallback: 尝试通过 Class 激活
    return TryActivateAbilityByClass(UGA_Drop::StaticClass());
}

bool UMAAbilitySystemComponent::TryActivateNavigate(FVector TargetLocation)
{
    // 先取消正在进行的导航
    CancelNavigate();
    
    if (!NavigateAbilityHandle.IsValid())
    {
        return false;
    }
    
    // 先设置目标位置到 Ability 实例
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(NavigateAbilityHandle);
    if (Spec)
    {
        // 获取或创建实例
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            // 强制创建实例
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Navigate* NavigateAbility = Cast<UGA_Navigate>(Instance))
        {
            NavigateAbility->SetTargetLocation(TargetLocation);
        }
    }
    
    return TryActivateAbility(NavigateAbilityHandle);
}

void UMAAbilitySystemComponent::CancelNavigate()
{
    // 通过 Handle 直接取消，避免使用已废弃的 AbilityTags
    if (NavigateAbilityHandle.IsValid())
    {
        CancelAbilityHandle(NavigateAbilityHandle);
    }
}

bool UMAAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    return TryActivateAbilitiesByTag(TagContainer);
}

void UMAAbilitySystemComponent::CancelAbilityByTag(FGameplayTag AbilityTag)
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    CancelAbilities(&TagContainer);
}

bool UMAAbilitySystemComponent::TryActivateTakePhoto()
{
    if (TakePhotoAbilityHandle.IsValid())
    {
        return TryActivateAbility(TakePhotoAbilityHandle);
    }
    
    // Fallback: 尝试通过 Class 激活
    return TryActivateAbilityByClass(UGA_TakePhoto::StaticClass());
}

bool UMAAbilitySystemComponent::HasGameplayTagFromContainer(FGameplayTag TagToCheck) const
{
    FGameplayTagContainer OwnedTags;
    GetOwnedGameplayTags(OwnedTags);
    return OwnedTags.HasTag(TagToCheck);
}

bool UMAAbilitySystemComponent::TryActivateFollow(AMACharacter* TargetCharacter, float FollowDistance)
{
    // 先取消正在进行的追踪和导航
    CancelFollow();
    CancelNavigate();
    
    if (!FollowAbilityHandle.IsValid() || !TargetCharacter)
    {
        return false;
    }
    
    // 设置目标到 Ability 实例
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(FollowAbilityHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Follow* FollowAbility = Cast<UGA_Follow>(Instance))
        {
            FollowAbility->SetTargetCharacter(TargetCharacter);
            FollowAbility->FollowDistance = FollowDistance;
        }
    }
    
    return TryActivateAbility(FollowAbilityHandle);
}

void UMAAbilitySystemComponent::CancelFollow()
{
    if (FollowAbilityHandle.IsValid())
    {
        CancelAbilityHandle(FollowAbilityHandle);
    }
}

// ========== 新增 Robot Abilities ==========
// 注意: TryActivatePatrol/TryActivatePatrolPath/CancelPatrol 已移除
// Patrol 功能现在由 StateTree 的 MASTTask_Patrol 实现

bool UMAAbilitySystemComponent::TryActivateSearch()
{
    if (!SearchAbilityHandle.IsValid())
    {
        return false;
    }
    
    return TryActivateAbility(SearchAbilityHandle);
}

void UMAAbilitySystemComponent::CancelSearch()
{
    if (SearchAbilityHandle.IsValid())
    {
        CancelAbilityHandle(SearchAbilityHandle);
    }
}

bool UMAAbilitySystemComponent::TryActivateObserve(AActor* Target)
{
    CancelObserve();
    
    if (!ObserveAbilityHandle.IsValid())
    {
        return false;
    }
    
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(ObserveAbilityHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Observe* ObserveAbility = Cast<UGA_Observe>(Instance))
        {
            ObserveAbility->SetObserveTarget(Target);
        }
    }
    
    return TryActivateAbility(ObserveAbilityHandle);
}

void UMAAbilitySystemComponent::CancelObserve()
{
    if (ObserveAbilityHandle.IsValid())
    {
        CancelAbilityHandle(ObserveAbilityHandle);
    }
}

bool UMAAbilitySystemComponent::TryActivateReport(const FString& Message)
{
    if (!ReportAbilityHandle.IsValid())
    {
        return false;
    }
    
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(ReportAbilityHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Report* ReportAbility = Cast<UGA_Report>(Instance))
        {
            ReportAbility->SetReportMessage(Message);
        }
    }
    
    return TryActivateAbility(ReportAbilityHandle);
}

bool UMAAbilitySystemComponent::TryActivateCharge()
{
    if (!ChargeAbilityHandle.IsValid())
    {
        return false;
    }
    
    return TryActivateAbility(ChargeAbilityHandle);
}

bool UMAAbilitySystemComponent::TryActivateFormation(AMACharacter* Leader, EFormationType Type, int32 Position, int32 TotalCount)
{
    CancelFormation();
    
    if (!FormationAbilityHandle.IsValid() || !Leader)
    {
        return false;
    }
    
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(FormationAbilityHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Formation* FormationAbility = Cast<UGA_Formation>(Instance))
        {
            FormationAbility->SetFormation(Leader, Type, Position, TotalCount);
        }
    }
    
    return TryActivateAbility(FormationAbilityHandle);
}

void UMAAbilitySystemComponent::CancelFormation()
{
    if (FormationAbilityHandle.IsValid())
    {
        CancelAbilityHandle(FormationAbilityHandle);
    }
}

bool UMAAbilitySystemComponent::TryActivateAvoid(FVector Destination)
{
    CancelAvoid();
    
    if (!AvoidAbilityHandle.IsValid())
    {
        return false;
    }
    
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AvoidAbilityHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (UGA_Avoid* AvoidAbility = Cast<UGA_Avoid>(Instance))
        {
            AvoidAbility->SetDestination(Destination);
        }
    }
    
    return TryActivateAbility(AvoidAbilityHandle);
}

void UMAAbilitySystemComponent::CancelAvoid()
{
    if (AvoidAbilityHandle.IsValid())
    {
        CancelAbilityHandle(AvoidAbilityHandle);
    }
}

// ========== 命令系统 ==========

void UMAAbilitySystemComponent::SendCommand(FGameplayTag CommandTag)
{
    if (!CommandTag.IsValid())
    {
        return;
    }
    
    // 先清除所有其他命令 (一次只能有一个命令)
    ClearAllCommands();
    
    // 添加新命令 Tag
    AddLooseGameplayTag(CommandTag);
    
    UE_LOG(LogTemp, Log, TEXT("[ASC] Command sent: %s"), *CommandTag.ToString());
}

void UMAAbilitySystemComponent::ClearCommand(FGameplayTag CommandTag)
{
    if (!CommandTag.IsValid())
    {
        return;
    }
    
    RemoveLooseGameplayTag(CommandTag);
    
    UE_LOG(LogTemp, Log, TEXT("[ASC] Command cleared: %s"), *CommandTag.ToString());
}

void UMAAbilitySystemComponent::ClearAllCommands()
{
    // 清除所有 Command.* 开头的 Tag
    FGameplayTagContainer OwnedTags;
    GetOwnedGameplayTags(OwnedTags);
    
    for (const FGameplayTag& Tag : OwnedTags)
    {
        if (Tag.ToString().StartsWith(TEXT("Command.")))
        {
            RemoveLooseGameplayTag(Tag);
        }
    }
}
