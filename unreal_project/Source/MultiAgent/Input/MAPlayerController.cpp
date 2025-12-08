// MAPlayerController.cpp
// 使用 Enhanced Input System - 自动配置，无需手动创建资产

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/MAAgentManager.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.h"
#include "MADroneCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "MAPickupItem.h"
#include "MAPatrolPath.h"
#include "MACoverageArea.h"
#include "MAAbilitySystemComponent.h"
#include "GA_Formation.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"

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
    
    // 设置输入模式
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    // 添加默认输入映射上下文
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        UMAInputActions* InputActions = UMAInputActions::Get();
        if (InputActions && InputActions->DefaultMappingContext)
        {
            Subsystem->AddMappingContext(InputActions->DefaultMappingContext, 0);
            UE_LOG(LogTemp, Log, TEXT("[Input] Added DefaultMappingContext"));
        }
    }
}

void AMAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UMAInputActions* InputActions = UMAInputActions::Get();
    if (!InputActions)
    {
        UE_LOG(LogTemp, Error, TEXT("[Input] Failed to get MAInputActions!"));
        return;
    }

    // 绑定 Enhanced Input
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 鼠标点击 - Started 只触发一次
        EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Started, this, &AMAPlayerController::OnLeftClick);
        EIC->BindAction(InputActions->IA_RightClick, ETriggerEvent::Started, this, &AMAPlayerController::OnRightClick);

        // GAS 技能
        EIC->BindAction(InputActions->IA_Pickup, ETriggerEvent::Started, this, &AMAPlayerController::OnPickup);
        EIC->BindAction(InputActions->IA_Drop, ETriggerEvent::Started, this, &AMAPlayerController::OnDrop);

        // 生成
        EIC->BindAction(InputActions->IA_SpawnItem, ETriggerEvent::Started, this, &AMAPlayerController::OnSpawnPickupItem);
        EIC->BindAction(InputActions->IA_SpawnRobotDog, ETriggerEvent::Started, this, &AMAPlayerController::OnSpawnRobotDog);

        // 调试
        EIC->BindAction(InputActions->IA_PrintInfo, ETriggerEvent::Started, this, &AMAPlayerController::OnPrintAgentInfo);
        EIC->BindAction(InputActions->IA_DestroyLast, ETriggerEvent::Started, this, &AMAPlayerController::OnDestroyLastAgent);

        // 视角切换
        EIC->BindAction(InputActions->IA_SwitchCamera, ETriggerEvent::Started, this, &AMAPlayerController::OnSwitchCamera);
        EIC->BindAction(InputActions->IA_ReturnSpectator, ETriggerEvent::Started, this, &AMAPlayerController::OnReturnToSpectator);

        // 巡逻
        EIC->BindAction(InputActions->IA_StartPatrol, ETriggerEvent::Started, this, &AMAPlayerController::OnStartPatrol);
        
        // 充电
        EIC->BindAction(InputActions->IA_StartCharge, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCharge);

        // 停止/空闲
        EIC->BindAction(InputActions->IA_StopIdle, ETriggerEvent::Started, this, &AMAPlayerController::OnStopIdle);

        // 区域覆盖
        EIC->BindAction(InputActions->IA_StartCoverage, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCoverage);

        // 跟随
        EIC->BindAction(InputActions->IA_StartFollow, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFollow);

        // 避障
        EIC->BindAction(InputActions->IA_StartAvoid, ETriggerEvent::Started, this, &AMAPlayerController::OnStartAvoid);

        // 编队
        EIC->BindAction(InputActions->IA_StartFormation, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFormation);

        // 拍照 (CARLA 风格 - 直接调用 Camera Sensor)
        EIC->BindAction(InputActions->IA_TakePhoto, ETriggerEvent::Started, this, &AMAPlayerController::OnTakePhoto);

        // 录像
        EIC->BindAction(InputActions->IA_ToggleRecording, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleRecording);

        // TCP 流
        EIC->BindAction(InputActions->IA_ToggleTCPStream, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleTCPStream);

        UE_LOG(LogTemp, Log, TEXT("[Input] Bound all input actions"));
    }
}


