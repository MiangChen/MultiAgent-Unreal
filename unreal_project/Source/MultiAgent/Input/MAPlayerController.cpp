// MAPlayerController.cpp (重构版)
// 使用 Enhanced Input System + MACommandManager
// 支持星际争霸风格的框选和编组

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/MAAgentManager.h"
#include "../UI/MAHUD.h"
#include "../Core/MACommandManager.h"
#include "../Core/MASquadManager.h"
#include "../Core/MASquad.h"
#include "../Core/MASelectionManager.h"
#include "../Core/MAViewportManager.h"
#include "../Core/MAEmergencyManager.h"
#include "../UI/MASelectionHUD.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.h"
#include "MADroneCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "MAPickupItem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"

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
    
    // 缓存 Subsystem 引用
    if (!InitializeSubsystems())
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerController] Failed to initialize subsystems!"));
    }
    
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

bool AMAPlayerController::InitializeSubsystems()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    
    AgentManager = World->GetSubsystem<UMAAgentManager>();
    CommandManager = World->GetSubsystem<UMACommandManager>();
    SelectionManager = World->GetSubsystem<UMASelectionManager>();
    SquadManager = World->GetSubsystem<UMASquadManager>();
    ViewportManager = World->GetSubsystem<UMAViewportManager>();
    EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
    
    return AgentManager && CommandManager && SelectionManager && SquadManager && ViewportManager && EmergencyManager;
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
        
        // UI 切换 (Z 键)
        EIC->BindAction(InputActions->IA_ToggleMainUI, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleMainUI);

        // 突发事件系统
        EIC->BindAction(InputActions->IA_TriggerEmergency, ETriggerEvent::Started, this, &AMAPlayerController::OnTriggerEmergency);
        EIC->BindAction(InputActions->IA_ToggleEmergencyUI, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleEmergencyUI);

        // ========== Agent View Mode 移动和视角控制 ==========
        // WASD 移动 (持续触发)
        EIC->BindAction(InputActions->IA_Move, ETriggerEvent::Triggered, this, &AMAPlayerController::OnMoveInput);
        
        // 鼠标视角 (持续触发)
        EIC->BindAction(InputActions->IA_Look, ETriggerEvent::Triggered, this, &AMAPlayerController::OnLookInput);
        
        // 垂直移动 (持续触发)
        EIC->BindAction(InputActions->IA_MoveUp, ETriggerEvent::Triggered, this, &AMAPlayerController::OnMoveUp);
        EIC->BindAction(InputActions->IA_MoveDown, ETriggerEvent::Triggered, this, &AMAPlayerController::OnMoveDown);
        
        // 方向键视角 (持续触发)
        EIC->BindAction(InputActions->IA_LookArrow, ETriggerEvent::Triggered, this, &AMAPlayerController::OnLookArrowInput);
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
        if (SelectionManager)
        {
            float MouseX, MouseY;
            if (GetMousePosition(MouseX, MouseY))
            {
                SelectionManager->BeginBoxSelect(FVector2D(MouseX, MouseY));
            }
        }
    }
    else // Navigate 模式
    {
        // Human 导航到点击位置
        FVector HitLocation;
        if (GetMouseHitLocation(HitLocation) && AgentManager)
        {
            for (AMACharacter* Agent : AgentManager->GetAgentsByType(EMAAgentType::Human))
            {
                if (Agent) Agent->TryNavigateTo(HitLocation);
            }
        }
    }
}

void AMAPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
    if (!SelectionManager || !SelectionManager->IsBoxSelecting()) return;

    // 更新终点
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
    }

    // 计算框选大小
    FVector2D Start = SelectionManager->GetBoxSelectStart();
    FVector2D End = SelectionManager->GetBoxSelectEnd();
    float BoxWidth = FMath::Abs(End.X - Start.X);
    float BoxHeight = FMath::Abs(End.Y - Start.Y);

    bool bCtrlPressed = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

    if (BoxWidth < 10.f && BoxHeight < 10.f)
    {
        // 点击选择：检查是否点击了 Agent
        SelectionManager->CancelBoxSelect();
        
        FHitResult HitResult;
        if (GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
        {
            if (AMACharacter* Agent = Cast<AMACharacter>(HitResult.GetActor()))
            {
                if (bCtrlPressed)
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
        if (!bCtrlPressed)
        {
            SelectionManager->ClearSelection();
        }
    }
    else
    {
        // 框选
        SelectionManager->EndBoxSelect(this, bCtrlPressed);
    }
}

void AMAPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新框选终点
    if (SelectionManager && SelectionManager->IsBoxSelecting())
    {
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
        }
    }

    // 更新 HUD 的框选状态 (MAHUD 继承自 MASelectionHUD)
    if (AMASelectionHUD* SelectionHUD = Cast<AMASelectionHUD>(GetHUD()))
    {
        if (SelectionManager)
        {
            SelectionHUD->bIsBoxSelecting = SelectionManager->IsBoxSelecting();
            SelectionHUD->BoxStart = SelectionManager->GetBoxSelectStart();
            SelectionHUD->BoxEnd = SelectionManager->GetBoxSelectEnd();
        }
    }
}

void AMAPlayerController::OnRightClick(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (!GetMouseHitLocation(HitLocation) || !CommandManager) return;

    FMACommandParams Params;
    Params.TargetLocation = HitLocation;
    Params.bShowMessage = false;  // 导航不显示消息

    // RobotDog + Drone 导航到点击位置
    for (AMACharacter* Agent : CommandManager->GetControllableAgents())
    {
        FVector Target = HitLocation;
        // Drone: 如果在空中则保持高度，如果在地面则让 TryNavigateTo 处理起飞
        if (Agent->AgentType == EMAAgentType::Drone ||
            Agent->AgentType == EMAAgentType::DronePhantom4 ||
            Agent->AgentType == EMAAgentType::DroneInspire2)
        {
            if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(Agent))
            {
                if (Drone->IsInAir())
                {
                    Target.Z = Agent->GetActorLocation().Z;  // 在空中，保持高度
                }
                // 在地面时不修改 Target.Z，让 TryNavigateTo 使用 DefaultFlightAltitude
            }
        }
        Agent->TryNavigateTo(Target);
    }
}

// ========== 拾取/放下 ==========

void AMAPlayerController::OnPickup(const FInputActionValue& Value)
{
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
    if (ViewportManager) ViewportManager->SwitchToNextCamera(this);
}

void AMAPlayerController::OnReturnToSpectator(const FInputActionValue& Value)
{
    if (ViewportManager) ViewportManager->ReturnToSpectator(this);
}

// ========== 命令 (通过 CommandManager) ==========

void AMAPlayerController::SendCommand(EMACommand Command)
{
    if (CommandManager) CommandManager->SendCommand(Command);
}

void AMAPlayerController::OnStartPatrol(const FInputActionValue& Value) { SendCommand(EMACommand::Patrol); }
void AMAPlayerController::OnStartCharge(const FInputActionValue& Value) { SendCommand(EMACommand::Charge); }
void AMAPlayerController::OnStopIdle(const FInputActionValue& Value) { SendCommand(EMACommand::Idle); }
void AMAPlayerController::OnStartCoverage(const FInputActionValue& Value) { SendCommand(EMACommand::Coverage); }
void AMAPlayerController::OnStartFollow(const FInputActionValue& Value) { SendCommand(EMACommand::Follow); }

void AMAPlayerController::OnStartAvoid(const FInputActionValue& Value)
{
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
    if (!SquadManager) return;

    UMASquad* Squad = SquadManager->GetOrCreateDefaultSquad();
    if (Squad) SquadManager->CycleFormation(Squad);
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
    return ViewportManager ? ViewportManager->GetCurrentCamera() : nullptr;
}

// ========== 编组快捷键 (星际争霸风格) ==========

