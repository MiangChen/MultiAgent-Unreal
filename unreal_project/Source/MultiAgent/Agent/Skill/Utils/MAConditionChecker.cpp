// MAConditionChecker.cpp

#include "MAConditionChecker.h"
#include "MASkillTemplateRegistry.h"
#include "MAEventTemplateRegistry.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Types/MATypes.h"
#include "../../Character/MACharacter.h"
#include "../MASkillComponent.h"

//=============================================================================
// 辅助方法
//=============================================================================

bool FMAConditionChecker::IsAerialRobot(EMAAgentType AgentType)
{
	return FMASkillTemplateRegistry::IsAerialRobot(AgentType);
}

bool FMAConditionChecker::IsEnvironmentExemptSkill(EMACommand Command)
{
	return FMASkillTemplateRegistry::IsEnvironmentExemptSkill(Command);
}

FString FMAConditionChecker::GetTargetObjectId(EMACommand Command, UMASkillComponent* SkillComp)
{
	if (!SkillComp) return FString();

	const FMASkillParams& Params = SkillComp->GetSkillParams();

	switch (Command)
	{
	case EMACommand::Guide:
		return Params.GuideTargetObjectId;

	case EMACommand::TakePhoto:
	case EMACommand::Broadcast:
	case EMACommand::HandleHazard:
		return Params.CommonTargetObjectId;

	case EMACommand::Place:
		// For Place, the primary target is PlaceObject1 (the cargo).
		// We look for a matching node by checking SearchResults.
		// The object1 node ID is stored in FeedbackContext during param processing.
		{
			const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
			const FString* NodeId = Ctx.ObjectAttributes.Find(TEXT("object1_node_id"));
			return NodeId ? *NodeId : FString();
		}

	default:
		return FString();
	}
}

//=============================================================================
// 环境检查
//=============================================================================

FMACheckResult FMAConditionChecker::CheckEnvironmentHazard(
	AMACharacter* Agent,
	UMASceneGraphManager* SceneGraphMgr,
	const FString& NodeType,
	float DefaultRadius,
	const FString& EventKey)
{
	if (!Agent || !SceneGraphMgr)
	{
		return FMACheckResult::Pass();
	}

	const FVector AgentPos = Agent->GetActorLocation();
	const TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr->GetAllNodes();

	for (const FMASceneGraphNode& Node : AllNodes)
	{
		if (!Node.Type.Equals(NodeType, ESearchCase::IgnoreCase))
		{
			continue;
		}

		// Read effect_radius from Features, fall back to default
		float EffectRadius = DefaultRadius;
		if (const FString* RadiusStr = Node.Features.Find(TEXT("effect_radius")))
		{
			float Parsed = FCString::Atof(**RadiusStr);
			if (Parsed > 0.f)
			{
				EffectRadius = Parsed;
			}
		}

		const float Distance = FVector::Dist(AgentPos, Node.Center);
		if (Distance <= EffectRadius)
		{
			TMap<FString, FString> Params;
			Params.Add(TEXT("robot_label"), Agent->AgentLabel);
			Params.Add(TEXT("node_id"), Node.Id);
			Params.Add(TEXT("node_label"), Node.GetDisplayName());
			return FMACheckResult::Fail(EventKey, Params);
		}
	}

	return FMACheckResult::Pass();
}

FMACheckResult FMAConditionChecker::CheckLowVisibility(AMACharacter* Agent, UMASceneGraphManager* SceneGraphMgr)
{
	return CheckEnvironmentHazard(Agent, SceneGraphMgr, TEXT("smoke"), 3000.f, TEXT("AREA_LOW_VISIBILITY"));
}

FMACheckResult FMAConditionChecker::CheckStrongWind(AMACharacter* Agent, UMASceneGraphManager* SceneGraphMgr)
{
	return CheckEnvironmentHazard(Agent, SceneGraphMgr, TEXT("wind"), 5000.f, TEXT("AREA_STRONG_WIND"));
}

//=============================================================================
// 目标检查 - 通用辅助
//=============================================================================

