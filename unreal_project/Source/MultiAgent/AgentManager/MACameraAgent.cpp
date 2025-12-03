// MACameraAgent.cpp
// 传感器摄像头 - 用于附着到 Agent 上拍照

#include "MACameraAgent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

AMACameraAgent::AMACameraAgent()
{
    AgentType = EAgentType::Camera;
    AgentName = TEXT("Camera");
    
    // 禁用重力 - Camera 附着到其他 Agent 上
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->GravityScale = 0.0f;
    }
    
    // 彻底禁用碰撞 - Camera 不需要碰撞体
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCapsuleSize(1.0f, 1.0f);
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
        Capsule->SetHiddenInGame(true);
        Capsule->SetVisibility(false);
    }
    
    // 创建摄像头组件
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);
    CameraComponent->SetRelativeLocation(FVector::ZeroVector);
    CameraComponent->FieldOfView = FOV;
    
    // 创建场景捕获组件（用于截图）
    SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
    SceneCaptureComponent->SetupAttachment(CameraComponent);
    SceneCaptureComponent->bCaptureEveryFrame = false;
    SceneCaptureComponent->bCaptureOnMovement = false;
}

void AMACameraAgent::BeginPlay()
{
    Super::BeginPlay();
    
    // 更新 FOV
    CameraComponent->FieldOfView = FOV;
    SceneCaptureComponent->FOVAngle = FOV;
    
    // 初始化渲染目标
    InitializeRenderTarget();
}

void AMACameraAgent::InitializeRenderTarget()
{
    RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    RenderTarget->InitCustomFormat(Resolution.X, Resolution.Y, PF_B8G8R8A8, false);
    RenderTarget->UpdateResourceImmediate();
    
    SceneCaptureComponent->TextureTarget = RenderTarget;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s initialized with resolution %dx%d, FOV %.0f"),
        *AgentName, Resolution.X, Resolution.Y, FOV);
}

bool AMACameraAgent::TakePhoto(const FString& FilePath)
{
    if (!RenderTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Camera] TakePhoto failed: RenderTarget not initialized"));
        return false;
    }
    
    // 捕获当前帧
    SceneCaptureComponent->CaptureScene();
    
    // 读取像素数据
    TArray<FColor> Pixels = CaptureFrame();
    if (Pixels.Num() == 0)
    {
        return false;
    }
    
    // 生成文件路径
    FString SavePath = FilePath;
    if (SavePath.IsEmpty())
    {
        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        SavePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString::Printf(TEXT("%s_%s.png"), *AgentName, *Timestamp);
    }
    
    // 确保目录存在
    FString Directory = FPaths::GetPath(SavePath);
    IFileManager::Get().MakeDirectory(*Directory, true);
    
    // 保存为 PNG
    TArray64<uint8> CompressedData;
    FImageUtils::PNGCompressImageArray(Resolution.X, Resolution.Y, TArrayView64<const FColor>(Pixels), CompressedData);
    
    // 转换为普通 TArray 用于保存
    TArray<uint8> SaveData;
    SaveData.Append(CompressedData.GetData(), CompressedData.Num());
    
    if (FFileHelper::SaveArrayToFile(SaveData, *SavePath))
    {
        UE_LOG(LogTemp, Log, TEXT("[Camera] %s saved photo to %s"), *AgentName, *SavePath);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Camera] Failed to save photo to %s"), *SavePath);
    return false;
}

TArray<FColor> AMACameraAgent::CaptureFrame()
{
    TArray<FColor> Pixels;
    
    if (!RenderTarget)
    {
        return Pixels;
    }
    
    // 捕获场景
    SceneCaptureComponent->CaptureScene();
    
    // 读取像素
    FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
    if (Resource)
    {
        Resource->ReadPixels(Pixels);
    }
    
    return Pixels;
}
