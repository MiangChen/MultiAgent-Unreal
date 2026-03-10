// MASquadManager.h
// Squad 管理器 - 管理所有 Squad 的生命周期
// 类似 AgentManager 管理 Agent，SquadManager 管理 Squad

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MASquadManager.generated.h"

class UMASquad;
class AMACharacter;
enum class EMAFormationType : uint8;

// ========== 委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSquadCreated, UMASquad*, Squad);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSquadDisbanded, UMASquad*, Squad);

UCLASS()
class MULTIAGENT_API UMASquadManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== Squad 生命周期 ==========
    
    // 创建 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    UMASquad* CreateSquad(const TArray<AMACharacter*>& Members, 
                          AMACharacter* Leader = nullptr,
                          const FString& SquadName = TEXT(""));

    // 解散 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    bool DisbandSquad(UMASquad* Squad);

    // 解散所有 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    void DisbandAllSquads();

    // ========== 查询 ==========
    
    // 通过 ID 获取 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    UMASquad* GetSquadByID(const FString& SquadID) const;

    // 通过名称获取 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    UMASquad* GetSquadByName(const FString& SquadName) const;

    // 获取 Agent 所属的 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    UMASquad* GetSquadByAgent(AMACharacter* Agent) const;

    // 获取所有 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    TArray<UMASquad*> GetAllSquads() const { return Squads; }

    // 获取 Squad 数量
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    int32 GetSquadCount() const { return Squads.Num(); }

    // ========== 快捷方法 ==========
    
    // 快速组队：选中的 Agent 组成 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    UMASquad* QuickFormSquad(const TArray<AMACharacter*>& SelectedAgents);

    // 让 Agent 离开当前 Squad（变回独立）
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    bool LeaveSquad(AMACharacter* Agent);

    // 获取或创建默认 Squad (Humanoid 为 Leader + 所有 Quadruped)
    // 用于快速编队，如果已存在则返回现有 Squad
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    UMASquad* GetOrCreateDefaultSquad();

    // 循环切换编队类型 (None -> Line -> Column -> Wedge -> Diamond -> Circle -> None)
    UFUNCTION(BlueprintCallable, Category = "SquadManager")
    void CycleFormation(UMASquad* Squad);

    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "SquadManager")
    FOnSquadCreated OnSquadCreated;

    UPROPERTY(BlueprintAssignable, Category = "SquadManager")
    FOnSquadDisbanded OnSquadDisbanded;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    UPROPERTY()
    TArray<UMASquad*> Squads;

    int32 NextSquadIndex = 0;

    // 生成唯一 Squad ID
    FString GenerateSquadID();

    // 清理空的 Squad
    void CleanupEmptySquads();
};
