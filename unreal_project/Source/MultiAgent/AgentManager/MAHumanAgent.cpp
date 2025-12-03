#include "MAHumanAgent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"

AMAHumanAgent::AMAHumanAgent()
{
    AgentType = EAgentType::Human;
    
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    
    // 设置骨骼网格 (SKM_Manny)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    }
    
    // 设置动画蓝图 (ABP_Manny)
    static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPClass(
        TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny"));
    if (AnimBPClass.Succeeded())
    {
        GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);
    }
    
    // 移动组件配置
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 400.f;
    GetCharacterMovement()->MaxAcceleration = 2048.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2048.f;
}

void AMAHumanAgent::BeginPlay()
{
    Super::BeginPlay();
}
