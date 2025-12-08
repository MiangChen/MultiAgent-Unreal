// MADroneCharacter.cpp

#include "MADroneCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "StateTree.h"
#include "MAAbilitySystemComponent.h"
#include "MAGameplayTags.h"
#include "MAStateTreeComponent.h"
#include "AIController.h"

AMADroneCharacter::AMADroneCharacter()
{
    AgentType = EMAAgentType::Drone;
    
    // Energy defaults (无人机耗电较慢)
    Energy = 100.f;
    MaxEnergy = 100.f;
    EnergyDrainRate = 0.5f;  // 每秒消耗 0.5%
    
    // 创建 StateTree 组件
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // 配置移动组件 - 支持飞行
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->MaxWalkSpeed = 600.f;
    MovementComp->MaxAcceleration = 1024.f;
    
    // 启用飞行模式
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->DefaultLandMovementMode = MOVE_Flying;
    MovementComp->BrakingDecelerationFlying = 1000.f;
    MovementComp->MaxFlySpeed = 600.f;
    
    // 禁用重力 (无人机悬停)
    MovementComp->GravityScale = 0.f;
    
    // 子类调用 SetupDroneAssets() 设置 Mesh/Anim/Capsule
}

void AMADroneCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] ========== Drone BeginPlay START =========="), *AgentName);
    
    // 检查 StateTreeComponent
    if (!StateTreeComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] StateTreeComponent is NULL!"), *AgentName);
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] StateTreeComponent exists"), *AgentName);
    UE_LOG(LogTemp, Warning, TEXT("[%s] HasStateTree: %s"), *AgentName, StateTreeComponent->HasStateTree() ? TEXT("YES") : TEXT("NO"));
    
    // 动态加载 StateTree Asset
    if (!StateTreeComponent->HasStateTree())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Loading StateTree from /Game/StateTree/ST_Drone..."), *AgentName);
        
        UStateTree* ST = LoadObject<UStateTree>(nullptr, 
            TEXT("/Game/StateTree/ST_Drone.ST_Drone"));
        
        if (ST)
        {
            UE_LOG(LogTemp, Warning, TEXT("[%s] StateTree loaded successfully: %s"), *AgentName, *ST->GetName());
            StateTreeComponent->SetStateTreeAsset(ST);
            
            UE_LOG(LogTemp, Warning, TEXT("[%s] After SetStateTreeAsset - HasStateTree: %s"), 
                *AgentName, StateTreeComponent->HasStateTree() ? TEXT("YES") : TEXT("NO"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[%s] Failed to load StateTree from /Game/StateTree/ST_Drone.ST_Drone"), *AgentName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] StateTree already set"), *AgentName);
    }
    
    // 检查 AIController
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] AIController: %s"), *AgentName, *AIC->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] No AIController (may be spawned later)"), *AgentName);
    }
    
    // 设置初始飞行高度
    FVector CurrentLocation = GetActorLocation();
    if (CurrentLocation.Z < FlightAltitude)
    {
        SetActorLocation(FVector(CurrentLocation.X, CurrentLocation.Y, FlightAltitude));
    }
    
    bIsHovering = true;
    bIsFlying = true;
    
    // 确保动画在 BeginPlay 时开始播放
    if (PropellerAnim)
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->SetAnimation(PropellerAnim);
        GetMesh()->SetPlayRate(1.0f);
        GetMesh()->Play(true);
        UE_LOG(LogTemp, Warning, TEXT("[%s] PropellerAnim started in BeginPlay"), *AgentName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] PropellerAnim is NULL in BeginPlay!"), *AgentName);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] ========== Drone BeginPlay END =========="), *AgentName);
}

void AMADroneCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 螺旋桨动画持续播放
    UpdatePropellerAnimation();
    
    // 飞行时消耗能量
    if (bIsFlying && HasEnergy())
    {
        DrainEnergy(DeltaTime);
    }
    
    // 更新能量显示和状态
    UpdateEnergyDisplay();
    CheckLowEnergyStatus();
}

