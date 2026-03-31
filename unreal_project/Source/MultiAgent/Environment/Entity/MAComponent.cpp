// MAComponent.cpp
// 通用组件环境对象实现 - 支持搬运操作

#include "MAComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Character/MAHumanoidCharacter.h"
#include "../../Agent/Character/MAUGVCharacter.h"

AMAComponent::AMAComponent()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建网格组件作为 Root (这样附着时整个物体会跟着移动)
    // 重要：MeshComponent 必须是 RootComponent，否则 AttachToActor 时 Mesh 不会跟随移动
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComponent;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 创建碰撞检测组件 (用于检测 Character 进入范围) - 附着到 Mesh
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->InitSphereRadius(150.f);
    CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionComponent->SetGenerateOverlapEvents(true);
}

void AMAComponent::BeginPlay()
{
    Super::BeginPlay();

    // 绑定 Overlap 事件
    if (CollisionComponent)
    {
        CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AMAComponent::OnOverlapBegin);
        CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AMAComponent::OnOverlapEnd);
    }
}

void AMAComponent::Configure(const FMAEnvironmentObjectConfig& Config)
{
    ObjectLabel = Config.Label;
    ObjectType = Config.Type;
    Features = Config.Features;
    SetActorRotation(Config.Rotation);

    // 根据 subtype 设置网格
    FString Subtype = Features.FindRef(TEXT("subtype"));
    if (!Subtype.IsEmpty())
    {
        SetComponentMesh(Subtype);
    }

    // 从配置读取自定义缩放
    FString ScaleStr = Features.FindRef(TEXT("scale"));
    if (!ScaleStr.IsEmpty())
    {
        float CustomScale = FCString::Atof(*ScaleStr);
        if (CustomScale > 0.f)
        {
            MeshComponent->SetRelativeScale3D(MeshComponent->GetRelativeScale3D() * CustomScale);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MAComponent] Configured: %s (subtype=%s)"), *ObjectLabel, *Subtype);
}

//=========================================================================
// IMAPickupItem 接口实现
//=========================================================================

float AMAComponent::GetBottomOffset() const
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

FVector AMAComponent::GetBoundsExtent() const
{
    if (!MeshComponent)
    {
        return FVector::ZeroVector;
    }
    
    return MeshComponent->Bounds.BoxExtent;
}

void AMAComponent::AttachToHand(AMACharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] AttachToHand: Character is null"));
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
    SetActorRelativeRotation(FRotator::ZeroRotator);

    // 记录承载者
    CurrentCarrier = Character;
    bCanBePickedUp = false;
    
    UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s attached to hand of %s at offset (%.1f, %.1f, %.1f)"), 
        *ObjectLabel, *Character->AgentLabel, AttachOffset.X, AttachOffset.Y, AttachOffset.Z);
    
    OnPickedUp(Character);
}

void AMAComponent::AttachToUGV(AMAUGVCharacter* UGV)
{
    if (!UGV)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] AttachToUGV: UGV is null"));
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
        
        UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s attached to UGV %s"), 
            *ObjectLabel, *UGV->AgentLabel);
    }
    else
    {
        SetPhysicsEnabled(true);
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] Failed to attach %s to UGV %s"), 
            *ObjectLabel, *UGV->AgentLabel);
    }
}

void AMAComponent::PlaceOnGround(FVector Location, bool bUprightPlacement)
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
    
    // 根据物体类型决定是否启用物理
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    if (ShouldEnablePhysicsOnPlace())
    {
        SetPhysicsEnabled(true);
    }
    
    bCanBePickedUp = true;
    
    UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s placed on ground at %s (Physics=%d)"), 
        *ObjectLabel, *PlaceLocation.ToString(), ShouldEnablePhysicsOnPlace());
    
    OnDropped(PlaceLocation);
}