FMACheckResult FMAConditionChecker::CheckObjectExists(
	const FString& ObjectId,
	UMASceneGraphManager* SceneGraphMgr,
	const FString& EventKey,
	const FString& LabelParamKey)
{
	if (ObjectId.IsEmpty())
	{
		// No target specified - skip check (pass)
		return FMACheckResult::Pass();
	}

	if (!SceneGraphMgr)
	{
		return FMACheckResult::Pass();
	}

	const FMASceneGraphNode Node = SceneGraphMgr->GetNodeById(ObjectId);
	if (Node.IsValid())
	{
		return FMACheckResult::Pass();
	}

	// Also check if the ID exists via IsIdExists (covers working copy nodes)
	if (SceneGraphMgr->IsIdExists(ObjectId))
	{
		return FMACheckResult::Pass();
	}

	TMap<FString, FString> Params;
	Params.Add(LabelParamKey, ObjectId);
	return FMACheckResult::Fail(EventKey, Params);
}

FMACheckResult FMAConditionChecker::CheckObjectNearby(
	AMACharacter* Agent,
	const FString& ObjectId,
	UMASceneGraphManager* SceneGraphMgr,
	const FString& EventKey,
	const FString& LabelParamKey)
{
	if (!Agent || ObjectId.IsEmpty() || !SceneGraphMgr)
	{
		return FMACheckResult::Pass();
	}

	const FMASceneGraphNode Node = SceneGraphMgr->GetNodeById(ObjectId);
	if (!Node.IsValid())
	{
		// Node not found - existence check should catch this, pass here
		return FMACheckResult::Pass();
	}

	const float Distance = FVector::Dist(Agent->GetActorLocation(), Node.Center);
	if (Distance <= DistanceThresholdCm)
	{
		return FMACheckResult::Pass();
	}

	const float DistanceMeters = Distance / 100.f;
	const float ThresholdMeters = DistanceThresholdCm / 100.f;

	TMap<FString, FString> Params;
	Params.Add(LabelParamKey, Node.GetDisplayName());
	Params.Add(TEXT("robot_label"), Agent->AgentLabel);
	Params.Add(TEXT("distance"), FString::Printf(TEXT("%.1f"), DistanceMeters));
	Params.Add(TEXT("threshold"), FString::Printf(TEXT("%.1f"), ThresholdMeters));
	return FMACheckResult::Fail(EventKey, Params);
}

//=============================================================================
// 目标检查 - 具体实现
//=============================================================================

FMACheckResult FMAConditionChecker::CheckTargetExists(
	AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command, UMASceneGraphManager* SceneGraphMgr)
{
	const FString TargetId = GetTargetObjectId(Command, SkillComp);
	return CheckObjectExists(TargetId, SceneGraphMgr, TEXT("TARGET_NOT_EXIST"), TEXT("target_label"));
}

FMACheckResult FMAConditionChecker::CheckTargetNearby(
	AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command, UMASceneGraphManager* SceneGraphMgr)
{
	const FString TargetId = GetTargetObjectId(Command, SkillComp);
	return CheckObjectNearby(Agent, TargetId, SceneGraphMgr, TEXT("TARGET_LOCATION_MISMATCH"), TEXT("target_label"));
}

FMACheckResult FMAConditionChecker::CheckSurfaceTargetExists(
	AMACharacter* Agent, UMASkillComponent* SkillComp, UMASceneGraphManager* SceneGraphMgr)
{
	if (!SkillComp) return FMACheckResult::Pass();

	// Surface target is PlaceObject2 - check if we have a resolved node ID
	const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
	const FString* SurfaceId = Ctx.ObjectAttributes.Find(TEXT("object2_node_id"));

	// If no surface ID stored, the surface might be "ground" which doesn't need existence check
	if (!SurfaceId || SurfaceId->IsEmpty())
	{
		return FMACheckResult::Pass();
	}

	return CheckObjectExists(*SurfaceId, SceneGraphMgr, TEXT("SURFACE_OBJECT_MISSING"), TEXT("surface_label"));
}

FMACheckResult FMAConditionChecker::CheckSurfaceTargetNearby(
	AMACharacter* Agent, UMASkillComponent* SkillComp, UMASceneGraphManager* SceneGraphMgr)
{
	if (!SkillComp) return FMACheckResult::Pass();

	const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
	const FString* SurfaceId = Ctx.ObjectAttributes.Find(TEXT("object2_node_id"));
	if (!SurfaceId || SurfaceId->IsEmpty())
	{
		return FMACheckResult::Pass();
	}

	return CheckObjectNearby(Agent, *SurfaceId, SceneGraphMgr, TEXT("SURFACE_OBJECT_LOCATION_MISMATCH"), TEXT("surface_label"));
}

