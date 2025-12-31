// MAUGVCharacter.cpp
// 无人车实现

#include "MAUGVCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../Skill/MASkillComponent.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "StateTree.h"

AMAUGVCharacter::AMAUGVCharacter()
{
    AgentType = EMAAgentType::UGV;
    
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // 地面移动设置
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->SetMovementMode(MOVE_Walking);
    MovementComp->MaxWalkSpeed = 300.f;
    MovementComp->bOrientRotationToMovement = true;
    MovementComp->RotationRate = FRotator(0.f, 120.f, 0.f);
    // MovementComp->GravityScale = 1.f;
    // MovementComp->bConstrainToPlane = false;
    // MovementComp->bSnapToPlaneAtStart = false;
    
    // 加载 UGV 模型
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(
        TEXT("/Game/Robot/ugv1/D-21-dark1.D-21-dark1"));
    if (MeshAsset.Succeeded())
    {
        // 创建静态网格组件来显示模型
        UGVMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UGVMesh"));
        UGVMeshComponent->SetupAttachment(RootComponent);
        UGVMeshComponent->SetStaticMesh(MeshAsset.Object);
        // 调整模型位置，使其底部与胶囊体底部对齐
        // 胶囊体半高默认 88cm，模型需要向下偏移
        UGVMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
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
    
    // TODO: 检查重量限制
    CarriedItems.Add(Item);
    
    // 附着到 UGV
    Item->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    
    // 计算物体放置高度：UGV 顶部甲板
    // UGV 模型向下偏移了 88cm（与胶囊体底部对齐）
    // UGV 模型高度大约 50-60cm，所以顶部甲板大约在 -88 + 50 = -38cm
    // 再加上物体自身的半高（假设 25cm 的方块）
    // 最终高度约 -38 + 25 = -13cm，取整为 -10cm
    // 
    // 但实际上我们需要根据物体的边界来计算
    float CargoHeight = -30.f;  // UGV 顶部甲板高度（相对于角色原点）
    
    // 如果物体有边界信息，加上物体的半高
    if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Item->GetRootComponent()))
    {
        FVector BoundsExtent = PrimComp->Bounds.BoxExtent;
        CargoHeight += BoundsExtent.Z;  // 加上物体半高，使物体底部贴在甲板上
    }
    
    Item->SetActorRelativeLocation(FVector(0.f, 0.f, CargoHeight));
    
    UE_LOG(LogTemp, Log, TEXT("[UGV %s] Loaded cargo at relative Z=%.1f"), *AgentName, CargoHeight);
    
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
