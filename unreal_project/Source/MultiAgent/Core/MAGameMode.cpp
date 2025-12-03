#include "MAGameMode.h"
#include "MAPlayerController.h"
#include "../AgentManager/MAAgentSubsystem.h"
#include "../AgentManager/MAHumanAgent.h"
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
    
    PlayerControllerClass = AMAPlayerController::StaticClass();
    
    // Agent 类型使用 C++ 类
    HumanAgentClass = AMAHumanAgent::StaticClass();
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
    
    // 生成人类 Agent（通过 AgentSubsystem）
    for (int32 i = 0; i < NumHumans; i++)
    {
        float Angle = (360.f / TotalAgents) * AgentIndex;
        float Radius = 400.f;
        
        FVector SpawnLocation = PlayerStart + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        AgentSubsystem->SpawnAgent(HumanAgentClass, SpawnLocation, FRotator::ZeroRotator, AgentIndex, EAgentType::Human);
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
    
    UE_LOG(LogTemp, Log, TEXT("Spawned %d humans and %d robot dogs via AgentSubsystem"), NumHumans, NumRobotDogs);
}
