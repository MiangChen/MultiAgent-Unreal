// MACharacter.h
// 可控角色基类 - 支持 GAS 技能系统

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MACharacter.generated.h"

class UMAAbilitySystemComponent;
class AMAPickupItem;
class UMASensorComponent;
class UMACameraSensorComponent;

// Actor 类型枚举
UENUM(BlueprintType)
enum class EMAActorType : uint8
{
    Human,
    RobotDog,
    Drone,
    Camera
};

UCLASS()
class MULTIAGENT_API AMACharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMACharacter();

    // ========== IAbilitySystemInterface ==========
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ========== 技能 (GAS Abilities) ==========
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryPickup();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryDrop();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryNavigateTo(FVector Destination);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void CancelNavigation();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryFollowActor(AMACharacter* TargetActor, float FollowDistance = 200.f);

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void StopFollowing();

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    AMAPickupItem* GetHeldItem() const;

    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool IsHoldingItem() const;

    // ========== Properties ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor")
    int32 ActorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor")
    FString ActorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor")
    EMAActorType ActorType;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

    // ========== 头顶状态显示 ==========
    UFUNCTION(BlueprintCallable, Category = "Status")
    void ShowStatus(const FString& Text, float Duration = 3.0f);

    void ShowAbilityStatus(const FString& AbilityName, const FString& Params = TEXT(""));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool bShowStatusAboveHead = true;

    // ========== Sensor 管理 ==========
    UFUNCTION(BlueprintCallable, Category = "Sensor")
    UMACameraSensorComponent* AddCameraSensor(FVector RelativeLocation, FRotator RelativeRotation = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    bool RemoveSensor(UMASensorComponent* Sensor);

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    TArray<UMASensorComponent*> GetAllSensors() const;

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    UMACameraSensorComponent* GetCameraSensor() const;

    UFUNCTION(BlueprintCallable, Category = "Sensor")
    int32 GetSensorCount() const;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PossessedBy(AController* NewController) override;

    // 导航期间每帧调用，子类可重写
    virtual void OnNavigationTick();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UMAAbilitySystemComponent* AbilitySystemComponent;

private:
    FString CurrentStatusText;
    float StatusDisplayEndTime = 0.f;
    void DrawStatusText();
};
