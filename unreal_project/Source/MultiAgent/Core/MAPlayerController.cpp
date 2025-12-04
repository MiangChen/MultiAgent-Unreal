// MAPlayerController.cpp

#include "MAPlayerController.h"
#include "MAGameMode.h"
#include "MAActorSubsystem.h"
#include "../Characters/MACharacter.h"
#include "../Characters/MARobotDogCharacter.h"
#include "../Actors/MACameraSensor.h"
#include "../Actors/MAPickupItem.h"
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
    
    // 右键点击 - 移动所有 Character
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
    
    // Y - 打印 Character 信息
    if (WasInputKeyJustPressed(EKeys::Y))
    {
        OnPrintAgentInfo();
    }
    
    // U - 销毁最后一个 Character
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
    
    // ===== 视角切换 =====
    // Tab - 切换到下一个 Camera 视角
    if (WasInputKeyJustPressed(EKeys::Tab))
    {
        OnSwitchCamera();
    }
    
    // 0 - 返回上帝视角
    if (WasInputKeyJustPressed(EKeys::Zero))
    {
        OnReturnToSpectator();
    }
}

void AMAPlayerController::OnLeftClick()
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

void AMAPlayerController::OnRightClick()
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
                // 跳过追踪者（名字包含 "Tracker"）
                if (Character && !Character->ActorName.Contains(TEXT("Tracker")))
                {
                    Character->TryNavigateTo(HitLocation);
                }
            }
        }
    }
}

void AMAPlayerController::MoveAllAgentsToLocation(FVector Destination)
{
    if (UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>())
    {
        ActorSubsystem->MoveAllCharactersTo(Destination, 150.f);
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
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    // 在玩家前方生成
    FVector SpawnLocation = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 300.f;
    
    AMACharacter* NewCharacter = ActorSubsystem->SpawnCharacter(
        AMARobotDogCharacter::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator,
        -1,  // 自动分配 ID
        EMAActorType::RobotDog
    );
    
    if (NewCharacter)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
            FString::Printf(TEXT("Spawned: %s at %s"), *NewCharacter->ActorName, *SpawnLocation.ToString()));
    }
}

void AMAPlayerController::OnPrintAgentInfo()
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    int32 Total = ActorSubsystem->GetCharacterCount();
    int32 Dogs = ActorSubsystem->GetCharactersByType(EMAActorType::RobotDog).Num();
    int32 Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human).Num();
    int32 Sensors = ActorSubsystem->GetSensorCount();
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("=== Actor Info ===\nCharacters: %d\nRobotDogs: %d\nHumans: %d\nSensors: %d"), Total, Dogs, Humans, Sensors));
    
    // 打印每个 Character 的详细信息
    for (AMACharacter* Character : ActorSubsystem->GetAllCharacters())
    {
        if (Character)
        {
            FString HoldingInfo = Character->IsHoldingItem() ? TEXT(" [Holding Item]") : TEXT("");
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("  [%d] %s at %s%s"), 
                    Character->ActorID, *Character->ActorName, *Character->GetActorLocation().ToString(), *HoldingInfo));
        }
    }
}

void AMAPlayerController::OnDestroyLastAgent()
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

// ===== GAS 测试按键实现 =====

void AMAPlayerController::OnPickup()
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;

    // 让所有 Human Character 尝试拾取
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
                    FString::Printf(TEXT("%s: No item nearby to pickup"), *Character->ActorName));
            }
        }
    }
}

void AMAPlayerController::OnDrop()
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;

    // 让所有持有物品的 Human Character 放下物品
    TArray<AMACharacter*> Humans = ActorSubsystem->GetCharactersByType(EMAActorType::Human);
    for (AMACharacter* Character : Humans)
    {
        if (Character && Character->IsHoldingItem())
        {
            Character->TryDrop();
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

// ===== 视角切换实现 =====

void AMAPlayerController::OnSwitchCamera()
{
    UMAActorSubsystem* ActorSubsystem = GetWorld()->GetSubsystem<UMAActorSubsystem>();
    if (!ActorSubsystem) return;
    
    // 获取所有 Camera Sensor
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
    
    // 保存原始 Pawn（首次切换时）
    if (!OriginalPawn && GetPawn())
    {
        OriginalPawn = GetPawn();
    }
    
    // 切换到下一个 Camera
    CurrentCameraIndex++;
    if (CurrentCameraIndex >= Cameras.Num())
    {
        CurrentCameraIndex = 0;
    }
    
    AMACameraSensor* Camera = Cameras[CurrentCameraIndex];
    if (Camera)
    {
        // 设置视角到 Camera
        SetViewTargetWithBlend(Camera, 0.3f);
        
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Switched to: %s (%d/%d)"), 
                *Camera->SensorName, CurrentCameraIndex + 1, Cameras.Num()));
    }
}

void AMAPlayerController::OnReturnToSpectator()
{
    if (OriginalPawn)
    {
        // 返回上帝视角
        SetViewTargetWithBlend(OriginalPawn, 0.3f);
        CurrentCameraIndex = -1;
        
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Returned to Spectator view"));
    }
    else
    {
        // 如果没有保存的 OriginalPawn，尝试使用当前 Pawn
        if (GetPawn())
        {
            SetViewTargetWithBlend(GetPawn(), 0.3f);
            CurrentCameraIndex = -1;
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Returned to default view"));
        }
    }
}
