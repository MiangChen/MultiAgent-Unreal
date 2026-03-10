#include "MAModifyWidgetInputParser.h"

bool FMAModifyWidgetInputParser::ParseAnnotationInput(
    UWorld* World,
    const FString& Input,
    FMAAnnotationInput& OutResult,
    FString& OutError) const
{
    OutResult.Reset();
    OutError.Empty();

    if (Input.IsEmpty())
    {
        OutError = TEXT("Input cannot be empty");
        return false;
    }

    const FString InputLower = Input.ToLower();
    if (InputLower.Contains(TEXT("cate:")) || InputLower.Contains(TEXT("category:")))
    {
        return ParseAnnotationInputV2(World, Input, OutResult, OutError);
    }

    TArray<FString> Pairs;
    Input.ParseIntoArray(Pairs, TEXT(","), true);

    for (const FString& Pair : Pairs)
    {
        const FString TrimmedPair = Pair.TrimStartAndEnd();
        if (TrimmedPair.IsEmpty())
        {
            continue;
        }

        FString Key;
        FString Value;
        if (!TrimmedPair.Split(TEXT(":"), &Key, &Value))
        {
            OutError = FString::Printf(TEXT("Invalid key-value format: %s (expected key:value)"), *TrimmedPair);
            return false;
        }

        Key = Key.TrimStartAndEnd().ToLower();
        Value = Value.TrimStartAndEnd();

        if (Key.IsEmpty())
        {
            OutError = TEXT("Key name cannot be empty");
            return false;
        }
        if (Value.IsEmpty())
        {
            OutError = FString::Printf(TEXT("Value for key '%s' cannot be empty"), *Key);
            return false;
        }

        if (Key == TEXT("id"))
        {
            OutResult.Id = Value;
        }
        else if (Key == TEXT("type"))
        {
            OutResult.Type = Value;
        }
        else if (Key == TEXT("shape"))
        {
            const FString ShapeLower = Value.ToLower();
            if (ShapeLower != TEXT("polygon") && ShapeLower != TEXT("linestring") &&
                ShapeLower != TEXT("point") && ShapeLower != TEXT("rectangle"))
            {
                OutError = FString::Printf(TEXT("Invalid shape value: %s (expected polygon, linestring, point or rectangle)"), *Value);
                return false;
            }
            OutResult.Shape = ShapeLower;
        }
        else
        {
            OutResult.Properties.Add(Key, Value);
        }
    }

    if (OutResult.Id.IsEmpty())
    {
        OutError = TEXT("Missing required field: id");
        return false;
    }
    if (OutResult.Type.IsEmpty())
    {
        OutError = TEXT("Missing required field: type");
        return false;
    }

    return true;
}

bool FMAModifyWidgetInputParser::ParseAnnotationInputV2(
    UWorld* World,
    const FString& Input,
    FMAAnnotationInput& OutResult,
    FString& OutError) const
{
    OutResult.Reset();
    OutError.Empty();

    if (Input.IsEmpty())
    {
        OutError = TEXT("Input cannot be empty");
        return false;
    }

    TArray<FString> Pairs;
    Input.ParseIntoArray(Pairs, TEXT(","), true);

    bool bHasCate = false;
    bool bHasType = false;
    bool bHasExplicitShape = false;

    for (const FString& Pair : Pairs)
    {
        const FString TrimmedPair = Pair.TrimStartAndEnd();
        if (TrimmedPair.IsEmpty())
        {
            continue;
        }

        FString Key;
        FString Value;
        if (!TrimmedPair.Split(TEXT(":"), &Key, &Value))
        {
            OutError = FString::Printf(TEXT("Invalid key-value format: %s (expected key:value)"), *TrimmedPair);
            return false;
        }

        Key = Key.TrimStartAndEnd().ToLower();
        Value = Value.TrimStartAndEnd();

        if (Key.IsEmpty())
        {
            OutError = TEXT("Key name cannot be empty");
            return false;
        }
        if (Value.IsEmpty())
        {
            OutError = FString::Printf(TEXT("Value for key '%s' cannot be empty"), *Key);
            return false;
        }

        if (Key == TEXT("cate") || Key == TEXT("category"))
        {
            const EMANodeCategory ParsedCategory = FMAAnnotationInput::ParseCategoryFromString(Value);
            if (ParsedCategory == EMANodeCategory::None)
            {
                OutError = FString::Printf(TEXT("Invalid cate value: %s (expected building, trans_facility or prop)"), *Value);
                return false;
            }
            OutResult.Category = ParsedCategory;
            bHasCate = true;
        }
        else if (Key == TEXT("id"))
        {
            OutResult.Id = Value;
        }
        else if (Key == TEXT("type"))
        {
            OutResult.Type = Value;
            bHasType = true;
        }
        else if (Key == TEXT("shape"))
        {
            const FString ShapeLower = Value.ToLower();
            if (ShapeLower != TEXT("polygon") && ShapeLower != TEXT("linestring") &&
                ShapeLower != TEXT("point") && ShapeLower != TEXT("rectangle") &&
                ShapeLower != TEXT("prism"))
            {
                OutError = FString::Printf(TEXT("Invalid shape value: %s (expected polygon, linestring, point, rectangle or prism)"), *Value);
                return false;
            }
            OutResult.Shape = ShapeLower;
            bHasExplicitShape = true;
        }
        else
        {
            OutResult.Properties.Add(Key, Value);
        }
    }

    if (!bHasCate)
    {
        OutError = TEXT("Missing required field: cate (expected building, trans_facility or prop)");
        return false;
    }
    if (!bHasType)
    {
        OutError = TEXT("Missing required field: type");
        return false;
    }

    if (OutResult.Id.IsEmpty())
    {
        OutResult.Id = NodeBuilder.GetNextAvailableId(World);
    }

    if (!bHasExplicitShape)
    {
        switch (OutResult.Category)
        {
        case EMANodeCategory::Building:
            OutResult.Shape = TEXT("prism");
            break;
        case EMANodeCategory::TransFacility:
            OutResult.Shape = TEXT("linestring");
            break;
        case EMANodeCategory::Prop:
            OutResult.Shape = TEXT("point");
            break;
        case EMANodeCategory::None:
        default:
            break;
        }
    }

    return true;
}

bool FMAModifyWidgetInputParser::ValidateSelectionForCategory(
    EMANodeCategory Category,
    int32 SelectionCount,
    FString& OutError) const
{
    OutError.Empty();

    if (SelectionCount <= 0)
    {
        OutError = TEXT("Please select at least one Actor");
        return false;
    }

    if (Category == EMANodeCategory::Building && SelectionCount != 1)
    {
        OutError = TEXT("Building type only supports single-select, please select only one Actor");
        return false;
    }

    return true;
}