void AMAPlayerController::HandleControlGroup(int32 GroupIndex)
{
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

void AMAPlayerController::OnControlGroup1(const FInputActionValue& Value) { HandleControlGroup(1); }
void AMAPlayerController::OnControlGroup2(const FInputActionValue& Value) { HandleControlGroup(2); }
void AMAPlayerController::OnControlGroup3(const FInputActionValue& Value) { HandleControlGroup(3); }
void AMAPlayerController::OnControlGroup4(const FInputActionValue& Value) { HandleControlGroup(4); }
void AMAPlayerController::OnControlGroup5(const FInputActionValue& Value) { HandleControlGroup(5); }

// ========== 创建 Squad ==========

void AMAPlayerController::OnCreateSquad(const FInputActionValue& Value)
{
    // Shift+Q 由 OnDisbandSquad 处理
    if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift)) return;
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
    if (!IsInputKeyDown(EKeys::LeftShift) && !IsInputKeyDown(EKeys::RightShift)) return;
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

// ========== UI 切换 (Z 键) ==========

void AMAPlayerController::OnToggleMainUI(const FInputActionValue& Value)
{
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ToggleMainUI();
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] ToggleMainUI called"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MAHUD not found!"));
    }
}

// ========== Agent View Mode 移动和视角控制 ==========

void AMAPlayerController::OnMoveInput(const FInputActionValue& Value)
{
    // 只在 Agent View Mode 下处理移动输入
    if (!ViewportManager || !ViewportManager->IsInAgentViewMode()) return;
    
    FVector2D MoveInput = Value.Get<FVector2D>();
    ViewportManager->ApplyMovementInput(MoveInput);
}

void AMAPlayerController::OnLookInput(const FInputActionValue& Value)
{
    // 只在 Agent View Mode 下处理视角输入
    if (!ViewportManager || !ViewportManager->IsInAgentViewMode()) return;
    
    FVector2D LookInput = Value.Get<FVector2D>();
    ViewportManager->ApplyLookInput(LookInput);
}

void AMAPlayerController::OnMoveUp(const FInputActionValue& Value)
{
    // 只在 Agent View Mode 下处理垂直移动输入
    if (!ViewportManager || !ViewportManager->IsInAgentViewMode()) return;
    
    // Space 键上升，传入正值
    ViewportManager->ApplyVerticalInput(1.0f);
}

void AMAPlayerController::OnMoveDown(const FInputActionValue& Value)
{
    // 只在 Agent View Mode 下处理垂直移动输入
    if (!ViewportManager || !ViewportManager->IsInAgentViewMode()) return;
    
    // Ctrl 键下降，传入负值
    ViewportManager->ApplyVerticalInput(-1.0f);
}

void AMAPlayerController::OnLookArrowInput(const FInputActionValue& Value)
{
    // 只在 Agent View Mode 下处理方向键视角输入
    if (!ViewportManager || !ViewportManager->IsInAgentViewMode()) return;
    
    FVector2D LookInput = Value.Get<FVector2D>();
    // 方向键灵敏度降低为鼠标的 0.125 倍
    LookInput *= 0.125f;
    ViewportManager->ApplyLookInput(LookInput);
}

// ========== 突发事件系统 ==========

void AMAPlayerController::OnTriggerEmergency(const FInputActionValue& Value)
{
    if (!EmergencyManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EmergencyManager not found!"));
        return;
    }
    
    // 切换事件状态（触发或结束）
    EmergencyManager->ToggleEvent();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnTriggerEmergency called, event active: %s"), 
        EmergencyManager->IsEventActive() ? TEXT("true") : TEXT("false"));
}

void AMAPlayerController::OnToggleEmergencyUI(const FInputActionValue& Value)
{
    if (!EmergencyManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EmergencyManager not found!"));
        return;
    }
    
    // 只有在事件激活时才能切换详情界面
    if (!EmergencyManager->IsEventActive())
    {
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleEmergencyUI: No active emergency event"));
        return;
    }
    
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ToggleEmergencyWidget();
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleEmergencyUI called"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MAHUD not found!"));
    }
}
