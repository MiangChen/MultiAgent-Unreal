#include "MAAgent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../GAS/MAGameplayTags.h"
#include "../Interaction/MAPickupItem.h"

AMAAgent::AMAAgent()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AgentID = 0;
    AgentName = TEXT("Agent");
    AgentType = EAgentType::Human;
    bIsMoving = false;
    TargetLocation = FVector::ZeroVector;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // AIController 配置 - NavMesh 导航必需
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // 角色旋转配置
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 移动组件配置
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    // RVO 避障
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 150.f;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;

    // 创建 GAS 组件
    AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* AMAAgent::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMAAgent::BeginPlay()
{
    Super::BeginPlay();

    // 初始化 Gameplay Tags
    FMAGameplayTags::InitializeNativeTags();
}

void AMAAgent::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 初始化 GAS
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        AbilitySystemComponent->InitializeAbilities(this);
    }
}

void AMAAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 为动画蓝图提供加速度输入
    UpdateAnimation();
    
    // 检查是否到达目标
    if (bIsMoving)
    {
        float Distance = FVector::Dist2D(GetActorLocation(), TargetLocation);
        if (Distance < 100.f)
        {
            bIsMoving = false;
            StopMovement();
        }
    }
}

void AMAAgent::MoveToLocation(FVector Destination)
{
    TargetLocation = Destination;
    bIsMoving = true;
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->MoveToLocation(Destination);
    }
}

void AMAAgent::UpdateAnimation()
{
    // 只有在移动状态时才添加输入向量
    if (!bIsMoving)
    {
        return;
    }
    
    FVector Velocity = GetCharacterMovement()->Velocity;
    float Speed = Velocity.Size2D();
    
    if (Speed > 3.0f)
    {
        FVector AccelDir = Velocity.GetSafeNormal2D();
        GetCharacterMovement()->AddInputVector(AccelDir);
    }
}

void AMAAgent::StopMovement()
{
    bIsMoving = false;
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }
    
    GetCharacterMovement()->StopMovementImmediately();
}

FVector AMAAgent::GetCurrentLocation() const
{
    return GetActorLocation();
}

// ========== GAS Abilities ==========

bool AMAAgent::TryPickup()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateAbilityByTag(FMAGameplayTags::Get().Ability_Pickup);
    }
    return false;
}

bool AMAAgent::TryDrop()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateAbilityByTag(FMAGameplayTags::Get().Ability_Drop);
    }
    return false;
}

AMAPickupItem* AMAAgent::GetHeldItem() const
{
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);

    for (AActor* Actor : AttachedActors)
    {
        if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
        {
            return Item;
        }
    }
    return nullptr;
}

bool AMAAgent::IsHoldingItem() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->HasGameplayTagFromContainer(FMAGameplayTags::Get().Status_Holding);
    }
    return GetHeldItem() != nullptr;
}