void AMAComponent::PlaceOnObject(AActor* TargetObject, bool bUprightPlacement)
{
    if (!TargetObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] PlaceOnObject: TargetObject is null"));
        return;
    }

    DetachFromCarrier();

    FVector TargetLocation = TargetObject->GetActorLocation();
    float TargetTopZ = TargetLocation.Z;

    // 获取目标物体的精确顶部位置
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

    // 获取目标物体的堆叠偏移（补偿 mesh 不对称造成的视觉偏移）
    FVector TargetStackOffset = FVector::ZeroVector;
    if (AMAComponent* TargetComp = Cast<AMAComponent>(TargetObject))
    {
        FString TargetSubtype = TargetComp->Features.FindRef(TEXT("subtype"));
        TargetStackOffset = GetComponentStackOffset(TargetSubtype);
        TargetStackOffset = TargetComp->GetActorRotation().RotateVector(TargetStackOffset);

        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] PlaceOnObject: TargetSubtype='%s', StackOffset=%s"),
            *TargetSubtype, *TargetStackOffset.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] PlaceOnObject: Cast<AMAComponent> FAILED for %s (Class=%s)"),
            *TargetObject->GetName(), *TargetObject->GetClass()->GetName());
    }

    float MyBottomOffset = GetBottomOffset();
    FVector PlaceLocation = FVector(
        TargetLocation.X + TargetStackOffset.X,
        TargetLocation.Y + TargetStackOffset.Y,
        TargetTopZ - MyBottomOffset);

    UE_LOG(LogTemp, Warning, TEXT("[MAComponent] PlaceOnObject: TargetLoc=%s, PlaceLoc=%s"),
        *TargetLocation.ToString(), *PlaceLocation.ToString());

    if (bUprightPlacement)
    {
        SetActorRotation(FRotator::ZeroRotator);
    }
    SetActorLocation(PlaceLocation);

    // 根据物体类型决定是否启用物理
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    if (ShouldEnablePhysicsOnPlace())
    {
        SetPhysicsEnabled(true);
    }

    bCanBePickedUp = true;

    UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s placed on %s at %s"),
        *ObjectLabel, *TargetObject->GetName(), *PlaceLocation.ToString());

    OnDropped(PlaceLocation);
}

void AMAComponent::DetachFromCarrier()
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
    
    UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s detached from carrier %s"), 
        *ObjectLabel, *Carrier->GetName());
}

void AMAComponent::SetPhysicsEnabled(bool bEnabled)
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
    }
}

bool AMAComponent::ShouldEnablePhysicsOnPlace() const
{
    // 某些不稳定物体（如天线）放置后不启用物理，避免倒塌
    FString Subtype = Features.FindRef(TEXT("subtype")).ToLower();
    
    static TSet<FString> UnstableSubtypes = {
        TEXT("antenna_module"),
        TEXT("solar_panel_large"),
        TEXT("address_speaker"),
        TEXT("solar_panel"),
        TEXT("stand")
    };
    
    return !UnstableSubtypes.Contains(Subtype);
}

void AMAComponent::OnPickedUp(AActor* PickerActor)
{
    UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s picked up by %s"), *ObjectLabel, *PickerActor->GetName());
}

void AMAComponent::OnDropped(FVector DropLocation)
{
    UE_LOG(LogTemp, Log, TEXT("[MAComponent] %s dropped at %s"), *ObjectLabel, *DropLocation.ToString());
}

void AMAComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bCanBePickedUp) return;

    if (AMACharacter* Character = Cast<AMACharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("%s entered pickup range of %s"), *Character->AgentLabel, *ObjectLabel);
    }
}

void AMAComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // 空实现
}

//=========================================================================
// 物理控制 (兼容旧接口)
//=========================================================================

void AMAComponent::EnablePhysics()
{
    SetPhysicsEnabled(true);
}

void AMAComponent::DisablePhysics()
{
    SetPhysicsEnabled(false);
}

//=========================================================================
// 网格设置
//=========================================================================

void AMAComponent::SetComponentMesh(const FString& Subtype)
{
    FString MeshPath = GetComponentMeshPath(Subtype);
    if (MeshPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] Unknown subtype: %s"), *Subtype);
        return;
    }

    UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    if (LoadedMesh && MeshComponent)
    {
        MeshComponent->SetStaticMesh(LoadedMesh);
        // MeshComponent 现在是 RootComponent，不需要设置 RelativeLocation
        // 只设置缩放
        MeshComponent->SetRelativeScale3D(GetComponentDefaultScale(Subtype));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAComponent] Failed to load mesh: %s"), *MeshPath);
    }
}

