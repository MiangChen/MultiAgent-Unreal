
// Edit mode panel widget.

#include "MAEditWidget.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ComboBoxString.h"
#include "Components/BackgroundBlur.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "../../Core/MAFrostedGlassUtils.h"
#include "../../Core/MAUITheme.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAEditWidget, Log, All);

namespace
{
    const TCHAR* DefaultEditHintText = TEXT("Select an Actor or POI to operate");
}

UMAEditWidget::UMAEditWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAEditWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAEditWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("ApplyTheme: Theme is null, using default colors"));
        return;
    }
    if (TitleText)
    {
        TitleText->SetColorAndOpacity(FSlateColor(Theme->PrimaryColor));
    }
    if (HintText)
    {
        HintText->SetColorAndOpacity(FSlateColor(Theme->HintTextColor));
    }
    if (ErrorText)
    {
        ErrorText->SetColorAndOpacity(FSlateColor(Theme->DangerColor));
    }

    UE_LOG(LogMAEditWidget, Log, TEXT("ApplyTheme: Theme applied successfully"));
}

void UMAEditWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ConfirmButton)
    {
        if (!ConfirmButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnConfirmButtonClicked))
        {
            ConfirmButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnConfirmButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("ConfirmButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("ConfirmButton is null!"));
    }

    if (DeleteButton)
    {
        if (!DeleteButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnDeleteButtonClicked))
        {
            DeleteButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnDeleteButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("DeleteButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("DeleteButton is null!"));
    }

    if (CreateGoalButton)
    {
        if (!CreateGoalButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnCreateGoalButtonClicked))
        {
            CreateGoalButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnCreateGoalButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("CreateGoalButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("CreateGoalButton is null!"));
    }

    if (CreateZoneButton)
    {
        if (!CreateZoneButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnCreateZoneButtonClicked))
        {
            CreateZoneButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnCreateZoneButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("CreateZoneButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("CreateZoneButton is null!"));
    }

    if (AddActorButton)
    {
        if (!AddActorButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnAddActorButtonClicked))
        {
            AddActorButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnAddActorButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("AddActorButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("AddActorButton is null!"));
    }

    if (DeletePOIButton)
    {
        if (!DeletePOIButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnDeletePOIButtonClicked))
        {
            DeletePOIButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnDeletePOIButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("DeletePOIButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("DeletePOIButton is null!"));
    }

    if (SetAsGoalButton)
    {
        if (!SetAsGoalButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnSetAsGoalButtonClicked))
        {
            SetAsGoalButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnSetAsGoalButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("SetAsGoalButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("SetAsGoalButton is null!"));
    }

    if (UnsetAsGoalButton)
    {
        if (!UnsetAsGoalButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnUnsetAsGoalButtonClicked))
        {
            UnsetAsGoalButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnUnsetAsGoalButtonClicked);
            UE_LOG(LogMAEditWidget, Log, TEXT("UnsetAsGoalButton event bound"));
        }
    }
    else
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("UnsetAsGoalButton is null!"));
    }

    SetPresetActorOptions({TEXT("(No preset Actors)")});
    ApplyViewModel(FMAEditWidgetViewModel());
    
    UE_LOG(LogMAEditWidget, Log, TEXT("MAEditWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMAEditWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    if (!WidgetTree->RootWidget)
    {
        BuildUI();
    }
    
    return Super::RebuildWidget();
}

void UMAEditWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAEditWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMAEditWidget, Log, TEXT("BuildUI: Starting UI construction..."));
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAEditWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;
    if (!Theme)
    {
        Theme = NewObject<UMAUITheme>();
    }
    FMAFrostedGlassResult FrostedGlass = MAFrostedGlassUtils::CreateFixedSizePanelFromTheme(
        WidgetTree,
        RootCanvas,
        Theme,
        FVector2D(20, -20),
        FVector2D(380, 600),
        FVector2D(0.0f, 1.0f),
        FAnchors(0.0f, 1.0f, 0.0f, 1.0f),
        TEXT("EditPanel")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMAEditWidget, Error, TEXT("BuildUI: Failed to create frosted glass panel!"));
        return;
    }
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    FrostedGlass.ContentContainer->AddChild(MainVBox);
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Edit Mode - Scene Editor")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    FLinearColor TitleColor = Theme ? Theme->PrimaryColor : FLinearColor(0.3f, 0.6f, 1.0f);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));

    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 8));
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(DefaultEditHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    FLinearColor HintColor = Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
    HintText->SetColorAndOpacity(FSlateColor(HintColor));
    HintText->SetAutoWrapText(true);
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));
    ErrorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ErrorText"));
    ErrorText->SetText(FText::GetEmpty());
    FSlateFontInfo ErrorFont = ErrorText->GetFont();
    ErrorFont.Size = 11;
    ErrorText->SetFont(ErrorFont);
    FLinearColor ErrorColor = Theme ? Theme->DangerColor : FLinearColor(1.0f, 0.3f, 0.3f);
    ErrorText->SetColorAndOpacity(FSlateColor(ErrorColor));
    ErrorText->SetAutoWrapText(true);
    ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ErrorSlot = MainVBox->AddChildToVerticalBox(ErrorText);
    ErrorSlot->SetPadding(FMargin(0, 0, 0, 8));
    ActorOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActorOperationBox"));
    ActorOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ActorOpSlot = MainVBox->AddChildToVerticalBox(ActorOperationBox);
    ActorOpSlot->SetPadding(FMargin(0, 0, 0, 10));
    NodeSwitchContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NodeSwitchContainer"));
    NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* NodeSwitchSlot = ActorOperationBox->AddChildToVerticalBox(NodeSwitchContainer);
    NodeSwitchSlot->SetPadding(FMargin(0, 0, 0, 8));
    UTextBlock* JsonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonLabel"));
    JsonLabel->SetText(FText::FromString(TEXT("JSON Editor:")));
    FSlateFontInfo JsonLabelFont = JsonLabel->GetFont();
    JsonLabelFont.Size = 12;
    JsonLabel->SetFont(JsonLabelFont);
    FLinearColor JsonLabelColor = Theme ? Theme->LabelTextColor : FLinearColor(0.8f, 0.8f, 0.8f);
    JsonLabel->SetColorAndOpacity(FSlateColor(JsonLabelColor));
    
    UVerticalBoxSlot* JsonLabelSlot = ActorOperationBox->AddChildToVerticalBox(JsonLabel);
    JsonLabelSlot->SetPadding(FMargin(0, 0, 0, 4));
    JsonEditBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditBox"));
    JsonEditBox->SetHintText(FText::FromString(TEXT("Select an Actor to display JSON")));

    FEditableTextBoxStyle JsonTextBoxStyle;
    FLinearColor JsonInputTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor BlackColor = FSlateColor(JsonInputTextColor);
    JsonTextBoxStyle.SetForegroundColor(BlackColor);
    JsonTextBoxStyle.SetFocusedForegroundColor(BlackColor);
    FSlateFontInfo JsonFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
    JsonTextBoxStyle.SetFont(JsonFont);
    FSlateBrush TransparentBrush1;
    TransparentBrush1.TintColor = FSlateColor(FLinearColor::Transparent);
    JsonTextBoxStyle.SetBackgroundImageNormal(TransparentBrush1);
    JsonTextBoxStyle.SetBackgroundImageHovered(TransparentBrush1);
    JsonTextBoxStyle.SetBackgroundImageFocused(TransparentBrush1);
    JsonTextBoxStyle.SetBackgroundImageReadOnly(TransparentBrush1);
    
    JsonEditBox->WidgetStyle = JsonTextBoxStyle;
    UBorder* JsonEditBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonEditBorder"));
    JsonEditBorder->SetPadding(FMargin(8.0f, 4.0f));
    FLinearColor JsonEditBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(JsonEditBorder, JsonEditBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    JsonEditBorder->AddChild(JsonEditBox);
    
    USizeBox* JsonEditSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditSizeBox"));
    JsonEditSizeBox->SetMinDesiredHeight(150.0f);
    JsonEditSizeBox->SetMaxDesiredHeight(200.0f);
    JsonEditSizeBox->AddChild(JsonEditBorder);
    
    UVerticalBoxSlot* JsonEditSlot = ActorOperationBox->AddChildToVerticalBox(JsonEditSizeBox);
    JsonEditSlot->SetPadding(FMargin(0, 0, 0, 10));
    UHorizontalBox* ActorButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActorButtonBox"));
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmText"));
    ConfirmText->SetText(FText::FromString(TEXT("  Confirm  ")));
    FLinearColor ButtonTextColor = Theme ? Theme->InputTextColor : FLinearColor::Black;
    ConfirmText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
    FSlateFontInfo ConfirmFont = ConfirmText->GetFont();
    ConfirmFont.Size = 14;
    ConfirmText->SetFont(ConfirmFont);
    ConfirmButton->AddChild(ConfirmText);
    
    UHorizontalBoxSlot* ConfirmSlot = ActorButtonBox->AddChildToHorizontalBox(ConfirmButton);
    ConfirmSlot->SetPadding(FMargin(0, 0, 10, 0));
    DeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteButton"));
    UTextBlock* DeleteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteText"));
    DeleteText->SetText(FText::FromString(TEXT("  Delete  ")));
    DeleteText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
    FSlateFontInfo DeleteFont = DeleteText->GetFont();
    DeleteFont.Size = 14;
    DeleteText->SetFont(DeleteFont);
    DeleteButton->AddChild(DeleteText);
    
    UHorizontalBoxSlot* DeleteSlot = ActorButtonBox->AddChildToHorizontalBox(DeleteButton);
    DeleteSlot->SetPadding(FMargin(0, 0, 10, 0));
    SetAsGoalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SetAsGoalButton"));
    UTextBlock* SetGoalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SetGoalText"));
    SetGoalText->SetText(FText::FromString(TEXT(" Set as Goal ")));
    SetGoalText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
    FSlateFontInfo SetGoalFont = SetGoalText->GetFont();
    SetGoalFont.Size = 14;
    SetGoalText->SetFont(SetGoalFont);
    SetAsGoalButton->AddChild(SetGoalText);
    
    UHorizontalBoxSlot* SetGoalSlot = ActorButtonBox->AddChildToHorizontalBox(SetAsGoalButton);
    SetGoalSlot->SetPadding(FMargin(0, 0, 10, 0));
    UnsetAsGoalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UnsetAsGoalButton"));
    UTextBlock* UnsetGoalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnsetGoalText"));
    UnsetGoalText->SetText(FText::FromString(TEXT(" Unset Goal ")));
    UnsetGoalText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
    FSlateFontInfo UnsetGoalFont = UnsetGoalText->GetFont();
    UnsetGoalFont.Size = 14;
    UnsetGoalText->SetFont(UnsetGoalFont);
    UnsetAsGoalButton->AddChild(UnsetGoalText);
    
    ActorButtonBox->AddChildToHorizontalBox(UnsetAsGoalButton);
    
    UVerticalBoxSlot* ActorButtonSlot = ActorOperationBox->AddChildToVerticalBox(ActorButtonBox);
    ActorButtonSlot->SetHorizontalAlignment(HAlign_Left);
    POIOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("POIOperationBox"));
    POIOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* POIOpSlot = MainVBox->AddChildToVerticalBox(POIOperationBox);
    POIOpSlot->SetPadding(FMargin(0, 0, 0, 10));
    UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescLabel"));
    DescLabel->SetText(FText::FromString(TEXT("Description:")));
    FSlateFontInfo DescLabelFont = DescLabel->GetFont();
    DescLabelFont.Size = 12;
    DescLabel->SetFont(DescLabelFont);
    FLinearColor DescLabelColor = Theme ? Theme->LabelTextColor : FLinearColor(0.8f, 0.8f, 0.8f);
    DescLabel->SetColorAndOpacity(FSlateColor(DescLabelColor));
    
    UVerticalBoxSlot* DescLabelSlot = POIOperationBox->AddChildToVerticalBox(DescLabel);
    DescLabelSlot->SetPadding(FMargin(0, 0, 0, 4));
    DescriptionBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("DescriptionBox"));
    DescriptionBox->SetHintText(FText::FromString(TEXT("Enter description for Goal/Zone...")));

    FEditableTextBoxStyle DescTextBoxStyle;
    FLinearColor DescInputTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor BlackColor2 = FSlateColor(DescInputTextColor);
    DescTextBoxStyle.SetForegroundColor(BlackColor2);
    DescTextBoxStyle.SetFocusedForegroundColor(BlackColor2);
    FSlateFontInfo DescFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
    DescTextBoxStyle.SetFont(DescFont);
    FSlateBrush TransparentBrush2;
    TransparentBrush2.TintColor = FSlateColor(FLinearColor::Transparent);
    DescTextBoxStyle.SetBackgroundImageNormal(TransparentBrush2);
    DescTextBoxStyle.SetBackgroundImageHovered(TransparentBrush2);
    DescTextBoxStyle.SetBackgroundImageFocused(TransparentBrush2);
    DescTextBoxStyle.SetBackgroundImageReadOnly(TransparentBrush2);
    
    DescriptionBox->WidgetStyle = DescTextBoxStyle;
    UBorder* DescriptionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DescriptionBorder"));
    DescriptionBorder->SetPadding(FMargin(8.0f, 4.0f));
    FLinearColor DescBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(DescriptionBorder, DescBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    DescriptionBorder->AddChild(DescriptionBox);
    
    USizeBox* DescSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DescSizeBox"));
    DescSizeBox->SetMinDesiredHeight(80.0f);
    DescSizeBox->SetMaxDesiredHeight(100.0f);
    DescSizeBox->AddChild(DescriptionBorder);
    
    UVerticalBoxSlot* DescSlot = POIOperationBox->AddChildToVerticalBox(DescSizeBox);
    DescSlot->SetPadding(FMargin(0, 0, 0, 10));
    UHorizontalBox* POIButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("POIButtonBox"));
    FLinearColor POIButtonTextColor = Theme ? Theme->InputTextColor : FLinearColor::Black;
    CreateGoalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateGoalButton"));
    UTextBlock* GoalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalText"));
    GoalText->SetText(FText::FromString(TEXT(" Create Goal ")));
    GoalText->SetColorAndOpacity(FSlateColor(POIButtonTextColor));
    FSlateFontInfo GoalFont = GoalText->GetFont();
    GoalFont.Size = 14;
    GoalText->SetFont(GoalFont);
    CreateGoalButton->AddChild(GoalText);
    
    UHorizontalBoxSlot* GoalSlot = POIButtonBox->AddChildToHorizontalBox(CreateGoalButton);
    GoalSlot->SetPadding(FMargin(0, 0, 10, 0));
    CreateZoneButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateZoneButton"));
    UTextBlock* ZoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneText"));
    ZoneText->SetText(FText::FromString(TEXT(" Create Zone ")));
    ZoneText->SetColorAndOpacity(FSlateColor(POIButtonTextColor));
    FSlateFontInfo ZoneFont = ZoneText->GetFont();
    ZoneFont.Size = 14;
    ZoneText->SetFont(ZoneFont);
    CreateZoneButton->AddChild(ZoneText);
    
    UHorizontalBoxSlot* ZoneSlot = POIButtonBox->AddChildToHorizontalBox(CreateZoneButton);
    ZoneSlot->SetPadding(FMargin(0, 0, 10, 0));
    DeletePOIButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeletePOIButton"));
    UTextBlock* DeletePOIText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeletePOIText"));
    DeletePOIText->SetText(FText::FromString(TEXT(" Delete POI ")));
    DeletePOIText->SetColorAndOpacity(FSlateColor(POIButtonTextColor));
    FSlateFontInfo DeletePOIFont = DeletePOIText->GetFont();
    DeletePOIFont.Size = 14;
    DeletePOIText->SetFont(DeletePOIFont);
    DeletePOIButton->AddChild(DeletePOIText);
    
    POIButtonBox->AddChildToHorizontalBox(DeletePOIButton);
    
    UVerticalBoxSlot* POIButtonSlot = POIOperationBox->AddChildToVerticalBox(POIButtonBox);
    POIButtonSlot->SetPadding(FMargin(0, 0, 0, 10));
    UHorizontalBox* PresetActorBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PresetActorBox"));
    PresetActorComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PresetActorComboBox"));
    PresetActorComboBox->AddOption(TEXT("(No preset Actors)"));
    PresetActorComboBox->SetSelectedIndex(0);
    
    UHorizontalBoxSlot* ComboSlot = PresetActorBox->AddChildToHorizontalBox(PresetActorComboBox);
    ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ComboSlot->SetPadding(FMargin(0, 0, 10, 0));
    AddActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddActorButton"));
    UTextBlock* AddActorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddActorText"));
    AddActorText->SetText(FText::FromString(TEXT(" Add ")));
    FLinearColor AddActorTextColor = Theme ? Theme->InputTextColor : FLinearColor::Black;
    AddActorText->SetColorAndOpacity(FSlateColor(AddActorTextColor));
    FSlateFontInfo AddActorFont = AddActorText->GetFont();
    AddActorFont.Size = 14;
    AddActorText->SetFont(AddActorFont);
    AddActorButton->AddChild(AddActorText);
    
    PresetActorBox->AddChildToHorizontalBox(AddActorButton);
    
    UVerticalBoxSlot* PresetSlot = POIOperationBox->AddChildToVerticalBox(PresetActorBox);
    PresetSlot->SetPadding(FMargin(0, 0, 0, 0));

    UE_LOG(LogMAEditWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}