// ========== Input Handlers ==========

void AMAPlayerController::OnLeftClick(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        if (UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>())
        {
            TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
            for (AMACharacter* Agent : Humans)
            {
                if (Agent)
                {
                    Agent->TryNavigateTo(HitLocation);
                }
            }
        }
    }
}

void AMAPlayerController::OnRightClick(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        if (UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>())
        {
            // RobotDog 导航
            TArray<AMACharacter*> RobotDogs = AgentManager->GetAgentsByType(EMAAgentType::RobotDog);
            for (AMACharacter* Agent : RobotDogs)
            {
                if (Agent && !Agent->AgentName.Contains(TEXT("Tracker")))
                {
                    Agent->TryNavigateTo(HitLocation);
                }
            }
            
            // Drone 导航 (保持当前高度，只改变 XY)
            TArray<AMACharacter*> Drones = AgentManager->GetAllDrones();
            for (AMACharacter* Agent : Drones)
            {
                if (Agent)
                {
                    FVector DroneTarget = HitLocation;
                    DroneTarget.Z = Agent->GetActorLocation().Z;  // 保持当前高度
                    Agent->TryNavigateTo(DroneTarget);
                }
            }
        }
    }
}

void AMAPlayerController::OnPickup(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
    for (AMACharacter* Agent : Humans)
    {
        if (Agent && !Agent->IsHoldingItem())
        {
            if (Agent->TryPickup())
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
                    FString::Printf(TEXT("%s picking up..."), *Agent->AgentName));
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
                    FString::Printf(TEXT("%s: No item nearby"), *Agent->AgentName));
            }
        }
    }
}

void AMAPlayerController::OnDrop(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
    for (AMACharacter* Agent : Humans)
    {
        if (Agent && Agent->IsHoldingItem())
        {
            Agent->TryDrop();
        }
    }
}

void AMAPlayerController::OnSpawnPickupItem(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        HitLocation.Z += 50.f;
        
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
                FString::Printf(TEXT("Spawned: %s"), *Item->ItemName));
        }
    }
}

void AMAPlayerController::OnSpawnRobotDog(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
    FVector SpawnLocation = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 300.f;
    
    static int32 SpawnCounter = 0;
    FString AgentID = FString::Printf(TEXT("robot_spawned_%d"), SpawnCounter++);
    
    AMACharacter* NewAgent = AgentManager->SpawnAgent(
        AMARobotDogCharacter::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator,
        AgentID,
        EMAAgentType::RobotDog
    );
    
    if (NewAgent)
    {
        // 添加 Camera Sensor
        NewAgent->AddCameraSensor(FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
        
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
            FString::Printf(TEXT("Spawned: %s"), *NewAgent->AgentName));
    }
}

void AMAPlayerController::OnPrintAgentInfo(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
    int32 Total = AgentManager->GetAgentCount();
    int32 Dogs = AgentManager->GetAgentsByType(EMAAgentType::RobotDog).Num();
    int32 Humans = AgentManager->GetAgentsByType(EMAAgentType::Human).Num();
    
    // 统计所有 Agent 上的 Sensor 数量
    int32 TotalSensors = 0;
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            TotalSensors += Agent->GetSensorCount();
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("=== Agents: %d | Dogs: %d | Humans: %d | Sensors: %d ==="), 
            Total, Dogs, Humans, TotalSensors));
    
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            FString HoldingInfo = Agent->IsHoldingItem() ? TEXT(" [Holding]") : TEXT("");
            FString SensorInfo = Agent->GetSensorCount() > 0 ? FString::Printf(TEXT(" [%d sensors]"), Agent->GetSensorCount()) : TEXT("");
            
            // 显示可用 Actions
            TArray<FString> Actions = Agent->GetAvailableActions();
            FString ActionsInfo = Actions.Num() > 0 ? FString::Printf(TEXT(" [%d actions]"), Actions.Num()) : TEXT("");
            
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("  [%s] %s%s%s%s"), *Agent->AgentID, *Agent->AgentName, *HoldingInfo, *SensorInfo, *ActionsInfo));
        }
    }
}

