// MATransportCoordinator.cpp
// 运输协作协调器实现

#include "MATransportCoordinator.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Environment/IMAPickupItem.h"
#include "Environment/Utils/MAPlacementSurfaceUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Stats/Stats.h"

DEFINE_LOG_CATEGORY_STATIC(LogMATransportCoordinator, Log, All);

FString UMATransportCoordinator::MakeSessionKey(const AActor* Object, const FVector& Destination)
{
    const FString ObjectKey = Object ? Object->GetName() : TEXT("null");
    // 目的地按米取整，避免浮点误差导致同一目标被拆成多个 Session
    return FString::Printf(TEXT("%s|%d_%d_%d"),
        *ObjectKey,
        FMath::RoundToInt(Destination.X / 100.f),
        FMath::RoundToInt(Destination.Y / 100.f),
        FMath::RoundToInt(Destination.Z / 100.f));
}

TStatId UMATransportCoordinator::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMATransportCoordinator, STATGROUP_Tickables);
}

void UMATransportCoordinator::JoinSession(const FString& SessionKey, AActor* Object, const FVector& Destination, AMACharacter* Agent)
{
    if (!Agent) return;

    FSession& Session = Sessions.FindOrAdd(SessionKey);
    Session.Object = Object;
    Session.Destination = Destination;

    // 已定稿后加入的迟到者不再纳入（保持分配稳定）
    if (Session.bFinalized)
    {
        UE_LOG(LogMATransportCoordinator, Warning,
            TEXT("[Transport] %s joined session '%s' after finalize, ignored"),
            *Agent->AgentLabel, *SessionKey);
        return;
    }

    if (!FindParticipant(Session, Agent))
    {
        FParticipant P;
        P.Agent = Agent;
        Session.Participants.Add(P);

        UE_LOG(LogMATransportCoordinator, Log,
            TEXT("[Transport] %s joined session '%s' (now %d participants)"),
            *Agent->AgentLabel, *SessionKey, Session.Participants.Num());
    }
}

FMATransportAssignment UMATransportCoordinator::FinalizeAndGetAssignment(const FString& SessionKey, AMACharacter* Agent)
{
    FMATransportAssignment Result;

    FSession* Session = Sessions.Find(SessionKey);
    if (!Session || !Agent)
    {
        return Result;
    }

    if (!Session->bFinalized)
    {
        FinalizeSession(*Session);
    }

    const FParticipant* P = FindParticipant(*Session, Agent);
    if (!P)
    {
        return Result;
    }

    Result.GraspPoint = P->GraspPoint;
    Result.FormationOffset = P->FormationOffset;
    Result.bIsLeader = (Session->Leader.Get() == Agent);
    Result.ParticipantCount = Session->Participants.Num();
    Result.bValid = true;
    return Result;
}

