#include "MAHumanAgent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"

AMAHumanAgent::AMAHumanAgent()
{
    AgentType = EAgentType::Human;
    
    // 设置胶囊体大小
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    
    // 设置 Mesh
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    }
    
    // 设置动画蓝图 - 使用 FClassFinder 加载 AnimInstance 类
    static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPClass(
        TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny"));
    if (AnimBPClass.Succeeded())
    {
        GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);
        UE_LOG(LogTemp, Log, TEXT("ABP_Manny loaded successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load ABP_Manny animation blueprint!"));
    }
    
    // 确保 CharacterMovement 配置正确，动画蓝图需要这些数据
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 400.f;
}
