// MACargo.cpp
// 货物类实现 - cargo 类型的可搬运物体

#include "MACargo.h"
#include "MAComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Character/MAHumanoidCharacter.h"
#include "../../Agent/Character/MAUGVCharacter.h"

AMACargo::AMACargo()
{
    PrimaryActorTick.bCanEverTick = false;

    // 默认值
    ItemName = TEXT("Cargo");
    ItemID = 0;
    bCanBePickedUp = true;
    ItemColor = FLinearColor::White;
    ObjectType = TEXT("cargo");

    // 创建网格组件作为 Root (这样附着时整个物体会跟着移动)
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // 使用引擎自带的 Cube
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(CubeMesh.Object);
        MeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
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

void AMACargo::BeginPlay()
{
    Super::BeginPlay();

    // 绑定 Overlap 事件
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AMACargo::OnOverlapBegin);
    CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AMACargo::OnOverlapEnd);
    
    // 如果颜色不是白色，应用颜色
    if (!ItemColor.Equals(FLinearColor::White))
    {
        SetItemColor(ItemColor);
    }
}

//=========================================================================
// IMAEnvironmentObject 接口实现
//=========================================================================

FString AMACargo::GetObjectLabel() const
{
    if (!ObjectLabel.IsEmpty())
    {
        return ObjectLabel;
    }
    return ItemName;
}

FString AMACargo::GetObjectType() const
{
    return ObjectType;
}

const TMap<FString, FString>& AMACargo::GetObjectFeatures() const
{
    return Features;
}

//=========================================================================
// 物品操作
//=========================================================================

void AMACargo::SetItemColor(FLinearColor NewColor)
{
    ItemColor = NewColor;
    
    if (MeshComponent)
    {
        UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (BaseMaterial)
        {
            UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            if (DynMaterial)
            {
                DynMaterial->SetVectorParameterValue(TEXT("Color"), NewColor);
                MeshComponent->SetMaterial(0, DynMaterial);
                
                UE_LOG(LogTemp, Log, TEXT("[MACargo] %s color set to (R=%.2f, G=%.2f, B=%.2f)"), 
                    *ItemName, NewColor.R, NewColor.G, NewColor.B);
            }
        }
        else
        {
            UMaterialInstanceDynamic* DynMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
            if (DynMaterial)
            {
                DynMaterial->SetVectorParameterValue(TEXT("BaseColor"), NewColor);
                DynMaterial->SetVectorParameterValue(TEXT("Color"), NewColor);
                
                UE_LOG(LogTemp, Log, TEXT("[MACargo] %s color set (fallback) to (R=%.2f, G=%.2f, B=%.2f)"), 
                    *ItemName, NewColor.R, NewColor.G, NewColor.B);
            }
        }
    }
}

void AMACargo::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bCanBePickedUp) return;

    if (AMACharacter* Character = Cast<AMACharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("%s entered pickup range of %s"), *Character->AgentLabel, *ItemName);
    }
}

void AMACargo::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // 空实现
}

void AMACargo::OnPickedUp(AActor* PickerActor)
{
    UE_LOG(LogTemp, Log, TEXT("%s picked up by %s"), *ItemName, *PickerActor->GetName());
}

void AMACargo::OnDropped(FVector DropLocation)
{
    UE_LOG(LogTemp, Log, TEXT("%s dropped at %s"), *ItemName, *DropLocation.ToString());
}

void AMACargo::SetPhysicsEnabled(bool bEnabled)
{
    if (MeshComponent)
    {
        MeshComponent->SetSimulatePhysics(bEnabled);
        MeshComponent->SetEnableGravity(bEnabled);
        if (bEnabled)
        {
            MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            MeshComponent->WakeAllRigidBodies();
        }
        // 注意：禁用物理时不自动禁用碰撞
        // 碰撞状态由调用者根据需要单独控制
        // 例如：放置到地面后需要保持碰撞以便再次拾取
    }
}

//=========================================================================
// 边界信息实现
//=========================================================================

float AMACargo::GetBottomOffset() const
{
    if (!MeshComponent || !MeshComponent->GetStaticMesh())
    {
        return 0.f;
    }
    
    // 获取 Mesh 的本地边界
    FBoxSphereBounds LocalBounds = MeshComponent->GetStaticMesh()->GetBounds();
    FVector Scale = MeshComponent->GetRelativeScale3D();
    
    // 计算缩放后的底部偏移
    // LocalBounds.Origin.Z 是边界中心相对于 Mesh 原点的偏移
    // LocalBounds.BoxExtent.Z 是边界半高
    // 底部位置 = Origin.Z - BoxExtent.Z
    float BottomZ = (LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z) * Scale.Z;
    
    return BottomZ;
}

FVector AMACargo::GetBoundsExtent() const
{
    if (!MeshComponent)
    {
        return FVector::ZeroVector;
    }
    
    return MeshComponent->Bounds.BoxExtent;
}

//=========================================================================
// 附着/分离系统实现
//=========================================================================

void AMACargo::AttachToHand(AMACharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MACargo] AttachToHand: Character is null"));
        return;
    }
    
    // 如果已经被其他承载者持有，先分离
    if (CurrentCarrier.IsValid())
    {
        DetachFromCarrier();
    }
    
    // 禁用物理模拟和碰撞（避免与角色碰撞）
    SetPhysicsEnabled(false);
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // 附着到角色
    AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
    
    // 设置相对位置到手部位置
    FVector AttachOffset = FVector(60.f, 0.f, 20.f);
    if (AMAHumanoidCharacter* Humanoid = Cast<AMAHumanoidCharacter>(Character))
    {
        AttachOffset = Humanoid->HandAttachOffset;
    }
    SetActorRelativeLocation(AttachOffset);
    
    // 记录承载者
    CurrentCarrier = Character;
    bCanBePickedUp = false;
    
    UE_LOG(LogTemp, Log, TEXT("[MACargo] %s attached to hand of %s at offset (%.1f, %.1f, %.1f)"), 
        *ItemName, *Character->AgentLabel, AttachOffset.X, AttachOffset.Y, AttachOffset.Z);
    
    OnPickedUp(Character);
}