FMACheckResult FMAConditionChecker::CheckHighPriorityTargetDiscovery(
	AMACharacter* Agent, UMASkillComponent* SkillComp, UMASceneGraphManager* SceneGraphMgr)
{
	if (!Agent || !SkillComp || !SceneGraphMgr)
	{
		return FMACheckResult::Pass();
	}

	// If the current skill's search target is itself a high-priority type,
	// skip the entire check — the agent is already tasked with finding such targets.
	const FMASemanticTarget& SearchTarget = SkillComp->GetSkillParams().SearchTarget;
	if (!SearchTarget.IsEmpty())
	{
		bool bTargetIsHighPriority = false;

		if (SearchTarget.Type.Equals(TEXT("fire"), ESearchCase::IgnoreCase) ||
			SearchTarget.Type.Equals(TEXT("hazard"), ESearchCase::IgnoreCase))
		{
			bTargetIsHighPriority = true;
		}

		const FString* IsFireVal = SearchTarget.Features.Find(TEXT("is_fire"));
		const FString* IsSpillVal = SearchTarget.Features.Find(TEXT("is_spill"));
		if ((IsFireVal && IsFireVal->Equals(TEXT("true"), ESearchCase::IgnoreCase)) ||
			(IsSpillVal && IsSpillVal->Equals(TEXT("true"), ESearchCase::IgnoreCase)))
		{
			bTargetIsHighPriority = true;
		}

		if (bTargetIsHighPriority)
		{
			return FMACheckResult::Pass();
		}
	}

	const FVector AgentPos = Agent->GetActorLocation();
	const TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr->GetAllNodes();

	static const float ScanRadius = 5000.f;

	for (const FMASceneGraphNode& Node : AllNodes)
	{
		const float Distance = FVector::Dist2D(AgentPos, Node.Center);
		if (Distance > ScanRadius)
		{
			continue;
		}

		bool bIsHighPriority = false;
		FString HazardType;

		// Case 1: Direct fire/hazard node types
		if (Node.Type.Equals(TEXT("fire"), ESearchCase::IgnoreCase))
		{
			bIsHighPriority = true;
			HazardType = TEXT("fire");
		}
		else if (Node.Type.Equals(TEXT("hazard"), ESearchCase::IgnoreCase))
		{
			bIsHighPriority = true;
			HazardType = TEXT("hazard");
		}
		// Case 2: Any node with is_fire or is_spill attributes set to true
		else
		{
			const FString* IsFireStr = Node.Features.Find(TEXT("is_fire"));
			const FString* IsSpillStr = Node.Features.Find(TEXT("is_spill"));

			if (IsFireStr && IsFireStr->Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bIsHighPriority = true;
				HazardType = TEXT("fire");
			}
			else if (IsSpillStr && IsSpillStr->Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bIsHighPriority = true;
				HazardType = TEXT("hazard");
			}
		}

		if (!bIsHighPriority)
		{
			continue;
		}

		TMap<FString, FString> Params;
		Params.Add(TEXT("hp_target_label"), Node.GetDisplayName());
		Params.Add(TEXT("hp_target_id"), Node.Id);
		Params.Add(TEXT("hp_target_type"), HazardType);
		return FMACheckResult::Fail(TEXT("NEW_HIGH_PRIORITY_TARGET_DISCOVERED"), Params);
	}

	return FMACheckResult::Pass();
}

//=============================================================================
// 机器人检查
//=============================================================================

FMACheckResult FMAConditionChecker::CheckBatteryLow(AMACharacter* Agent, UMASkillComponent* SkillComp)
{
	if (!Agent || !SkillComp)
	{
		return FMACheckResult::Pass();
	}

	const float EnergyPercent = SkillComp->GetEnergyPercent();
	if (EnergyPercent >= BatteryMinPercent)
	{
		return FMACheckResult::Pass();
	}

	TMap<FString, FString> Params;
	Params.Add(TEXT("robot_label"), Agent->AgentLabel);
	Params.Add(TEXT("battery_level"), FString::Printf(TEXT("%.0f"), EnergyPercent));
	return FMACheckResult::Fail(TEXT("ROBOT_BATTERY_LOW"), Params);
}

