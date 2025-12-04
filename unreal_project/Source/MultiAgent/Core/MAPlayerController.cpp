// MAPlayerController.cpp
// 使用 Enhanced Input System

#include "MAPlayerController.h"
#include "MAActorSubsystem.h"
#include "../Characters/MACharacter.h"
#include "../Characters/MARobotDogCharacter.h"
#include "../Actors/MACameraSensor.h"
#include "../Actors/MAPickupItem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

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

    // 添加输入映射上下文
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AMAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // 绑定 Enhanced Input
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 鼠标点击
        if (IA_LeftClick)
        {
            EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Triggered, this, &AMAPlayerController::OnLeftClick);
        }
        if (IA_RightClick)
        {
            EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Triggered, this, &AMAPlayerController::OnRightClick);
        }

        // GAS 技能
        if (IA_Pickup)
        {
            EnhancedInputComponent->BindAction(IA_Pickup, ETriggerEvent::Triggered, this, &AMAPlayerController::OnPickup);
        }
        if (IA_Drop)
        {
            EnhancedInputComponent->BindAction(IA_Drop, ETriggerEvent::Triggered, this, &AMAPlayerController::OnDrop);
        }

        // 生成
        if (IA_SpawnItem)
        {
            EnhancedInputComponent->BindAction(IA_SpawnItem, ETriggerEvent::Triggered, this, &AMAPlayerController::OnSpawnPickupItem);
        }
        if (IA_SpawnRobotDog)
        {
            EnhancedInputComponent->BindAction(IA_SpawnRobotDog, ETriggerEvent::Triggered, this, &AMAPlayerController::OnSpawnRobotDog);
        }

        // 调试
        if (IA_PrintInfo)
        {
            EnhancedInputComponent->BindAction(IA_PrintInfo, ETriggerEvent::Triggered, this, &AMAPlayerController::OnPrintAgentInfo);
        }
        if (IA_DestroyLast)
        {
            EnhancedInputComponent->BindAction(IA_DestroyLast, ETriggerEvent::Triggered, this, &AMAPlayerController::OnDestroyLastAgent);
        }

        // 视角切换
        if (IA_SwitchCamera)
        {
            EnhancedInputComponent->BindAction(IA_SwitchCamera, ETriggerEvent::Triggered, this, &AMAPlayerController::OnSwitchCamera);
        }
        if (IA_ReturnSpectator)
        {
            EnhancedInputComponent->BindAction(IA_ReturnSpectator, ETriggerEvent::Triggered, this, &AMAPlayerController::OnReturnToSpectator);
        }
    }
}

// ========== Input Handlers ==========

void AMAPlayerController::OnLeftClick(const FInputActionValue& Value)
{
    FVector HitLocation;
    if (GetMouseHitLocation(HitLocation))
    {
        // 左键：移动所有 Human Character 到目标位置
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
        // 右键：只移动 RobotDog（不包括追踪者）
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
                    FString::Printf(TEXT("%s trying to pickup..."), *Character->ActorName));
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
        FString::Printf(TEXT("=== Actor Info ===\nCharacters: %d | Dogs: %d | Humans: %d | Sensors: %d"), 
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
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("No cameras available!"));
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
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Returned to Spectator"));
    }
    else if (GetPawn())
    {
        SetViewTargetWithBlend(GetPawn(), 0.3f);
        CurrentCameraIndex = -1;
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Returned to default view"));
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
