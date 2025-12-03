#include "MAPlayerController.h"
#include "MAGameMode.h"
#include "../AgentManager/MAAgentSubsystem.h"
#include "../AgentManager/MAAgent.h"
#include "../AgentManager/MARobotDogAgent.h"
#include "../Interaction/MAPickupItem.h"
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

    // ===== GAS 测试按键 =====
    // P - 拾取物品
    if (WasInputKeyJustPressed(EKeys::P))
    {
        OnPickup();
    }
    
    // O - 放下物品
    if (WasInputKeyJustPressed(EKeys::O))
    {
        OnDrop();
    }
    
    // I - 生成可拾取方块
    if (WasInputKeyJustPressed(EKeys::I))
    {
        OnSpawnPickupItem();
    }
}

void AMAPlayerController::OnLeftClick()
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        // 左键：移动所有 Human Agent 到目标位置
        if (UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>())
        {
            TArray<AMAAgent*> Humans = AgentSubsystem->GetAgentsByType(EAgentType::Human);
            for (AMAAgent* Agent : Humans)
            {
                if (Agent)
                {
                    Agent->MoveToLocation(HitLocation);
                }
            }
        }
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
            FString HoldingInfo = Agent->IsHoldingItem() ? TEXT(" [Holding Item]") : TEXT("");
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("  [%d] %s at %s%s"), 
                    Agent->AgentID, *Agent->AgentName, *Agent->GetActorLocation().ToString(), *HoldingInfo));
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

// ===== GAS 测试按键实现 =====

void AMAPlayerController::OnPickup()
{
    UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>();
    if (!AgentSubsystem) return;

    // 让所有 Human Agent 尝试拾取
    TArray<AMAAgent*> Humans = AgentSubsystem->GetAgentsByType(EAgentType::Human);
    for (AMAAgent* Agent : Humans)
    {
        if (Agent && !Agent->IsHoldingItem())
        {
            if (Agent->TryPickup())
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
                    FString::Printf(TEXT("%s trying to pickup..."), *Agent->AgentName));
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
                    FString::Printf(TEXT("%s: No item nearby to pickup"), *Agent->AgentName));
            }
        }
    }
}

void AMAPlayerController::OnDrop()
{
    UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>();
    if (!AgentSubsystem) return;

    // 让所有持有物品的 Human Agent 放下物品
    TArray<AMAAgent*> Humans = AgentSubsystem->GetAgentsByType(EAgentType::Human);
    for (AMAAgent* Agent : Humans)
    {
        if (Agent && Agent->IsHoldingItem())
        {
            Agent->TryDrop();
        }
    }
}

void AMAPlayerController::OnSpawnPickupItem()
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        // 在鼠标位置生成可拾取方块
        HitLocation.Z += 50.f; // 稍微抬高
        
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        AMAPickupItem* Item = GetWorld()->SpawnActor<AMAPickupItem>(
            AMAPickupItem::StaticClass(),
            HitLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );
        
        if (Item)
        {
            static int32 ItemCounter = 0;
            Item->ItemName = FString::Printf(TEXT("Cube_%d"), ItemCounter++);
            Item->ItemID = ItemCounter;
            
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                FString::Printf(TEXT("Spawned: %s at %s"), *Item->ItemName, *HitLocation.ToString()));
        }
    }
}
