#include "MAPlayerController.h"
#include "MAGameMode.h"
#include "../AgentManager/MAAgentSubsystem.h"
#include "../AgentManager/MAAgent.h"
#include "../AgentManager/MARobotDogAgent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AIController.h"

AMAPlayerController::AMAPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AMAPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
}

void AMAPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    
    // 左键点击 - 移动玩家
    if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        OnLeftClick();
    }
    
    // 右键点击 - 移动所有 Agent
    if (WasInputKeyJustPressed(EKeys::RightMouseButton))
    {
        OnRightClick();
    }
    
    // ===== 测试按键 =====
    // T - 生成机器狗
    if (WasInputKeyJustPressed(EKeys::T))
    {
        OnSpawnRobotDog();
    }
    
    // Y - 打印 Agent 信息
    if (WasInputKeyJustPressed(EKeys::Y))
    {
        OnPrintAgentInfo();
    }
    
    // U - 销毁最后一个 Agent
    if (WasInputKeyJustPressed(EKeys::U))
    {
        OnDestroyLastAgent();
    }
}

void AMAPlayerController::OnLeftClick()
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitLocation);
    }
}

void AMAPlayerController::OnRightClick()
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        MoveAllAgentsToLocation(HitLocation);
    }
}

void AMAPlayerController::MoveAllAgentsToLocation(FVector Destination)
{
    // 所有 Agent 统一由 AgentSubsystem 管理
    if (UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>())
    {
        AgentSubsystem->MoveAllAgentsTo(Destination, 150.f);
    }
}

bool AMAPlayerController::GetMouseHitLocation(FVector& OutLocation)
{
    FHitResult HitResult;
    if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        OutLocation = HitResult.Location;
        return true;
    }
    return false;
}

// ===== 测试按键实现 =====

void AMAPlayerController::OnSpawnRobotDog()
{
    UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>();
    if (!AgentSubsystem) return;
    
    // 在玩家前方生成
    FVector SpawnLocation = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 300.f;
    
    AMAAgent* NewAgent = AgentSubsystem->SpawnAgent(
        AMARobotDogAgent::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator,
        -1,  // 自动分配 ID
        EAgentType::RobotDog
    );
    
    if (NewAgent)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
            FString::Printf(TEXT("Spawned: %s at %s"), *NewAgent->AgentName, *SpawnLocation.ToString()));
    }
}

void AMAPlayerController::OnPrintAgentInfo()
{
    UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>();
    if (!AgentSubsystem) return;
    
    int32 Total = AgentSubsystem->GetAgentCount();
    int32 Dogs = AgentSubsystem->GetAgentsByType(EAgentType::RobotDog).Num();
    int32 Humans = AgentSubsystem->GetAgentsByType(EAgentType::Human).Num();
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("=== Agent Info ===\nTotal: %d\nRobotDogs: %d\nHumans: %d"), Total, Dogs, Humans));
    
    // 打印每个 Agent 的详细信息
    for (AMAAgent* Agent : AgentSubsystem->GetAllAgents())
    {
        if (Agent)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("  [%d] %s at %s"), Agent->AgentID, *Agent->AgentName, *Agent->GetActorLocation().ToString()));
        }
    }
}

void AMAPlayerController::OnDestroyLastAgent()
{
    UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>();
    if (!AgentSubsystem) return;
    
    TArray<AMAAgent*> AllAgents = AgentSubsystem->GetAllAgents();
    if (AllAgents.Num() > 0)
    {
        AMAAgent* LastAgent = AllAgents.Last();
        FString Name = LastAgent->AgentName;
        
        if (AgentSubsystem->DestroyAgent(LastAgent))
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, 
                FString::Printf(TEXT("Destroyed: %s"), *Name));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No agents to destroy!"));
    }
}
