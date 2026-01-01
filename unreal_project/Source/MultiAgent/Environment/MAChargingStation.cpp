// MAChargingStation.cpp

#include "MAChargingStation.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MACharacter.h"
#include "MAQuadrupedCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMAChargingStation::AMAChargingStation()
{
    PrimaryActorTick.bCanEverTick = false;

    // 自动添加 Actor Tag，用于 StateTree Task 查找
    Tags.Add(FName("ChargingStation"));

    // Create mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // 让充电站不影响 NavMesh（机器人可以走到充电站位置）
    MeshComponent->SetCanEverAffectNavigation(false);
    
    // 设置碰撞：允许射线检测（Visibility 通道），但不阻挡移动（Pawn 通道）
    // 这样 Modify 模式可以点击选中充电站，但机器人可以"穿过"充电站
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);  // 允许射线检测
    MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);      // 允许相机碰撞;
    
    // Set default soft references (can be changed in editor)
    StationMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/charge/SM_StationRecharge.SM_StationRecharge")));
    StationMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/charge/M_ChargingStation.M_ChargingStation")));

    // 在构造函数中加载默认 Mesh（编辑器中可见）
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Game/charge/SM_StationRecharge.SM_StationRecharge"));
    if (DefaultMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(DefaultMesh.Object);
    }
    else
    {
        // Fallback: 使用引擎自带的 Cube
        static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh.Succeeded())
        {
            MeshComponent->SetStaticMesh(CubeMesh.Object);
            MeshComponent->SetRelativeScale3D(FVector(1.f, 1.f, 2.f));
        }
    }

    // Create interaction sphere
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(InteractionRadius);
    InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    InteractionSphere->SetGenerateOverlapEvents(true);
    
    // Visual debug
    InteractionSphere->SetHiddenInGame(false);
    InteractionSphere->ShapeColor = FColor::Green;
}

void AMAChargingStation::BeginPlay()
{
    Super::BeginPlay();
    
    // Load and apply mesh from soft reference
    if (!StationMeshAsset.IsNull())
    {
        UStaticMesh* LoadedMesh = StationMeshAsset.LoadSynchronous();
        if (LoadedMesh)
        {
            MeshComponent->SetStaticMesh(LoadedMesh);
        }
    }
    
    // Load and apply material from soft reference
    if (!StationMaterialAsset.IsNull())
    {
        UMaterialInterface* LoadedMaterial = StationMaterialAsset.LoadSynchronous();
        if (LoadedMaterial)
        {
            MeshComponent->SetMaterial(0, LoadedMaterial);
        }
    }
    
    // Fallback to cube if no mesh loaded
    if (!MeshComponent->GetStaticMesh())
    {
        UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh)
        {
            MeshComponent->SetStaticMesh(CubeMesh);
            MeshComponent->SetRelativeScale3D(FVector(2.f, 2.f, 1.f));
        }
    }
    
    // Update sphere radius and ensure collision is enabled
    InteractionSphere->SetSphereRadius(InteractionRadius);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    InteractionSphere->SetGenerateOverlapEvents(true);
    
    // Bind overlap events
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMAChargingStation::OnOverlapBegin);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AMAChargingStation::OnOverlapEnd);
}

void AMAChargingStation::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AMACharacter* Robot = Cast<AMACharacter>(OtherActor))
    {
        RobotsInRange.AddUnique(Robot);
        
        // 充电由 Charge 技能处理
        UE_LOG(LogTemp, Log, TEXT("[ChargingStation] %s entered range"), *Robot->AgentName);
    }
}

void AMAChargingStation::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AMACharacter* Robot = Cast<AMACharacter>(OtherActor))
    {
        RobotsInRange.RemoveAll([Robot](const TWeakObjectPtr<AMACharacter>& Ptr) {
            return Ptr.Get() == Robot;
        });
        
        UE_LOG(LogTemp, Log, TEXT("[ChargingStation] %s left range"), *Robot->AgentName);
    }
}

bool AMAChargingStation::IsRobotInRange(AMACharacter* Robot) const
{
    if (!Robot) return false;
    
    for (const TWeakObjectPtr<AMACharacter>& Ptr : RobotsInRange)
    {
        if (Ptr.Get() == Robot)
        {
            return true;
        }
    }
    return false;
}

TArray<AMACharacter*> AMAChargingStation::GetRobotsInRange() const
{
    TArray<AMACharacter*> Result;
    for (const TWeakObjectPtr<AMACharacter>& Ptr : RobotsInRange)
    {
        if (AMACharacter* Robot = Ptr.Get())
        {
            Result.Add(Robot);
        }
    }
    return Result;
}


void AMAChargingStation::SetStationMesh(UStaticMesh* NewMesh)
{
    if (NewMesh && MeshComponent)
    {
        StationMeshAsset = NewMesh;
        MeshComponent->SetStaticMesh(NewMesh);
    }
}
