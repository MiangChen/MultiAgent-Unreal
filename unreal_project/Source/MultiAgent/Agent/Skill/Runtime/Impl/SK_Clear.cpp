// SK_Clear.cpp
// 清洗技能实现

#include "SK_Clear.h"
#include "MAObservationSkillRuntimeHelpers.h"
#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Infrastructure/MASkillConfigBridge.h"
#include "../../Domain/MASkillTags.h"
#include "../MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "../../../Environment/Effect/MAWaterSpray.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace
{
/** 喷射方向重新对齐的周期 (秒)：机器人飞行中朝向会变化，需周期性把水柱方向重新钉到法向量 */
constexpr float SprayRefreshInterval = 0.1f;
}

USK_Clear::USK_Clear()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_Clear::ResetClearRuntimeState()
{
    CurrentPhase = EClearPhase::MoveToWaypoint;
    bClearSucceeded = false;
    ClearResultMessage.Reset();
    StartTime = 0.f;
    bIsAircraft = false;
    MinFlightAltitude = 800.f;
    Waypoints.Reset();
    CurrentWaypointIndex = 0;
    SprayDirection = FVector::ForwardVector;
    NavigationService = nullptr;
    WaterSpray = nullptr;
    TargetDynMaterial = nullptr;
    TargetActor.Reset();
}

