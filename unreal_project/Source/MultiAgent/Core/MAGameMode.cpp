#include "MAGameMode.h"
#include "MAPlayerController.h"
#include "../AgentManager/MAAgentSubsystem.h"
#include "../AgentManager/MAHumanAgent.h"
#include "../AgentManager/MARobotDogAgent.h"
#include "../AgentManager/MACameraAgent.h"
#include "GameFramework/SpectatorPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AMAGameMode::AMAGameMode()
{
    // 玩家使用 SpectatorPawn 作为上帝视角（原生飞行控制）
    DefaultPawnClass = ASpectatorPawn::StaticClass();
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

    // 使用 PlayerStart 位置作为参考点
    FVector PlayerStart = FVector(0.f, 0.f, 100.f);
    AActor* PlayerStartActor = FindPlayerStart(nullptr);
    if (PlayerStartActor)
    {
        PlayerStart = PlayerStartActor->GetActorLocation();
    }
    
    // 获取导航系统
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    
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
        
        // 投影到 NavMesh 上确保位置有效
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
            {
                SpawnLocation = NavLocation.Location;
            }
        }
        
        AMAAgent* Human = AgentSubsystem->SpawnAgent(HumanAgentClass, SpawnLocation, FRotator::ZeroRotator, AgentIndex, EAgentType::Human);
        
        // 为 Human 添加第三人称摄像头（后方、稍高、向下看，类似 GTA）
        if (Human)
        {
            // X=-200 后方, Z=100 高于肩膀, Pitch=-10 向下看
            SpawnAndAttachCamera(AgentSubsystem, Human, FVector(-200.f, 0.f, 100.f), FRotator(-10.f, 0.f, 0.f));
        }
        
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
        
        // 投影到 NavMesh 上确保位置有效
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
            {
                SpawnLocation = NavLocation.Location;
            }
        }
        
        AMAAgent* RobotDog = AgentSubsystem->SpawnAgent(RobotDogAgentClass, SpawnLocation, FRotator::ZeroRotator, AgentIndex, EAgentType::RobotDog);
        
        // 为 RobotDog 添加第三人称摄像头（后方、稍高、向下看）
        if (RobotDog)
        {
            // X=-150 后方, Z=80 高于背部, Pitch=-15 向下看
            SpawnAndAttachCamera(AgentSubsystem, RobotDog, FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
        }
        
        AgentIndex++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Spawned %d humans and %d robot dogs with cameras via AgentSubsystem"), NumHumans, NumRobotDogs);
    
    // 生成一个追踪者 RobotDog，自动追踪第一个 Human
    SpawnTrackerAgent(AgentSubsystem);
}

void AMAGameMode::SpawnTrackerAgent(UMAAgentSubsystem* AgentSubsystem)
{
    if (!AgentSubsystem) return;
    
    // 获取第一个 Human 作为追踪目标
    TArray<AMAAgent*> Humans = AgentSubsystem->GetAgentsByType(EAgentType::Human);
    if (Humans.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] No Human to track!"));
        return;
    }
    
    AMAAgent* TargetHuman = Humans[0];
    
    // 在目标后方生成追踪者
    FVector SpawnLocation = TargetHuman->GetActorLocation() - TargetHuman->GetActorForwardVector() * 500.f;
    
    // 投影到 NavMesh
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation NavLocation;
        if (NavSys->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
        {
            SpawnLocation = NavLocation.Location;
        }
    }
    
    // 生成追踪者
    TrackerAgent = Cast<AMAAgent>(AgentSubsystem->SpawnAgent(
        RobotDogAgentClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        100,  // 特殊 ID
        EAgentType::RobotDog
    ));
    
    if (TrackerAgent)
    {
        TrackerAgent->AgentName = TEXT("Tracker_Dog");
        
        // 开始追踪第一个 Human
        TrackerAgent->TryFollowAgent(TargetHuman, 300.f);
        
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Spawned Tracker_Dog, following %s"), *TargetHuman->AgentName);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta,
                FString::Printf(TEXT("Tracker_Dog spawned, following %s"), *TargetHuman->AgentName));
        }
    }
}

void AMAGameMode::SpawnAndAttachCamera(UMAAgentSubsystem* AgentSubsystem, AMAAgent* ParentAgent, FVector RelativeLocation, FRotator RelativeRotation)
{
    if (!AgentSubsystem || !ParentAgent)
    {
        return;
    }
    
    // 在父 Agent 位置生成摄像头
    AMACameraAgent* Camera = Cast<AMACameraAgent>(
        AgentSubsystem->SpawnAgent(AMACameraAgent::StaticClass(), ParentAgent->GetActorLocation(), ParentAgent->GetActorRotation(), -1, EAgentType::Camera)
    );
    
    if (Camera)
    {
        // 附着到父 Agent
        Camera->AttachToAgent(ParentAgent, RelativeLocation, RelativeRotation);
        
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Attached camera %s to %s at offset (%.0f, %.0f, %.0f)"),
            *Camera->AgentName, *ParentAgent->AgentName, RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z);
    }
}
