// MADronePhantom4Character.cpp

#include "MADronePhantom4Character.h"
#include "UObject/ConstructorHelpers.h"

AMADronePhantom4Character::AMADronePhantom4Character()
{
    AgentType = EMAAgentType::DronePhantom4;
    
    SetupDroneAssets();
}

void AMADronePhantom4Character::SetupDroneAssets()
{
    // CapsuleComponent 大小由基类 BeginPlay 中 AutoFitCapsuleToMesh() 自动计算
    
    // 加载 Phantom 4 模型
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/dji_phantom4/dji_phantom4.dji_phantom4"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -25.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
        GetMesh()->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
    }

    // 加载螺旋桨动画
    static ConstructorHelpers::FObjectFinder<UAnimSequence> PropellerAnimAsset(
        TEXT("/Game/Robot/dji_phantom4/dji_phantom4_Anim.dji_phantom4_Anim"));
    if (PropellerAnimAsset.Succeeded())
    {
        PropellerAnim = PropellerAnimAsset.Object;
        UE_LOG(LogTemp, Log, TEXT("[DronePhantom4] PropellerAnim loaded: %s"), *PropellerAnim->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[DronePhantom4] Failed to load PropellerAnim!"));
    }

    // 设置动画模式
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    if (PropellerAnim)
    {
        GetMesh()->SetAnimation(PropellerAnim);
        GetMesh()->SetPlayRate(1.0f);
        GetMesh()->Play(true);
    }
}