void UMAEditWidget::ApplyViewModel(const FMAEditWidgetViewModel& InViewModel)
{
    ViewModel = InViewModel;

    UpdateUIState();
    UpdateJsonEditBox();
    UpdateNodeSwitchButtons();

    if (DescriptionBox && !ViewModel.bShowPOIOperations)
    {
        DescriptionBox->SetText(FText::GetEmpty());
    }

    if (HintText)
    {
        HintText->SetText(FText::FromString(ViewModel.HintText));
        HintText->SetColorAndOpacity(FSlateColor(ResolveHintColor(ViewModel.HintTone)));
    }

    if (ViewModel.ErrorText.IsEmpty())
    {
        ClearError();
    }
    else
    {
        ShowError(ViewModel.ErrorText);
    }
}

void UMAEditWidget::SetPresetActorOptions(const TArray<FString>& Options)
{
    if (!PresetActorComboBox)
    {
        return;
    }

    PresetActorComboBox->ClearOptions();
    if (Options.Num() == 0)
    {
        PresetActorComboBox->AddOption(TEXT("(No preset Actors)"));
    }
    else
    {
        for (const FString& Option : Options)
        {
            PresetActorComboBox->AddOption(Option);
        }
    }
    PresetActorComboBox->SetSelectedIndex(0);
}

