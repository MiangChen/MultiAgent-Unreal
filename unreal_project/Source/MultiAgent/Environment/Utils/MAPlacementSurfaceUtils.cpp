// MAPlacementSurfaceUtils.cpp

#include "MAPlacementSurfaceUtils.h"

#include "../IMAPickupItem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Math/Box.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"

namespace
{
    /** 目标顶面信息 */
    struct FSurfaceInfo
    {
        FVector Center = FVector::ZeroVector; // 目标 Actor 当前世界位置
        float TopZ = 0.f;                     // 目标顶面世界 Z
        float HalfX = 0.f;                    // 目标顶面 XY 半尺寸
        float HalfY = 0.f;
        bool bValid = false;
    };

    /** 已占用的 XY 区域 */
    struct FOccupiedRect
    {
        float MinX = 0.f;
        float MaxX = 0.f;
        float MinY = 0.f;
        float MaxY = 0.f;
    };

    static FSurfaceInfo BuildSurfaceInfo(AActor& TargetActor)
    {
        FSurfaceInfo Info;
        Info.Center = TargetActor.GetActorLocation();

        if (IMAPickupItem* TargetItem = Cast<IMAPickupItem>(&TargetActor))
        {
            const FVector Extent = TargetItem->GetBoundsExtent();
            const float BottomOffset = TargetItem->GetBottomOffset();
            Info.TopZ = Info.Center.Z - BottomOffset + Extent.Z * 2.f;
            Info.HalfX = Extent.X;
            Info.HalfY = Extent.Y;
            Info.bValid = (Info.HalfX > KINDA_SMALL_NUMBER && Info.HalfY > KINDA_SMALL_NUMBER);
            return Info;
        }

        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(TargetActor.GetRootComponent()))
        {
            const FVector Extent = Prim->Bounds.BoxExtent;
            Info.TopZ = Info.Center.Z + Extent.Z;
            Info.HalfX = Extent.X;
            Info.HalfY = Extent.Y;
            Info.bValid = (Info.HalfX > KINDA_SMALL_NUMBER && Info.HalfY > KINDA_SMALL_NUMBER);
            return Info;
        }

        return Info;
    }

    /** 取物体在世界空间中的 XY 半尺寸（用于占用矩形 / 待放物体尺寸） */
    static FVector2D GetItemHalfXY(AActor& ItemActor)
    {
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(ItemActor.GetRootComponent()))
        {
            const FVector Extent = Prim->Bounds.BoxExtent;
            return FVector2D(Extent.X, Extent.Y);
        }

        if (IMAPickupItem* Item = Cast<IMAPickupItem>(&ItemActor))
        {
            const FVector Extent = Item->GetBoundsExtent();
            return FVector2D(Extent.X, Extent.Y);
        }

        return FVector2D::ZeroVector;
    }

    /** 物体顶面之上的容差：物体底面在该范围内才视为坐落在表面上 */
    constexpr float NearTopTolerance = 80.f;

    /** 判断 OtherItem 是否当前坐落在 Surface 顶面上 */
    static bool IsItemRestingOnSurface(
        AActor& Other,
        IMAPickupItem& OtherItem,
        const FSurfaceInfo& Surface)
    {
        // 被携带中的物体不算坐落在表面（它跟着角色走）
        if (OtherItem.IsBeingCarried())
        {
            return false;
        }

        UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Other.GetRootComponent());
        if (!Prim)
        {
            return false;
        }

        const FVector OtherCenter = Other.GetActorLocation();
        const FVector OtherExtent = Prim->Bounds.BoxExtent;
        const float OtherBottomZ = OtherCenter.Z - OtherExtent.Z;

        if (FMath::Abs(OtherBottomZ - Surface.TopZ) > NearTopTolerance)
        {
            return false;
        }

        const float DX = OtherCenter.X - Surface.Center.X;
        const float DY = OtherCenter.Y - Surface.Center.Y;
        if (FMath::Abs(DX) > Surface.HalfX + OtherExtent.X ||
            FMath::Abs(DY) > Surface.HalfY + OtherExtent.Y)
        {
            return false;
        }

        return true;
    }

    /** 收集已坐落在目标顶面附近的占用矩形 */
    static TArray<FOccupiedRect> CollectOccupiedRects(
        AActor& TargetActor,
        AActor& ItemActor,
        const FSurfaceInfo& Surface,
        float Margin)
    {
        TArray<FOccupiedRect> Rects;
        UWorld* World = TargetActor.GetWorld();
        if (!World)
        {
            return Rects;
        }

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Other = *It;
            if (!Other || Other == &TargetActor || Other == &ItemActor)
            {
                continue;
            }

            IMAPickupItem* OtherItem = Cast<IMAPickupItem>(Other);
            if (!OtherItem)
            {
                continue;
            }

            if (!IsItemRestingOnSurface(*Other, *OtherItem, Surface))
            {
                continue;
            }

            UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Other->GetRootComponent());
            const FVector OtherCenter = Other->GetActorLocation();
            const FVector OtherExtent = Prim->Bounds.BoxExtent;

            FOccupiedRect Rect;
            Rect.MinX = OtherCenter.X - OtherExtent.X - Margin;
            Rect.MaxX = OtherCenter.X + OtherExtent.X + Margin;
            Rect.MinY = OtherCenter.Y - OtherExtent.Y - Margin;
            Rect.MaxY = OtherCenter.Y + OtherExtent.Y + Margin;
            Rects.Add(Rect);
        }

        return Rects;
    }

    /** 测试候选中心是否与任何已占用矩形相交 */
    static bool IsCandidateFree(
        float CandidateX,
        float CandidateY,
        float ItemHalfX,
        float ItemHalfY,
        const TArray<FOccupiedRect>& Rects)
    {
        const float MinX = CandidateX - ItemHalfX;
        const float MaxX = CandidateX + ItemHalfX;
        const float MinY = CandidateY - ItemHalfY;
        const float MaxY = CandidateY + ItemHalfY;

        for (const FOccupiedRect& Rect : Rects)
        {
            const bool bDisjoint =
                MaxX <= Rect.MinX || MinX >= Rect.MaxX ||
                MaxY <= Rect.MinY || MinY >= Rect.MaxY;
            if (!bDisjoint)
            {
                return false;
            }
        }
        return true;
    }
}