void UMATransportCoordinator::DistributeGraspPointsOnRectanglePerimeter(
    const FVector& ObjectCenter,
    const float HalfX,
    const float HalfY,
    const float TopZ,
    const int32 Count,
    TArray<FVector>& OutGraspPoints,
    TArray<FVector>& OutFormationOffsets)
{
    OutGraspPoints.Reset();
    OutFormationOffsets.Reset();

    if (Count <= 0)
    {
        return;
    }

    if (Count == 1)
    {
        // 单机器人：抓取点取顶面中心
        OutGraspPoints.Add(FVector(ObjectCenter.X, ObjectCenter.Y, TopZ));
        OutFormationOffsets.Add(FVector(0.f, 0.f, TopZ - ObjectCenter.Z));
        return;
    }

    // 矩形顶面四角（按 X+ 起点逆时针）。每条边长度依次为 2*HalfY、2*HalfX、2*HalfY、2*HalfX
    const FVector2D Corners[4] = {
        FVector2D(+HalfX, -HalfY),  // 右下
        FVector2D(+HalfX, +HalfY),  // 右上
        FVector2D(-HalfX, +HalfY),  // 左上
        FVector2D(-HalfX, -HalfY),  // 左下
    };
    const float EdgeLengths[4] = {
        2.f * HalfY,
        2.f * HalfX,
        2.f * HalfY,
        2.f * HalfX,
    };
    const float Perimeter = 2.f * (2.f * HalfX + 2.f * HalfY);
    if (Perimeter <= KINDA_SMALL_NUMBER)
    {
        OutGraspPoints.Add(FVector(ObjectCenter.X, ObjectCenter.Y, TopZ));
        OutFormationOffsets.Add(FVector(0.f, 0.f, TopZ - ObjectCenter.Z));
        return;
    }

    // 特殊情形：N=2 取一对对角；N=4 取四个角点。
    // 对一般矩形，沿周长均匀采样不会精确落在角上，因此显式枚举更稳。
    auto EmitCorner = [&](int32 CornerIdx)
    {
        const FVector2D& C = Corners[CornerIdx];
        OutGraspPoints.Add(FVector(ObjectCenter.X + C.X, ObjectCenter.Y + C.Y, TopZ));
        OutFormationOffsets.Add(FVector(C.X, C.Y, TopZ - ObjectCenter.Z));
    };

    if (Count == 2)
    {
        EmitCorner(0);  // 右下
        EmitCorner(2);  // 左上（对角）
        return;
    }

    if (Count == 4)
    {
        EmitCorner(0);  // 右下
        EmitCorner(1);  // 右上
        EmitCorner(2);  // 左上
        EmitCorner(3);  // 左下
        return;
    }

    // 其他个数：沿周长按弧长均匀采样
    const float Step = Perimeter / static_cast<float>(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        // 从右下角出发，沿边走 i*Step 弧长
        float Remaining = i * Step;
        int32 EdgeIdx = 0;
        while (EdgeIdx < 4 && Remaining > EdgeLengths[EdgeIdx])
        {
            Remaining -= EdgeLengths[EdgeIdx];
            ++EdgeIdx;
        }
        EdgeIdx = FMath::Clamp(EdgeIdx, 0, 3);

        const FVector2D A = Corners[EdgeIdx];
        const FVector2D B = Corners[(EdgeIdx + 1) % 4];
        const float T = (EdgeLengths[EdgeIdx] > KINDA_SMALL_NUMBER)
            ? FMath::Clamp(Remaining / EdgeLengths[EdgeIdx], 0.f, 1.f)
            : 0.f;
        const FVector2D Point = FMath::Lerp(A, B, T);

        OutGraspPoints.Add(FVector(ObjectCenter.X + Point.X, ObjectCenter.Y + Point.Y, TopZ));
        OutFormationOffsets.Add(FVector(Point.X, Point.Y, TopZ - ObjectCenter.Z));
    }
}

TArray<int32> UMATransportCoordinator::AssignParticipantsToSlots(
    const TArray<FParticipant>& Participants,
    const TArray<FVector>& GraspPoints)
{
    // 输出 SlotForParticipant[i] = 第 i 个参与者被分配到的槽位索引
    const int32 N = Participants.Num();
    TArray<int32> SlotForParticipant;
    SlotForParticipant.Init(INDEX_NONE, N);

    if (N == 0 || GraspPoints.Num() < N)
    {
        return SlotForParticipant;
    }

    // 贪心：每轮挑出当前 (参与者i, 槽位s) 距离最小的对，固定下来；O(N^3) 对 N<=10 完全够用
    TArray<bool> SlotUsed;
    SlotUsed.Init(false, GraspPoints.Num());
    TArray<bool> ParticipantAssigned;
    ParticipantAssigned.Init(false, N);

    for (int32 Round = 0; Round < N; ++Round)
    {
        float BestDistSq = TNumericLimits<float>::Max();
        int32 BestParticipant = INDEX_NONE;
        int32 BestSlot = INDEX_NONE;

        for (int32 i = 0; i < N; ++i)
        {
            if (ParticipantAssigned[i]) continue;
            const AMACharacter* Agent = Participants[i].Agent.Get();
            if (!Agent) continue;
            const FVector AgentLoc = Agent->GetActorLocation();

            for (int32 s = 0; s < GraspPoints.Num(); ++s)
            {
                if (SlotUsed[s]) continue;
                const float DistSq = FVector::DistSquared2D(AgentLoc, GraspPoints[s]);
                if (DistSq < BestDistSq)
                {
                    BestDistSq = DistSq;
                    BestParticipant = i;
                    BestSlot = s;
                }
            }
        }

        if (BestParticipant == INDEX_NONE || BestSlot == INDEX_NONE)
        {
            break;
        }

        SlotForParticipant[BestParticipant] = BestSlot;
        ParticipantAssigned[BestParticipant] = true;
        SlotUsed[BestSlot] = true;
    }

    // 兜底：未分配到的（如失效参与者）按顺序填入剩余槽位
    for (int32 i = 0; i < N; ++i)
    {
        if (SlotForParticipant[i] != INDEX_NONE) continue;
        for (int32 s = 0; s < GraspPoints.Num(); ++s)
        {
            if (!SlotUsed[s])
            {
                SlotForParticipant[i] = s;
                SlotUsed[s] = true;
                break;
            }
        }
    }

    return SlotForParticipant;
}

