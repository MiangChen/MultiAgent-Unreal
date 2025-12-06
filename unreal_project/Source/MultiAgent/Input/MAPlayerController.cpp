// MAPlayerController.cpp
// 使用 Enhanced Input System - 自动配置，无需手动创建资产

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/MAActorSubsystem.h"
#include "../Character/MACharacter.h"
#include "../Character/MARobotDogCharacter.h"
#include "../Actor/MACameraSensor.h"
#include "../Actor/MAPickupItem.h"
#include "../Actor/MAPatrolPath.h"
#include "../Actor/MACoverageArea.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../GAS/Ability/GA_Formation.h"
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

        UE_LOG(LogTemp, Log, TEXT("[Input] Bound all input actions"));
    }
}


// ========== Input Handlers ==========

void AMAPlayerController::OnLeftClick(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        if (UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>())
        {
            TArray<AMACharacter*> Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human);
            for (AMACharacter* Character : Humans)
            {
                if (Character)
                {
                    Character->TryNavigateTo(HitLocation);
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
        if (UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>())
        {
            TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
            for (AMACharacter* Character : RobotDogs)
            {
                if (Character && !Character->ActorName.Contains(TEXT("Tracker")))
                {
                    Character->TryNavigateTo(HitLocation);
                }
            }
        }
    }
}

void AMAPlayerController::OnPickup(const FInputActionValue& Value)
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;

    TArray<AMACharacter*> Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human);
    for (AMACharacter* Character : Humans)
    {
        if (Character && !Character->IsHoldingItem())
        {
            if (Character->TryPickup())
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
                    FString::Printf(TEXT("%s picking up..."), *Character->ActorName));
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
                    FString::Printf(TEXT("%s: No item nearby"), *Character->ActorName));
            }
        }
    }
}

void AMAPlayerController::OnDrop(const FInputActionValue& Value)
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;

    TArray<AMACharacter*> Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human);
    for (AMACharacter* Character : Humans)
    {
        if (Character && Character->IsHoldingItem())
        {
            Character->TryDrop();
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
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    FVector SpawnLocation = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 300.f;
    
    AMACharacter* NewCharacter = ActorSubsystem->SpawnCharacter(
        AMARobotDogCharacter::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator,
        -1,
        EMAActorType::RobotDog
    );
    
    if (NewCharacter)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
            FString::Printf(TEXT("Spawned: %s"), *NewCharacter->ActorName));
    }
}

void AMAPlayerController::OnPrintAgentInfo(const FInputActionValue& Value)
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    int32 Total = ActorSubsystem->GetCharacterCount();
    int32 Dogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog).Num();
    int32 Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human).Num();
    int32 Sensors = ActorSubsystem->GetSensorCount();
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("=== Actors: %d | Dogs: %d | Humans: %d | Sensors: %d ==="), 
            Total, Dogs, Humans, Sensors));
    
    for (AMACharacter* Character : ActorSubsystem->GetAllCharacters())
    {
        if (Character)
        {
            FString HoldingInfo = Character->IsHoldingItem() ? TEXT(" [Holding]") : TEXT("");
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("  [%d] %s%s"), Character->ActorID, *Character->ActorName, *HoldingInfo));
        }
    }
}

void AMAPlayerController::OnDestroyLastAgent(const FInputActionValue& Value)
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    TArray<AMACharacter*> AllCharacters = ActorSubsystem->GetAllCharacters();
    if (AllCharacters.Num() > 0)
    {
        AMACharacter* LastCharacter = AllCharacters.Last();
        FString Name = LastCharacter->ActorName;
        
        if (ActorSubsystem->DestroyCharacter(LastCharacter))
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, 
                FString::Printf(TEXT("Destroyed: %s"), *Name));
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No characters to destroy!"));
    }
}

void AMAPlayerController::OnSwitchCamera(const FInputActionValue& Value)
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    TArray<AMASensor*> Sensors = ActorSubsystem->GetAllSensors();
    TArray<AMACameraSensor*> Cameras;
    for (AMASensor* Sensor : Sensors)
    {
        if (AMACameraSensor* Camera = Cast<AMACameraSensor>(Sensor))
        {
            Cameras.Add(Camera);
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
    
    AMACameraSensor* Camera = Cameras[CurrentCameraIndex];
    if (Camera)
    {
        SetViewTargetWithBlend(Camera, 0.3f);
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
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
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
    
    // 获取所有 RobotDog
    TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
    
    int32 CommandCount = 0;
    FGameplayTag PatrolCommand = FGameplayTag::RequestGameplayTag(FName("Command.Patrol"));
    
    for (AMACharacter* Character : RobotDogs)
    {
        if (!Character) continue;
        if (Character->ActorName.Contains(TEXT("Tracker"))) continue;
        
        AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character);
        if (!Robot) continue;
        
        // 设置 PatrolPath（CARLA 风格）
        Robot->SetPatrolPath(FirstPatrolPath);
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 清除其他命令，添加 Patrol 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
        ASC->AddLooseGameplayTag(PatrolCommand);
        
        CommandCount++;
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] %s: Command.Patrol set, PatrolPath=%s"),
            *Character->ActorName, *FirstPatrolPath->GetName());
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("%s: Patrol started"), *Character->ActorName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog found!"));
    }
}

