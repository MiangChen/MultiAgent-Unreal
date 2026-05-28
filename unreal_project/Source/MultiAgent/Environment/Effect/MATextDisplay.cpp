// MATextDisplay.cpp
// 滚动文本显示屏实现 - 字符依次弹出效果

#include "MATextDisplay.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

AMATextDisplay::AMATextDisplay()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建根组件
    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootScene;

    // 创建背景板
    BackgroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundMesh"));
    BackgroundMesh->SetupAttachment(RootScene);
    BackgroundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BackgroundMesh->CastShadow = false;

    // 创建边框
    BorderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BorderMesh"));
    BorderMesh->SetupAttachment(RootScene);
    BorderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BorderMesh->CastShadow = false;
}

void AMATextDisplay::BeginPlay()
{
    Super::BeginPlay();
    CreateDisplayComponents();
}

void AMATextDisplay::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 始终面向摄像机
    FaceCameraUpdate();

    // 更新字符
    if (bIsScrolling)
    {
        UpdateCharacters(DeltaTime);
    }
}

void AMATextDisplay::SetText(const FString& InText)
{
    DisplayText = InText;
    
    // 清理旧字符
    ClearAllCharacters();
    
    // 重置状态
    NextCharIndex = 0;
    TimeToNextChar = 0.f;
    bAllCharsSpawned = false;
    
    UE_LOG(LogTemp, Log, TEXT("[MATextDisplay] SetText: '%s' (%d chars)"), 
        *DisplayText, DisplayText.Len());
}

void AMATextDisplay::StartScrolling()
{
    bIsScrolling = true;
    NextCharIndex = 0;
    TimeToNextChar = 0.f;
    bAllCharsSpawned = false;
    UE_LOG(LogTemp, Log, TEXT("[MATextDisplay] Started scrolling"));
}

void AMATextDisplay::StopScrolling()
{
    bIsScrolling = false;
    UE_LOG(LogTemp, Log, TEXT("[MATextDisplay] Stopped scrolling"));
}

void AMATextDisplay::SetDisplaySize(float Width, float Height)
{
    DisplayWidth = Width;
    DisplayHeight = Height;
    CreateDisplayComponents();
}

void AMATextDisplay::CreateDisplayComponents()
{
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    
    if (!CubeMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MATextDisplay] Failed to load Cube mesh"));
        return;
    }

    // 设置背景板
    if (BackgroundMesh)
    {
        BackgroundMesh->SetStaticMesh(CubeMesh);
        float ScaleX = 0.02f;
        float ScaleY = DisplayWidth / 100.f;
        float ScaleZ = DisplayHeight / 100.f;
        BackgroundMesh->SetRelativeScale3D(FVector(ScaleX, ScaleY, ScaleZ));
        BackgroundMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
        BackgroundMesh->SetRelativeRotation(FRotator::ZeroRotator);
        BackgroundMesh->SetVectorParameterValueOnMaterials(FName("BaseColor"), 
            FVector(BackgroundColor.R, BackgroundColor.G, BackgroundColor.B));
    }

    // 设置边框
    if (BorderMesh)
    {
        BorderMesh->SetStaticMesh(CubeMesh);
        float ScaleX = 0.03f;
        float ScaleY = (DisplayWidth + BorderThickness * 2) / 100.f;
        float ScaleZ = (DisplayHeight + BorderThickness * 2) / 100.f;
        BorderMesh->SetRelativeScale3D(FVector(ScaleX, ScaleY, ScaleZ));
        BorderMesh->SetRelativeLocation(FVector(-2.f, 0.f, 0.f));
        BorderMesh->SetRelativeRotation(FRotator::ZeroRotator);
        BorderMesh->SetVectorParameterValueOnMaterials(FName("BaseColor"), 
            FVector(BorderColor.R, BorderColor.G, BorderColor.B));
    }

    UE_LOG(LogTemp, Log, TEXT("[MATextDisplay] Created display: %.0fx%.0f"), DisplayWidth, DisplayHeight);
}

