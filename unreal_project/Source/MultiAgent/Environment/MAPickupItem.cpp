// MAPickupItem.cpp
// 可拾取物品 - 物理/碰撞处理由 GA_Place 负责

#include "MAPickupItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MAHumanoidCharacter.h"
#include "../Agent/Character/MAUGVCharacter.h"

AMAPickupItem::AMAPickupItem()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemName = TEXT("PickupItem");
    ItemID = 0;
    bCanBePickedUp = true;
    ItemColor = FLinearColor::White;

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
    
    // 如果颜色不是白色，应用颜色
    if (!ItemColor.Equals(FLinearColor::White))
    {
        SetItemColor(ItemColor);
    }
}

void AMAPickupItem::SetItemColor(FLinearColor NewColor)
{
    ItemColor = NewColor;
    
    if (MeshComponent)
    {
        // 创建动态材质实例
        UMaterialInstanceDynamic* DynMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMaterial)
        {
            // 设置基础颜色
            DynMaterial->SetVectorParameterValue(TEXT("BaseColor"), NewColor);
            // 也尝试设置其他常见的颜色参数名
            DynMaterial->SetVectorParameterValue(TEXT("Color"), NewColor);
            DynMaterial->SetVectorParameterValue(TEXT("Tint"), NewColor);
            
            UE_LOG(LogTemp, Log, TEXT("[MAPickupItem] %s color set to (R=%.2f, G=%.2f, B=%.2f)"), 
                *ItemName, NewColor.R, NewColor.G, NewColor.B);
        }
    }
}

void AMAPickupItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bCanBePickedUp) return;

    if (AMACharacter* Character = Cast<AMACharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("%s entered pickup range of %s"), *Character->AgentName, *ItemName);
    }
}

void AMAPickupItem::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Pickup 功能已移除，此处仅保留空实现
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

// ========== 附着/分离系统实现 ==========

void AMAPickupItem::AttachToHand(AMACharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAPickupItem] AttachToHand: Character is null"));
        return;
    }
    
    // 如果已经被其他承载者持有，先分离
    if (CurrentCarrier.IsValid())
    {
        DetachFromCarrier();
    }
    
    // 禁用物理模拟
    SetPhysicsEnabled(false);
    
    // 附着到角色
    AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
    
    // 设置相对位置到手部位置
    // 使用 Humanoid 的 HandAttachOffset，如果是其他角色类型则使用默认偏移
    // 默认偏移：身体前方 60cm，高度 20cm（大约在腰部/手部位置）
    FVector AttachOffset = FVector(60.f, 0.f, 20.f);
    if (AMAHumanoidCharacter* Humanoid = Cast<AMAHumanoidCharacter>(Character))
    {
        AttachOffset = Humanoid->HandAttachOffset;
    }
    SetActorRelativeLocation(AttachOffset);
    
    // 记录承载者
    CurrentCarrier = Character;
    bCanBePickedUp = false;
    
    UE_LOG(LogTemp, Log, TEXT("[MAPickupItem] %s attached to hand of %s at offset (%.1f, %.1f, %.1f)"), 
        *ItemName, *Character->AgentName, AttachOffset.X, AttachOffset.Y, AttachOffset.Z);
    
    // 调用 OnPickedUp 回调
    OnPickedUp(Character);
}

void AMAPickupItem::AttachToUGV(AMAUGVCharacter* UGV)
{
    if (!UGV)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAPickupItem] AttachToUGV: UGV is null"));
        return;
    }
    
    // 如果已经被其他承载者持有，先分离
    if (CurrentCarrier.IsValid())
    {
        DetachFromCarrier();
    }
    
    // 禁用物理模拟
    SetPhysicsEnabled(false);
    
    // 使用 UGV 的 LoadCargo 方法附着
    // LoadCargo 会处理附着和添加到 CarriedItems 列表
    if (UGV->LoadCargo(this))
    {
        // 记录承载者
        CurrentCarrier = UGV;
        bCanBePickedUp = false;
        
        UE_LOG(LogTemp, Log, TEXT("[MAPickupItem] %s attached to UGV %s"), 
            *ItemName, *UGV->AgentName);
    }
    else
    {
        // LoadCargo 失败，恢复物理
        SetPhysicsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("[MAPickupItem] Failed to attach %s to UGV %s"), 
            *ItemName, *UGV->AgentName);
    }
}

void AMAPickupItem::PlaceOnGround(FVector Location)
{
    // 从当前承载者分离
    DetachFromCarrier();
    
    // 设置位置
    SetActorLocation(Location);
    
    // 重新启用物理模拟
    SetPhysicsEnabled(true);
    
    bCanBePickedUp = true;
    
    UE_LOG(LogTemp, Log, TEXT("[MAPickupItem] %s placed on ground at %s"), 
        *ItemName, *Location.ToString());
    
    // 调用 OnDropped 回调
    OnDropped(Location);
}

void AMAPickupItem::PlaceOnObject(AMAPickupItem* TargetObject)
{
    if (!TargetObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAPickupItem] PlaceOnObject: TargetObject is null"));
        return;
    }
    
    // 从当前承载者分离
    DetachFromCarrier();
    
    // 计算目标位置：在目标物体上方
    // 获取目标物体的边界框来确定堆叠高度
    FVector TargetLocation = TargetObject->GetActorLocation();
    FVector TargetBoundsExtent = FVector::ZeroVector;
    FVector MyBoundsExtent = FVector::ZeroVector;
    
    if (TargetObject->MeshComponent)
    {
        TargetBoundsExtent = TargetObject->MeshComponent->Bounds.BoxExtent;
    }
    if (MeshComponent)
    {
        MyBoundsExtent = MeshComponent->Bounds.BoxExtent;
    }
    
    // 放置在目标物体顶部
    FVector PlaceLocation = TargetLocation;
    PlaceLocation.Z += TargetBoundsExtent.Z + MyBoundsExtent.Z;
    
    SetActorLocation(PlaceLocation);
    
    // 重新启用物理模拟
    SetPhysicsEnabled(true);
    
    bCanBePickedUp = true;
    
    UE_LOG(LogTemp, Log, TEXT("[MAPickupItem] %s placed on object %s at %s"), 
        *ItemName, *TargetObject->ItemName, *PlaceLocation.ToString());
    
    // 调用 OnDropped 回调
    OnDropped(PlaceLocation);
}

void AMAPickupItem::DetachFromCarrier()
{
    if (!CurrentCarrier.IsValid())
    {
        return;
    }
    
    AActor* Carrier = CurrentCarrier.Get();
    
    // 如果承载者是 UGV，使用 UnloadCargo 方法
    if (AMAUGVCharacter* UGV = Cast<AMAUGVCharacter>(Carrier))
    {
        // UnloadCargo 会处理分离和从 CarriedItems 列表移除
        // 但我们不想让它设置位置，所以直接操作
        if (UGV->CarriedItems.Contains(this))
        {
            UGV->CarriedItems.Remove(this);
        }
    }
    
    // 从父 Actor 分离
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    // 清除承载者引用
    CurrentCarrier.Reset();
    
    UE_LOG(LogTemp, Log, TEXT("[MAPickupItem] %s detached from carrier %s"), 
        *ItemName, *Carrier->GetName());
}