void UMATransportCoordinator::FinalizeSession(FSession& Session)
{
    Session.bFinalized = true;

    AActor* Object = Session.Object.Get();
    if (!Object)
    {
        return;
    }

    // 物体世界 AABB（注意：使用 Bounds 即 AABB；物体被旋转后这是世界轴向膨胀的盒子）
    FVector ObjectCenter = Object->GetActorLocation();
    FVector BoxExtent(100.f, 100.f, 50.f);
    float TopZ = ObjectCenter.Z;

    if (const UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Object->GetRootComponent()))
    {
        const FBoxSphereBounds Bounds = Prim->Bounds;
        ObjectCenter = Bounds.Origin;
        BoxExtent = Bounds.BoxExtent;
        TopZ = ObjectCenter.Z + BoxExtent.Z;
    }

    const int32 Count = Session.Participants.Num();
    if (Count <= 0)
    {
        return;
    }

    // 沿矩形顶面边缘按周长均匀采样 N 个抓取点（落在边缘上）
    TArray<FVector> GraspPoints;
    TArray<FVector> FormationOffsets;
    DistributeGraspPointsOnRectanglePerimeter(
        ObjectCenter, BoxExtent.X, BoxExtent.Y, TopZ, Count,
        GraspPoints, FormationOffsets);

    // 按当前位置就近匹配槽位
    const TArray<int32> SlotForParticipant = AssignParticipantsToSlots(Session.Participants, GraspPoints);
    for (int32 i = 0; i < Count; ++i)
    {
        const int32 Slot = SlotForParticipant.IsValidIndex(i) ? SlotForParticipant[i] : i;
        if (GraspPoints.IsValidIndex(Slot))
        {
            Session.Participants[i].GraspPoint = GraspPoints[Slot];
            Session.Participants[i].FormationOffset = FormationOffsets[Slot];
        }
    }

    // 选 leader：离物体中心 2D 最近的参与者
    int32 LeaderIdx = 0;
    float BestDistSq = TNumericLimits<float>::Max();
    for (int32 i = 0; i < Count; ++i)
    {
        if (AMACharacter* A = Session.Participants[i].Agent.Get())
        {
            const float DistSq = FVector::DistSquared2D(A->GetActorLocation(), ObjectCenter);
            if (DistSq < BestDistSq)
            {
                BestDistSq = DistSq;
                LeaderIdx = i;
            }
        }
    }
    Session.Leader = Session.Participants[LeaderIdx].Agent;

    // 立即冻结物体的物理积分（保留 collision 作为承载面）：
    // 在 MoveToGrasp / WaitReady 期间，UAV 飞向抓取点时可能与物体表面发生短暂接触；
    // 关掉 SimulatePhysics 后，这些接触不会再把物体推走，但 collision 仍能挡住表面上
    // 已放置的物品。真正的 lift（把物体跟随 leader）仍然在 TryBeginLift 中触发。
    if (AActor* Object_ = Session.Object.Get())
    {
        if (IMAPickupItem* Pickup = Cast<IMAPickupItem>(Object_))
        {
            Pickup->SetPhysicsEnabled(false);
        }
        else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Object_->GetRootComponent()))
        {
            Prim->SetSimulatePhysics(false);
            Prim->SetEnableGravity(false);
        }
    }

    UE_LOG(LogMATransportCoordinator, Log,
        TEXT("[Transport] Session finalized: %d participants, leader=%s, AABB extent=(%.0f, %.0f, %.0f)"),
        Count, Session.Leader.IsValid() ? *Session.Leader->AgentLabel : TEXT("none"),
        BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
}

