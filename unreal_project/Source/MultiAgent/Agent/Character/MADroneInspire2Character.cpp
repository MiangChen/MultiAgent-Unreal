// MADroneInspire2Character.cpp

#include "MADroneInspire2Character.h"
#include "UObject/ConstructorHelpers.h"

AMADroneInspire2Character::AMADroneInspire2Character()
{
    AgentType = EMAAgentType::DroneInspire2;
    
    SetupDroneAssets();
}

void AMADroneInspire2Character::SetupDroneAssets()
{
    // CapsuleComponent 大小由基类 BeginPlay 中 AutoFitCapsuleToMesh() 自动计算
    
    // 加载 Inspire 2 模型
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/dji_inspire2/dji_inspire2.dji_inspire2"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -75.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));  // 修正模型朝向
        GetMesh()->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
    }

    // 加载螺旋桨动画
    static ConstructorHelpers::FObjectFinder<UAnimSequence> PropellerAnimAsset(
        TEXT("/Game/Robot/dji_inspire2/dji_inspire2_2Hovering.dji_inspire2_2Hovering"));
    if (PropellerAnimAsset.Succeeded())
    {
        PropellerAnim = PropellerAnimAsset.Object;
        UE_LOG(LogTemp, Log, TEXT("[DroneInspire2] PropellerAnim loaded: %s"), *PropellerAnim->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[DroneInspire2] Failed to load PropellerAnim!"));
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
