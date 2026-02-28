// MAUGVCharacter.cpp
// 无人车实现
//
// ============================================================================
// 重要提示：StaticMesh 碰撞与 NavMesh 导航
// ============================================================================
// 如果你的 Character 使用 StaticMeshComponent 来显示模型（而不是 SkeletalMesh），
// 必须禁用 StaticMesh 的碰撞，否则会导致 NavMesh 路径查询失败！
//
// 原因：
// - ACharacter 使用 CapsuleComponent 作为碰撞体和 NavMesh 查询的依据
// - 如果 StaticMeshComponent 启用了碰撞，它会与 CapsuleComponent 产生冲突
// - NavMesh 系统可能会把角色判定为"在障碍物内部"，导致 MoveToLocation 返回 Failed
// - SkeletalMesh 在基类 MACharacter 中已经设置了 SetCollisionEnabled(NoCollision)
//
// 解决方案：
// UGVMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//
// 症状：
// - MoveToLocation 返回 0 (Failed)
// - PathStatus=0, HasValidPath=false
// - 其他使用 SkeletalMesh 的角色（如 Humanoid）导航正常
// ============================================================================

#include "MAUGVCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../Skill/MASkillComponent.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "../../Environment/IMAPickupItem.h"
#include "StateTree.h"

AMAUGVCharacter::AMAUGVCharacter()
{
    AgentType = EMAAgentType::UGV;
    
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(false);
    
    // 地面移动设置
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->MaxWalkSpeed = 500.f;
    MovementComp->bOrientRotationToMovement = true;
    MovementComp->RotationRate = FRotator(0.f, 120.f, 0.f);
    
    // 加载 UGV 模型
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(
        TEXT("/Game/Robot/ugv1/D-21-dark1.D-21-dark1"));
    if (MeshAsset.Succeeded())
    {
        UGVMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UGVMesh"));
        UGVMeshComponent->SetupAttachment(RootComponent);
        UGVMeshComponent->SetStaticMesh(MeshAsset.Object);
        
        // 禁用 StaticMesh 的碰撞！
        // 如果不禁用，StaticMesh 的碰撞会干扰 NavMesh 路径查询，
        // 导致 AIController::MoveToLocation() 返回 Failed (0)。
        // 碰撞应该由 CapsuleComponent 统一处理，StaticMesh 仅用于视觉显示。
        // 详见文件顶部的注释说明。
        UGVMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAUGVCharacter] Failed to load UGV mesh!"));
    }
}

void AMAUGVCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // 根据 StaticMesh 自动调整胶囊体和 Mesh 位置（与 MACharacter::AutoFitCapsuleToMesh 类似）
    AutoFitCapsuleToStaticMesh();
    
    if (SkillComponent)
    {
        SkillComponent->OnEnergyDepleted.AddDynamic(this, &AMAUGVCharacter::OnEnergyDepleted);
        SkillComponent->EnergyDrainRate = 0.8f;
        SkillComponent->MaxEnergy = 200.f;
        SkillComponent->Energy = 200.f;
    }
    
    // 加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled() && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_UGV.ST_UGV"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
            UE_LOG(LogTemp, Log, TEXT("[UGV %s] StateTree loaded: ST_UGV"), *AgentID);
        }
    }
    else if (StateTreeComponent && !StateTreeComponent->IsStateTreeEnabled())
    {
        UE_LOG(LogTemp, Log, TEXT("[UGV %s] StateTree disabled, using direct skill activation"), *AgentID);
    }
}

void AMAUGVCharacter::InitializeSkillSet()
{
    // UGV 技能: Navigate, Carry
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Carry);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAUGVCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 移动时消耗能量（载货时消耗更多）
    if (bIsMoving && ShouldDrainEnergy() && SkillComponent && SkillComponent->HasEnergy())
    {
        float DrainMultiplier = 1.f + (CurrentCargoWeight / MaxCargoWeight) * 0.5f;
        SkillComponent->DrainEnergy(DeltaTime * DrainMultiplier);
    }
}

