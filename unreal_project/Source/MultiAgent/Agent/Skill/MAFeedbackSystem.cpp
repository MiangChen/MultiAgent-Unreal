// MAFeedbackSystem.cpp

#include "MAFeedbackSystem.h"
#include "../../Core/Manager/MACommandManager.h"

FMAFeedbackTemplates& FMAFeedbackTemplates::Get()
{
    static FMAFeedbackTemplates Instance;
    return Instance;
}

FMAFeedbackTemplates::FMAFeedbackTemplates()
{
    InitializeTemplates();
}

void FMAFeedbackTemplates::InitializeTemplates()
{
    Templates.Add(EMACommand::Idle, {
        TEXT("{agent} is now idle."),
        TEXT("{agent} failed to stop."),
        TEXT("{agent} is idle.")
    });
    
    Templates.Add(EMACommand::Navigate, {
        TEXT("{agent} navigated to ({x}, {y}) successfully."),
        TEXT("{agent} failed to navigate to ({x}, {y})."),
        TEXT("{agent} is navigating to ({x}, {y}).")
    });
    
    Templates.Add(EMACommand::Follow, {
        TEXT("{agent} is following {target}."),
        TEXT("{agent} lost track of {target}."),
        TEXT("{agent} is following {target}.")
    });
    
    Templates.Add(EMACommand::Charge, {
        TEXT("{agent} charged from {energy_before}% to {energy_after}%."),
        TEXT("{agent} charging interrupted at {energy_after}%."),
        TEXT("{agent} is charging ({energy_after}%).")
    });
    
    Templates.Add(EMACommand::Search, {
        TEXT("{agent} completed search. Found: {found_objects}."),
        TEXT("{agent} completed search. No targets found."),
        TEXT("{agent} is searching ({current}/{total}).")
    });
    
    Templates.Add(EMACommand::Place, {
        TEXT("{agent} placed {placed_object} on {place_target}."),
        TEXT("{agent} failed to place {placed_object} on {place_target}."),
        TEXT("{agent} is placing {placed_object} on {place_target}.")
    });
}

FString FMAFeedbackTemplates::GenerateMessage(const FString& AgentName, EMACommand Command, bool bSuccess, const FMAFeedbackContext& Context) const
{
    const FMessageTemplate* Template = Templates.Find(Command);
    if (!Template)
    {
        return FString::Printf(TEXT("%s: %s"), *AgentName, *UMACommandManager::CommandToString(Command));
    }
    
    FString Message = bSuccess ? Template->InProgressTemplate : Template->FailureTemplate;
    
    // 替换占位符
    Message = Message.Replace(TEXT("{agent}"), *AgentName);
    Message = Message.Replace(TEXT("{x}"), *FString::Printf(TEXT("%.0f"), Context.TargetLocation.X));
    Message = Message.Replace(TEXT("{y}"), *FString::Printf(TEXT("%.0f"), Context.TargetLocation.Y));
    Message = Message.Replace(TEXT("{target}"), *Context.TargetName);
    Message = Message.Replace(TEXT("{current}"), *FString::FromInt(Context.CurrentStep));
    Message = Message.Replace(TEXT("{total}"), *FString::FromInt(Context.TotalSteps));
    Message = Message.Replace(TEXT("{energy_before}"), *FString::Printf(TEXT("%.0f"), Context.EnergyBefore));
    Message = Message.Replace(TEXT("{energy_after}"), *FString::Printf(TEXT("%.0f"), Context.EnergyAfter));
    Message = Message.Replace(TEXT("{placed_object}"), *Context.PlacedObjectName);
    Message = Message.Replace(TEXT("{place_target}"), *Context.PlaceTargetName);
    
    if (Context.FoundObjects.Num() > 0)
    {
        Message = Message.Replace(TEXT("{found_objects}"), *FString::Join(Context.FoundObjects, TEXT(", ")));
    }
    else
    {
        Message = Message.Replace(TEXT("{found_objects}"), TEXT("none"));
    }
    
    return Message;
}
