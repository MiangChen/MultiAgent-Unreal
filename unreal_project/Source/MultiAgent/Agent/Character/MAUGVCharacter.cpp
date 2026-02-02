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
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // 地面移动设置
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->MaxWalkSpeed = 500.f;
    MovementComp->bOrientRotationToMovement = true;
    MovementComp->RotationRate = FRotator(0.f, 120.f, 0.f);
    
    // UGV 胶囊体设置 - 适配小车尺寸
    GetCapsuleComponent()->SetCapsuleSize(50.f, 30.f);  // 半径50，半高50
    
    // 加载 UGV 模型
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(
        TEXT("/Game/Robot/ugv1/D-21-dark1.D-21-dark1"));
    if (MeshAsset.Succeeded())
    {
        // 创建静态网格组件来显示模型
        UGVMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UGVMesh"));
        UGVMeshComponent->SetupAttachment(RootComponent);
        UGVMeshComponent->SetStaticMesh(MeshAsset.Object);
        
        // 禁用 StaticMesh 的碰撞！
        // 如果不禁用，StaticMesh 的碰撞会干扰 NavMesh 路径查询，
        // 导致 AIController::MoveToLocation() 返回 Failed (0)。
        // 碰撞应该由 CapsuleComponent 统一处理，StaticMesh 仅用于视觉显示。
        // 详见文件顶部的注释说明。
        UGVMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        
        // 调整模型位置，使其底部与胶囊体底部对齐
        UGVMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
        UGVMeshComponent->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAUGVCharacter] Failed to load UGV mesh!"));
    }
}

void AMAUGVCharacter::BeginPlay()
{
    Super::BeginPlay();
    
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

void AMAUGVCharacter::SnapToGround()
{
    FVector CurrentLocation = GetActorLocation();
    FVector Start = CurrentLocation + FVector(0.f, 0.f, 1000.f);  // 从更高处开始
    FVector End = Start - FVector(0.f, 0.f, 5000.f);  // 向下检测
    
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params))
    {
        // 获取胶囊体半高，确保角色站在地面上
        float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        FVector NewLocation = FVector(CurrentLocation.X, CurrentLocation.Y, HitResult.Location.Z + CapsuleHalfHeight);
        SetActorLocation(NewLocation);
        UE_LOG(LogTemp, Log, TEXT("[UGV %s] Snapped to ground at Z=%.1f"), *AgentID, NewLocation.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGV %s] SnapToGround failed - no ground detected"), *AgentID);
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
    
    // 附着到 UGV
    Item->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    
    // 计算 UGV 顶部甲板高度（相对于角色原点）
    // 使用运行时世界边界，更准确
    float DeckHeight = 0.f;
    if (UGVMeshComponent)
    {
        // 获取 UGV 模型的世界边界
        FVector MeshWorldLocation = UGVMeshComponent->GetComponentLocation();
        FVector ActorLocation = GetActorLocation();
        FBoxSphereBounds WorldBounds = UGVMeshComponent->Bounds;
        
        // 甲板高度 = 模型世界顶部 - 角色世界位置
        float MeshTopZ = WorldBounds.Origin.Z + WorldBounds.BoxExtent.Z;
        DeckHeight = MeshTopZ - ActorLocation.Z;
        
        UE_LOG(LogTemp, Log, TEXT("[UGV %s] Bounds: Origin.Z=%.1f, Extent.Z=%.1f, ActorZ=%.1f, DeckHeight=%.1f"), 
            *AgentLabel, WorldBounds.Origin.Z, WorldBounds.BoxExtent.Z, ActorLocation.Z, DeckHeight);
    }
    else
    {
        // 回退：使用胶囊体顶部
        DeckHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }
    
    float CargoHeight = DeckHeight;
    
    // 使用 IMAPickupItem 接口获取精确的底部偏移
    if (IMAPickupItem* PickupItem = Cast<IMAPickupItem>(Item))
    {
        float BottomOffset = PickupItem->GetBottomOffset();
        CargoHeight = DeckHeight - BottomOffset;
        
        UE_LOG(LogTemp, Log, TEXT("[UGV %s] LoadCargo: %s, DeckHeight=%.1f, BottomOffset=%.1f, CargoHeight=%.1f"), 
            *AgentLabel, *PickupItem->GetItemName(), DeckHeight, BottomOffset, CargoHeight);
    }
    else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Item->GetRootComponent()))
    {
        FVector BoundsExtent = PrimComp->Bounds.BoxExtent;
        CargoHeight = DeckHeight + BoundsExtent.Z;
    }
    
    // 设置相对位置，并摆正姿态
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
