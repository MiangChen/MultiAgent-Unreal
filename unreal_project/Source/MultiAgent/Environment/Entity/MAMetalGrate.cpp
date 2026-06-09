// MAMetalGrate.cpp
// 金属网格平板实现 - 静态承载平台，可被一个或多个机器人抬起或拉起

#include "MAMetalGrate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../../Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "../../Agent/CharacterRuntime/Runtime/MAHumanoidCharacter.h"
#include "../../Agent/CharacterRuntime/Runtime/MAUGVCharacter.h"
#include "../Utils/MAPlacementSurfaceUtils.h"
#include "UObject/ConstructorHelpers.h"

AMAMetalGrate::AMAMetalGrate()
{
    PrimaryActorTick.bCanEverTick = false;

    // MeshComponent 作为 Root，附着时整个平板会跟随移动
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComponent;

    // 默认资产路径（可在编辑器中通过 GrateMeshAsset / GrateMaterialAsset 替换）
    GrateMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Props/MetalGrate/MetalGrate.MetalGrate")));
    GrateMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Props/MetalGrate/M_MetalGrate.M_MetalGrate")));

    // 构造期同步加载默认 Mesh，使编辑器中可见
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Game/Props/MetalGrate/MetalGrate.MetalGrate"));
    if (DefaultMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(DefaultMesh.Object);
    }
    else
    {
        // Fallback：保证类总是可用，避免缺资产时 Spawn 出空体
        static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh.Succeeded())
        {
            MeshComponent->SetStaticMesh(CubeMesh.Object);
            MeshComponent->SetRelativeScale3D(FVector(2.f, 2.f, 0.05f));
        }
    }

    // 物理设置：作为承载平台，启用刚体并使用 PhysicsActor 配置（与 MACargo 一致）
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetEnableGravity(true);
    MeshComponent->SetLinearDamping(LinearDamping);
    MeshComponent->SetAngularDamping(AngularDamping);
    MeshComponent->SetMassOverrideInKg(NAME_None, DefaultMassKg, true);

    // 拾取交互范围
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->InitSphereRadius(200.f);
    CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionComponent->SetGenerateOverlapEvents(true);
}

void AMAMetalGrate::BeginPlay()
{
    Super::BeginPlay();

    // 通过软引用加载替换 Mesh / Material（允许编辑器中调整）
    if (!GrateMeshAsset.IsNull())
    {
        if (UStaticMesh* LoadedMesh = GrateMeshAsset.LoadSynchronous())
        {
            MeshComponent->SetStaticMesh(LoadedMesh);
        }
    }
    if (!GrateMaterialAsset.IsNull())
    {
        if (UMaterialInterface* LoadedMaterial = GrateMaterialAsset.LoadSynchronous())
        {
            MeshComponent->SetMaterial(0, LoadedMaterial);
        }
    }

    if (CollisionComponent)
    {
        CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AMAMetalGrate::OnOverlapBegin);
        CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AMAMetalGrate::OnOverlapEnd);
    }
}

void AMAMetalGrate::Configure(const FMAEnvironmentObjectConfig& Config)
{
    ObjectLabel = Config.Label;
    ObjectType = Config.Type;
    Features = Config.Features;
    SetActorRotation(Config.Rotation);

    ApplyFeatureOverrides();

    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] Configured: %s"), *ObjectLabel);
}

void AMAMetalGrate::ApplyFeatureOverrides()
{
    if (!MeshComponent)
    {
        return;
    }

    if (const FString* ScaleStr = Features.Find(TEXT("scale")))
    {
        const float CustomScale = FCString::Atof(**ScaleStr);
        if (CustomScale > 0.f)
        {
            MeshComponent->SetRelativeScale3D(MeshComponent->GetRelativeScale3D() * CustomScale);
        }
    }

    if (const FString* MassStr = Features.Find(TEXT("mass")))
    {
        const float CustomMass = FCString::Atof(**MassStr);
        if (CustomMass > 0.f)
        {
            MeshComponent->SetMassOverrideInKg(NAME_None, CustomMass, true);
        }
    }
}

//=========================================================================
// IMAPickupItem 接口实现
//=========================================================================

float AMAMetalGrate::GetBottomOffset() const
{
    if (!MeshComponent || !MeshComponent->GetStaticMesh())
    {
        return 0.f;
    }

    const FBoxSphereBounds LocalBounds = MeshComponent->GetStaticMesh()->GetBounds();
    const FVector Scale = MeshComponent->GetRelativeScale3D();
    return (LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z) * Scale.Z;
}

