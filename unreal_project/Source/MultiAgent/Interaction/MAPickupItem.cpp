// MAPickupItem.cpp

#include "MAPickupItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../AgentManager/MAAgent.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../GAS/MAGameplayTags.h"

AMAPickupItem::AMAPickupItem()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemName = TEXT("PickupItem");
    ItemID = 0;
    bCanBePickedUp = true;

    // 创建碰撞组件 (用于检测 Agent 进入范围)
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitSphereRadius(100.f);
    CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionComponent->SetGenerateOverlapEvents(true);
    RootComponent = CollisionComponent;

    // 创建网格组件 (可视化)
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    
    // 使用引擎自带的 Cube
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(CubeMesh.Object);
        MeshComponent->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.3f)); // 30cm 方块
    }

    // 设置物理
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
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

    // 检查是否是 Agent
    if (AMAAgent* Agent = Cast<AMAAgent>(OtherActor))
    {
        // 给 Agent 添加 CanPickup Tag
        if (UMAAbilitySystemComponent* ASC = Agent->FindComponentByClass<UMAAbilitySystemComponent>())
        {
            ASC->AddLooseGameplayTag(FMAGameplayTags::Get().Status_CanPickup);
            
            UE_LOG(LogTemp, Log, TEXT("%s entered pickup range of %s"), 
                *Agent->AgentName, *ItemName);
        }
    }
}

void AMAPickupItem::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AMAAgent* Agent = Cast<AMAAgent>(OtherActor))
    {
        // 移除 CanPickup Tag
        if (UMAAbilitySystemComponent* ASC = Agent->FindComponentByClass<UMAAbilitySystemComponent>())
        {
            ASC->RemoveLooseGameplayTag(FMAGameplayTags::Get().Status_CanPickup);
        }
    }
}

void AMAPickupItem::OnPickedUp(AActor* PickerActor)
{
    bCanBePickedUp = false;
    SetPhysicsEnabled(false);
    
    // 隐藏碰撞检测
    CollisionComponent->SetGenerateOverlapEvents(false);
    
    UE_LOG(LogTemp, Log, TEXT("%s picked up by %s"), *ItemName, *PickerActor->GetName());
}

void AMAPickupItem::OnDropped(FVector DropLocation)
{
    // 从父级分离
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    // 设置位置
    SetActorLocation(DropLocation);
    
    // 恢复物理
    SetPhysicsEnabled(true);
    
    // 恢复可拾取状态
    bCanBePickedUp = true;
    CollisionComponent->SetGenerateOverlapEvents(true);
    
    UE_LOG(LogTemp, Log, TEXT("%s dropped at %s"), *ItemName, *DropLocation.ToString());
}

void AMAPickupItem::SetPhysicsEnabled(bool bEnabled)
{
    MeshComponent->SetSimulatePhysics(bEnabled);
}
