#include "MAHumanAgent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
        
        // 设置动画蓝图 - 使用 AnimBlueprintGeneratedClass
        static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(
            TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny"));
        if (AnimBP.Succeeded())
        {
            GetMesh()->SetAnimInstanceClass(AnimBP.Class);
        }
    }
}
