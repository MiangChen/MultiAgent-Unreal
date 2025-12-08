// MAPlayerController.cpp (重构版)
// 使用 Enhanced Input System + MACommandManager
// 支持星际争霸风格的框选和编组

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/MAAgentManager.h"
#include "../Core/MACommandManager.h"
#include "../Core/MASquadManager.h"
#include "../Core/MASquad.h"
#include "../Core/MASelectionManager.h"
#include "../UI/MASelectionHUD.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "MAPickupItem.h"
#include "MAPatrolPath.h"
#include "MACoverageArea.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Blueprint/WidgetLayoutLibrary.h"

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
    
    // 初始为 Select 模式，禁用视角控制
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    SetIgnoreLookInput(true);  // Select 模式禁用视角旋转

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        UMAInputActions* InputActions = UMAInputActions::Get();
        if (InputActions && InputActions->DefaultMappingContext)
        {
            Subsystem->AddMappingContext(InputActions->DefaultMappingContext, 0);
        }
    }
}

void AMAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UMAInputActions* InputActions = UMAInputActions::Get();
    if (!InputActions) return;

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 鼠标点击 (框选)
        EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Started, this, &AMAPlayerController::OnLeftClick);
        EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Completed, this, &AMAPlayerController::OnLeftClickReleased);
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

        // 命令 (通过 CommandManager)
        EIC->BindAction(InputActions->IA_StartPatrol, ETriggerEvent::Started, this, &AMAPlayerController::OnStartPatrol);
        EIC->BindAction(InputActions->IA_StartCharge, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCharge);
        EIC->BindAction(InputActions->IA_StopIdle, ETriggerEvent::Started, this, &AMAPlayerController::OnStopIdle);
        EIC->BindAction(InputActions->IA_StartCoverage, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCoverage);
        EIC->BindAction(InputActions->IA_StartFollow, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFollow);
        EIC->BindAction(InputActions->IA_StartAvoid, ETriggerEvent::Started, this, &AMAPlayerController::OnStartAvoid);
        EIC->BindAction(InputActions->IA_StartFormation, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFormation);

        // 相机
        EIC->BindAction(InputActions->IA_TakePhoto, ETriggerEvent::Started, this, &AMAPlayerController::OnTakePhoto);
        EIC->BindAction(InputActions->IA_ToggleRecording, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleRecording);
        EIC->BindAction(InputActions->IA_ToggleTCPStream, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleTCPStream);

        // 编组快捷键 (星际争霸风格: 1~5 选择, Ctrl+1~5 保存)
        EIC->BindAction(InputActions->IA_ControlGroup1, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup1);
        EIC->BindAction(InputActions->IA_ControlGroup2, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup2);
        EIC->BindAction(InputActions->IA_ControlGroup3, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup3);
        EIC->BindAction(InputActions->IA_ControlGroup4, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup4);
        EIC->BindAction(InputActions->IA_ControlGroup5, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup5);

        // 创建/解散 Squad
        EIC->BindAction(InputActions->IA_CreateSquad, ETriggerEvent::Started, this, &AMAPlayerController::OnCreateSquad);
        EIC->BindAction(InputActions->IA_DisbandSquad, ETriggerEvent::Started, this, &AMAPlayerController::OnDisbandSquad);

        // 鼠标模式切换
        EIC->BindAction(InputActions->IA_ToggleMouseMode, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleMouseMode);
    }
}

// ========== 辅助方法 ==========

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

// ========== 框选和选择 ==========

void AMAPlayerController::OnLeftClick(const FInputActionValue& Value)
{
    if (CurrentMouseMode == EMAMouseMode::Select)
    {
        // Select 模式：开始框选
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            bIsBoxSelecting = true;
            BoxSelectStart = FVector2D(MouseX, MouseY);
            BoxSelectEnd = BoxSelectStart;
        }
    }
    else // Navigate 模式
    {
        // Human 导航到点击位置
        FVector HitLocation;
        if (GetMouseHitLocation(HitLocation))
        {
            UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
            if (AgentManager)
            {
                for (AMACharacter* Agent : AgentManager->GetAgentsByType(EMAAgentType::Human))
                {
                    if (Agent) Agent->TryNavigateTo(HitLocation);
                }
            }
        }
    }
}

void AMAPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
    if (!bIsBoxSelecting) return;
    bIsBoxSelecting = false;

    UMASelectionManager* SelectionManager = GetWorld()->GetSubsystem<UMASelectionManager>();
    if (!SelectionManager) return;

    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        BoxSelectEnd = FVector2D(MouseX, MouseY);
    }

    // 计算框选大小
    float BoxWidth = FMath::Abs(BoxSelectEnd.X - BoxSelectStart.X);
    float BoxHeight = FMath::Abs(BoxSelectEnd.Y - BoxSelectStart.Y);

    if (BoxWidth < 10.f && BoxHeight < 10.f)
    {
        // 点击选择：检查是否点击了 Agent
        FHitResult HitResult;
        if (GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
        {
            if (AMACharacter* Agent = Cast<AMACharacter>(HitResult.GetActor()))
            {
                // Ctrl 按住时添加到选择，否则单选
                if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl))
                {
                    SelectionManager->ToggleSelection(Agent);
                }
                else
                {
                    SelectionManager->SelectAgent(Agent);
                }
                return;
            }
        }
        
        // 点击空地，清除选择
        if (!IsInputKeyDown(EKeys::LeftControl) && !IsInputKeyDown(EKeys::RightControl))
        {
            SelectionManager->ClearSelection();
        }
    }
    else
    {
        // 框选
        TArray<AMACharacter*> AgentsInBox = SelectionManager->GetAgentsInScreenRect(
            BoxSelectStart, BoxSelectEnd, this);

        if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl))
        {
            SelectionManager->AddAgentsToSelection(AgentsInBox);
        }
        else
        {
            SelectionManager->SelectAgents(AgentsInBox);
        }
    }
}

void AMAPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新框选终点
    if (bIsBoxSelecting)
    {
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            BoxSelectEnd = FVector2D(MouseX, MouseY);
        }
    }

    // 更新 HUD 的框选状态
    if (AMASelectionHUD* SelectionHUD = Cast<AMASelectionHUD>(GetHUD()))
    {
        SelectionHUD->bIsBoxSelecting = bIsBoxSelecting;
        SelectionHUD->BoxStart = BoxSelectStart;
        SelectionHUD->BoxEnd = BoxSelectEnd;
    }
}

void AMAPlayerController::OnRightClick(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (!GetMouseHitLocation(HitLocation)) return;

    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    if (!CommandManager) return;

    FMACommandParams Params;
    Params.TargetLocation = HitLocation;
    Params.bShowMessage = false;  // 导航不显示消息

    // RobotDog + Drone 导航到点击位置
    for (AMACharacter* Agent : CommandManager->GetControllableAgents())
    {
        FVector Target = HitLocation;
        // Drone 保持高度
        if (Agent->AgentType == EMAAgentType::Drone ||
            Agent->AgentType == EMAAgentType::DronePhantom4 ||
            Agent->AgentType == EMAAgentType::DroneInspire2)
        {
            Target.Z = Agent->GetActorLocation().Z;
        }
        Agent->TryNavigateTo(Target);
    }
}

// ========== 拾取/放下 ==========

void AMAPlayerController::OnPickup(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    for (AMACharacter* Agent : AgentManager->GetAgentsByType(EMAAgentType::Human))
    {
        if (Agent && !Agent->IsHoldingItem())
        {
            if (Agent->TryPickup())
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
                    FString::Printf(TEXT("%s: Picking up"), *Agent->AgentName));
            }
        }
    }
}

void AMAPlayerController::OnDrop(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    for (AMACharacter* Agent : AgentManager->GetAgentsByType(EMAAgentType::Human))
    {
        if (Agent && Agent->IsHoldingItem())
        {
            Agent->TryDrop();
        }
    }
}

// ========== 生成 ==========