bool UMAEditWidget::ValidateJsonDocument(const FString& Json, FString& OutError) const
{
    if (Json.IsEmpty())
    {
        OutError = TEXT("JSON content cannot be empty");
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format, please check syntax");
        return false;
    }

    if (!JsonObject->HasField(TEXT("id")))
    {
        OutError = TEXT("Missing required field: id");
        return false;
    }

    OutError.Empty();
    return true;
}

FString UMAEditWidget::BuildEditableJson(const FMASceneGraphNode& Node) const
{
    if (!Node.RawJson.IsEmpty())
    {
        return Node.RawJson;
    }

    return FString::Printf(
        TEXT("{\n  \"id\": \"%s\",\n  \"type\": \"%s\",\n  \"label\": \"%s\",\n  \"shape\": {\n    \"type\": \"%s\",\n    \"center\": [%.0f, %.0f, %.0f]\n  }\n}"),
        *Node.Id,
        *Node.Type,
        *Node.Label,
        *Node.ShapeType,
        Node.Center.X, Node.Center.Y, Node.Center.Z);
}

bool UMAEditWidget::IsPointTypeNode(const FMASceneGraphNode& Node) const
{
    return Node.ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase);
}

bool UMAEditWidget::IsGoalOrZoneActor(const AActor* Actor) const
{
    return Actor && (Actor->IsA<AMAGoalActor>() || Actor->IsA<AMAZoneActor>());
}

