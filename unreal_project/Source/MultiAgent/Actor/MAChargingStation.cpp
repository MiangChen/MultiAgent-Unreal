// MAChargingStation.cpp

#include "MAChargingStation.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../Character/MACharacter.h"
#include "../Character/MARobotDogCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMAChargingStation::AMAChargingStation()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // Set default soft references (can be changed in editor)
    StationMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/charge/SM_StationRecharge.SM_StationRecharge")));
    StationMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/charge/M_ChargingStation.M_ChargingStation")));

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
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(OtherActor))
    {
        RobotsInRange.AddUnique(Robot);
        
        // Auto trigger charge
        Robot->TryCharge();
    }
}

void AMAChargingStation::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(OtherActor))
    {
        RobotsInRange.RemoveAll([Robot](const TWeakObjectPtr<AMACharacter>& Ptr) {
            return Ptr.Get() == Robot;
        });
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