FVector AMAMetalGrate::GetBoundsExtent() const
{
    if (!MeshComponent)
    {
        return FVector::ZeroVector;
    }
    return MeshComponent->Bounds.BoxExtent;
}

void AMAMetalGrate::AttachToHand(AMACharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAMetalGrate] AttachToHand: Character is null"));
        return;
    }

    if (CurrentCarrier.IsValid())
    {
        DetachFromCarrier();
    }

    SetPhysicsEnabled(false);
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);

    const FVector AttachOffset = Character->GetCarryAttachOffset();
    SetActorRelativeLocation(AttachOffset);

    CurrentCarrier = Character;
    bCanBePickedUp = false;

    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s attached to hand of %s"),
        *ObjectLabel, *Character->AgentLabel);

    OnPickedUp(Character);
}

void AMAMetalGrate::AttachToUGV(AMAUGVCharacter* UGV)
{
    if (!UGV)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAMetalGrate] AttachToUGV: UGV is null"));
        return;
    }

    if (CurrentCarrier.IsValid())
    {
        DetachFromCarrier();
    }

    SetPhysicsEnabled(false);
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (UGV->LoadCargo(this))
    {
        CurrentCarrier = UGV;
        bCanBePickedUp = false;
        UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s attached to UGV %s"),
            *ObjectLabel, *UGV->AgentLabel);
    }
    else
    {
        SetPhysicsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("[MAMetalGrate] Failed to attach %s to UGV %s"),
            *ObjectLabel, *UGV->AgentLabel);
    }
}

void AMAMetalGrate::PlaceOnGround(FVector Location, bool bUprightPlacement)
{
    DetachFromCarrier();

    const FVector TraceStart = Location + FVector(0.f, 0.f, 500.f);
    const FVector TraceEnd = Location - FVector(0.f, 0.f, 1000.f);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    float GroundZ = Location.Z;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        GroundZ = HitResult.Location.Z;
    }

    const float BottomOffset = GetBottomOffset();
    const FVector PlaceLocation(Location.X, Location.Y, GroundZ - BottomOffset);

    SetActorLocation(PlaceLocation);

    if (bUprightPlacement)
    {
        SetActorRotation(FRotator::ZeroRotator);
    }

    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    SetPhysicsEnabled(true);

    bCanBePickedUp = true;

    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s placed on ground at %s"),
        *ObjectLabel, *PlaceLocation.ToString());

    OnDropped(PlaceLocation);
}

void AMAMetalGrate::PlaceOnObject(AActor* TargetObject, bool bUprightPlacement)
{
    if (!TargetObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAMetalGrate] PlaceOnObject: TargetObject is null"));
        return;
    }

    DetachFromCarrier();

    const FVector PlaceLocation = FMAPlacementSurfaceUtils::ComputePlacementWorldLocation(*TargetObject, *this);

    SetActorLocation(PlaceLocation);

    if (bUprightPlacement)
    {
        SetActorRotation(FRotator::ZeroRotator);
    }

    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    SetPhysicsEnabled(true);

    bCanBePickedUp = true;

    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s placed on %s at %s"),
        *ObjectLabel, *TargetObject->GetName(), *PlaceLocation.ToString());

    OnDropped(PlaceLocation);
}

void AMAMetalGrate::DetachFromCarrier()
{
    if (!CurrentCarrier.IsValid())
    {
        return;
    }

    AActor* Carrier = CurrentCarrier.Get();

    if (AMAUGVCharacter* UGV = Cast<AMAUGVCharacter>(Carrier))
    {
        if (UGV->CarriedItems.Contains(this))
        {
            UGV->CarriedItems.Remove(this);
        }
    }

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    CurrentCarrier.Reset();

    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s detached from carrier %s"),
        *ObjectLabel, *Carrier->GetName());
}

void AMAMetalGrate::SetPhysicsEnabled(bool bEnabled)
{
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetSimulatePhysics(bEnabled);
    MeshComponent->SetEnableGravity(bEnabled);
    if (bEnabled)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComponent->WakeAllRigidBodies();
    }
}

void AMAMetalGrate::OnPickedUp(AActor* PickerActor)
{
    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s picked up by %s"),
        *ObjectLabel, *PickerActor->GetName());
}

void AMAMetalGrate::OnDropped(FVector DropLocation)
{
    UE_LOG(LogTemp, Log, TEXT("[MAMetalGrate] %s dropped at %s"),
        *ObjectLabel, *DropLocation.ToString());
}

void AMAMetalGrate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bCanBePickedUp) return;

    if (AMACharacter* Character = Cast<AMACharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("%s entered pickup range of %s"),
            *Character->AgentLabel, *ObjectLabel);
    }
}

void AMAMetalGrate::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // 空实现
}
