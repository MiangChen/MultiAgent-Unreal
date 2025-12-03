#include "MAAgentSubsystem.h"
#include "AIController.h"

void UMAAgentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMAAgentSubsystem::Deinitialize()
{
    SpawnedAgents.Empty();
    Super::Deinitialize();
}

AMAAgent* UMAAgentSubsystem::SpawnAgent(TSubclassOf<AMAAgent> AgentClass, FVector Location, FRotator Rotation, int32 AgentID, EAgentType Type)
{
    if (!AgentClass)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMAAgent* NewAgent = World->SpawnActor<AMAAgent>(AgentClass, Location, Rotation, SpawnParams);
    if (NewAgent)
    {
        NewAgent->AgentID = AgentID >= 0 ? AgentID : NextAgentID++;
        NewAgent->AgentType = Type;
        NewAgent->AgentName = FString::Printf(TEXT("%s_%d"),
            Type == EAgentType::Human ? TEXT("Human") :
            Type == EAgentType::RobotDog ? TEXT("RobotDog") : TEXT("Drone"),
            NewAgent->AgentID);

        NewAgent->SpawnDefaultController();
        SpawnedAgents.Add(NewAgent);

        OnAgentSpawned.Broadcast(NewAgent);
    }

    return NewAgent;
}

bool UMAAgentSubsystem::DestroyAgent(AMAAgent* Agent)
{
    if (!Agent)
    {
        return false;
    }

    if (SpawnedAgents.Remove(Agent) > 0)
    {
        OnAgentDestroyed.Broadcast(Agent);
        Agent->Destroy();
        return true;
    }

    return false;
}

TArray<AMAAgent*> UMAAgentSubsystem::GetAgentsByType(EAgentType Type) const
{
    TArray<AMAAgent*> Result;
    for (AMAAgent* Agent : SpawnedAgents)
    {
        if (Agent && Agent->AgentType == Type)
        {
            Result.Add(Agent);
        }
    }
    return Result;
}

AMAAgent* UMAAgentSubsystem::GetAgentByID(int32 AgentID) const
{
    for (AMAAgent* Agent : SpawnedAgents)
    {
        if (Agent && Agent->AgentID == AgentID)
        {
            return Agent;
        }
    }
    return nullptr;
}

AMAAgent* UMAAgentSubsystem::GetAgentByName(const FString& Name) const
{
    for (AMAAgent* Agent : SpawnedAgents)
    {
        if (Agent && Agent->AgentName == Name)
        {
            return Agent;
        }
    }
    return nullptr;
}

void UMAAgentSubsystem::StopAllAgents()
{
    for (AMAAgent* Agent : SpawnedAgents)
    {
        if (Agent)
        {
            Agent->CancelNavigation();
        }
    }
}

void UMAAgentSubsystem::MoveAllAgentsTo(FVector Destination, float Radius)
{
    int32 Count = SpawnedAgents.Num();
    if (Count == 0) return;

    for (int32 i = 0; i < Count; i++)
    {
        AMAAgent* Agent = SpawnedAgents[i];
        if (!Agent) continue;

        float Angle = (360.f / Count) * i;
        FVector TargetLocation = Destination + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );

        // 通过司能 (GAS) 导航
        Agent->TryNavigateTo(TargetLocation);
    }
}
