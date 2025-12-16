// MAEnergyComponent.cpp

#include "MAEnergyComponent.h"
#include "../../Character/MACharacter.h"
#include "../../GAS/MAAbilitySystemComponent.h"
#include "../../GAS/MAGameplayTags.h"
#include "../../../UI/MAHUD.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

UMAEnergyComponent::UMAEnergyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;  // 每 0.1 秒 Tick 一次，节省性能
}

void UMAEnergyComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 缓存 ASC
    GetOwnerASC();
}

void UMAEnergyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (bShowEnergyDisplay)
    {
        UpdateEnergyDisplay();
    }
    
    CheckLowEnergyStatus();
}

// ========== IMAChargeable Interface ==========

void UMAEnergyComponent::RestoreEnergy(float Amount)
{
    float OldEnergy = Energy;
    
    if (Amount >= MaxEnergy)
    {
        Energy = MaxEnergy;
    }
    else
    {
        Energy = FMath::Min(MaxEnergy, Energy + Amount);
    }
    
    if (!FMath::IsNearlyEqual(OldEnergy, Energy))
    {
        OnEnergyChanged.Broadcast(Energy, MaxEnergy);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Energy restored: %.1f -> %.1f (Amount: %.1f)"), 
        *GetOwner()->GetName(), OldEnergy, Energy, Amount);
}

void UMAEnergyComponent::DrainEnergy(float DeltaTime)
{
    if (Energy <= 0.f)
    {
        return;
    }
    
    float OldEnergy = Energy;
    Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);
    
    if (!FMath::IsNearlyEqual(OldEnergy, Energy))
    {
        OnEnergyChanged.Broadcast(Energy, MaxEnergy);
    }
    
    // 能量耗尽
    if (Energy <= 0.f && OldEnergy > 0.f)
    {
        OnEnergyDepleted.Broadcast();
        UE_LOG(LogTemp, Warning, TEXT("[%s] Energy depleted!"), *GetOwner()->GetName());
    }
}

void UMAEnergyComponent::SetEnergy(float NewEnergy)
{
    float OldEnergy = Energy;
    Energy = FMath::Clamp(NewEnergy, 0.f, MaxEnergy);
    
    if (!FMath::IsNearlyEqual(OldEnergy, Energy))
    {
        OnEnergyChanged.Broadcast(Energy, MaxEnergy);
    }
}

// ========== 显示与状态 ==========

void UMAEnergyComponent::UpdateEnergyDisplay()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;
    
    // 当 MainUI 显示时，不绘制 Energy 文字（避免透过 UI 显示）
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD());
        if (HUD && HUD->IsMainUIVisible())
        {
            return;  // MainUI 显示时跳过绘制
        }
    }
    
    FVector TextLocation = Owner->GetActorLocation() + FVector(0.f, 0.f, 120.f);
    FString EnergyText = FString::Printf(TEXT("Energy: %.0f%%"), GetEnergyPercent());
    
    FColor DisplayColor = FColor::Green;
    if (Energy < LowEnergyThreshold)
    {
        DisplayColor = FColor::Red;
    }
    else if (Energy < 50.f)
    {
        DisplayColor = FColor::Yellow;
    }
    
    // 修复闪烁问题：
    // - bPersistent = false: 不持久化，每帧重新绘制
    // - Duration = 0.15f: 略大于 TickInterval (0.1f)，确保文字在下次更新前不会消失
    DrawDebugString(
        GetWorld(),
        TextLocation,
        EnergyText,
        nullptr,
        DisplayColor,
        0.15f,    // Duration: 略大于 TickInterval，防止闪烁
        false,    // bPersistent: false，每帧重新绘制而不是累积
        1.0f
    );
}

void UMAEnergyComponent::CheckLowEnergyStatus()
{
    UMAAbilitySystemComponent* ASC = GetOwnerASC();
    if (!ASC) return;
    
    const FMAGameplayTags& Tags = FMAGameplayTags::Get();
    bool bIsLowEnergy = Energy < LowEnergyThreshold;
    bool bHasLowEnergyTag = ASC->HasGameplayTagFromContainer(Tags.Status_LowEnergy);
    
    if (bIsLowEnergy && !bHasLowEnergyTag)
    {
        ASC->AddLooseGameplayTag(Tags.Status_LowEnergy);
        
        // 只在状态变化时广播
        if (!bWasLowEnergy)
        {
            OnLowEnergy.Broadcast();
            UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy warning! %.0f%%"), 
                *GetOwner()->GetName(), GetEnergyPercent());
        }
    }
    else if (!bIsLowEnergy && bHasLowEnergyTag)
    {
        ASC->RemoveLooseGameplayTag(Tags.Status_LowEnergy);
    }
    
    bWasLowEnergy = bIsLowEnergy;
}

UMAAbilitySystemComponent* UMAEnergyComponent::GetOwnerASC()
{
    if (CachedASC.IsValid())
    {
        return CachedASC.Get();
    }
    
    AMACharacter* Character = Cast<AMACharacter>(GetOwner());
    if (Character)
    {
        CachedASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        return CachedASC.Get();
    }
    
    return nullptr;
}