void USK_Clear::FailClear(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage)
{
    bClearSucceeded = false;
    ClearResultMessage = ResultMessage;

    UE_LOG(LogTemp, Error, TEXT("[SK_Clear] %s"), *ErrorReason);

    if (!StatusMessage.IsEmpty())
    {
        if (AMACharacter* Character = GetOwningCharacter())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_Clear::InitializeClearContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    FMASkillConfigBridge::ApplyClearConfig(
        Character,
        StandoffDistance,
        SpraySpeed,
        SprayWidth,
        MoveSpeed,
        AcceptanceRadius);

    UE_LOG(LogTemp, Log, TEXT("[SK_Clear] Loaded config: Standoff=%.0f, SpraySpeed=%.0f, SprayWidth=%.0f, MoveSpeed=%.0f, AcceptRadius=%.0f"),
        StandoffDistance, SpraySpeed, SprayWidth, MoveSpeed, AcceptanceRadius);

    AActor* Target = SkillComp.GetSkillRuntimeTargets().ClearTargetActor.Get();
    if (!Target)
    {
        FailClear(TEXT("Clear failed: No valid target"), TEXT("ClearTargetActor not found"), TEXT("[Clear] Target not found"));
        return false;
    }
    TargetActor = Target;

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailClear(TEXT("Clear failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    bIsAircraft = MAObservationSkillRuntime::IsAircraft(Character);
    MinFlightAltitude = MAObservationSkillRuntime::ResolveMinFlightAltitude(Character, MinFlightAltitude);

    if (!ComputeFaceWaypoints(Character, *Target))
    {
        FailClear(TEXT("Clear failed: Could not compute target face"), TEXT("ComputeFaceWaypoints failed"), TEXT("[Clear] Invalid target shape"));
        return false;
    }

    CurrentWaypointIndex = 0;
    CurrentPhase = EClearPhase::MoveToWaypoint;
    return true;
}

bool USK_Clear::ComputeFaceWaypoints(const AMACharacter& Character, const AActor& Target)
{
    const UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Target.GetRootComponent());
    if (!Prim)
    {
        return false;
    }

    // 目标定向包围盒：局部包围盒 + 组件世界变换
    const FBoxSphereBounds LocalBounds = Prim->CalcLocalBounds();
    const FTransform Xform = Prim->GetComponentTransform();
    const FVector Scale = Xform.GetScale3D();

    const FVector WorldCenter = Xform.TransformPosition(LocalBounds.Origin);

    // 三个世界轴方向及其对应的半尺寸
    const FVector AxisDir[3] = {
        Xform.GetUnitAxis(EAxis::X),
        Xform.GetUnitAxis(EAxis::Y),
        Xform.GetUnitAxis(EAxis::Z)
    };
    const float AxisExtent[3] = {
        LocalBounds.BoxExtent.X * FMath::Abs(Scale.X),
        LocalBounds.BoxExtent.Y * FMath::Abs(Scale.Y),
        LocalBounds.BoxExtent.Z * FMath::Abs(Scale.Z)
    };

    // 最薄的轴 = 大平整面的法向轴；另外两个轴张成该平整面
    int32 ThinAxis = 0;
    for (int32 i = 1; i < 3; ++i)
    {
        if (AxisExtent[i] < AxisExtent[ThinAxis])
        {
            ThinAxis = i;
        }
    }
    const int32 AxisA = (ThinAxis + 1) % 3;
    const int32 AxisB = (ThinAxis + 2) % 3;

    // 外法向量：选朝向机器人的一侧
    FVector OutwardNormal = AxisDir[ThinAxis].GetSafeNormal();
    const FVector ToRobot = Character.GetActorLocation() - WorldCenter;
    if (FVector::DotProduct(ToRobot, OutwardNormal) < 0.f)
    {
        OutwardNormal = -OutwardNormal;
    }

    // 喷射方向 = 内法向量（从作业平面指向目标面）
    SprayDirection = -OutwardNormal;

    // 平整面中心（贴合目标表面）
    const FVector FaceCenter = WorldCenter + OutwardNormal * AxisExtent[ThinAxis];

    const FVector SpanA = AxisDir[AxisA] * AxisExtent[AxisA];
    const FVector SpanB = AxisDir[AxisB] * AxisExtent[AxisB];

    // 作业平面 = 平整面沿外法向偏移 StandoffDistance
    const FVector PlaneOffset = OutwardNormal * StandoffDistance;

    // 航点：四角 + 中心；四角按矩形顺序排列，避免对角穿越
    TArray<FVector> Raw;
    Raw.Add(FaceCenter - SpanA - SpanB + PlaneOffset);
    Raw.Add(FaceCenter + SpanA - SpanB + PlaneOffset);
    Raw.Add(FaceCenter + SpanA + SpanB + PlaneOffset);
    Raw.Add(FaceCenter - SpanA + SpanB + PlaneOffset);
    Raw.Add(FaceCenter + PlaneOffset);

    // 飞行器：抬升任何低于最小飞行高度的航点，避免撞地
    Waypoints.Reset();
    for (FVector Point : Raw)
    {
        if (bIsAircraft && Point.Z < MinFlightAltitude)
        {
            Point.Z = MinFlightAltitude;
        }
        Waypoints.Add(Point);
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_Clear] %s: Face normal=%s, Standoff=%.0f, Waypoints=%d"),
        *Character.AgentLabel, *OutwardNormal.ToString(), StandoffDistance, Waypoints.Num());

    return Waypoints.Num() > 0;
}

void USK_Clear::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetClearRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailClear(TEXT("Clear failed: Character not found"), TEXT("Character not found"));
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailClear(TEXT("Clear failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializeClearContext(*Character, *SkillComp))
    {
        return;
    }

    if (UWorld* World = Character->GetWorld())
    {
        StartTime = World->GetTimeSeconds();
    }

    Character->ShowAbilityStatus(TEXT("Clear"), TEXT("Starting cleaning..."));

    // 全程开启直线喷水，方向平行于平面法向量、长度等于 standoff
    StartSpray();

    // 初始化目标表面材质为脏色，后续随航点推进渐变到白色
    InitializeTargetMaterial();

    MoveToCurrentWaypoint();
}

void USK_Clear::MoveToCurrentWaypoint()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !NavigationService)
    {
        FailClear(TEXT("Clear failed: Lost reference during navigation"), TEXT("MoveToCurrentWaypoint lost Character or NavigationService"));
        return;
    }

    if (!Waypoints.IsValidIndex(CurrentWaypointIndex))
    {
        CurrentPhase = EClearPhase::Complete;
        CompleteClear();
        return;
    }

    if (MoveSpeed > 0.f)
    {
        if (bIsAircraft)
        {
            NavigationService->SetFlightSpeed(MoveSpeed);
        }
        else
        {
            NavigationService->SetMoveSpeed(MoveSpeed);
        }
    }

    const FVector Waypoint = Waypoints[CurrentWaypointIndex];

    Character->ShowAbilityStatus(TEXT("Clear"),
        FString::Printf(TEXT("Cleaning %d/%d"), CurrentWaypointIndex + 1, Waypoints.Num()));

    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Clear::OnNavigationCompleted);

    if (!NavigationService->NavigateTo(Waypoint, AcceptanceRadius))
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Clear::OnNavigationCompleted);
        FailClear(TEXT("Clear failed: Could not start navigation"), TEXT("NavigateTo returned false"));
    }
}

void USK_Clear::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Clear::OnNavigationCompleted);
    }

    if (!bSuccess)
    {
        FailClear(FString::Printf(TEXT("Clear failed: %s"), *Message), Message);
        return;
    }

    AdvanceWaypoint();
}

void USK_Clear::AdvanceWaypoint()
{
    CurrentWaypointIndex++;

    // 按进度更新目标材质颜色（从脏到干净）
    UpdateTargetMaterialProgress();

    if (Waypoints.IsValidIndex(CurrentWaypointIndex))
    {
        MoveToCurrentWaypoint();
    }
    else
    {
        CurrentPhase = EClearPhase::Complete;
        CompleteClear();
    }
}

