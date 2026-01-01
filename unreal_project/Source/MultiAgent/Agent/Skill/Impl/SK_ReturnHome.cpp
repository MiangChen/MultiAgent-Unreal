// SK_ReturnHome.cpp

#include "SK_ReturnHome.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "TimerManager.h"

USK_ReturnHome::USK_ReturnHome()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
    bReturnHomeSucceeded = false;
    ReturnHomeResultMessage = TEXT("");
}

void USK_ReturnHome::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    bIsLanding = false;
    
    // 重置结果状态
    bReturnHomeSucceeded = false;
    ReturnHomeResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_ReturnHome] Character is null!"));
        bReturnHomeSucceeded = false;
        ReturnHomeResultMessage = TEXT("ReturnHome failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_ReturnHome] SkillComponent is null!"));
        bReturnHomeSucceeded = false;
        ReturnHomeResultMessage = TEXT("ReturnHome failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    const FMASkillParams& Params = SkillComp->GetSkillParams();
    HomeLocation = Params.HomeLocation;
    LandHeight = Params.LandHeight;
    
    // 保持当前高度飞回 HomeLocation
    FVector CurrentLocation = Character->GetActorLocation();
    HomeLocation.Z = CurrentLocation.Z;
    
    Character->ShowAbilityStatus(TEXT("ReturnHome"), TEXT("Flying home..."));
    Character->bIsMoving = true;
    
    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USK_ReturnHome::UpdateReturnHome, 0.05f, true);
    }
}

void USK_ReturnHome::UpdateReturnHome()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bReturnHomeSucceeded = false;
        ReturnHomeResultMessage = TEXT("ReturnHome failed: Character lost during flight");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    FVector CurrentLocation = Character->GetActorLocation();
    float Distance2D = FVector::Dist2D(CurrentLocation, HomeLocation);
    
    if (Distance2D < 50.f)
    {
        // 到达 HomeLocation 上空，开始降落
        StartLanding();
        return;
    }
    
    // 向 HomeLocation 移动
    FVector Direction = (HomeLocation - CurrentLocation).GetSafeNormal2D();
    float DeltaTime = 0.05f;
    FVector NewLocation = CurrentLocation + Direction * FlySpeed * DeltaTime;
    NewLocation.Z = HomeLocation.Z;
    Character->SetActorLocation(NewLocation);
    
    // 平滑朝向移动方向
    if (!Direction.IsNearlyZero())
    {
        FRotator CurrentRotation = Character->GetActorRotation();
        FRotator TargetRotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f);
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);
        Character->SetActorRotation(NewRotation);
    }
}

void USK_ReturnHome::StartLanding()
{
    bIsLanding = true;
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("ReturnHome"), TEXT("Landing..."));
        
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
            World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USK_ReturnHome::UpdateLanding, 0.05f, true);
        }
    }
}

void USK_ReturnHome::UpdateLanding()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bReturnHomeSucceeded = false;
        ReturnHomeResultMessage = TEXT("ReturnHome failed: Character lost during landing");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    FVector CurrentLocation = Character->GetActorLocation();
    float DistanceToGround = CurrentLocation.Z - LandHeight;
    
    if (DistanceToGround < 10.f)
    {
        // 降落完成
        FVector FinalLocation = CurrentLocation;
        FinalLocation.Z = LandHeight;
        Character->SetActorLocation(FinalLocation);
        
        bReturnHomeSucceeded = true;
        ReturnHomeResultMessage = FString::Printf(TEXT("ReturnHome succeeded: Landed at (%.0f, %.0f, %.0f)"), 
            HomeLocation.X, HomeLocation.Y, LandHeight);
        Character->ShowAbilityStatus(TEXT("ReturnHome"), TEXT("Complete!"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
        return;
    }
    
    // 向下移动
    FVector NewLocation = CurrentLocation;
    NewLocation.Z = FMath::Max(NewLocation.Z - LandSpeed * 0.05f, LandHeight);
    Character->SetActorLocation(NewLocation);
}

void USK_ReturnHome::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bReturnHomeSucceeded;
    FString MessageToNotify = ReturnHomeResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
        UpdateTimerHandle.Invalidate();
        
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && ReturnHomeResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FVector CurrentLocation = Character->GetActorLocation();
            if (bIsLanding)
            {
                MessageToNotify = FString::Printf(TEXT("ReturnHome cancelled: Stopped during landing at (%.0f, %.0f, %.0f)"), 
                    CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
            }
            else
            {
                float Distance2D = FVector::Dist2D(CurrentLocation, HomeLocation);
                MessageToNotify = FString::Printf(TEXT("ReturnHome cancelled: Stopped %.0fm from home at (%.0f, %.0f, %.0f)"), 
                    Distance2D, CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
            }
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag ReturnHomeTag = FGameplayTag::RequestGameplayTag(FName("Command.ReturnHome"));
            if (SkillComp->HasMatchingGameplayTag(ReturnHomeTag))
            {
                SkillComp->RemoveLooseGameplayTag(ReturnHomeTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
