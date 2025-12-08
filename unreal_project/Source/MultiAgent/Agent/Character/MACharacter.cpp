// MACharacter.cpp

#include "MACharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "MAAbilitySystemComponent.h"
#include "MAGameplayTags.h"
#include "MAPickupItem.h"
#include "../Component/Sensor/MASensorComponent.h"
#include "../Component/Sensor/MACameraSensorComponent.h"

AMACharacter::AMACharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AgentID = TEXT("");
    AgentName = TEXT("Agent");
    AgentType = EMAAgentType::Human;
    bIsMoving = false;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 160.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 150.f;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;

    AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* AMACharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMACharacter::BeginPlay()
{
    Super::BeginPlay();
    FMAGameplayTags::InitializeNativeTags();
}

void AMACharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        AbilitySystemComponent->InitializeAbilities(this);
    }
}

void AMACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bIsMoving)
    {
        OnNavigationTick();
    }
    
    DrawStatusText();
}

void AMACharacter::OnNavigationTick()
{
}

// ========== 技能 (GAS Abilities) ==========

bool AMACharacter::TryPickup()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivatePickup();
    }
    return false;
}

bool AMACharacter::TryDrop()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateDrop();
    }
    return false;
}

bool AMACharacter::TryNavigateTo(FVector Destination)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateNavigate(Destination);
    }
    return false;
}

void AMACharacter::CancelNavigation()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelNavigate();
    }
}

AMAPickupItem* AMACharacter::GetHeldItem() const
{
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);
    for (AActor* Actor : AttachedActors)
    {
        if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
        {
            return Item;
        }
    }
    return nullptr;
}

bool AMACharacter::IsHoldingItem() const
{
    return GetHeldItem() != nullptr;
}

bool AMACharacter::TryFollowActor(AMACharacter* TargetActor, float FollowDistance)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateFollow(TargetActor, FollowDistance);
    }
    return false;
}

void AMACharacter::StopFollowing()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelFollow();
    }
}

// ========== 头顶状态显示 ==========