void USK_Clear::StartSpray()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Character->GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Clear] StartSpray: Invalid Character or World"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Character;

    WaterSpray = Character->GetWorld()->SpawnActor<AMAWaterSpray>(
        AMAWaterSpray::StaticClass(),
        Character->GetActorLocation(),
        Character->GetActorRotation(),
        SpawnParams);

    if (!WaterSpray)
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Clear] %s: Failed to spawn water spray"), *Character->AgentLabel);
        return;
    }

    WaterSpray->SetSprayParameters(SpraySpeed, SprayWidth);
    WaterSpray->AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
    // 直线喷水：方向平行于平面法向量（指向目标面），长度固定为 standoff
    WaterSpray->StartSprayDirectional(SprayDirection, StandoffDistance);

    // 机器人飞行中朝向不断变化，附着的特效需周期性把世界朝向重新钉到法向量
    Character->GetWorld()->GetTimerManager().SetTimer(
        SprayRefreshTimerHandle,
        this,
        &USK_Clear::RefreshSprayDirection,
        SprayRefreshInterval,
        true);
}

void USK_Clear::RefreshSprayDirection()
{
    if (WaterSpray)
    {
        WaterSpray->UpdateDirection(SprayDirection);
    }
}

void USK_Clear::CleanupSpray()
{
    if (AMACharacter* Character = GetOwningCharacter())
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(SprayRefreshTimerHandle);
        }
    }

    if (WaterSpray)
    {
        WaterSpray->StopSpray();
        WaterSpray->Destroy();
        WaterSpray = nullptr;
    }
}

void USK_Clear::InitializeTargetMaterial()
{
    AActor* Target = TargetActor.Get();
    if (!Target)
    {
        return;
    }

    UStaticMeshComponent* MeshComp = Target->FindComponentByClass<UStaticMeshComponent>();
    if (!MeshComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Clear] Target '%s' has no StaticMeshComponent, skipping material tint"),
            *Target->GetName());
        return;
    }

    // 对 element 0 创建 DMI，继承材质中 TintColor 的默认值（已在资产里设为脏色）。
    // 后续只需随清洗进度把 TintColor Lerp 到白色。
    TargetDynMaterial = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
    if (TargetDynMaterial)
    {
        // 读取材质当前的 TintColor 作为起始脏色（与资产保持一致）
        FLinearColor CurrentTint;
        if (TargetDynMaterial->GetVectorParameterValue(TEXT("TintColor"), CurrentTint))
        {
            DirtyTintColor = CurrentTint;
        }

        UE_LOG(LogTemp, Log, TEXT("[SK_Clear] Initialized target material DMI, dirty tint from asset (%.2f, %.2f, %.2f)"),
            DirtyTintColor.R, DirtyTintColor.G, DirtyTintColor.B);
    }
}

void USK_Clear::UpdateTargetMaterialProgress()
{
    if (!TargetDynMaterial)
    {
        return;
    }

    const int32 TotalWaypoints = Waypoints.Num();
    if (TotalWaypoints <= 0)
    {
        return;
    }

    // Progress: 0 → 完全脏，1 → 完全干净
    const float Progress = FMath::Clamp(
        static_cast<float>(CurrentWaypointIndex) / static_cast<float>(TotalWaypoints),
        0.f, 1.f);

    const FLinearColor Current = FMath::Lerp(DirtyTintColor, CleanTintColor, Progress);
    TargetDynMaterial->SetVectorParameterValue(TEXT("TintColor"), Current);
}

void USK_Clear::CompleteClear()
{
    AMACharacter* Character = GetOwningCharacter();

    bClearSucceeded = true;
    ClearResultMessage = TEXT("Clear completed successfully");

    if (Character)
    {
        Character->ShowAbilityStatus(TEXT("Clear"), TEXT("Complete!"));

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
            Context.ClearWaypointCount = Waypoints.Num();
            if (UWorld* World = Character->GetWorld())
            {
                Context.ClearDurationSeconds = World->GetTimeSeconds() - StartTime;
            }
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void USK_Clear::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();

    if (Character)
    {
        if (NavigationService)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Clear::OnNavigationCompleted);
            NavigationService->CancelNavigation();
            NavigationService->RestoreDefaultSpeed();
        }

        CleanupSpray();
        Character->ShowStatus(TEXT(""), 0.f);
    }

    bool bSuccessToNotify = bClearSucceeded;
    FString MessageToNotify = ClearResultMessage;

    if (bWasCancelled && ClearResultMessage.IsEmpty())
    {
        bSuccessToNotify = false;
        MessageToNotify = TEXT("Clear cancelled");
    }

    NavigationService = nullptr;
    WaterSpray = nullptr;
    TargetActor.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Clear, bSuccessToNotify, MessageToNotify);
        }
    }
}