void AMAPlayerController::OnStartCharge(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartCharge called (H key) - Sending Command.Charge to StateTree"));
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
        return;
    }
    
    // 获取所有 RobotDog，发送 Command.Charge 命令
    TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Found %d RobotDogs"), RobotDogs.Num());
    
    int32 CommandCount = 0;
    FGameplayTag ChargeCommand = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
    
    for (AMACharacter* Character : RobotDogs)
    {
        if (!Character) continue;
        
        // 排除 Tracker（Follow 机器人）
        if (Character->ActorName.Contains(TEXT("Tracker"))) continue;
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        if (!ASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[PlayerController] %s has no ASC!"), *Character->ActorName);
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
            FString::Printf(TEXT("%s: Charge started"), *Character->ActorName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog found!"));
    }
}

void AMAPlayerController::OnStopIdle(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStopIdle called (J key) - Sending Command.Idle to StateTree"));
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
        return;
    }
    
    // 获取所有 RobotDog，发送 Command.Idle 命令
    TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Found %d RobotDogs"), RobotDogs.Num());
    
    int32 CommandCount = 0;
    FGameplayTag IdleCommand = FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
    
    for (AMACharacter* Character : RobotDogs)
    {
        if (!Character) continue;
        
        // 排除 Tracker（Follow 机器人）
        if (Character->ActorName.Contains(TEXT("Tracker"))) continue;
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        if (!ASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[PlayerController] %s has no ASC!"), *Character->ActorName);
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
            FString::Printf(TEXT("%s: Idle"), *Character->ActorName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog found!"));
    }
}

void AMAPlayerController::OnStartCoverage(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartCoverage called (K key)"));
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
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
    
    // 获取所有 RobotDog
    TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
    
    int32 CommandCount = 0;
    FGameplayTag CoverageCommand = FGameplayTag::RequestGameplayTag(FName("Command.Coverage"));
    
    for (AMACharacter* Character : RobotDogs)
    {
        if (!Character) continue;
        if (Character->ActorName.Contains(TEXT("Tracker"))) continue;
        
        AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character);
        if (!Robot) continue;
        
        // 设置 CoverageArea（CARLA 风格）
        Robot->SetCoverageArea(FirstCoverageArea);
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 清除其他命令，添加 Coverage 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
        ASC->AddLooseGameplayTag(CoverageCommand);
        
        CommandCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
            FString::Printf(TEXT("%s: Coverage started"), *Character->ActorName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog found!"));
    }
}

void AMAPlayerController::OnStartFollow(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartFollow called (F key)"));
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
        return;
    }
    
    // 查找场景中的 Human 作为跟随目标
    TArray<AMACharacter*> Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human);
    AMACharacter* FirstHuman = Humans.Num() > 0 ? Humans[0] : nullptr;
    
    if (!FirstHuman)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No Human in scene to follow!"));
        return;
    }
    
    // 获取所有 RobotDog
    TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
    
    int32 CommandCount = 0;
    FGameplayTag FollowCommand = FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
    
    for (AMACharacter* Character : RobotDogs)
    {
        if (!Character) continue;
        
        AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character);
        if (!Robot) continue;
        
        // 设置 FollowTarget（CARLA 风格）
        Robot->SetFollowTarget(FirstHuman);
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 清除其他命令，添加 Follow 命令
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
        ASC->AddLooseGameplayTag(FollowCommand);
        
        CommandCount++;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
            FString::Printf(TEXT("%s: Following %s"), *Character->ActorName, *FirstHuman->ActorName));
    }
    
    if (CommandCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog found!"));
    }
}

void AMAPlayerController::OnStartAvoid(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartAvoid called (A key)"));
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
        return;
    }
    
    // 获取鼠标点击位置作为目标
    FVector TargetLocation;
    if (!GetMouseHitLocation(TargetLocation))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Click on ground to set avoid target!"));
        return;
    }
    
    // 获取所有 RobotDog
    TArray<AMACharacter*> RobotDogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog);
    
    int32 ActivatedCount = 0;
    
    for (AMACharacter* Character : RobotDogs)
    {
        if (!Character) continue;
        if (Character->ActorName.Contains(TEXT("Tracker"))) continue;
        
        UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
        if (!ASC) continue;
        
        // 激活 Avoid 技能
        if (ASC->TryActivateAvoid(TargetLocation))
        {
            ActivatedCount++;
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
                FString::Printf(TEXT("%s: Avoid activated"), *Character->ActorName));
        }
    }
    
    if (ActivatedCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No RobotDog found or Avoid failed!"));
    }
}

void AMAPlayerController::OnStartFormation(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnStartFormation called (B key)"));
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
        return;
    }
    
    // 查找 Human 作为 Leader
    TArray<AMACharacter*> Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human);
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
        ActorSubsystem->StopFormation();
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
    
    // 使用 Subsystem 统一管理编队
    ActorSubsystem->StartFormation(Leader, FormationType);
    
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("%s Formation started, Leader: %s"), *FormationName, *Leader->ActorName));
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
    
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] No ActorSubsystem!"));
        return;
    }
    
    // 获取所有 Camera Sensor 并拍照 (CARLA 风格)
    int32 PhotoCount = 0;
    TArray<AMASensor*> Sensors = ActorSubsystem->GetAllSensors();
    
    for (AMASensor* Sensor : Sensors)
    {
        if (AMACameraSensor* Camera = Cast<AMACameraSensor>(Sensor))
        {
            if (Camera->TakePhoto())
            {
                PhotoCount++;
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                    FString::Printf(TEXT("%s: Photo saved"), *Camera->SensorName));
            }
        }
    }
    
    if (PhotoCount == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No cameras found!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("Took %d photos (saved to Saved/Screenshots/)"), PhotoCount));
    }
}