void UMATransportCoordinator::ReportReady(const FString& SessionKey, AMACharacter* Agent)
{
    FSession* Session = Sessions.Find(SessionKey);
    if (!Session) return;

    if (FParticipant* P = FindParticipant(*Session, Agent))
    {
        P->bReady = true;
        UE_LOG(LogMATransportCoordinator, Log, TEXT("[Transport] %s ready in session '%s'"),
            Agent ? *Agent->AgentLabel : TEXT("?"), *SessionKey);
    }
}

bool UMATransportCoordinator::AreAllReady(const FString& SessionKey) const
{
    const FSession* Session = Sessions.Find(SessionKey);
    if (!Session || !Session->bFinalized || Session->Participants.Num() == 0)
    {
        return false;
    }

    for (const FParticipant& P : Session->Participants)
    {
        if (P.Agent.IsValid() && !P.bReady)
        {
            return false;
        }
    }
    return true;
}

bool UMATransportCoordinator::TryBeginLift(const FString& SessionKey, AMACharacter* Agent)
{
    FSession* Session = Sessions.Find(SessionKey);
    if (!Session) return false;

    if (Session->bLiftStarted)
    {
        return true;
    }

    if (Session->Leader.Get() != Agent || !AreAllReady(SessionKey))
    {
        return false;
    }

    AActor* Object = Session->Object.Get();
    AMACharacter* Leader = Session->Leader.Get();
    if (!Object || !Leader)
    {
        return false;
    }

    // 抬起：禁用物体物理，但不做 AttachToActor —— 避免 leader 旋转 yaw 时把物体一起转。
    // 物体的世界位移由 Tick() 跟随 leader 平移驱动，世界朝向保持抬起瞬间快照。
    if (IMAPickupItem* Pickup = Cast<IMAPickupItem>(Object))
    {
        Pickup->SetPhysicsEnabled(false);
    }
    else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Object->GetRootComponent()))
    {
        Prim->SetSimulatePhysics(false);
    }

    // 关闭物体碰撞，避免运输中与机器人/世界发生碰撞干扰
    if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Object->GetRootComponent()))
    {
        Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 记录跟随条目：物体跟随 leader 平移，世界朝向不变
    FCarryTrack Track;
    Track.Object = Object;
    Track.Leader = Leader;
    Track.OffsetFromLeader = Object->GetActorLocation() - Leader->GetActorLocation();
    Track.FixedWorldRotation = Object->GetActorRotation();
    CarryTracks.Add(Track);

    // 把当前坐落在物体顶面上的可拾取物（"乘客"）也加入跟随列表，
    // 让它们随 leader 一起平移。同样不修改世界朝向，保持各自原有姿态。
    // 物理与碰撞先关掉，避免被空中环境碰撞影响位置；落地后由后续放置逻辑负责重新启用。
    const TArray<AActor*> Passengers = FMAPlacementSurfaceUtils::CollectItemsOnSurface(*Object);
    for (AActor* Passenger : Passengers)
    {
        if (!Passenger)
        {
            continue;
        }

        if (IMAPickupItem* PassengerPickup = Cast<IMAPickupItem>(Passenger))
        {
            PassengerPickup->SetPhysicsEnabled(false);
        }
        if (UPrimitiveComponent* PassengerPrim = Cast<UPrimitiveComponent>(Passenger->GetRootComponent()))
        {
            PassengerPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        FCarryTrack PassengerTrack;
        PassengerTrack.Object = Passenger;
        PassengerTrack.Leader = Leader;
        PassengerTrack.OffsetFromLeader = Passenger->GetActorLocation() - Leader->GetActorLocation();
        PassengerTrack.FixedWorldRotation = Passenger->GetActorRotation();
        CarryTracks.Add(PassengerTrack);
    }

    Session->bLiftStarted = true;

    UE_LOG(LogMATransportCoordinator, Log,
        TEXT("[Transport] Lift started, object '%s' follows leader %s with %d passenger(s)"),
        *Object->GetName(), *Leader->AgentLabel, Passengers.Num());
    return true;
}

bool UMATransportCoordinator::IsLiftStarted(const FString& SessionKey) const
{
    const FSession* Session = Sessions.Find(SessionKey);
    return Session && Session->bLiftStarted;
}

void UMATransportCoordinator::LeaveSession(const FString& SessionKey, AMACharacter* Agent)
{
    FSession* Session = Sessions.Find(SessionKey);
    if (!Session) return;

    Session->Participants.RemoveAll([Agent](const FParticipant& P)
    {
        return !P.Agent.IsValid() || P.Agent.Get() == Agent;
    });

    // 注意：故意不解除物体跟随 —— 运输结束后物体仍由 Tick 跟随 leader（很可能在空中）

    if (Session->Participants.Num() == 0)
    {
        Sessions.Remove(SessionKey);
        UE_LOG(LogMATransportCoordinator, Log, TEXT("[Transport] Session '%s' destroyed"), *SessionKey);
    }
}

void UMATransportCoordinator::Tick(float DeltaTime)
{
    // 每帧把每个 carry track 的物体世界位置 = leader 当前世界位置 + 抬起瞬间记录的偏移；
    // 世界朝向保持抬起瞬间快照不变，不随 leader 旋转。
    for (int32 i = CarryTracks.Num() - 1; i >= 0; --i)
    {
        FCarryTrack& Track = CarryTracks[i];
        AActor* Object = Track.Object.Get();
        AMACharacter* Leader = Track.Leader.Get();

        if (!Object || !Leader)
        {
            CarryTracks.RemoveAt(i);
            continue;
        }

        // 物体已被另一个机器人/技能接管（例如被 SK_Place 抓起以放到别处）：
        // 让它彻底脱离运输跟随，否则 Tick 会每帧把它拉回 leader 偏移位置，
        // 覆盖 AttachToHand / PlaceOnObject 设置的新世界 transform。
        // Grate_1 自身在抬起时未做 AttachToActor，IsBeingCarried() 一直为 false，
        // 不会被这条规则误删。
        if (const IMAPickupItem* Pickup = Cast<IMAPickupItem>(Object))
        {
            if (Pickup->IsBeingCarried() && Pickup->GetCurrentCarrier() != Leader)
            {
                CarryTracks.RemoveAt(i);
                continue;
            }
        }

        const FVector NewLocation = Leader->GetActorLocation() + Track.OffsetFromLeader;
        Object->SetActorLocationAndRotation(NewLocation, Track.FixedWorldRotation);
    }
}

UMATransportCoordinator::FParticipant* UMATransportCoordinator::FindParticipant(FSession& Session, const AMACharacter* Agent)
{
    for (FParticipant& P : Session.Participants)
    {
        if (P.Agent.Get() == Agent)
        {
            return &P;
        }
    }
    return nullptr;
}

const UMATransportCoordinator::FParticipant* UMATransportCoordinator::FindParticipant(const FSession& Session, const AMACharacter* Agent) const
{
    for (const FParticipant& P : Session.Participants)
    {
        if (P.Agent.Get() == Agent)
        {
            return &P;
        }
    }
    return nullptr;
}