void AMAPlayerController::OnDestroyLastAgent(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
    TArray<AMACharacter*> AllAgents = AgentManager->GetAllAgents();
    if (AllAgents.Num() > 0)
    {
        AMACharacter* LastAgent = AllAgents.Last();
        FString Name = LastAgent->AgentName;
        
        if (AgentManager->DestroyAgent(LastAgent))
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

void AMAPlayerController::OnSwitchCamera(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
    // 收集所有 Agent 上的 Camera Component
    TArray<UMACameraSensorComponent*> Cameras;
    TArray<AMACharacter*> CameraOwners;
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            if (UMACameraSensorComponent* Camera = Agent->GetCameraSensor())
            {
                Cameras.Add(Camera);
                CameraOwners.Add(Agent);
            }
        }
    }
    
    if (Cameras.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No cameras!"));
        return;
    }
    
    if (!OriginalPawn && GetPawn())
    {
        OriginalPawn = GetPawn();
    }
    
    CurrentCameraIndex++;
    if (CurrentCameraIndex >= Cameras.Num())
    {
        CurrentCameraIndex = 0;
    }
    
    // 切换到 Agent 的视角（Camera Component 附着在 Agent 上）
    AMACharacter* TargetAgent = CameraOwners[CurrentCameraIndex];
    UMACameraSensorComponent* Camera = Cameras[CurrentCameraIndex];
    if (TargetAgent && Camera)
    {
        SetViewTargetWithBlend(TargetAgent, 0.3f);
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Camera: %s (%d/%d)"), *Camera->SensorName, CurrentCameraIndex + 1, Cameras.Num()));
    }
}

void AMAPlayerController::OnReturnToSpectator(const FInputActionValue& Value)
{
    if (OriginalPawn)
    {
        SetViewTargetWithBlend(OriginalPawn, 0.3f);
        CurrentCameraIndex = -1;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Spectator view"));
    }
    else if (GetPawn())
    {
        SetViewTargetWithBlend(GetPawn(), 0.3f);
        CurrentCameraIndex = -1;
    }
}


void AMAPlayerController::OnStartPatrol(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartPatrol called (G key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 查找场景中的 PatrolPath
    TArray<AActor*> PatrolPaths;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMAPatrolPath::StaticClass(), PatrolPaths);
    AMAPatrolPath* FirstPatrolPath = PatrolPaths.Num() > 0 ? Cast<AMAPatrolPath>(PatrolPaths[0]) : nullptr;
    
    if (!FirstPatrolPath)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No PatrolPath in scene!"));
        return;
    }
    
    // 收集所有 RobotDog 和 Drone
    TArray<AMACharacter*> Agents;
    Agents.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    Agents.Append(AgentManager->GetAllDrones());
    
    int32 CommandCount = 0;
    FGameplayTag PatrolCommand = FGameplayTag::RequestGameplayTag(FName("Command.Patrol"));
    
    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        if (Agent->AgentName.Contains(TEXT("Tracker"))) continue;
        
        // 设置 PatrolPath - 支持 RobotDog 和 Drone
        if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Agent))
        {
            Robot->SetPatrolPath(FirstPatrolPath);
        }
        // Drone 暂不支持 PatrolPath，但可以接收 Command
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 清除其他命令，添加 Patrol 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
        ASC->AddLooseGameplayTag(PatrolCommand);
        
        CommandCount++;
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] %s: Command.Patrol set, PatrolPath=%s"),
            *Agent->AgentName, *FirstPatrolPath->GetName());
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("%s: Patrol started"), *Agent->AgentName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog/Drone found!"));
    }
}