FMACheckResult FMAConditionChecker::CheckRobotFault(AMACharacter* Agent, UMASceneGraphManager* SceneGraphMgr)
{
	if (!Agent)
	{
		return FMACheckResult::Pass();
	}

	// Check if the agent is still valid in the world
	if (!IsValid(Agent) || Agent->IsPendingKillPending())
	{
		TMap<FString, FString> Params;
		Params.Add(TEXT("robot_label"), Agent->AgentLabel);
		return FMACheckResult::Fail(TEXT("ROBOT_FAULT"), Params);
	}

	// Check robot status from scene graph node
	// Robot nodes have a "status" feature, "error" indicates fault
	if (SceneGraphMgr)
	{
		const FMASceneGraphNode Node = SceneGraphMgr->GetNodeById(Agent->AgentID);
		if (Node.IsValid())
		{
			const FString* StatusStr = Node.Features.Find(TEXT("status"));
			if (StatusStr && StatusStr->Equals(TEXT("error"), ESearchCase::IgnoreCase))
			{
				TMap<FString, FString> Params;
				Params.Add(TEXT("robot_label"), Agent->AgentLabel);
				return FMACheckResult::Fail(TEXT("ROBOT_FAULT"), Params);
			}
		}
	}

	return FMACheckResult::Pass();
}

//=============================================================================
// 单项检查分发
//=============================================================================

FMACheckResult FMAConditionChecker::ExecuteCheckItem(
	EMAConditionCheckItem Item,
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp,
	UMASceneGraphManager* SceneGraphMgr)
{
	switch (Item)
	{
	case EMAConditionCheckItem::LowVisibility:
		return CheckLowVisibility(Agent, SceneGraphMgr);

	case EMAConditionCheckItem::StrongWind:
		return CheckStrongWind(Agent, SceneGraphMgr);

	case EMAConditionCheckItem::TargetExists:
		return CheckTargetExists(Agent, SkillComp, Command, SceneGraphMgr);

	case EMAConditionCheckItem::TargetNearby:
		return CheckTargetNearby(Agent, SkillComp, Command, SceneGraphMgr);

	case EMAConditionCheckItem::SurfaceTargetExists:
		return CheckSurfaceTargetExists(Agent, SkillComp, SceneGraphMgr);

	case EMAConditionCheckItem::SurfaceTargetNearby:
		return CheckSurfaceTargetNearby(Agent, SkillComp, SceneGraphMgr);

	case EMAConditionCheckItem::HighPriorityTargetDiscovery:
		return CheckHighPriorityTargetDiscovery(Agent, SkillComp, SceneGraphMgr);

	case EMAConditionCheckItem::BatteryLow:
		return CheckBatteryLow(Agent, SkillComp);

	case EMAConditionCheckItem::RobotFault:
		return CheckRobotFault(Agent, SceneGraphMgr);

	default:
		return FMACheckResult::Pass();
	}
}

//=============================================================================
// 聚合逻辑
//=============================================================================

FMAPrecheckResult FMAConditionChecker::RunChecks(
	const TArray<EMAConditionCheckItem>& Items,
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp,
	UMASceneGraphManager* SceneGraphMgr)
{
	FMAPrecheckResult Result;

	for (const EMAConditionCheckItem& Item : Items)
	{
		FMACheckResult CheckResult = ExecuteCheckItem(Item, Agent, Command, SkillComp, SceneGraphMgr);

		if (CheckResult.bPassed)
		{
			continue;
		}

		// Render the event from the template
		FMARenderedEvent Event = FMAEventTemplateRegistry::RenderEvent(CheckResult.EventKey, CheckResult.EventParams);

		// Store the event key in the payload for downstream consumers
		Event.Payload.Add(TEXT("event_key"), CheckResult.EventKey);

		if (Event.IsAbort())
		{
			Result.bAllPassed = false;
			Result.FailedEvents.Add(Event);
		}
		else
		{
			// Info-level event
			Result.InfoEvents.Add(Event);
		}
	}

	return Result;
}

FMAPrecheckResult FMAConditionChecker::RunPrecheck(
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp,
	UMASceneGraphManager* SceneGraphMgr)
{
	// Boundary: if Agent or SkillComponent is invalid, pass (don't block)
	if (!Agent || !SkillComp)
	{
		return FMAPrecheckResult();
	}

	const FMASkillCheckTemplate Template = FMASkillTemplateRegistry::GetTemplate(Command, Agent->AgentType);
	return RunChecks(Template.PrecheckItems, Agent, Command, SkillComp, SceneGraphMgr);
}

FMAPrecheckResult FMAConditionChecker::RunRuntimeCheck(
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp,
	UMASceneGraphManager* SceneGraphMgr)
{
	if (!Agent || !SkillComp)
	{
		return FMAPrecheckResult();
	}

	const FMASkillCheckTemplate Template = FMASkillTemplateRegistry::GetTemplate(Command, Agent->AgentType);
	return RunChecks(Template.RuntimeCheckItems, Agent, Command, SkillComp, SceneGraphMgr);
}