void AMACharacter::ShowStatus(const FString& Text, float Duration)
{
    if (!bShowStatusAboveHead) return;
    
    CurrentStatusText = Text;
    StatusDisplayEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

void AMACharacter::ShowAbilityStatus(const FString& AbilityName, const FString& Params)
{
    FString DisplayText;
    if (Params.IsEmpty())
    {
        DisplayText = FString::Printf(TEXT("[%s]"), *AbilityName);
    }
    else
    {
        DisplayText = FString::Printf(TEXT("[%s] %s"), *AbilityName, *Params);
    }
    
    ShowStatus(DisplayText, 3.0f);
}

void AMACharacter::DrawStatusText()
{
    if (!bShowStatusAboveHead) return;
    if (CurrentStatusText.IsEmpty()) return;
    
    if (GetWorld()->GetTimeSeconds() > StatusDisplayEndTime)
    {
        CurrentStatusText = TEXT("");
        return;
    }
    
    FVector TextLocation = GetActorLocation() + FVector(0.f, 0.f, 150.f);
    
    DrawDebugString(
        GetWorld(),
        TextLocation,
        CurrentStatusText,
        nullptr,
        FColor::Yellow,
        0.f,
        true,
        1.2f
    );
}

// ========== Sensor 管理 ==========

UMACameraSensorComponent* AMACharacter::AddCameraSensor(FVector RelativeLocation, FRotator RelativeRotation)
{
    static int32 SensorCounter = 0;
    
    FName ComponentName = *FString::Printf(TEXT("CameraSensor_%d"), SensorCounter++);
    UMACameraSensorComponent* CameraSensor = NewObject<UMACameraSensorComponent>(this, ComponentName);
    
    if (CameraSensor)
    {
        CameraSensor->SetupAttachment(GetRootComponent());
        CameraSensor->SetRelativeLocation(RelativeLocation);
        CameraSensor->SetRelativeRotation(RelativeRotation);
        CameraSensor->RegisterComponent();
        
        CameraSensor->SensorName = FString::Printf(TEXT("%s_Camera"), *AgentName);
        
        UE_LOG(LogTemp, Log, TEXT("[Character] %s added camera sensor at (%.0f, %.0f, %.0f)"),
            *AgentName, RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z);
    }
    
    return CameraSensor;
}

bool AMACharacter::RemoveSensor(UMASensorComponent* Sensor)
{
    if (!Sensor || Sensor->GetOwner() != this)
    {
        return false;
    }
    
    FString SensorName = Sensor->SensorName;
    Sensor->DestroyComponent();
    
    UE_LOG(LogTemp, Log, TEXT("[Character] %s removed sensor %s"), *AgentName, *SensorName);
    return true;
}

TArray<UMASensorComponent*> AMACharacter::GetAllSensors() const
{
    TArray<UMASensorComponent*> Sensors;
    GetComponents<UMASensorComponent>(Sensors);
    return Sensors;
}

UMACameraSensorComponent* AMACharacter::GetCameraSensor() const
{
    return FindComponentByClass<UMACameraSensorComponent>();
}

int32 AMACharacter::GetSensorCount() const
{
    TArray<UMASensorComponent*> Sensors;
    GetComponents<UMASensorComponent>(Sensors);
    return Sensors.Num();
}

// ========== Action 动态发现与执行 ==========

TArray<FString> AMACharacter::GetAvailableActions() const
{
    TArray<FString> Actions;
    
    // Agent 自身的 Actions (基于 GAS 技能)
    Actions.Add(TEXT("Agent.NavigateTo"));
    Actions.Add(TEXT("Agent.CancelNavigation"));
    Actions.Add(TEXT("Agent.Pickup"));
    Actions.Add(TEXT("Agent.Drop"));
    Actions.Add(TEXT("Agent.FollowActor"));
    Actions.Add(TEXT("Agent.StopFollowing"));
    
    // 收集所有 Sensor 的 Actions
    TArray<UMASensorComponent*> Sensors = GetAllSensors();
    for (UMASensorComponent* Sensor : Sensors)
    {
        if (Sensor)
        {
            TArray<FString> SensorActions = Sensor->GetAvailableActions();
            Actions.Append(SensorActions);
        }
    }
    
    return Actions;
}

bool AMACharacter::ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params)
{
    // Agent 自身的 Actions
    if (ActionName == TEXT("Agent.NavigateTo"))
    {
        FVector Destination = FVector::ZeroVector;
        if (const FString* XParam = Params.Find(TEXT("x")))
        {
            Destination.X = FCString::Atof(**XParam);
        }
        if (const FString* YParam = Params.Find(TEXT("y")))
        {
            Destination.Y = FCString::Atof(**YParam);
        }
        if (const FString* ZParam = Params.Find(TEXT("z")))
        {
            Destination.Z = FCString::Atof(**ZParam);
        }
        return TryNavigateTo(Destination);
    }
    
    if (ActionName == TEXT("Agent.CancelNavigation"))
    {
        CancelNavigation();
        return true;
    }
    
    if (ActionName == TEXT("Agent.Pickup"))
    {
        return TryPickup();
    }
    
    if (ActionName == TEXT("Agent.Drop"))
    {
        return TryDrop();
    }
    
    if (ActionName == TEXT("Agent.StopFollowing"))
    {
        StopFollowing();
        return true;
    }
    
    // 尝试在 Sensors 中执行
    TArray<UMASensorComponent*> Sensors = GetAllSensors();
    for (UMASensorComponent* Sensor : Sensors)
    {
        if (Sensor)
        {
            // 检查该 Sensor 是否支持此 Action
            TArray<FString> SensorActions = Sensor->GetAvailableActions();
            if (SensorActions.Contains(ActionName))
            {
                return Sensor->ExecuteAction(ActionName, Params);
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Agent] %s: Unknown action '%s'"), *AgentName, *ActionName);
    return false;
}
