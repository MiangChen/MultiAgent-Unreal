// MAFeedbackGenerator.cpp
// 反馈生成器实现

#include "MAFeedbackGenerator.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Manager/MACommandManager.h"

FMASkillExecutionFeedback FMAFeedbackGenerator::Generate(AMACharacter* Agent, EMACommand Command, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    if (!Agent)
    {
        Feedback.bSuccess = false;
        Feedback.Message = TEXT("Invalid agent");
        return Feedback;
    }
    
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = CommandToSkillName(Command);
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    
    switch (Command)
    {
        case EMACommand::Navigate:
            return GenerateNavigateFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Search:
            return GenerateSearchFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Follow:
            return GenerateFollowFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Charge:
            return GenerateChargeFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Place:
            return GeneratePlaceFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::TakeOff:
            return GenerateTakeOffFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Land:
            return GenerateLandFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::ReturnHome:
            return GenerateReturnHomeFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Idle:
            return GenerateIdleFeedback(Agent, bSuccess, Message);
        default:
            Feedback.bSuccess = bSuccess;
            Feedback.Message = Message;
            return Feedback;
    }
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateNavigateFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Navigate");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        FVector Loc = Params.TargetLocation;
        Feedback.Data.Add(TEXT("target_x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Feedback.Data.Add(TEXT("target_y"), FString::Printf(TEXT("%.1f"), Loc.Y));
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("Navigation completed") : TEXT("Navigation failed")) 
        : Message;
    
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateSearchFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Search");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        int32 FoundCount = Context.FoundObjects.Num();
        Feedback.Data.Add(TEXT("found_count"), FString::FromInt(FoundCount));
        
        if (FoundCount > 0)
        {
            // 添加找到的对象信息
            for (int32 i = 0; i < FoundCount; ++i)
            {
                Feedback.Data.Add(FString::Printf(TEXT("object_%d"), i), Context.FoundObjects[i]);
                if (Context.FoundLocations.IsValidIndex(i))
                {
                    FVector Loc = Context.FoundLocations[i];
                    Feedback.Data.Add(FString::Printf(TEXT("location_%d"), i), 
                        FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), Loc.X, Loc.Y, Loc.Z));
                }
            }
        }
    }
    
    // 优先使用传入的 Message
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        int32 FoundCount = SkillComp->GetFeedbackContext().FoundObjects.Num();
        Feedback.Message = FoundCount > 0 
            ? FString::Printf(TEXT("Search completed, found %d object(s)"), FoundCount)
            : TEXT("Search completed, no objects found");
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Search completed") : TEXT("Search failed");
    }
    
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateFollowFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Follow");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        if (!Context.TargetName.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target"), Context.TargetName);
        }
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("Follow completed") : TEXT("Follow failed")) 
        : Message;
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateChargeFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Charge");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        Feedback.Data.Add(TEXT("energy_before"), FString::Printf(TEXT("%.1f%%"), Context.EnergyBefore));
        Feedback.Data.Add(TEXT("energy_after"), FString::Printf(TEXT("%.1f%%"), Context.EnergyAfter));
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("Charge completed") : TEXT("Charge interrupted")) 
        : Message;
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GeneratePlaceFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Place");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        if (!Context.PlacedObjectName.IsEmpty())
        {
            Feedback.Data.Add(TEXT("object"), Context.PlacedObjectName);
        }
        if (!Context.PlaceTargetName.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target"), Context.PlaceTargetName);
        }
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("Place completed") : TEXT("Place failed")) 
        : Message;
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateTakeOffFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("TakeOff");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        Feedback.Data.Add(TEXT("target_height"), FString::Printf(TEXT("%.1f"), Params.TakeOffHeight));
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("TakeOff completed") : TEXT("TakeOff failed")) 
        : Message;
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateLandFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Land");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        Feedback.Data.Add(TEXT("target_height"), FString::Printf(TEXT("%.1f"), Params.LandHeight));
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("Land completed") : TEXT("Land failed")) 
        : Message;
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateReturnHomeFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("ReturnHome");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        Feedback.Data.Add(TEXT("home_x"), FString::Printf(TEXT("%.1f"), Params.HomeLocation.X));
        Feedback.Data.Add(TEXT("home_y"), FString::Printf(TEXT("%.1f"), Params.HomeLocation.Y));
        Feedback.Data.Add(TEXT("land_height"), FString::Printf(TEXT("%.1f"), Params.LandHeight));
    }
    
    Feedback.Message = Message.IsEmpty() 
        ? (bSuccess ? TEXT("ReturnHome completed") : TEXT("ReturnHome failed")) 
        : Message;
    return Feedback;
}

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateIdleFeedback(AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Idle");
    Feedback.bSuccess = true;
    Feedback.Message = Message.IsEmpty() ? TEXT("Idle") : Message;
    return Feedback;
}

FString FMAFeedbackGenerator::CommandToSkillName(EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Idle: return TEXT("Idle");
        case EMACommand::Navigate: return TEXT("Navigate");
        case EMACommand::Follow: return TEXT("Follow");
        case EMACommand::Charge: return TEXT("Charge");
        case EMACommand::Search: return TEXT("Search");
        case EMACommand::Place: return TEXT("Place");
        case EMACommand::TakeOff: return TEXT("TakeOff");
        case EMACommand::Land: return TEXT("Land");
        case EMACommand::ReturnHome: return TEXT("ReturnHome");
        default: return TEXT("Unknown");
    }
}