void AMATextDisplay::UpdateCharacters(float DeltaTime)
{
    // 弹出新字符
    if (!bAllCharsSpawned)
    {
        TimeToNextChar -= DeltaTime;
        if (TimeToNextChar <= 0.f)
        {
            SpawnNextCharacter();
            TimeToNextChar = CharacterInterval;
        }
    }

    // 更新所有激活字符的位置
    float RightBound = GetRightBoundary();
    
    for (int32 i = CharacterSlots.Num() - 1; i >= 0; --i)
    {
        FCharacterSlot& Slot = CharacterSlots[i];
        if (!Slot.bIsActive || !Slot.TextComponent) continue;

        // 向左移动（Y 坐标增加）
        Slot.CurrentY += ScrollSpeed * DeltaTime;

        // 更新位置
        Slot.TextComponent->SetRelativeLocation(FVector(3.f, Slot.CurrentY, 0.f));

        // 检查是否超出右边界
        if (Slot.CurrentY - Slot.CharWidth > RightBound)
        {
            Slot.TextComponent->DestroyComponent();
            Slot.TextComponent = nullptr;
            Slot.bIsActive = false;
        }
    }

    // 清理已销毁的槽位
    CharacterSlots.RemoveAll([](const FCharacterSlot& Slot) {
        return !Slot.bIsActive;
    });

    // 检查是否需要循环
    if (bAllCharsSpawned && CharacterSlots.Num() == 0)
    {
        NextCharIndex = 0;
        bAllCharsSpawned = false;
        TimeToNextChar = 0.5f;
    }
}


void AMATextDisplay::SpawnNextCharacter()
{
    if (NextCharIndex >= DisplayText.Len())
    {
        bAllCharsSpawned = true;
        return;
    }

    TCHAR CurrentChar = DisplayText[NextCharIndex];
    FString CharStr = FString::Chr(CurrentChar);
    
    UTextRenderComponent* CharComponent = NewObject<UTextRenderComponent>(this);
    if (CharComponent)
    {
        CharComponent->RegisterComponent();
        CharComponent->AttachToComponent(RootScene, FAttachmentTransformRules::KeepRelativeTransform);
        
        CharComponent->SetText(FText::FromString(CharStr));
        CharComponent->SetTextRenderColor(TextColor);
        CharComponent->SetWorldSize(TextSize);
        CharComponent->SetHorizontalAlignment(EHTA_Center);
        CharComponent->SetVerticalAlignment(EVRTA_TextCenter);
        
        // 禁用阴影
        CharComponent->bAffectDistanceFieldLighting = false;
        CharComponent->bCastDynamicShadow = false;
        CharComponent->CastShadow = false;
        
        // 初始位置：屏幕左边界
        float StartY = GetLeftBoundary();
        CharComponent->SetRelativeLocation(FVector(3.f, StartY, 0.f));
        CharComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));

        FCharacterSlot NewSlot;
        NewSlot.TextComponent = CharComponent;
        NewSlot.Character = CharStr;
        NewSlot.CurrentY = StartY;
        NewSlot.CharWidth = GetCharWidth(CurrentChar);
        NewSlot.bIsActive = true;
        
        CharacterSlots.Add(NewSlot);
    }

    NextCharIndex++;
}

void AMATextDisplay::FaceCameraUpdate()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
    if (!CameraManager) return;

    FVector CameraLocation = CameraManager->GetCameraLocation();
    FVector MyLocation = GetActorLocation();
    
    FVector DirectionToCamera = CameraLocation - MyLocation;
    DirectionToCamera.Z = 0.f;
    
    if (!DirectionToCamera.IsNearlyZero())
    {
        FRotator NewRotation = DirectionToCamera.Rotation();
        SetActorRotation(NewRotation);
    }
}

float AMATextDisplay::GetCharWidth(TCHAR Char) const
{
    if (Char > 127)
    {
        return TextSize * 1.0f;
    }
    else if (Char == ' ')
    {
        return TextSize * 0.4f;
    }
    else if (Char >= 'A' && Char <= 'Z')
    {
        return TextSize * 0.7f;
    }
    else
    {
        return TextSize * 0.6f;
    }
}

void AMATextDisplay::ClearAllCharacters()
{
    for (FCharacterSlot& Slot : CharacterSlots)
    {
        if (Slot.TextComponent && Slot.TextComponent->IsValidLowLevel())
        {
            Slot.TextComponent->DestroyComponent();
        }
    }
    CharacterSlots.Empty();
}
