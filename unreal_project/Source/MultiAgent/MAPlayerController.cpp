#include "MAPlayerController.h"
#include "MAAgent.h"
#include "MAGameMode.h"
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
    AMAGameMode* GameMode = Cast<AMAGameMode>(GetWorld()->GetAuthGameMode());
    if (!GameMode)
    {
        return;
    }
    
    TArray<APawn*> AllPawns = GameMode->GetAllPawns();
    int32 Count = AllPawns.Num();
    
    for (int32 i = 0; i < Count; i++)
    {
        APawn* Pawn = AllPawns[i];
        if (!Pawn)
        {
            continue;
        }
        
        // 计算每个 Agent 的目标位置（围绕点击点散开）
        float Angle = (360.f / Count) * i;
        float Radius = 150.f;
        
        FVector TargetLocation = Destination + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        // 让 Pawn 移动
        AAIController* AIController = Cast<AAIController>(Pawn->GetController());
        if (AIController)
        {
            AIController->MoveToLocation(TargetLocation);
        }
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