void AMAPlayerController::OnStartCharge(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartCharge called (H key) - Sending Command.Charge to StateTree"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 收集所有 RobotDog 和 Drone
    TArray<AMACharacter*> Agents;
    Agents.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    Agents.Append(AgentManager->GetAllDrones());
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Found %d RobotDogs/Drones"), Agents.Num());
    
    int32 CommandCount = 0;
    FGameplayTag ChargeCommand = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
    
    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        
        // 排除 Tracker（Follow 机器人）
        if (Agent->AgentName.Contains(TEXT("Tracker"))) continue;
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
        if (!ASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[PlayerController] %s has no ASC!"), *Agent->AgentName);
            continue;
        }
        
        // 先清除其他命令，再添加 Charge 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
        ASC->AddLooseGameplayTag(ChargeCommand);
        
        CommandCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("%s: Charge started"), *Agent->AgentName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog/Drone found!"));
    }
}

void AMAPlayerController::OnStopIdle(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStopIdle called (J key) - Sending Command.Idle to StateTree"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 收集所有 RobotDog 和 Drone
    TArray<AMACharacter*> Agents;
    Agents.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    Agents.Append(AgentManager->GetAllDrones());
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Found %d RobotDogs/Drones"), Agents.Num());
    
    int32 CommandCount = 0;
    FGameplayTag IdleCommand = FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
    
    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        
        // 排除 Tracker（Follow 机器人）
        if (Agent->AgentName.Contains(TEXT("Tracker"))) continue;
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
        if (!ASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[PlayerController] %s has no ASC!"), *Agent->AgentName);
            continue;
        }
        
        // 先清除其他命令，再添加 Idle 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
        ASC->AddLooseGameplayTag(IdleCommand);
        
        // 取消所有直接激活的 Ability（不受 StateTree 控制的）
        ASC->CancelAvoid();
        ASC->CancelNavigate();
        
        CommandCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White,
            FString::Printf(TEXT("%s: Idle"), *Agent->AgentName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog/Drone found!"));
    }
}

void AMAPlayerController::OnStartCoverage(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartCoverage called (K key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 查找场景中的 CoverageArea
    TArray<AActor*> CoverageAreas;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMACoverageArea::StaticClass(), CoverageAreas);
    AActor* FirstCoverageArea = CoverageAreas.Num() > 0 ? CoverageAreas[0] : nullptr;
    
    if (!FirstCoverageArea)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No CoverageArea in scene!"));
        return;
    }
    
    // 收集所有 RobotDog 和 Drone
    TArray<AMACharacter*> Agents;
    Agents.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    Agents.Append(AgentManager->GetAllDrones());
    
    int32 CommandCount = 0;
    FGameplayTag CoverageCommand = FGameplayTag::RequestGameplayTag(FName("Command.Coverage"));
    
    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        if (Agent->AgentName.Contains(TEXT("Tracker"))) continue;
        
        // 设置 CoverageArea - 支持 RobotDog (Drone 暂不支持 CoverageArea 属性)
        if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Agent))
        {
            Robot->SetCoverageArea(FirstCoverageArea);
        }
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 清除其他命令，添加 Coverage 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
        ASC->AddLooseGameplayTag(CoverageCommand);
        
        CommandCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
            FString::Printf(TEXT("%s: Coverage started"), *Agent->AgentName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog/Drone found!"));
    }
}

