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

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 160.0f, 0.0f);  // 降低自转速度（原 640）
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 150.f;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;

    // 司能组件 (ASC)
    AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* AMAAgent::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMAAgent::BeginPlay()
{
    Super::BeginPlay();
    FMAGameplayTags::InitializeNativeTags();
}

void AMAAgent::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        AbilitySystemComponent->InitializeAbilities(this);
    }
}


void AMAAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 导航期间调用虚函数，子类可重写
    if (bIsMoving)
    {
        OnNavigationTick();
    }
    
    // 绘制头顶状态文本
    DrawStatusText();
}

void AMAAgent::OnNavigationTick()
{
    // 基类默认空实现，子类可重写
}

// ========== 司能 (GAS ASC Abilities) ==========

bool AMAAgent::TryPickup()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivatePickup();
    }
    return false;
}

bool AMAAgent::TryDrop()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateDrop();
    }
    return false;
}

bool AMAAgent::TryNavigateTo(FVector Destination)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateNavigate(Destination);
    }
    return false;
}

void AMAAgent::CancelNavigation()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelNavigate();
    }
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
    return GetHeldItem() != nullptr;
}

bool AMAAgent::TryFollowAgent(AMAAgent* TargetAgent, float FollowDistance)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateFollow(TargetAgent, FollowDistance);
    }
    return false;
}

void AMAAgent::StopFollowing()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelFollow();
    }
}

// ========== 头顶状态显示 ==========

void AMAAgent::ShowStatus(const FString& Text, float Duration)
{
    if (!bShowStatusAboveHead) return;
    
    CurrentStatusText = Text;
    StatusDisplayEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

void AMAAgent::ShowAbilityStatus(const FString& AbilityName, const FString& Params)
{
    FString DisplayText;
    if (Params.IsEmpty())
    {
        DisplayText = FString::Printf(TEXT("[%s]"), *AbilityName);
    }
    else
    {
        DisplayText = FString::Printf(TEXT("[%s] %s"), *AbilityName, *Params);
    }
    
    ShowStatus(DisplayText, 3.0f);
}

void AMAAgent::DrawStatusText()
{
    if (!bShowStatusAboveHead) return;
    if (CurrentStatusText.IsEmpty()) return;
    
    // 检查是否过期
    if (GetWorld()->GetTimeSeconds() > StatusDisplayEndTime)
    {
        CurrentStatusText = TEXT("");
        return;
    }
    
    // 在头顶上方绘制文本
    FVector TextLocation = GetActorLocation() + FVector(0.f, 0.f, 150.f);
    
    // 使用 DrawDebugString 绘制 3D 文本
    DrawDebugString(
        GetWorld(),
        TextLocation,
        CurrentStatusText,
        nullptr,
        FColor::Yellow,
        0.f,  // Duration = 0 表示只绘制一帧
        true, // bDrawShadow
        1.2f  // FontScale
    );
}
