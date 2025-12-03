#include "MAGameMode.h"
#include "MAPlayerController.h"
#include "../AgentManager/MAAgentSubsystem.h"
#include "../AgentManager/MARobotDogAgent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

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
    
    // 延迟一帧生成 Agent，确保玩家和子系统已经初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        SpawnInitialAgents();
    });
}

UMAAgentSubsystem* AMAGameMode::GetAgentSubsystem() const
{
    return GetWorld()->GetSubsystem<UMAAgentSubsystem>();
}

void AMAGameMode::SpawnInitialAgents()
{
    UMAAgentSubsystem* AgentSubsystem = GetAgentSubsystem();
    if (!AgentSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("AgentSubsystem not found!"));
        return;
    }

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
    
    // 生成机器狗 Agent（通过 AgentSubsystem）
    for (int32 i = 0; i < NumRobotDogs; i++)
    {
        float Angle = (360.f / TotalAgents) * AgentIndex;
        float Radius = 400.f;
        
        FVector SpawnLocation = PlayerStart + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        AgentSubsystem->SpawnAgent(RobotDogAgentClass, SpawnLocation, FRotator::ZeroRotator, AgentIndex, EAgentType::RobotDog);
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

TArray<APawn*> AMAGameMode::GetAllPawns() const
{
    TArray<APawn*> Result;
    Result.Append(SpawnedHumanPawns);
    
    // 从 AgentSubsystem 获取所有 Agent
    if (UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>())
    {
        for (AMAAgent* Agent : AgentSubsystem->GetAllAgents())
        {
            Result.Add(Agent);
        }
    }
    
    return Result;
}
