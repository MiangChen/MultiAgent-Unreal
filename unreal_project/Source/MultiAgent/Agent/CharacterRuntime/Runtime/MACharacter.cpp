// MACharacter.cpp
// 机器人角色基类实现（构造/生命周期壳）

#include "MACharacter.h"

#include "Agent/CharacterRuntime/Infrastructure/MACharacterRuntimeBridge.h"
#include "Agent/Skill/Domain/MASkillTags.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"

AMACharacter::AMACharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    AgentID = TEXT("");
    AgentLabel = TEXT("Agent");
    AgentType = EMAAgentType::Humanoid;
    bIsMoving = false;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->MaxStepHeight = 100.f;

    SkillComponent = CreateDefaultSubobject<UMASkillComponent>(TEXT("SkillComponent"));
    NavigationService = CreateDefaultSubobject<UMANavigationService>(TEXT("NavigationService"));

    SpeechBubbleComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SpeechBubbleComponent"));
    SpeechBubbleComponent->SetupAttachment(GetRootComponent());
    FMACharacterRuntimeBridge::ConfigureSpeechBubbleComponent(*SpeechBubbleComponent);

    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UAbilitySystemComponent* AMACharacter::GetAbilitySystemComponent() const
{
    return SkillComponent;
}

void AMACharacter::BeginPlay()
{
    Super::BeginPlay();
    FMASkillTags::InitializeNativeTags();
    AutoFitCapsuleToMesh();
    InitializeSkillSet();
    InitializeSpeechBubble();
    InitialLocation = GetActorLocation();

    if (SkillComponent)
    {
        SkillComponent->OnLowEnergy.AddDynamic(this, &AMACharacter::OnLowEnergyReturn);
    }
}

void AMACharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (SkillComponent)
    {
        SkillComponent->InitAbilityActorInfo(this, this);
        SkillComponent->InitializeSkills(this);
    }
}

void AMACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateStatusText();
    UpdateSpeechBubbleFacing();
}

void AMACharacter::AutoFitCapsuleToMesh()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp || !MeshComp->GetSkeletalMeshAsset()) return;

    const FBoxSphereBounds LocalBounds = MeshComp->GetSkeletalMeshAsset()->GetBounds();
    const float ExtentX = LocalBounds.BoxExtent.X;
    const float ExtentY = LocalBounds.BoxExtent.Y;
    const float ExtentZ = LocalBounds.BoxExtent.Z;

    float Radius = (ExtentX + ExtentY) * 0.5f * 0.8f;
    float HalfHeight = ExtentZ * 0.85f;

    Radius = FMath::Max(Radius, 10.f);
    HalfHeight = FMath::Max(HalfHeight, Radius);

    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);

    const float MeshLocalBottomZ = LocalBounds.Origin.Z - ExtentZ;
    const float MeshOffsetZ = -HalfHeight - MeshLocalBottomZ;

    const FVector CurrentOffset = MeshComp->GetRelativeLocation();
    MeshComp->SetRelativeLocation(FVector(CurrentOffset.X, CurrentOffset.Y, MeshOffsetZ));
}