void AMAPlayerController::OnSpawnPickupItem(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (!GetMouseHitLocation(HitLocation)) return;

    HitLocation.Z += 50.f;
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AMAPickupItem* Item = GetWorld()->SpawnActor<AMAPickupItem>(
        AMAPickupItem::StaticClass(), HitLocation, FRotator::ZeroRotator, SpawnParams);
    
    if (Item)
    {
        static int32 ItemCounter = 0;
        Item->ItemName = FString::Printf(TEXT("Cube_%d"), ItemCounter++);
        Item->ItemID = ItemCounter;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Spawned: %s"), *Item->ItemName));
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
        AMARobotDogCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, AgentID, EMAAgentType::RobotDog);
    
    if (NewAgent)
    {
        NewAgent->AddCameraSensor(FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Spawned: %s"), *NewAgent->AgentName));
    }
}

// ========== 调试 ==========

void AMAPlayerController::OnPrintAgentInfo(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
    int32 Total = AgentManager->GetAgentCount();
    int32 Dogs = AgentManager->GetAgentsByType(EMAAgentType::RobotDog).Num();
    int32 Humans = AgentManager->GetAgentsByType(EMAAgentType::Human).Num();
    
    int32 TotalSensors = 0;
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent) TotalSensors += Agent->GetSensorCount();
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("=== Agents: %d | Dogs: %d | Humans: %d | Sensors: %d ==="), Total, Dogs, Humans, TotalSensors));
    
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            FString Info = FString::Printf(TEXT("  [%s] %s"), *Agent->AgentID, *Agent->AgentName);
            if (Agent->IsHoldingItem()) Info += TEXT(" [Holding]");
            if (Agent->GetSensorCount() > 0) Info += FString::Printf(TEXT(" [%d sensors]"), Agent->GetSensorCount());
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Info);
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
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("Destroyed: %s"), *Name));
        }
    }
}

// ========== 视角切换 ==========

void AMAPlayerController::OnSwitchCamera(const FInputActionValue& Value)
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;
    
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
    
    if (Cameras.Num() == 0) return;
    
    if (!OriginalPawn && GetPawn()) OriginalPawn = GetPawn();
    
    CurrentCameraIndex = (CurrentCameraIndex + 1) % Cameras.Num();
    
    AMACharacter* TargetAgent = CameraOwners[CurrentCameraIndex];
    if (TargetAgent)
    {
        SetViewTargetWithBlend(TargetAgent, 0.3f);
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Camera: %s (%d/%d)"), *Cameras[CurrentCameraIndex]->SensorName, CurrentCameraIndex + 1, Cameras.Num()));
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
}

// ========== 命令 (通过 CommandManager) ==========

void AMAPlayerController::OnStartPatrol(const FInputActionValue& Value)
{
    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    if (!CommandManager) return;

    // 查找 PatrolPath
    TArray<AActor*> PatrolPaths;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMAPatrolPath::StaticClass(), PatrolPaths);
    
    if (PatrolPaths.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No PatrolPath in scene!"));
        return;
    }

    FMACommandParams Params;
    Params.PatrolPath = Cast<AMAPatrolPath>(PatrolPaths[0]);
    CommandManager->SendCommand(EMACommand::Patrol, Params);
}

void AMAPlayerController::OnStartCharge(const FInputActionValue& Value)
{
    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    if (CommandManager)
    {
        CommandManager->SendCommand(EMACommand::Charge);
    }
}

void AMAPlayerController::OnStopIdle(const FInputActionValue& Value)
{
    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    if (CommandManager)
    {
        CommandManager->SendCommand(EMACommand::Idle);
    }
}

void AMAPlayerController::OnStartCoverage(const FInputActionValue& Value)
{
    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    if (!CommandManager) return;

    TArray<AActor*> CoverageAreas;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMACoverageArea::StaticClass(), CoverageAreas);
    
    if (CoverageAreas.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No CoverageArea in scene!"));
        return;
    }

    FMACommandParams Params;
    Params.CoverageArea = CoverageAreas[0];
    CommandManager->SendCommand(EMACommand::Coverage, Params);
}

void AMAPlayerController::OnStartFollow(const FInputActionValue& Value)
{
    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!CommandManager || !AgentManager) return;

    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
    if (Humans.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No Human to follow!"));
        return;
    }

    FMACommandParams Params;
    Params.FollowTarget = Humans[0];
    CommandManager->SendCommand(EMACommand::Follow, Params);
}

void AMAPlayerController::OnStartAvoid(const FInputActionValue& Value)
{
    UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
    if (!CommandManager) return;

    FVector TargetLocation;
    if (!GetMouseHitLocation(TargetLocation))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Click on ground!"));
        return;
    }

    FMACommandParams Params;
    Params.TargetLocation = TargetLocation;
    CommandManager->SendCommand(EMACommand::Avoid, Params);
}