void AMAPlayerController::OnStartFollow(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartFollow called (F key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 查找场景中的 Human 作为跟随目标
    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
    AMACharacter* FirstHuman = Humans.Num() > 0 ? Humans[0] : nullptr;
    
    if (!FirstHuman)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No Human in scene to follow!"));
        return;
    }
    
    // 收集所有 RobotDog 和 Drone
    TArray<AMACharacter*> Agents;
    Agents.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    Agents.Append(AgentManager->GetAllDrones());
    
    int32 CommandCount = 0;
    FGameplayTag FollowCommand = FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
    
    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        
        // 设置 FollowTarget - 支持 RobotDog 和 Drone
        if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Agent))
        {
            Robot->SetFollowTarget(FirstHuman);
        }
        else if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(Agent))
        {
            Drone->SetFollowTarget(FirstHuman);
        }
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 清除其他命令，添加 Follow 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
        ASC->AddLooseGameplayTag(FollowCommand);
        
        CommandCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
            FString::Printf(TEXT("%s: Following %s"), *Agent->AgentName, *FirstHuman->AgentName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog/Drone found!"));
    }
}

void AMAPlayerController::OnStartAvoid(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartAvoid called (A key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 获取鼠标点击位置作为目标
    FVector TargetLocation;
    if (!GetMouseHitLocation(TargetLocation))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Click on ground to set avoid target!"));
        return;
    }
    
    // 收集所有 RobotDog 和 Drone
    TArray<AMACharacter*> Agents;
    Agents.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    Agents.Append(AgentManager->GetAllDrones());
    
    int32 ActivatedCount = 0;
    
    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        if (Agent->AgentName.Contains(TEXT("Tracker"))) continue;
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // Drone 保持高度
        FVector FinalTarget = TargetLocation;
        if (Agent->AgentType == EMAAgentType::Drone ||
            Agent->AgentType == EMAAgentType::DronePhantom4 ||
            Agent->AgentType == EMAAgentType::DroneInspire2)
        {
            FinalTarget.Z = Agent->GetActorLocation().Z;
        }
        
        // 激活 Avoid 技能
        if (ASC->TryActivateAvoid(FinalTarget))
        {
            ActivatedCount++;
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
                FString::Printf(TEXT("%s: Avoid activated"), *Agent->AgentName));
        }
    }
    
    if (ActivatedCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog/Drone found or Avoid failed!"));
    }
}

void AMAPlayerController::OnStartFormation(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartFormation called (B key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 查找 Human 作为 Leader
    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
    AMACharacter* Leader = Humans.Num() > 0 ? Humans[0] : nullptr;
    
    if (!Leader)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No Human found as Leader!"));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No Human found as Leader!"));
        return;
    }
    
    // 循环切换编队类型 (5 种 + 停止)
    CurrentFormationIndex = (CurrentFormationIndex + 1) % 6;
    
    // 如果是 0，停止编队
    if (CurrentFormationIndex == 0)
    {
        AgentManager->StopFormation();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, TEXT("Formation stopped"));
        return;
    }
    
    // 转换为 EMAFormationType (1-5 对应 Line-Circle)
    EMAFormationType FormationType = static_cast<EMAFormationType>(CurrentFormationIndex);
    
    // 编队名称
    FString FormationName;
    switch (FormationType)
    {
        case EMAFormationType::Line: FormationName = TEXT("Line"); break;
        case EMAFormationType::Column: FormationName = TEXT("Column"); break;
        case EMAFormationType::Wedge: FormationName = TEXT("Wedge"); break;
        case EMAFormationType::Diamond: FormationName = TEXT("Diamond"); break;
        case EMAFormationType::Circle: FormationName = TEXT("Circle"); break;
        default: FormationName = TEXT("Unknown"); break;
    }
    
    // 使用 AgentManager 统一管理编队
    AgentManager->StartFormation(Leader, FormationType);
    
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("%s Formation started, Leader: %s"), *FormationName, *Leader->AgentName));
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