void AMADroneCharacter::UpdatePropellerAnimation()
{
    // 螺旋桨动画始终播放（无论静止还是运动）
    if (PropellerAnim)
    {
        if (!GetMesh()->IsPlaying())
        {
            GetMesh()->SetAnimation(PropellerAnim);
            GetMesh()->Play(true);
        }
    }
}

// ========== Energy System ==========

void AMADroneCharacter::DrainEnergy(float DeltaTime)
{
    Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);
    
    // 能量耗尽时强制降落
    if (Energy <= 0.f)
    {
        Land();
        UE_LOG(LogTemp, Warning, TEXT("[%s] Energy depleted! Emergency landing."), *AgentName);
    }
}

void AMADroneCharacter::RestoreEnergy(float Amount)
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
    UE_LOG(LogTemp, Warning, TEXT("[%s] Energy restored: %.1f -> %.1f (Amount: %.1f)"), 
        *AgentName, OldEnergy, Energy, Amount);
}

void AMADroneCharacter::UpdateEnergyDisplay()
{
    FVector TextLocation = GetActorLocation() + FVector(0.f, 0.f, 80.f);
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
    
    DrawDebugString(
        GetWorld(),
        TextLocation,
        EnergyText,
        nullptr,
        DisplayColor,
        0.f,
        true,
        1.0f
    );
}

void AMADroneCharacter::CheckLowEnergyStatus()
{
    if (!AbilitySystemComponent) return;
    
    const FMAGameplayTags& Tags = FMAGameplayTags::Get();
    bool bHasLowEnergyTag = AbilitySystemComponent->HasGameplayTagFromContainer(Tags.Status_LowEnergy);
    
    if (Energy < LowEnergyThreshold && !bHasLowEnergyTag)
    {
        AbilitySystemComponent->AddLooseGameplayTag(Tags.Status_LowEnergy);
        UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy warning! %.0f%%"), *AgentName, GetEnergyPercent());
    }
    else if (Energy >= LowEnergyThreshold && bHasLowEnergyTag)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(Tags.Status_LowEnergy);
    }
}

// ========== Flight System ==========

void AMADroneCharacter::TakeOff(float TargetAltitude)
{
    if (TargetAltitude < 0.f)
    {
        TargetAltitude = FlightAltitude;
    }
    
    FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = FVector(CurrentLocation.X, CurrentLocation.Y, TargetAltitude);
    
    bIsFlying = true;
    bIsHovering = false;
    
    // 设置飞行模式
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    
    // 飞往目标高度
    FlyTo(TargetLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Taking off to altitude %.1f"), *AgentName, TargetAltitude);
}

void AMADroneCharacter::Land()
{
    FVector CurrentLocation = GetActorLocation();
    
    // 找到地面高度 (简单实现：假设地面在 Z=0)
    FVector GroundLocation = FVector(CurrentLocation.X, CurrentLocation.Y, 100.f);
    
    bIsHovering = false;
    
    // 飞往地面
    FlyTo(GroundLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Landing..."), *AgentName);
}

void AMADroneCharacter::Hover()
{
    // 停止当前移动
    GetCharacterMovement()->StopMovementImmediately();
    
    bIsHovering = true;
    bIsMoving = false;
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Hovering at %.1f"), *AgentName, GetActorLocation().Z);
}

bool AMADroneCharacter::FlyTo(FVector Destination)
{
    if (!HasEnergy())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Cannot fly - no energy!"), *AgentName);
        return false;
    }
    
    TargetFlightLocation = Destination;
    bIsHovering = false;
    bIsMoving = true;
    
    // 使用 AIController 进行导航
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC)
    {
        AIC->MoveToLocation(Destination, 50.f);  // 50 单位的接受半径
        UE_LOG(LogTemp, Warning, TEXT("[%s] Flying to (%.1f, %.1f, %.1f)"), 
            *AgentName, Destination.X, Destination.Y, Destination.Z);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] No AIController for navigation"), *AgentName);
    return false;
}

// ========== Drone Abilities ==========

bool AMADroneCharacter::TrySearch()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateSearch();
    }
    return false;
}

void AMADroneCharacter::StopSearch()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelSearch();
    }
}

bool AMADroneCharacter::TryCharge()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateCharge();
    }
    return false;
}
