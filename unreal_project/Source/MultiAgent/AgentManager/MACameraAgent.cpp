#include "MACameraAgent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AMACameraAgent::AMACameraAgent()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AgentID = -1;
    AgentName = TEXT("Camera");
    bIsMoving = false;
    TargetLocation = FVector::ZeroVector;
    MoveSpeed = 1000.f;
    
    // 配置移动组件
    if (UFloatingPawnMovement* Movement = Cast<UFloatingPawnMovement>(GetMovementComponent()))
    {
        Movement->MaxSpeed = 2000.f;
        Movement->Acceleration = 4000.f;
        Movement->Deceleration = 8000.f;
    }
}

void AMACameraAgent::BeginPlay()
{
    Super::BeginPlay();
    
    // 默认设置俯视角度
    SetTopDownView();
}

void AMACameraAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 平滑移动到目标位置
    if (bIsMoving)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector Direction = TargetLocation - CurrentLocation;
        float Distance = Direction.Size();
        
        if (Distance < 10.f)
        {
            bIsMoving = false;
        }
        else
        {
            FVector NewLocation = CurrentLocation + Direction.GetSafeNormal() * MoveSpeed * DeltaTime;
            SetActorLocation(NewLocation);
        }
    }
}

void AMACameraAgent::MoveToLocation(FVector Destination)
{
    // 保持当前高度
    TargetLocation = FVector(Destination.X, Destination.Y, GetActorLocation().Z);
    bIsMoving = true;
}

void AMACameraAgent::SetTopDownView(float Height, float Pitch)
{
    FVector Location = GetActorLocation();
    Location.Z = Height;
    SetActorLocation(Location);
    
    SetActorRotation(FRotator(Pitch, 0.f, 0.f));
}
