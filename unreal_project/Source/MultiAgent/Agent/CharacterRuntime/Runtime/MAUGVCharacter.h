// MAUGVCharacter.h
// 无人车 (UGV) - 地面运输载具
// 技能: Navigate, Carry

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAUGVCharacter.generated.h"

class UMAStateTreeComponent;
class UStaticMeshComponent;

UCLASS()
class MULTIAGENT_API AMAUGVCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAUGVCharacter();
    virtual void Tick(float DeltaTime) override;

    // UGV 模型
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    UStaticMeshComponent* UGVMeshComponent;

    // 载货系统
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cargo")
    float MaxCargoWeight = 100.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Cargo")
    float CurrentCargoWeight = 0.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Cargo")
    TArray<AActor*> CarriedItems;
    
    // 货物放置高度（相对于 Actor 原点，即胶囊体中心）
    // 需要根据实际 UGV 模型的甲板位置手动调整
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cargo")
    float CargoDeckHeight = -10.f;

    // 载货操作
    UFUNCTION(BlueprintCallable, Category = "Cargo")
    bool LoadCargo(AActor* Item);
    
    UFUNCTION(BlueprintCallable, Category = "Cargo")
    bool UnloadCargo(AActor* Item);
    
    UFUNCTION(BlueprintCallable, Category = "Cargo")
    void UnloadAllCargo();
    
    UFUNCTION(BlueprintCallable, Category = "Cargo")
    bool HasCargo() const { return CarriedItems.Num() > 0; }

    // StateTree
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void InitializeSkillSet() override;
    
    // 根据 StaticMesh 自动调整胶囊体尺寸和 Mesh 位置
    void AutoFitCapsuleToStaticMesh();

private:
    UFUNCTION()
    void OnEnergyDepleted();
};
