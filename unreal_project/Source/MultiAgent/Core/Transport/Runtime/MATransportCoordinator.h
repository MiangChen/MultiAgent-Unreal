// MATransportCoordinator.h
// 运输协作协调器 - 负责多机器人协同运输同一物体时的局部协商
//
// 职责（局部协商层，介于命令分发层与个体技能层之间）:
// - 将同一时间步内、运输同一物体到同一目的地的多个机器人归入一个 Session
// - 为每个参与者分配抓取点与编队偏移，并选出 leader
// - 提供"全部就位"栅栏同步：所有参与者到达抓取点后才允许抬起
// - 抬起时把物体附着到 leader（一个物体只能附着到一个机器人），运输与到达后保持附着
//
// 设计要点:
// - 所有参与机器人位于同一 UWorld，协商通过共享内存完成，无需网络协议
// - 懒加入 + 延迟一帧定稿：同一帧内同步派发的技能会先后 JoinSession，
//   下一帧首次查询时统一定稿参与者集合并计算分配，保证分配结果一致

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MATransportCoordinator.generated.h"

class AMACharacter;

/** 单个机器人在一次运输协作中的分配结果 */
USTRUCT()
struct FMATransportAssignment
{
    GENERATED_BODY()

    /** 抓取点（世界坐标，含悬停高度偏移） */
    UPROPERTY()
    FVector GraspPoint = FVector::ZeroVector;

    /** 抓取点相对物体中心的偏移（用于运输阶段保持刚性编队） */
    UPROPERTY()
    FVector FormationOffset = FVector::ZeroVector;

    /** 是否为 leader（物体附着到 leader 上） */
    UPROPERTY()
    bool bIsLeader = false;

    /** 参与者总数 */
    UPROPERTY()
    int32 ParticipantCount = 0;

    bool bValid = false;
};

/**
 * 运输协作协调器
 */
UCLASS()
class MULTIAGENT_API UMATransportCoordinator : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    /** 生成 Session 键：同一物体 + 同一目的地视为一次协作 */
    static FString MakeSessionKey(const AActor* Object, const FVector& Destination);

    /** 加入一次运输协作（在 SK_Transport 激活时调用） */
    void JoinSession(const FString& SessionKey, AActor* Object, const FVector& Destination, AMACharacter* Agent);

    /**
     * 定稿参与者集合并返回本机器人的分配结果。
     * 首次被任意参与者调用时统一计算所有分配；之后直接返回缓存结果。
     */
    FMATransportAssignment FinalizeAndGetAssignment(const FString& SessionKey, AMACharacter* Agent);

    /** 报告本机器人已到达抓取点 */
    void ReportReady(const FString& SessionKey, AMACharacter* Agent);

    /** 是否所有参与者都已就位 */
    bool AreAllReady(const FString& SessionKey) const;

    /** leader 发起抬起：把物体附着到 leader 并禁用物体物理。返回是否已进入抬起状态 */
    bool TryBeginLift(const FString& SessionKey, AMACharacter* Agent);

    /** 是否已开始抬起（follower 据此判断可以进入运输阶段） */
    bool IsLiftStarted(const FString& SessionKey) const;

    /** 退出协作（技能结束/取消时调用），Session 空了则销毁。注意：不解除物体附着 */
    void LeaveSession(const FString& SessionKey, AMACharacter* Agent);

    //=========================================================================
    // FTickableGameObject
    //=========================================================================

    virtual void Tick(float DeltaTime) override;
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
    struct FParticipant
    {
        TWeakObjectPtr<AMACharacter> Agent;
        FVector GraspPoint = FVector::ZeroVector;
        FVector FormationOffset = FVector::ZeroVector;
        bool bReady = false;
    };

    struct FSession
    {
        TWeakObjectPtr<AActor> Object;
        FVector Destination = FVector::ZeroVector;
        TArray<FParticipant> Participants;
        TWeakObjectPtr<AMACharacter> Leader;
        bool bFinalized = false;
        bool bLiftStarted = false;
    };

    /** 抬起后持续跟踪：物体跟随 leader 平移，世界朝向保持抬起瞬间的快照不变 */
    struct FCarryTrack
    {
        TWeakObjectPtr<AActor> Object;
        TWeakObjectPtr<AMACharacter> Leader;
        /** 抬起瞬间记录的"物体世界位置 - leader 世界位置"偏移 */
        FVector OffsetFromLeader = FVector::ZeroVector;
        /** 抬起瞬间记录的物体世界朝向；运输全过程保持不变 */
        FRotator FixedWorldRotation = FRotator::ZeroRotator;
    };

    TMap<FString, FSession> Sessions;
    TArray<FCarryTrack> CarryTracks;

    /** 定稿：计算抓取点、编队偏移、选出 leader */
    void FinalizeSession(FSession& Session);

    FParticipant* FindParticipant(FSession& Session, const AMACharacter* Agent);
    const FParticipant* FindParticipant(const FSession& Session, const AMACharacter* Agent) const;

    /**
     * 在 OBB 顶面矩形的周界上分配 N 个抓取点（世界坐标），并输出对应的相对中心偏移。
     *
     * 物体的水平包围盒由两个世界向量给出：AxisXVec、AxisYVec。
     * 它们方向沿物体局部 X / Y 轴，长度等于该轴半尺寸 ×|缩放|（已含旋转、缩放）。
     * 顶面 Z 由 ObjectCenter.Z + 局部 Z 半尺寸计算（沿世界 Z 取最高点）。
     */
    static void DistributeGraspPointsOnRectanglePerimeter(
        const FVector& ObjectCenter,
        const FVector& AxisXVec,
        const FVector& AxisYVec,
        float TopZ,
        int32 Count,
        TArray<FVector>& OutGraspPoints,
        TArray<FVector>& OutFormationOffsets);

    /** 把 N 个候选抓取点指派给 N 个机器人：最小化总移动距离的贪心最近邻匹配 */
    static TArray<int32> AssignParticipantsToSlots(
        const TArray<FParticipant>& Participants,
        const TArray<FVector>& GraspPoints);
};
