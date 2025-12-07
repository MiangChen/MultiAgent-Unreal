// MAPickupItem.cpp
// 可拾取物品 - 物理/碰撞处理由 GA_Pickup/GA_Drop 负责

#include "MAPickupItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MACharacter.h"
#include "MAAbilitySystemComponent.h"
#include "MAGameplayTags.h"

AMAPickupItem::AMAPickupItem()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemName = TEXT("PickupItem");
    ItemID = 0;
    bCanBePickedUp = true;

    // 创建网格组件作为 Root (这样附着时整个物体会跟着移动)
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // 使用引擎自带的 Cube
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(CubeMesh.Object);
        MeshComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f)); // 50cm 方块
    }

    // 设置物理
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));

    // 创建碰撞组件 (用于检测 Character 进入范围) - 附着到 Mesh
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->InitSphereRadius(100.f);
    CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionComponent->SetGenerateOverlapEvents(true);
}

void AMAPickupItem::BeginPlay()
{
    Super::BeginPlay();

    // 绑定 Overlap 事件
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AMAPickupItem::OnOverlapBegin);
    CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AMAPickupItem::OnOverlapEnd);
}

void AMAPickupItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bCanBePickedUp) return;

    if (AMACharacter* Character = Cast<AMACharacter>(OtherActor))
    {
        if (UMAAbilitySystemComponent* ASC = Character->FindComponentByClass<UMAAbilitySystemComponent>())
        {
            ASC->AddLooseGameplayTag(FMAGameplayTags::Get().Status_CanPickup);
            UE_LOG(LogTemp, Log, TEXT("%s entered pickup range of %s"), *Character->ActorName, *ItemName);
        }
    }
}

void AMAPickupItem::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AMACharacter* Character = Cast<AMACharacter>(OtherActor))
    {
        if (UMAAbilitySystemComponent* ASC = Character->FindComponentByClass<UMAAbilitySystemComponent>())
        {
            ASC->RemoveLooseGameplayTag(FMAGameplayTags::Get().Status_CanPickup);
        }
    }
}

void AMAPickupItem::OnPickedUp(AActor* PickerActor)
{
    // 只做日志，物理/碰撞处理由 GA_Pickup 负责
    UE_LOG(LogTemp, Log, TEXT("%s picked up by %s"), *ItemName, *PickerActor->GetName());
}

void AMAPickupItem::OnDropped(FVector DropLocation)
{
    // 只做日志，物理/碰撞处理由 GA_Drop 负责
    UE_LOG(LogTemp, Log, TEXT("%s dropped at %s"), *ItemName, *DropLocation.ToString());
}

void AMAPickupItem::SetPhysicsEnabled(bool bEnabled)
{
    MeshComponent->SetSimulatePhysics(bEnabled);
    if (bEnabled)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    else
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}