void AMAPlayerController::OnStartFormation(const FInputActionValue& Value)
{
    UMASquadManager* SquadManager = GetWorld()->GetSubsystem<UMASquadManager>();
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!SquadManager || !AgentManager) return;

    // 查找 Human 作为 Leader
    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
    if (Humans.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No Human as Leader!"));
        return;
    }

    AMACharacter* Leader = Humans[0];

    // 检查 Leader 是否已有 Squad
    UMASquad* Squad = Leader->CurrentSquad;

    if (!Squad)
    {
        // 没有 Squad，创建一个（Leader + 所有 RobotDog）
        TArray<AMACharacter*> Members;
        Members.Add(Leader);
        
        for (AMACharacter* Robot : AgentManager->GetAgentsByType(EMAAgentType::RobotDog))
        {
            if (Robot && !Robot->AgentName.Contains(TEXT("Tracker")))
            {
                Members.Add(Robot);
            }
        }

        if (Members.Num() < 2)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("Need at least 2 agents for squad!"));
            return;
        }

        Squad = SquadManager->CreateSquad(Members, Leader, TEXT("MainSquad"));
    }

    // 循环切换编队类型
    CurrentFormationIndex = (CurrentFormationIndex + 1) % 6;

    if (CurrentFormationIndex == 0)
    {
        Squad->StopFormation();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, TEXT("Formation stopped"));
        return;
    }

    EMAFormationType FormationType = static_cast<EMAFormationType>(CurrentFormationIndex);
    Squad->StartFormation(FormationType);
}

// ========== 相机操作 ==========

void AMAPlayerController::OnTakePhoto(const FInputActionValue& Value)
{
    UMACameraSensorComponent* Camera = GetCurrentCamera();
    if (Camera && Camera->TakePhoto())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("%s: Photo saved"), *Camera->SensorName));
    }
}

void AMAPlayerController::OnToggleRecording(const FInputActionValue& Value)
{
    UMACameraSensorComponent* Camera = GetCurrentCamera();
    if (!Camera) return;

    if (Camera->bIsRecording)
    {
        Camera->StopRecording();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("%s: Recording stopped"), *Camera->SensorName));
    }
    else
    {
        Camera->StartRecording(Camera->StreamFPS);
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("%s: Recording started"), *Camera->SensorName));
    }
}

void AMAPlayerController::OnToggleTCPStream(const FInputActionValue& Value)
{
    UMACameraSensorComponent* Camera = GetCurrentCamera();
    if (!Camera) return;

    // 停止其他相机的流
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (AgentManager)
    {
        for (AMACharacter* Agent : AgentManager->GetAllAgents())
        {
            if (Agent)
            {
                if (UMACameraSensorComponent* OtherCamera = Agent->GetCameraSensor())
                {
                    if (OtherCamera != Camera && OtherCamera->bIsStreaming)
                    {
                        OtherCamera->StopTCPStream();
                    }
                }
            }
        }
    }

    if (Camera->bIsStreaming)
    {
        Camera->StopTCPStream();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("%s: TCP stopped"), *Camera->SensorName));
    }
    else
    {
        if (Camera->StartTCPStream(9000, Camera->StreamFPS))
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("%s: TCP on port 9000"), *Camera->SensorName));
        }
    }
}

// ========== 辅助：获取当前相机 ==========

UMACameraSensorComponent* AMAPlayerController::GetCurrentCamera()
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return nullptr;

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

    if (Cameras.Num() == 0) return nullptr;

    int32 Index = (CurrentCameraIndex >= 0 && CurrentCameraIndex < Cameras.Num()) ? CurrentCameraIndex : 0;
    return Cameras[Index];
}

// ========== 编组快捷键 (星际争霸风格) ==========

void AMAPlayerController::HandleControlGroup(int32 GroupIndex)
{
    UMASelectionManager* SelectionManager = GetWorld()->GetSubsystem<UMASelectionManager>();
    if (!SelectionManager) return;

    bool bCtrlPressed = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

    if (bCtrlPressed)
    {
        // Ctrl + 数字：保存当前选择到编组
        SelectionManager->SaveToControlGroup(GroupIndex);
    }
    else
    {
        // 数字：选中编组
        SelectionManager->SelectControlGroup(GroupIndex);
    }
}

