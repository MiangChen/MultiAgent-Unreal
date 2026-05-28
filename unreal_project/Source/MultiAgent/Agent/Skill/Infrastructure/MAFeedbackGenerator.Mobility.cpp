#include "Agent/Skill/Infrastructure/MAFeedbackGeneratorInternal.h"

#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"

void FMAFeedbackGenerator::GenerateNavigateFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        const FVector Loc = Params.TargetLocation;
        Feedback.Data.Add(TEXT("target_x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Feedback.Data.Add(TEXT("target_y"), FString::Printf(TEXT("%.1f"), Loc.Y));

        if (!Context.NearbyLandmarkLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("nearby_landmark_label"), Context.NearbyLandmarkLabel);
            Feedback.Data.Add(
                TEXT("nearby_landmark_distance"),
                FString::Printf(TEXT("%.1f"), MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Context.NearbyLandmarkDistance)));
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FMASkillParams& Params = SkillComp->GetSkillParams();

        if (bSuccess)
        {
            Feedback.Message = !Context.NearbyLandmarkLabel.IsEmpty()
                ? FString::Printf(TEXT("Navigation completed to (%.0f, %.0f), near %s"),
                    Params.TargetLocation.X, Params.TargetLocation.Y, *Context.NearbyLandmarkLabel)
                : FString::Printf(TEXT("Navigation completed to (%.0f, %.0f)"),
                    Params.TargetLocation.X, Params.TargetLocation.Y);
        }
        else
        {
            Feedback.Message = TEXT("Navigation failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Navigation completed") : TEXT("Navigation failed");
    }
}

void FMAFeedbackGenerator::GenerateFollowFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);

        if (!Context.FollowTargetId.IsEmpty())
        {
            MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
                Feedback, AllNodes, TEXT("target_id"), TEXT("target_label"), Context.FollowTargetId);
        }
        else if (!Context.FollowTargetName.IsEmpty())
        {
            MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
                Feedback,
                AllNodes,
                TEXT("target_id"),
                TEXT("target_label"),
                FMASceneGraphQueryUseCases::GetIdByLabel(AllNodes, Context.FollowTargetName),
                Context.FollowTargetName);
        }

        MAFeedbackGeneratorInternal::AddDurationSecondsField(Feedback, Context.FollowDurationSeconds);

        if (Context.FollowTargetDistance > 0.f)
        {
            Feedback.Data.Add(
                TEXT("distance"),
                FString::Printf(TEXT("%.1f"), MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Context.FollowTargetDistance)));
        }

        Feedback.Data.Add(TEXT("target_found"), Context.bFollowTargetFound ? TEXT("true") : TEXT("false"));

        if (!Context.bFollowTargetFound && !Context.FollowErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.FollowErrorReason);
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FString TargetLabel = Feedback.Data.FindRef(TEXT("target_label"));

        if (!Context.bFollowTargetFound)
        {
            Feedback.Message = Context.FollowErrorReason.IsEmpty()
                ? TEXT("Follow failed: target not found")
                : FString::Printf(TEXT("Follow failed: %s"), *Context.FollowErrorReason);
            Feedback.bSuccess = false;
        }
        else if (bSuccess)
        {
            if (!TargetLabel.IsEmpty() && Context.FollowTargetDistance > 0.f)
            {
                Feedback.Message = FString::Printf(TEXT("Follow completed: followed %s, final distance %.1fm"),
                    *TargetLabel,
                    MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Context.FollowTargetDistance));
            }
            else if (!TargetLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Follow completed: followed %s"), *TargetLabel);
            }
            else
            {
                Feedback.Message = TEXT("Follow completed");
            }
        }
        else
        {
            Feedback.Message = TargetLabel.IsEmpty()
                ? TEXT("Follow failed")
                : FString::Printf(TEXT("Follow failed: could not follow %s"), *TargetLabel);
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Follow completed") : TEXT("Follow failed");
    }
}

void FMAFeedbackGenerator::GenerateChargeFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        Feedback.Data.Add(TEXT("energy_before"), FString::Printf(TEXT("%.1f%%"), Context.EnergyBefore));
        Feedback.Data.Add(TEXT("energy_after"), FString::Printf(TEXT("%.1f%%"), Context.EnergyAfter));

        if (!Context.ChargingStationId.IsEmpty())
        {
            const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);
            MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
                Feedback, AllNodes, TEXT("station_id"), TEXT("station_label"), Context.ChargingStationId);
        }

        Feedback.Data.Add(TEXT("station_found"), Context.bChargingStationFound ? TEXT("true") : TEXT("false"));

        if (!Context.bChargingStationFound && !Context.ChargeErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.ChargeErrorReason);
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FString StationLabel = Feedback.Data.FindRef(TEXT("station_label"));

        if (!Context.bChargingStationFound)
        {
            Feedback.Message = Context.ChargeErrorReason.IsEmpty()
                ? TEXT("Charge failed: no charging station available")
                : FString::Printf(TEXT("Charge failed: %s"), *Context.ChargeErrorReason);
            Feedback.bSuccess = false;
        }
        else if (bSuccess)
        {
            Feedback.Message = !StationLabel.IsEmpty()
                ? FString::Printf(TEXT("Charge completed at %s, energy: %.1f%% -> %.1f%%"),
                    *StationLabel, Context.EnergyBefore, Context.EnergyAfter)
                : FString::Printf(TEXT("Charge completed, energy: %.1f%% -> %.1f%%"),
                    Context.EnergyBefore, Context.EnergyAfter);
        }
        else
        {
            Feedback.Message = FString::Printf(TEXT("Charge interrupted, energy: %.1f%%"), Context.EnergyAfter);
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Charge completed") : TEXT("Charge interrupted");
    }
}

