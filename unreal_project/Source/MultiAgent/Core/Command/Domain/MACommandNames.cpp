#include "Core/Command/Domain/MACommandNames.h"

FString FMACommandNames::ToString(const EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Idle: return TEXT("idle");
        case EMACommand::Navigate: return TEXT("navigate");
        case EMACommand::Follow: return TEXT("follow");
        case EMACommand::Charge: return TEXT("charge");
        case EMACommand::Search: return TEXT("search");
        case EMACommand::Place: return TEXT("place");
        case EMACommand::TakeOff: return TEXT("take_off");
        case EMACommand::Land: return TEXT("land");
        case EMACommand::ReturnHome: return TEXT("return_home");
        case EMACommand::TakePhoto: return TEXT("take_photo");
        case EMACommand::Broadcast: return TEXT("broadcast");
        case EMACommand::HandleHazard: return TEXT("handle_hazard");
        case EMACommand::Guide: return TEXT("guide");
        default: return TEXT("None");
    }
}

EMACommand FMACommandNames::FromString(const FString& CommandString)
{
    if (CommandString.Equals(TEXT("idle"), ESearchCase::IgnoreCase)) return EMACommand::Idle;
    if (CommandString.Equals(TEXT("navigate"), ESearchCase::IgnoreCase)) return EMACommand::Navigate;
    if (CommandString.Equals(TEXT("follow"), ESearchCase::IgnoreCase)) return EMACommand::Follow;
    if (CommandString.Equals(TEXT("charge"), ESearchCase::IgnoreCase)) return EMACommand::Charge;
    if (CommandString.Equals(TEXT("search"), ESearchCase::IgnoreCase)) return EMACommand::Search;
    if (CommandString.Equals(TEXT("place"), ESearchCase::IgnoreCase)) return EMACommand::Place;
    if (CommandString.Equals(TEXT("take_off"), ESearchCase::IgnoreCase)) return EMACommand::TakeOff;
    if (CommandString.Equals(TEXT("land"), ESearchCase::IgnoreCase)) return EMACommand::Land;
    if (CommandString.Equals(TEXT("return_home"), ESearchCase::IgnoreCase)) return EMACommand::ReturnHome;
    if (CommandString.Equals(TEXT("take_photo"), ESearchCase::IgnoreCase)) return EMACommand::TakePhoto;
    if (CommandString.Equals(TEXT("broadcast"), ESearchCase::IgnoreCase)) return EMACommand::Broadcast;
    if (CommandString.Equals(TEXT("handle_hazard"), ESearchCase::IgnoreCase)) return EMACommand::HandleHazard;
    if (CommandString.Equals(TEXT("guide"), ESearchCase::IgnoreCase)) return EMACommand::Guide;
    return EMACommand::None;
}