void AMAPlayerController::OnTakePhoto(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnTakePhoto called (L key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No AgentManager!"));
        return;
    }
    
    // 收集所有 Agent 上的 Camera Component
    TArray<UMACameraSensorComponent*> Cameras;
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            if (UMACameraSensorComponent* Camera = Agent->GetCameraSensor())
            {
                Cameras.Add(Camera);
            }
        }
    }
    
    if (Cameras.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No cameras found!"));
        return;
    }
    
    // 根据 CurrentCameraIndex 选择相机 (与 Tab 切换一致)
    UMACameraSensorComponent* TargetCamera = nullptr;
    if (CurrentCameraIndex < 0 || CurrentCameraIndex >= Cameras.Num())
    {
        // 未选择相机时，使用第一个
        TargetCamera = Cameras[0];
    }
    else
    {
        TargetCamera = Cameras[CurrentCameraIndex];
    }
    
    if (TargetCamera && TargetCamera->TakePhoto())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("%s: Photo saved"), *TargetCamera->SensorName));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Failed to take photo!"));
    }
}

void AMAPlayerController::OnToggleRecording(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleRecording called (R key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        return;
    }
    
    // 收集所有 Agent 上的 Camera Component
    TArray<UMACameraSensorComponent*> Cameras;
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            if (UMACameraSensorComponent* Camera = Agent->GetCameraSensor())
            {
                Cameras.Add(Camera);
            }
        }
    }
    
    if (Cameras.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No cameras found!"));
        return;
    }
    
    // 根据 CurrentCameraIndex 选择相机
    UMACameraSensorComponent* TargetCamera = nullptr;
    if (CurrentCameraIndex < 0 || CurrentCameraIndex >= Cameras.Num())
    {
        TargetCamera = Cameras[0];
    }
    else
    {
        TargetCamera = Cameras[CurrentCameraIndex];
    }
    
    if (!TargetCamera)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No active camera!"));
        return;
    }
    
    // 切换当前相机的录像状态
    if (TargetCamera->bIsRecording)
    {
        TargetCamera->StopRecording();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("%s: Recording stopped"), *TargetCamera->SensorName));
    }
    else
    {
        // 使用相机自身的 StreamFPS 设置
        TargetCamera->StartRecording(TargetCamera->StreamFPS);
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            FString::Printf(TEXT("%s: Recording started at %.0f FPS (Saved/Recordings/)"), *TargetCamera->SensorName, TargetCamera->StreamFPS));
    }
}

void AMAPlayerController::OnToggleTCPStream(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleTCPStream called (V key)"));
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        return;
    }
    
    // 收集所有 Agent 上的 Camera Component
    TArray<UMACameraSensorComponent*> Cameras;
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            if (UMACameraSensorComponent* Camera = Agent->GetCameraSensor())
            {
                Cameras.Add(Camera);
            }
        }
    }
    
    if (Cameras.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No cameras found!"));
        return;
    }
    
    // 根据 CurrentCameraIndex 选择相机
    // -1 表示上帝视角，使用第一个相机
    UMACameraSensorComponent* TargetCamera = nullptr;
    if (CurrentCameraIndex < 0 || CurrentCameraIndex >= Cameras.Num())
    {
        TargetCamera = Cameras[0];
    }
    else
    {
        TargetCamera = Cameras[CurrentCameraIndex];
    }
    
    if (!TargetCamera)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No active camera!"));
        return;
    }
    
    // 先停止所有其他相机的流
    for (UMACameraSensorComponent* Camera : Cameras)
    {
        if (Camera != TargetCamera && Camera->bIsStreaming)
        {
            Camera->StopTCPStream();
        }
    }
    
    // 切换当前相机的流状态
    if (TargetCamera->bIsStreaming)
    {
        TargetCamera->StopTCPStream();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
            FString::Printf(TEXT("%s: TCP stream stopped"), *TargetCamera->SensorName));
    }
    else
    {
        // 使用相机自身的 StreamFPS 设置
        if (TargetCamera->StartTCPStream(9000, TargetCamera->StreamFPS))
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                FString::Printf(TEXT("%s: TCP stream started on port 9000"), *TargetCamera->SensorName));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
                FString::Printf(TEXT("%s: Failed to start TCP stream"), *TargetCamera->SensorName));
        }
    }
}