void FMAFeedbackGenerator::GeneratePlaceFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);

        MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
            Feedback,
            AllNodes,
            TEXT("object_id"),
            TEXT("object_label"),
            Context.ObjectAttributes.FindRef(TEXT("object1_node_id")),
            Context.PlacedObjectName);

        MAFeedbackGeneratorInternal::AddSceneGraphNodeFields(
            Feedback,
            AllNodes,
            TEXT("target_id"),
            TEXT("target_label"),
            Context.ObjectAttributes.FindRef(TEXT("object2_node_id")),
            Context.PlaceTargetName);

        if (!Context.PlaceFinalLocation.IsZero())
        {
            Feedback.Data.Add(TEXT("final_location"), FString::Printf(TEXT("(%.1f, %.1f, %.1f)"),
                Context.PlaceFinalLocation.X, Context.PlaceFinalLocation.Y, Context.PlaceFinalLocation.Z));
        }

        if (!bSuccess && !Context.PlaceErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.PlaceErrorReason);
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FString ObjectLabel = Feedback.Data.FindRef(TEXT("object_label"));
        const FString TargetLabel = Feedback.Data.FindRef(TEXT("target_label"));

        if (bSuccess)
        {
            Feedback.Message = (!ObjectLabel.IsEmpty() && !TargetLabel.IsEmpty())
                ? FString::Printf(TEXT("Place succeeded: Moved %s to %s"), *ObjectLabel, *TargetLabel)
                : TEXT("Place completed");
        }
        else
        {
            Feedback.Message = !Context.PlaceErrorReason.IsEmpty()
                ? FString::Printf(TEXT("Place failed: %s"), *Context.PlaceErrorReason)
                : TEXT("Place failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Place completed") : TEXT("Place failed");
    }
}

void FMAFeedbackGenerator::GenerateTakeOffFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        Feedback.Data.Add(
            TEXT("target_height"),
            FString::Printf(TEXT("%.1f"), MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Params.TakeOffHeight)));
        Feedback.Data.Add(TEXT("height_adjusted"), Context.bTakeOffHeightAdjusted ? TEXT("true") : TEXT("false"));

        if (!Context.TakeOffNearbyBuildingLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("nearby_building_label"), Context.TakeOffNearbyBuildingLabel);
            Feedback.Data.Add(
                TEXT("nearby_building_height"),
                FString::Printf(TEXT("%.1f"), MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Context.TakeOffNearbyBuildingHeight)));
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        if (bSuccess)
        {
            Feedback.Message = (Context.bTakeOffHeightAdjusted && !Context.TakeOffNearbyBuildingLabel.IsEmpty())
                ? FString::Printf(TEXT("TakeOff completed at height %.0fm (adjusted above %s)"),
                    MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Params.TakeOffHeight),
                    *Context.TakeOffNearbyBuildingLabel)
                : FString::Printf(TEXT("TakeOff completed at height %.0fm"),
                    MAFeedbackGeneratorInternal::FeedbackDistanceMeters(Params.TakeOffHeight));
        }
        else
        {
            Feedback.Message = TEXT("TakeOff failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("TakeOff completed") : TEXT("TakeOff failed");
    }
}

void FMAFeedbackGenerator::GenerateLandFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        if (!Context.LandGroundType.IsEmpty())
        {
            Feedback.Data.Add(TEXT("ground_type"), Context.LandGroundType);
        }

        if (!Context.LandNearbyLandmarkLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("nearby_landmark_label"), Context.LandNearbyLandmarkLabel);
        }

        Feedback.Data.Add(TEXT("location_safe"), Context.bLandLocationSafe ? TEXT("true") : TEXT("false"));
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        if (bSuccess)
        {
            Feedback.Message = !Context.LandNearbyLandmarkLabel.IsEmpty()
                ? FString::Printf(TEXT("Land completed near %s"), *Context.LandNearbyLandmarkLabel)
                : TEXT("Land completed");
        }
        else
        {
            Feedback.Message = !Context.bLandLocationSafe
                ? TEXT("Land failed: unsafe landing location")
                : TEXT("Land failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Land completed") : TEXT("Land failed");
    }
}

void FMAFeedbackGenerator::GenerateReturnHomeFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        Feedback.Data.Add(TEXT("home_x"), FString::Printf(TEXT("%.1f"), Params.HomeLocation.X));
        Feedback.Data.Add(TEXT("home_y"), FString::Printf(TEXT("%.1f"), Params.HomeLocation.Y));

        if (!Context.HomeLandmarkLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("home_landmark_label"), Context.HomeLandmarkLabel);
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();

        if (bSuccess)
        {
            Feedback.Message = !Context.HomeLandmarkLabel.IsEmpty()
                ? FString::Printf(TEXT("ReturnHome completed at (%.0f, %.0f), near %s"),
                    Params.HomeLocation.X, Params.HomeLocation.Y, *Context.HomeLandmarkLabel)
                : FString::Printf(TEXT("ReturnHome completed at (%.0f, %.0f)"),
                    Params.HomeLocation.X, Params.HomeLocation.Y);
        }
        else
        {
            Feedback.Message = TEXT("ReturnHome failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("ReturnHome completed") : TEXT("ReturnHome failed");
    }
}

void FMAFeedbackGenerator::GenerateIdleFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    const bool bSuccess,
    const FString& Message)
{
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
    {
        AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    }

    Feedback.Message = Message.IsEmpty() ? TEXT("Idle") : Message;
}