void AMACargo::AttachToUGV(AMAUGVCharacter* UGV)
{
    if (!UGV)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MACargo] AttachToUGV: UGV is null"));
        return;
    }
    
    if (CurrentCarrier.IsValid())
    {
        DetachFromCarrier();
    }
    
    SetPhysicsEnabled(false);
    
    if (UGV->LoadCargo(this))
    {
        CurrentCarrier = UGV;
        bCanBePickedUp = false;
        
        UE_LOG(LogTemp, Log, TEXT("[MACargo] %s attached to UGV %s"), 
            *ItemName, *UGV->AgentLabel);
    }
    else
    {
        SetPhysicsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("[MACargo] Failed to attach %s to UGV %s"), 
            *ItemName, *UGV->AgentLabel);
    }
}

void AMACargo::PlaceOnGround(FVector Location, bool bUprightPlacement)
{
    DetachFromCarrier();
    
    // 射线检测找到真实地面高度
    FVector TraceStart = Location + FVector(0.f, 0.f, 500.f);
    FVector TraceEnd = Location - FVector(0.f, 0.f, 1000.f);
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    
    float GroundZ = Location.Z;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        GroundZ = HitResult.Location.Z;
    }
    
    // 计算精确放置位置：地面高度 - 底部偏移
    float BottomOffset = GetBottomOffset();
    FVector PlaceLocation = FVector(Location.X, Location.Y, GroundZ - BottomOffset);
    
    SetActorLocation(PlaceLocation);
    
    if (bUprightPlacement)
    {
        SetActorRotation(FRotator::ZeroRotator);
    }
    
    // 精确放置后启用物理
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    SetPhysicsEnabled(true);
    
    bCanBePickedUp = true;
    
    UE_LOG(LogTemp, Log, TEXT("[MACargo] %s placed on ground at %s (GroundZ=%.1f, BottomOffset=%.1f)"), 
        *ItemName, *PlaceLocation.ToString(), GroundZ, BottomOffset);
    
    OnDropped(PlaceLocation);
}

void AMACargo::PlaceOnObject(AActor* TargetObject, bool bUprightPlacement)
{
    if (!TargetObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MACargo] PlaceOnObject: TargetObject is null"));
        return;
    }
    
    DetachFromCarrier();

    FVector TargetLocation = TargetObject->GetActorLocation();
    float TargetTopZ = TargetLocation.Z;

    if (IMAPickupItem* TargetItem = Cast<IMAPickupItem>(TargetObject))
    {
        FVector TargetExtent = TargetItem->GetBoundsExtent();
        float TargetBottomOffset = TargetItem->GetBottomOffset();
        TargetTopZ = TargetLocation.Z + TargetBottomOffset + TargetExtent.Z * 2.f;
    }
    else if (UPrimitiveComponent* TargetPrim = Cast<UPrimitiveComponent>(TargetObject->GetRootComponent()))
    {
        TargetTopZ = TargetLocation.Z + TargetPrim->Bounds.BoxExtent.Z;
    }

    // 获取目标物体的堆叠偏移
    FVector TargetStackOffset = FVector::ZeroVector;
    if (AMAComponent* TargetComp = Cast<AMAComponent>(TargetObject))
    {
        FString TargetSubtype = TargetComp->GetObjectFeatures().FindRef(TEXT("subtype"));
        TargetStackOffset = AMAComponent::GetComponentStackOffset(TargetSubtype);
        TargetStackOffset = TargetComp->GetActorRotation().RotateVector(TargetStackOffset);
    }

    float MyBottomOffset = GetBottomOffset();
    FVector PlaceLocation = FVector(
        TargetLocation.X + TargetStackOffset.X,
        TargetLocation.Y + TargetStackOffset.Y,
        TargetTopZ - MyBottomOffset);

    SetActorLocation(PlaceLocation);
    
    if (bUprightPlacement)
    {
        SetActorRotation(FRotator::ZeroRotator);
    }
    
    // 精确放置后启用物理
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    SetPhysicsEnabled(true);
    
    bCanBePickedUp = true;
    
    UE_LOG(LogTemp, Log, TEXT("[MACargo] %s placed on %s at %s"), 
        *ItemName, *TargetObject->GetName(), *PlaceLocation.ToString());
    
    OnDropped(PlaceLocation);
}

void AMACargo::DetachFromCarrier()
{
    if (!CurrentCarrier.IsValid())
    {
        return;
    }
    
    AActor* Carrier = CurrentCarrier.Get();
    
    // 如果承载者是 UGV，从 CarriedItems 列表移除
    if (AMAUGVCharacter* UGV = Cast<AMAUGVCharacter>(Carrier))
    {
        if (UGV->CarriedItems.Contains(this))
        {
            UGV->CarriedItems.Remove(this);
        }
    }
    
    // 从父 Actor 分离
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    // 清除承载者引用
    CurrentCarrier.Reset();
    
    UE_LOG(LogTemp, Log, TEXT("[MACargo] %s detached from carrier %s"), 
        *ItemName, *Carrier->GetName());
}
