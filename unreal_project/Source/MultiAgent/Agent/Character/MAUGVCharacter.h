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

    // 重写描边函数以支持 StaticMesh
    virtual void EnableOutline() override;
    virtual void DisableOutline() override;

protected:
    virtual void BeginPlay() override;
    virtual void InitializeSkillSet() override;

    // 地面吸附
    void SnapToGround();

private:
    UFUNCTION()
    void OnEnergyDepleted();
};