bool UMAEditWidget::IsNodeMarkedGoal(const FMASceneGraphNode& Node) const
{
    return Node.RawJson.Contains(TEXT("\"is_goal\"")) &&
        (Node.RawJson.Contains(TEXT("\"is_goal\": true")) ||
         Node.RawJson.Contains(TEXT("\"is_goal\":true")));
}

FString UMAEditWidget::GetDescriptionText() const
{
    return DescriptionBox ? DescriptionBox->GetText().ToString() : FString();
}

FString UMAEditWidget::GetJsonEditContent() const
{
    return JsonEditBox ? JsonEditBox->GetText().ToString() : FString();
}

void UMAEditWidget::UpdateUIState()
{
    if (ActorOperationBox)
    {
        ActorOperationBox->SetVisibility(ViewModel.bShowActorOperations ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (POIOperationBox)
    {
        POIOperationBox->SetVisibility(ViewModel.bShowPOIOperations ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (CreateGoalButton)
    {
        CreateGoalButton->SetVisibility(ViewModel.bShowCreateGoal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (CreateZoneButton)
    {
        CreateZoneButton->SetVisibility(ViewModel.bShowCreateZone ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (PresetActorComboBox)
    {
        PresetActorComboBox->SetVisibility(ViewModel.bShowPresetActorControls ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (AddActorButton)
    {
        AddActorButton->SetVisibility(ViewModel.bShowPresetActorControls ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (DeleteButton)
    {
        DeleteButton->SetVisibility(ViewModel.bShowDeleteActor ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (SetAsGoalButton)
    {
        SetAsGoalButton->SetVisibility(ViewModel.bShowSetAsGoal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (UnsetAsGoalButton)
    {
        UnsetAsGoalButton->SetVisibility(ViewModel.bShowUnsetAsGoal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UMAEditWidget::UpdateJsonEditBox()
{
    if (!JsonEditBox)
    {
        return;
    }

    JsonEditBox->SetText(FText::FromString(ViewModel.JsonContent));
    JsonEditBox->SetIsReadOnly(ViewModel.bJsonReadOnly);
}

void UMAEditWidget::UpdateNodeSwitchButtons()
{
    if (!NodeSwitchContainer)
    {
        return;
    }

    NodeSwitchContainer->ClearChildren();
    NodeSwitchButtons.Empty();

    if (ViewModel.NodeTabs.Num() <= 1)
    {
        NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    NodeSwitchContainer->SetVisibility(ESlateVisibility::Visible);

    for (int32 Index = 0; Index < ViewModel.NodeTabs.Num(); ++Index)
    {
        const FMAEditWidgetNodeTabViewModel& Tab = ViewModel.NodeTabs[Index];

        UButton* NodeButton = NewObject<UButton>(this);
        UTextBlock* ButtonText = NewObject<UTextBlock>(this);
        ButtonText->SetText(FText::FromString(Tab.Label));
        ButtonText->SetColorAndOpacity(FSlateColor(Tab.bSelected
            ? (Theme ? Theme->ModeEditColor : FLinearColor(0.0f, 0.5f, 1.0f))
            : (Theme ? Theme->InputTextColor : FLinearColor::Black)));

        FSlateFontInfo ButtonFont = ButtonText->GetFont();
        ButtonFont.Size = 10;
        ButtonText->SetFont(ButtonFont);
        NodeButton->AddChild(ButtonText);

        NodeSwitchButtons.Add(NodeButton);
        NodeButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnNodeSwitchButtonClickedInternal);

        UHorizontalBoxSlot* ButtonSlot = NodeSwitchContainer->AddChildToHorizontalBox(NodeButton);
        ButtonSlot->SetPadding(FMargin(0, 0, 5, 0));
    }
}

void UMAEditWidget::ShowError(const FString& ErrorMessage)
{
    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(ErrorMessage));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
    }

    UE_LOG(LogMAEditWidget, Warning, TEXT("ShowError: %s"), *ErrorMessage);
}

void UMAEditWidget::ClearError()
{
    if (ErrorText)
    {
        ErrorText->SetText(FText::GetEmpty());
        ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

FLinearColor UMAEditWidget::ResolveHintColor(EMAEditWidgetHintTone Tone) const
{
    switch (Tone)
    {
    case EMAEditWidgetHintTone::Info:
        return Theme ? Theme->PrimaryColor : FLinearColor(0.3f, 0.6f, 1.0f);
    case EMAEditWidgetHintTone::Success:
        return Theme ? Theme->SuccessColor : FLinearColor(0.3f, 0.8f, 0.3f);
    case EMAEditWidgetHintTone::Warning:
        return Theme ? Theme->WarningColor : FLinearColor(1.0f, 0.6f, 0.0f);
    case EMAEditWidgetHintTone::Error:
        return Theme ? Theme->DangerColor : FLinearColor(1.0f, 0.3f, 0.3f);
    case EMAEditWidgetHintTone::Default:
    default:
        return Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
    }
}

void UMAEditWidget::OnConfirmButtonClicked()
{
    OnEditConfirmRequested.Broadcast(GetJsonEditContent());
}

void UMAEditWidget::OnDeleteButtonClicked()
{
    OnDeleteActorRequested.Broadcast();
}

void UMAEditWidget::OnCreateGoalButtonClicked()
{
    OnCreateGoalRequested.Broadcast(GetDescriptionText());
}

void UMAEditWidget::OnCreateZoneButtonClicked()
{
    OnCreateZoneRequested.Broadcast(GetDescriptionText());
}

void UMAEditWidget::OnAddActorButtonClicked()
{
    OnAddPresetActorRequested.Broadcast(PresetActorComboBox ? PresetActorComboBox->GetSelectedOption() : FString());
}

void UMAEditWidget::OnNodeSwitchButtonClicked(int32 NodeIndex)
{
    OnNodeSwitchRequested.Broadcast(NodeIndex);
}

void UMAEditWidget::OnNodeSwitchButtonClickedInternal()
{
    // Dynamic buttons do not carry an index payload, so resolve the source from focus/cursor state.
    for (int32 Index = 0; Index < NodeSwitchButtons.Num(); ++Index)
    {
        if (NodeSwitchButtons[Index] && NodeSwitchButtons[Index]->HasKeyboardFocus())
        {
            OnNodeSwitchButtonClicked(Index);
            return;
        }
    }

    const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (FocusedWidget.IsValid())
    {
        for (int32 Index = 0; Index < NodeSwitchButtons.Num(); ++Index)
        {
            if (NodeSwitchButtons[Index])
            {
                const TSharedPtr<SWidget> ButtonWidget = NodeSwitchButtons[Index]->GetCachedWidget();
                if (ButtonWidget.IsValid() && ButtonWidget == FocusedWidget)
                {
                    OnNodeSwitchButtonClicked(Index);
                    return;
                }
            }
        }
    }

    const FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
    for (int32 Index = 0; Index < NodeSwitchButtons.Num(); ++Index)
    {
        if (NodeSwitchButtons[Index])
        {
            const FGeometry Geometry = NodeSwitchButtons[Index]->GetCachedGeometry();
            const FVector2D LocalPos = Geometry.AbsoluteToLocal(MousePos);
            const FVector2D Size = Geometry.GetLocalSize();

            if (LocalPos.X >= 0 && LocalPos.X <= Size.X && LocalPos.Y >= 0 && LocalPos.Y <= Size.Y)
            {
                OnNodeSwitchButtonClicked(Index);
                return;
            }
        }
    }

    UE_LOG(LogMAEditWidget, Warning, TEXT("OnNodeSwitchButtonClickedInternal: Could not determine which button was clicked"));
}

void UMAEditWidget::OnSetAsGoalButtonClicked()
{
    OnSetAsGoalRequested.Broadcast();
}

void UMAEditWidget::OnUnsetAsGoalButtonClicked()
{
    OnUnsetAsGoalRequested.Broadcast();
}

void UMAEditWidget::OnDeletePOIButtonClicked()
{
    OnDeletePOIsRequested.Broadcast();
}