void AMAPlayerController::OnControlGroup0(const FInputActionValue& Value) { HandleControlGroup(0); }
void AMAPlayerController::OnControlGroup1(const FInputActionValue& Value) { HandleControlGroup(1); }
void AMAPlayerController::OnControlGroup2(const FInputActionValue& Value) { HandleControlGroup(2); }
void AMAPlayerController::OnControlGroup3(const FInputActionValue& Value) { HandleControlGroup(3); }
void AMAPlayerController::OnControlGroup4(const FInputActionValue& Value) { HandleControlGroup(4); }
void AMAPlayerController::OnControlGroup5(const FInputActionValue& Value) { HandleControlGroup(5); }
void AMAPlayerController::OnControlGroup6(const FInputActionValue& Value) { HandleControlGroup(6); }
void AMAPlayerController::OnControlGroup7(const FInputActionValue& Value) { HandleControlGroup(7); }
void AMAPlayerController::OnControlGroup8(const FInputActionValue& Value) { HandleControlGroup(8); }
void AMAPlayerController::OnControlGroup9(const FInputActionValue& Value) { HandleControlGroup(9); }

// ========== 创建 Squad ==========

void AMAPlayerController::OnCreateSquad(const FInputActionValue& Value)
{
    // Shift+Q 由 OnDisbandSquad 处理
    if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
    {
        return;
    }

    UMASelectionManager* SelectionManager = GetWorld()->GetSubsystem<UMASelectionManager>();
    UMASquadManager* SquadManager = GetWorld()->GetSubsystem<UMASquadManager>();
    if (!SelectionManager || !SquadManager) return;

    TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();
    if (Selected.Num() < 2)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, 
            TEXT("Select at least 2 agents to create squad (Q)"));
        return;
    }

    // 第一个选中的作为 Leader
    UMASquad* Squad = SquadManager->CreateSquad(Selected, Selected[0]);
    
    if (Squad)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("Created Squad '%s' with %d members"), 
                *Squad->SquadName, Squad->GetMemberCount()));
    }
}

void AMAPlayerController::OnDisbandSquad(const FInputActionValue& Value)
{
    // 只有 Shift+Q 才解散
    if (!IsInputKeyDown(EKeys::LeftShift) && !IsInputKeyDown(EKeys::RightShift))
    {
        return;
    }

    UMASelectionManager* SelectionManager = GetWorld()->GetSubsystem<UMASelectionManager>();
    UMASquadManager* SquadManager = GetWorld()->GetSubsystem<UMASquadManager>();
    if (!SelectionManager || !SquadManager) return;

    TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();
    
    // 收集选中 Agent 所属的 Squad
    TSet<UMASquad*> SquadsToDisbandSet;
    for (AMACharacter* Agent : Selected)
    {
        if (Agent && Agent->CurrentSquad)
        {
            SquadsToDisbandSet.Add(Agent->CurrentSquad);
        }
    }

    if (SquadsToDisbandSet.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, 
            TEXT("No squad to disband (Shift+Q)"));
        return;
    }

    // 解散所有相关 Squad
    int32 DisbandedCount = 0;
    for (UMASquad* Squad : SquadsToDisbandSet)
    {
        if (SquadManager->DisbandSquad(Squad))
        {
            DisbandedCount++;
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
        FString::Printf(TEXT("Disbanded %d squad(s)"), DisbandedCount));
}

// ========== 鼠标模式切换 ==========

void AMAPlayerController::OnToggleMouseMode(const FInputActionValue& Value)
{
    // 循环切换: Select <-> Navigate
    if (CurrentMouseMode == EMAMouseMode::Select)
    {
        CurrentMouseMode = EMAMouseMode::Navigate;
    }
    else
    {
        CurrentMouseMode = EMAMouseMode::Select;
    }

    // 根据模式调整输入设置
    if (CurrentMouseMode == EMAMouseMode::Select)
    {
        // Select 模式：显示鼠标，禁用视角控制（框选时不要转视角）
        bShowMouseCursor = true;
        SetIgnoreLookInput(true);
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
    else
    {
        // Navigate 模式：显示鼠标，允许视角控制
        bShowMouseCursor = true;
        SetIgnoreLookInput(false);
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }

    FString ModeName = MouseModeToString(CurrentMouseMode);
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("Mouse Mode: %s (M to switch)"), *ModeName));

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Mouse mode: %s"), *ModeName);
}

FString AMAPlayerController::MouseModeToString(EMAMouseMode Mode)
{
    switch (Mode)
    {
        case EMAMouseMode::Select: return TEXT("Select");
        case EMAMouseMode::Navigate: return TEXT("Navigate");
        default: return TEXT("Unknown");
    }
}