FVector FMAPlacementSurfaceUtils::ComputePlacementWorldLocation(
    AActor& TargetActor,
    AActor& ItemActor,
    float Margin)
{
    const FSurfaceInfo Surface = BuildSurfaceInfo(TargetActor);

    // 待放物体本身的 XY 半尺寸（外加安全间距，避免边缘擦到已有物体）
    const FVector2D ItemHalfXY = GetItemHalfXY(ItemActor);
    const float ItemHalfX = ItemHalfXY.X + Margin;
    const float ItemHalfY = ItemHalfXY.Y + Margin;

    // 计算目标 Z（用于回填底面贴合）
    float ItemBottomOffset = 0.f;
    if (IMAPickupItem* Item = Cast<IMAPickupItem>(&ItemActor))
    {
        ItemBottomOffset = Item->GetBottomOffset();
    }
    const float PlaceZ = Surface.TopZ - ItemBottomOffset;

    // 表面信息无效（极少见，例如目标既不是 IMAPickupItem 也没有 PrimitiveComponent），
    // 直接退回到目标中心。
    if (!Surface.bValid)
    {
        return FVector(Surface.Center.X, Surface.Center.Y, PlaceZ);
    }

    const TArray<FOccupiedRect> Rects = CollectOccupiedRects(TargetActor, ItemActor, Surface, Margin);

    // 中心可用就直接用中心，保持单物体放置时的原有行为。
    if (IsCandidateFree(Surface.Center.X, Surface.Center.Y, ItemHalfX, ItemHalfY, Rects))
    {
        return FVector(Surface.Center.X, Surface.Center.Y, PlaceZ);
    }

    // 由中心向外的螺旋格点搜索。Step 取待放物体直径的一半，保证与已占用矩形不重叠时
    // 至少能找到一个对齐位置。
    const float CellSize = FMath::Max(50.f, FMath::Max(ItemHalfX, ItemHalfY));
    const int32 MaxRing = 6; // 上限，避免极端情况下的长时间循环

    for (int32 Ring = 1; Ring <= MaxRing; ++Ring)
    {
        for (int32 IX = -Ring; IX <= Ring; ++IX)
        {
            for (int32 IY = -Ring; IY <= Ring; ++IY)
            {
                // 仅遍历当前外圈
                if (FMath::Max(FMath::Abs(IX), FMath::Abs(IY)) != Ring)
                {
                    continue;
                }

                const float CandidateX = Surface.Center.X + IX * CellSize;
                const float CandidateY = Surface.Center.Y + IY * CellSize;

                // 候选中心必须留出物体半尺寸以保证物体整体仍在目标顶面内
                if (FMath::Abs(CandidateX - Surface.Center.X) + ItemHalfX > Surface.HalfX ||
                    FMath::Abs(CandidateY - Surface.Center.Y) + ItemHalfY > Surface.HalfY)
                {
                    continue;
                }

                if (IsCandidateFree(CandidateX, CandidateY, ItemHalfX, ItemHalfY, Rects))
                {
                    return FVector(CandidateX, CandidateY, PlaceZ);
                }
            }
        }
    }

    // 没空位了：按需求允许重叠，回退到目标中心。
    return FVector(Surface.Center.X, Surface.Center.Y, PlaceZ);
}

TArray<AActor*> FMAPlacementSurfaceUtils::CollectItemsOnSurface(AActor& TargetActor)
{
    TArray<AActor*> Result;

    UWorld* World = TargetActor.GetWorld();
    if (!World)
    {
        return Result;
    }

    const FSurfaceInfo Surface = BuildSurfaceInfo(TargetActor);
    if (!Surface.bValid)
    {
        return Result;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Other = *It;
        if (!Other || Other == &TargetActor)
        {
            continue;
        }

        IMAPickupItem* OtherItem = Cast<IMAPickupItem>(Other);
        if (!OtherItem)
        {
            continue;
        }

        if (IsItemRestingOnSurface(*Other, *OtherItem, Surface))
        {
            Result.Add(Other);
        }
    }

    return Result;
}
