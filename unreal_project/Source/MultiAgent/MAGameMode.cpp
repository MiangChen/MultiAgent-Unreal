#include "MAGameMode.h"
#include "MACharacter.h"
#include "MAPlayerController.h"
#include "MAHumanAgent.h"
#include "MARobotDogAgent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

AMAGameMode::AMAGameMode()
{
    // 使用原来 TopDown 模板的角色蓝图作为玩家
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
        TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
    if (PlayerPawnBPClass.Succeeded())
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
    
    // 人类 Agent 使用无摄像机的蓝图
    static ConstructorHelpers::FClassFinder<APawn> HumanAgentBP(
        TEXT("/Game/Characters/BP_HumanAgent"));
    if (HumanAgentBP.Succeeded())
    {
        HumanAgentBPClass = HumanAgentBP.Class;
    }
    
    PlayerControllerClass = AMAPlayerController::StaticClass();
    
    // 机器狗使用 C++ 类
    RobotDogAgentClass = AMARobotDogAgent::StaticClass();
}

void AMAGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // 延迟一帧生成 Agent，确保玩家已经生成
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        SpawnInitialAgents();
    });
}

void AMAGameMode::SpawnInitialAgents()
{
    // 获取玩家位置作为参考点
    FVector PlayerStart = FVector(0.f, 0.f, 100.f);
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerPawn)
    {
        PlayerStart = PlayerPawn->GetActorLocation();
    }
    
    int32 TotalAgents = NumHumans + NumRobotDogs;
    int32 AgentIndex = 0;
    
    // 生成人类 Agent（使用蓝图）
    for (int32 i = 0; i < NumHumans; i++)
    {
        float Angle = (360.f / TotalAgents) * AgentIndex;
        float Radius = 400.f;
        
        FVector SpawnLocation = PlayerStart + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        SpawnHumanAgent(SpawnLocation, FRotator::ZeroRotator, AgentIndex);
        AgentIndex++;
    }
    
    // 生成机器狗 Agent
    for (int32 i = 0; i < NumRobotDogs; i++)
    {
        float Angle = (360.f / TotalAgents) * AgentIndex;
        float Radius = 400.f;
        
        FVector SpawnLocation = PlayerStart + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        SpawnAgent(RobotDogAgentClass, SpawnLocation, FRotator::ZeroRotator, AgentIndex, EAgentType::RobotDog);
        AgentIndex++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Spawned %d humans and %d robot dogs"), NumHumans, NumRobotDogs);
}

APawn* AMAGameMode::SpawnHumanAgent(FVector Location, FRotator Rotation, int32 AgentID)
{
    if (!HumanAgentBPClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APawn* NewAgent = GetWorld()->SpawnActor<APawn>(HumanAgentBPClass, Location, Rotation, SpawnParams);
    if (NewAgent)
    {
        NewAgent->SpawnDefaultController();
        SpawnedHumanPawns.Add(NewAgent);
    }

    return NewAgent;
}

AMAAgent* AMAGameMode::SpawnAgent(TSubclassOf<AMAAgent> AgentClass, FVector Location, FRotator Rotation, int32 AgentID, EAgentType Type)
{
    if (!AgentClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMAAgent* NewAgent = GetWorld()->SpawnActor<AMAAgent>(AgentClass, Location, Rotation, SpawnParams);
    if (NewAgent)
    {
        NewAgent->AgentID = AgentID;
        NewAgent->AgentType = Type;
        NewAgent->AgentName = FString::Printf(TEXT("%s_%d"), 
            Type == EAgentType::Human ? TEXT("Human") : TEXT("RobotDog"), AgentID);
        
        NewAgent->SpawnDefaultController();
        SpawnedAgents.Add(NewAgent);
    }

    return NewAgent;
}

TArray<AMAAgent*> AMAGameMode::GetAgentsByType(EAgentType Type) const
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

TArray<APawn*> AMAGameMode::GetAllPawns() const
{
    TArray<APawn*> Result;
    Result.Append(SpawnedHumanPawns);
    for (AMAAgent* Agent : SpawnedAgents)
    {
        Result.Add(Agent);
    }
    return Result;
}