void AMAUGVCharacter::OnEnergyDepleted()
{
    CancelNavigation();
}

bool AMAUGVCharacter::LoadCargo(AActor* Item)
{
    if (!Item) return false;
    if (CarriedItems.Contains(Item)) return false;
    
    CarriedItems.Add(Item);
    Item->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    
    // 计算货物放置高度
    // 
    // 坐标系说明：
    // - GetActorLocation() 返回胶囊体中心位置（Actor 原点）
    // - SetActorRelativeLocation() 设置相对于 Actor 原点的偏移
    // - CargoDeckHeight 是甲板相对于 Actor 原点的高度（可为负值）
    // - BottomOffset 是货物底部相对于货物原点的偏移（通常为负值）
    //
    // 最终高度 = CargoDeckHeight - BottomOffset
    // 例如：CargoDeckHeight=-20, BottomOffset=-50 → 高度=-20-(-50)=30
    
    float CargoHeight = CargoDeckHeight;
    if (IMAPickupItem* PickupItem = Cast<IMAPickupItem>(Item))
    {
        CargoHeight = CargoDeckHeight - PickupItem->GetBottomOffset();
    }
    
    Item->SetActorRelativeLocation(FVector(0.f, 0.f, CargoHeight));
    Item->SetActorRelativeRotation(FRotator::ZeroRotator);
    
    return true;
}

bool AMAUGVCharacter::UnloadCargo(AActor* Item)
{
    if (!Item) return false;
    if (!CarriedItems.Contains(Item)) return false;
    
    CarriedItems.Remove(Item);
    
    // 分离并放置在地面
    Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 150.f;
    Item->SetActorLocation(DropLocation);
    
    return true;
}

void AMAUGVCharacter::UnloadAllCargo()
{
    TArray<AActor*> ItemsToUnload = CarriedItems;
    for (AActor* Item : ItemsToUnload)
    {
        UnloadCargo(Item);
    }
}

void AMAUGVCharacter::AutoFitCapsuleToStaticMesh()
{
    if (!UGVMeshComponent || !UGVMeshComponent->GetStaticMesh()) return;
    
    FBoxSphereBounds LocalBounds = UGVMeshComponent->GetStaticMesh()->GetBounds();
    float ExtentX = LocalBounds.BoxExtent.X;
    float ExtentY = LocalBounds.BoxExtent.Y;
    float ExtentZ = LocalBounds.BoxExtent.Z;
    
    // 胶囊体尺寸：半径取 XY 平均值的 80%，半高取 Z 的 85%
    // 注意：如果模型有天线等突出物，ExtentZ 会偏大，胶囊体会偏高
    // 但这通常是可接受的，因为胶囊体主要用于碰撞检测
    float Radius = (ExtentX + ExtentY) * 0.5f * 0.8f;
    float HalfHeight = ExtentZ * 0.85f;
    
    Radius = FMath::Max(Radius, 10.f);
    HalfHeight = FMath::Max(HalfHeight, Radius);
    
    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
    
    // Mesh 底部相对于 Mesh 原点的 Z 偏移
    float MeshBottomZ = LocalBounds.Origin.Z - ExtentZ;
    // Mesh 需要向下偏移，使其底部与胶囊体底部对齐
    float MeshOffsetZ = -HalfHeight - MeshBottomZ;
    
    UGVMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, MeshOffsetZ));
    
    // CargoDeckHeight 不自动计算，保持手动配置值
    // 原因：GetBounds() 返回整个模型的包围盒（包括天线等突出物），
    // 无法准确反映实际的货物放置面位置
    
    UE_LOG(LogTemp, Log, TEXT("[UGV] AutoFit: Capsule(R=%.1f, H=%.1f), MeshOffsetZ=%.1f, CargoDeckHeight=%.1f (manual)"),
        Radius, HalfHeight, MeshOffsetZ, CargoDeckHeight);
}