FString AMAComponent::GetComponentMeshPath(const FString& Subtype)
{
    static TMap<FString, FString> ComponentMeshMap = {
        // 太阳能板
        {TEXT("solar_panel"), TEXT("/Game/Props/ElectricCentral/Meshes/SM_Solar_pannel_Standard.SM_Solar_pannel_Standard")},
        {TEXT("solar_panel_small"), TEXT("/Game/Props/ElectricCentral/Meshes/SM_Solar_pannel_Small_1.SM_Solar_pannel_Small_1")},
        {TEXT("solar_panel_medium"), TEXT("/Game/Props/ElectricCentral/Meshes/SM_Solar_pannel_Medium_1.SM_Solar_pannel_Medium_1")},
        {TEXT("solar_panel_large"), TEXT("/Game/Props/ElectricCentral/Meshes/SM_Solar_pannel_Large_1.SM_Solar_pannel_Large_1")},
        
        // 天线
        {TEXT("antenna_module"), TEXT("/Game/Props/Antenna/Meshes/NN/SM_Satellite_Dish_NN_01a.SM_Satellite_Dish_NN_01a")},
        
        // 音响
        {TEXT("address_speaker"), TEXT("/Game/Props/Speakers/SM_Speaker_01.SM_Speaker_01")},
        
        // 支架/高脚凳
        {TEXT("stand"), TEXT("/Game/Props/WoodBarStool/wood_bar_stool.wood_bar_stool")},
    };

    FString SubtypeLower = Subtype.ToLower();
    if (const FString* Path = ComponentMeshMap.Find(SubtypeLower))
    {
        return *Path;
    }
    return FString();
}

FVector AMAComponent::GetComponentDefaultScale(const FString& Subtype)
{
    static TMap<FString, FVector> ScaleMap = {
        {TEXT("solar_panel"), FVector(1.0f)},
        {TEXT("solar_panel_small"), FVector(1.0f)},
        {TEXT("solar_panel_medium"), FVector(1.0f)},
        {TEXT("solar_panel_large"), FVector(1.0f)},
        {TEXT("antenna_module"), FVector(1.0f)},
        {TEXT("address_speaker"), FVector(5.0f)},
        {TEXT("stand"), FVector(2.0f, 2.0f, 4.0f)},
    };

    FString SubtypeLower = Subtype.ToLower();
    if (const FVector* Scale = ScaleMap.Find(SubtypeLower))
    {
        return *Scale;
    }
    return FVector(1.0f);
}

FVector AMAComponent::GetComponentDefaultOffset(const FString& Subtype)
{
    static TMap<FString, FVector> OffsetMap = {
        {TEXT("solar_panel"), FVector(0.f, 0.f, 0.f)},
        {TEXT("solar_panel_small"), FVector(0.f, 0.f, 0.f)},
        {TEXT("solar_panel_medium"), FVector(0.f, 0.f, 0.f)},
        {TEXT("solar_panel_large"), FVector(0.f, 0.f, 0.f)},
        {TEXT("antenna_module"), FVector(0.f, 0.f, 0.f)},
        {TEXT("address_speaker"), FVector(0.f, 0.f, 0.f)},
        {TEXT("stand"), FVector(0.f, 0.f, 0.f)},
    };

    FString SubtypeLower = Subtype.ToLower();
    if (const FVector* Offset = OffsetMap.Find(SubtypeLower))
    {
        return *Offset;
    }
    return FVector::ZeroVector;
}

FVector AMAComponent::GetComponentStackOffset(const FString& Subtype)
{
    // 堆叠偏移：补偿 mesh 不对称导致的视觉中心与 pivot 的偏差
    // 例如 bar stool 的 pivot 在底部中心，但座面可能不在正上方
    // 值为本地空间偏移（未缩放），会在 PlaceOnObject 中根据目标旋转变换
    TMap<FString, FVector> StackOffsetMap = {
        {TEXT("stand"), FVector(0.f, -50.f, 0.f)},
    };

    FString SubtypeLower = Subtype.ToLower();
    if (const FVector* Offset = StackOffsetMap.Find(SubtypeLower))
    {
        return *Offset;
    }
    return FVector::ZeroVector;
}
