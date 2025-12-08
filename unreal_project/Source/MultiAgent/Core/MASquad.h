// MASquad.h
// Squad 实体 - 表示一组 Agent 的组合
// 参考 Company of Heroes 的 Squad 系统
// Squad 拥有自己的技能（编队、协同攻击等）

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MATypes.h"
#include "MASquad.generated.h"

class AMACharacter;

// ========== Squad 技能参数 ==========
USTRUCT(BlueprintType)
struct FMASquadSkillParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> TargetActor;
};

// ========== 委托 ==========
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSquadMemberAdded, UMASquad*, Squad, AMACharacter*, Member);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSquadMemberRemoved, UMASquad*, Squad, AMACharacter*, Member);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSquadLeaderChanged, UMASquad*, Squad, AMACharacter*, NewLeader);

// ========== Squad 实体 ==========
UCLASS(BlueprintType)
class MULTIAGENT_API UMASquad : public UObject
{
    GENERATED_BODY()

public:
    UMASquad();

    // ========== 基本属性 ==========
    
    UPROPERTY(BlueprintReadOnly, Category = "Squad")
    FString SquadID;

    UPROPERTY(BlueprintReadWrite, Category = "Squad")
    FString SquadName;

    // ========== 成员管理 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Squad")
    bool AddMember(AMACharacter* Agent);

    UFUNCTION(BlueprintCallable, Category = "Squad")
    bool RemoveMember(AMACharacter* Agent);

    UFUNCTION(BlueprintCallable, Category = "Squad")
    void SetLeader(AMACharacter* NewLeader);

    UFUNCTION(BlueprintCallable, Category = "Squad")
    AMACharacter* GetLeader() const { return Leader.Get(); }

    UFUNCTION(BlueprintCallable, Category = "Squad")
    TArray<AMACharacter*> GetMembers() const;

    UFUNCTION(BlueprintCallable, Category = "Squad")
    int32 GetMemberCount() const;

    UFUNCTION(BlueprintCallable, Category = "Squad")
    bool HasMember(AMACharacter* Agent) const;

    UFUNCTION(BlueprintCallable, Category = "Squad")
    bool IsEmpty() const { return GetMemberCount() == 0; }

    // ========== 编队技能 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Squad|Formation")
    void StartFormation(EMAFormationType Type);

    UFUNCTION(BlueprintCallable, Category = "Squad|Formation")
    void StopFormation();

    UFUNCTION(BlueprintCallable, Category = "Squad|Formation")
    void SetFormationType(EMAFormationType Type);

    UFUNCTION(BlueprintCallable, Category = "Squad|Formation")
    EMAFormationType GetFormationType() const { return CurrentFormationType; }

    UFUNCTION(BlueprintCallable, Category = "Squad|Formation")
    bool IsInFormation() const { return CurrentFormationType != EMAFormationType::None; }

    // ========== Squad 技能 (可扩展) ==========
    
    UFUNCTION(BlueprintCallable, Category = "Squad|Skills")
    TArray<FString> GetAvailableSkills() const;

    UFUNCTION(BlueprintCallable, Category = "Squad|Skills")
    bool ExecuteSkill(const FString& SkillName, const FMASquadSkillParams& Params = FMASquadSkillParams());

    // ========== 编队参数 ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Formation")
    float FormationSpacing = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Formation")
    float FormationUpdateInterval = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Formation")
    float FormationStartMoveThreshold = 60.f;  // 距离超过60就开始移动

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad|Formation")
    float FormationStopMoveThreshold = 30.f;   // 距离小于30才停止

    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Squad")
    FOnSquadMemberAdded OnMemberAdded;

    UPROPERTY(BlueprintAssignable, Category = "Squad")
    FOnSquadMemberRemoved OnMemberRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Squad")
    FOnSquadLeaderChanged OnLeaderChanged;

    // ========== 辅助 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Squad")
    static FString FormationTypeToString(EMAFormationType Type);

    // 设置 World 引用 (由 SquadManager 调用)
    void SetWorld(UWorld* InWorld) { WorldPtr = InWorld; }

private:
    // ========== 成员数据 ==========
    
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> Leader;

    UPROPERTY()
    TArray<TWeakObjectPtr<AMACharacter>> Members;

    // ========== 编队状态 ==========
    
    EMAFormationType CurrentFormationType = EMAFormationType::None;
    FTimerHandle FormationTimerHandle;

    UPROPERTY()
    TWeakObjectPtr<UWorld> WorldPtr;

    // ========== 编队内部方法 ==========
    
    void UpdateFormation();
    TArray<FVector> CalculateFormationPositions(const FVector& LeaderLocation, const FRotator& LeaderRotation) const;
    FVector CalculateFormationOffset(int32 MemberIndex, int32 TotalCount) const;
    bool HasEnergy(AMACharacter* Agent) const;

    // 清理无效的弱引用
    void CleanupInvalidMembers();
};
