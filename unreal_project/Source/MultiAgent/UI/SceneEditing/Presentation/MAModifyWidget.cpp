
// Modify mode panel widget.

#include "MAModifyWidget.h"
#include "../Application/MAModifyWidgetCoordinator.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/BackgroundBlur.h"
#include "Blueprint/WidgetTree.h"
#include "../Infrastructure/MAModifyWidgetRuntimeAdapter.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "../../Core/MAFrostedGlassUtils.h"
#include "../../Core/MAUITheme.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAModifyWidget, Log, All);
namespace
{
    const FString ModifyDefaultHintText = TEXT("Input format: cate:building/trans_facility/prop,type:xxx\n• building: Buildings (prism modeling, single-select only)\n• trans_facility: Transport facilities (OBB rectangle)\n• prop: Props (point modeling)");
    const FString ModifyMultiSelectHintText = TEXT("Multi-select mode - Input format: cate:trans_facility/prop,type:xxx\nNote: building type only supports single-select");
    const FString ModifyBuildingHintText = TEXT("Building type (prism modeling)\nInput format: cate:building,type:xxx\n• Single-select only\n• Auto-calculates base polygon and height");
    const FString ModifyTransFacilityHintText = TEXT("TransFacility type (OBB rectangle modeling)\nInput format: cate:trans_facility,type:xxx\n• Supports single or multi-select\n• Auto-calculates minimum bounding rectangle");
    const FString ModifyPropHintText = TEXT("Prop type (point modeling)\nInput format: cate:prop,type:xxx\n• Supports single or multi-select\n• Auto-calculates geometric center");

    FLinearColor ResolvePreviewToneColor(UMAUITheme* Theme, EMAModifyPreviewTone Tone)
    {
        switch (Tone)
        {
        case EMAModifyPreviewTone::Info:
            return Theme ? Theme->PrimaryColor : FLinearColor(0.5f, 0.7f, 0.9f);
        case EMAModifyPreviewTone::Success:
            return Theme ? Theme->SuccessColor : FLinearColor(0.7f, 0.9f, 0.7f);
        case EMAModifyPreviewTone::Warning:
            return Theme ? Theme->WarningColor : FLinearColor(0.9f, 0.7f, 0.5f);
        case EMAModifyPreviewTone::Error:
            return Theme ? Theme->WarningColor : FLinearColor(0.9f, 0.4f, 0.4f);
        case EMAModifyPreviewTone::Default:
        default:
            return Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
        }
    }

    const FMAModifyWidgetRuntimeAdapter& ModifyWidgetRuntimeAdapter()
    {
        static const FMAModifyWidgetRuntimeAdapter Adapter;
        return Adapter;
    }
}

UMAModifyWidget::UMAModifyWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAModifyWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAModifyWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("ApplyTheme: Theme is null, using default colors"));
        return;
    }
    if (TitleText)
    {
        TitleText->SetColorAndOpacity(FSlateColor(Theme->ModeModifyColor));
    }
    if (HintText)
    {
        HintText->SetColorAndOpacity(FSlateColor(Theme->HintTextColor));
    }
    if (JsonPreviewText)
    {
        JsonPreviewText->SetColorAndOpacity(FSlateColor(Theme->SuccessColor));
    }

    UE_LOG(LogMAModifyWidget, Log, TEXT("ApplyTheme: Theme applied successfully"));
}

void UMAModifyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ConfirmButton && !ConfirmButton->OnClicked.IsAlreadyBound(this, &UMAModifyWidget::OnConfirmButtonClicked))
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UMAModifyWidget::OnConfirmButtonClicked);
        UE_LOG(LogMAModifyWidget, Log, TEXT("ConfirmButton event bound"));
    }
    ClearSelection();
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("MAModifyWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMAModifyWidget::RebuildWidget()
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

void UMAModifyWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("BuildUI: Starting UI construction..."));
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
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
        FVector2D(350, 520),
        FVector2D(0.0f, 1.0f),
        FAnchors(0.0f, 1.0f, 0.0f, 1.0f),
        TEXT("ModifyPanel")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMAModifyWidget, Error, TEXT("BuildUI: Failed to create frosted glass panel!"));
        return;
    }
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    FrostedGlass.ContentContainer->AddChild(MainVBox);
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Modify Panel")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    FLinearColor TitleColor = Theme ? Theme->ModeModifyColor : FLinearColor(1.0f, 0.6f, 0.0f);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 8));
    ModeIndicatorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeIndicatorText"));
    ModeIndicatorText->SetText(FText::FromString(TEXT("➕ Add New Node")));
    FSlateFontInfo ModeFont = ModeIndicatorText->GetFont();
    ModeFont.Size = 12;
    ModeIndicatorText->SetFont(ModeFont);
    FLinearColor ModeColor = Theme ? Theme->SuccessColor : FLinearColor(0.3f, 0.8f, 0.3f);
    ModeIndicatorText->SetColorAndOpacity(FSlateColor(ModeColor));
    
    UVerticalBoxSlot* ModeSlot = MainVBox->AddChildToVerticalBox(ModeIndicatorText);
    ModeSlot->SetPadding(FMargin(0, 0, 0, 10));
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(ModifyDefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    FLinearColor HintColor = Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
    HintText->SetColorAndOpacity(FSlateColor(HintColor));
    HintText->SetAutoWrapText(true);
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));
    UBorder* JsonPreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonPreviewBorder"));
    JsonPreviewBorder->SetPadding(FMargin(8.0f));
    FLinearColor JsonPreviewBgColor = Theme ? Theme->CanvasBackgroundColor : FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(JsonPreviewBorder, JsonPreviewBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    UScrollBox* JsonScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JsonScrollBox"));
    JsonPreviewBorder->AddChild(JsonScrollBox);
    
    JsonPreviewText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonPreviewText"));
    JsonPreviewText->SetText(FText::FromString(TEXT("Select an Actor to display JSON preview")));
    FSlateFontInfo JsonFont = JsonPreviewText->GetFont();
    JsonFont.Size = 11;
    JsonPreviewText->SetFont(JsonFont);
    FLinearColor JsonPreviewTextColor = Theme ? Theme->SuccessColor : FLinearColor(0.7f, 0.9f, 0.7f);
    JsonPreviewText->SetColorAndOpacity(FSlateColor(JsonPreviewTextColor));
    JsonPreviewText->SetAutoWrapText(true);
    JsonScrollBox->AddChild(JsonPreviewText);
    
    USizeBox* JsonPreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonPreviewSizeBox"));
    JsonPreviewSizeBox->SetMinDesiredHeight(80.0f);
    JsonPreviewSizeBox->SetMaxDesiredHeight(100.0f);
    JsonPreviewSizeBox->AddChild(JsonPreviewBorder);
    
    UVerticalBoxSlot* JsonPreviewSlot = MainVBox->AddChildToVerticalBox(JsonPreviewSizeBox);
    JsonPreviewSlot->SetPadding(FMargin(0, 0, 0, 10));
    LabelTextBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("LabelTextBox"));
    LabelTextBox->SetHintText(FText::FromString(ModifyDefaultHintText));
    FEditableTextBoxStyle TextBoxStyle;
    FLinearColor InputTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor BlackColor = FSlateColor(InputTextColor);
    TextBoxStyle.SetForegroundColor(BlackColor);
    TextBoxStyle.SetFocusedForegroundColor(BlackColor);
    FSlateFontInfo TextBoxFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
    TextBoxStyle.SetFont(TextBoxFont);
    FSlateBrush TransparentBrush;
    TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);
    TextBoxStyle.SetBackgroundImageNormal(TransparentBrush);
    TextBoxStyle.SetBackgroundImageHovered(TransparentBrush);
    TextBoxStyle.SetBackgroundImageFocused(TransparentBrush);
    TextBoxStyle.SetBackgroundImageReadOnly(TransparentBrush);
    
    LabelTextBox->WidgetStyle = TextBoxStyle;
    UBorder* LabelTextBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LabelTextBorder"));
    LabelTextBorder->SetPadding(FMargin(8.0f, 4.0f));
    FLinearColor LabelTextBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(LabelTextBorder, LabelTextBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    LabelTextBorder->AddChild(LabelTextBox);
    USizeBox* TextBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TextBoxSizeBox"));
    TextBoxSizeBox->SetMinDesiredHeight(150.0f);
    TextBoxSizeBox->AddChild(LabelTextBorder);
    
    UVerticalBoxSlot* TextBoxSlot = MainVBox->AddChildToVerticalBox(TextBoxSizeBox);
    TextBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TextBoxSlot->SetPadding(FMargin(0, 0, 0, 15));
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
    ButtonText->SetText(FText::FromString(TEXT("  Confirm  ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo ButtonFont = ButtonText->GetFont();
    ButtonFont.Size = 14;
    ButtonText->SetFont(ButtonFont);
    ConfirmButton->AddChild(ButtonText);
    
    UVerticalBoxSlot* ButtonSlot = MainVBox->AddChildToVerticalBox(ConfirmButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Center);

    UE_LOG(LogMAModifyWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}

void UMAModifyWidget::ApplySelectionViewModel(const FMAModifySelectionViewModel& ViewModel)
{
    SetAnnotationMode(ViewModel.AnnotationMode, ViewModel.EditingNodeId);

    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::FromString(ViewModel.LabelInputText));
        if (!ViewModel.LabelInputHintText.IsEmpty())
        {
            LabelTextBox->SetHintText(FText::FromString(ViewModel.LabelInputHintText));
        }
    }

    if (HintText)
    {
        HintText->SetText(FText::FromString(ViewModel.HintText));
        const bool bHasSelection = !ViewModel.HintText.Equals(ModifyDefaultHintText);
        const FLinearColor HintColor = bHasSelection
            ? (Theme ? Theme->SuccessColor : FLinearColor(0.3f, 0.8f, 0.3f))
            : (Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f));
        HintText->SetColorAndOpacity(FSlateColor(HintColor));
    }
}

void UMAModifyWidget::ApplyPreviewModel(const FMAModifyPreviewModel& PreviewModel)
{
    if (!JsonPreviewText)
    {
        return;
    }

    JsonPreviewText->SetText(FText::FromString(PreviewModel.Text));
    JsonPreviewText->SetColorAndOpacity(FSlateColor(ResolvePreviewToneColor(Theme, PreviewModel.Tone)));
}


void UMAModifyWidget::SetSelectedActor(AActor* Actor)
{
    if (!Actor)
    {
        ClearSelection();
        return;
    }

    SelectedActors.Empty();
    SelectedActors.Add(Actor);
    const FMAModifyWidgetSelectionModels Models = Coordinator.BuildSingleSelectionModels(this, Actor);
    ApplySelectionViewModel(Models.SelectionViewModel);
    ApplyPreviewModel(Models.PreviewModel);
}

void UMAModifyWidget::SetSelectedActors(const TArray<AActor*>& Actors)
{
    SelectedActors.Empty();
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            SelectedActors.Add(Actor);
        }
    }
    
    if (SelectedActors.Num() == 0)
    {
        ClearSelection();
        return;
    }
    const FMAModifyWidgetSelectionModels Models = Coordinator.BuildMultiSelectionModels(this, SelectedActors);
    ApplySelectionViewModel(Models.SelectionViewModel);
    ApplyPreviewModel(Models.PreviewModel);
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("SetSelectedActors: %d actors selected"), SelectedActors.Num());
}

void UMAModifyWidget::ClearSelection()
{
    SelectedActors.Empty();
    ApplySelectionViewModel(Coordinator.BuildClearedSelectionViewModel(ModifyDefaultHintText, ModifyMultiSelectHintText));
    ApplyPreviewModel(Coordinator.BuildClearedPreviewModel());

    UE_LOG(LogMAModifyWidget, Log, TEXT("ClearSelection: Selection cleared"));
}

FString UMAModifyWidget::GetLabelText() const
{
    if (LabelTextBox)
    {
        return LabelTextBox->GetText().ToString();
    }
    return FString();
}

void UMAModifyWidget::SetLabelText(const FString& Text)
{
    if (LabelTextBox)
    {
        LabelTextBox->SetText(FText::FromString(Text));
    }
}


void UMAModifyWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMAModifyWidget, Log, TEXT("ConfirmButton clicked, Mode=%s"),
        CurrentAnnotationMode == EMAAnnotationMode::AddNew ? TEXT("AddNew") : TEXT("EditExisting"));
    
    if (SelectedActors.Num() == 0)
    {
        UE_LOG(LogMAModifyWidget, Warning, TEXT("OnConfirmButtonClicked: No Actor selected, ignoring"));
        return;
    }
    
    const FString LabelContent = GetLabelText();
    const FMAModifyWidgetSubmitResult SubmitResult = Coordinator.BuildSubmitResult(
        this,
        SelectedActors,
        LabelContent,
        CurrentAnnotationMode,
        EditingNodeId);

    switch (SubmitResult.Kind)
    {
    case EMAModifyWidgetSubmitKind::Multi:
        OnMultiSelectModifyConfirmed.Broadcast(SelectedActors, SubmitResult.LabelText, SubmitResult.GeneratedJson);
        break;
    case EMAModifyWidgetSubmitKind::Single:
        OnModifyConfirmed.Broadcast(SelectedActors[0], SubmitResult.LabelText);
        break;
    case EMAModifyWidgetSubmitKind::None:
    default:
        break;
    }
}

void UMAModifyWidget::SetAnnotationMode(EMAAnnotationMode Mode, const FString& NodeId)
{
    CurrentAnnotationMode = Mode;
    EditingNodeId = NodeId;
    UpdateModeIndicator();
    
    UE_LOG(LogMAModifyWidget, Log, TEXT("SetAnnotationMode: %s, NodeId=%s"),
        Mode == EMAAnnotationMode::AddNew ? TEXT("AddNew") : TEXT("EditExisting"),
        *NodeId);
}

void UMAModifyWidget::UpdateModeIndicator()
{
    if (!ModeIndicatorText)
    {
        return;
    }
    
    if (CurrentAnnotationMode == EMAAnnotationMode::EditExisting)
    {
        ModeIndicatorText->SetText(FText::FromString(
            FString::Printf(TEXT("✏️ Editing: %s"), *EditingNodeId)));
        FLinearColor EditColor = Theme ? Theme->WarningColor : FLinearColor(1.0f, 0.8f, 0.0f);
        ModeIndicatorText->SetColorAndOpacity(FSlateColor(EditColor));
    }
    else
    {
        ModeIndicatorText->SetText(FText::FromString(TEXT("➕ Add New Node")));
        FLinearColor AddColor = Theme ? Theme->SuccessColor : FLinearColor(0.3f, 0.8f, 0.3f);
        ModeIndicatorText->SetColorAndOpacity(FSlateColor(AddColor));
    }
}

FMAModifyWidgetSelectionModels UMAModifyWidget::RuntimeBuildSingleSelectionModels(AActor* Actor) const
{
    return ModifyWidgetRuntimeAdapter().BuildSingleSelectionModels(
        this,
        Actor,
        ModifyDefaultHintText,
        ModifyMultiSelectHintText);
}

FMAModifyWidgetSelectionModels UMAModifyWidget::RuntimeBuildMultiSelectionModels(const TArray<AActor*>& Actors) const
{
    return ModifyWidgetRuntimeAdapter().BuildMultiSelectionModels(
        this,
        Actors,
        ModifyDefaultHintText,
        ModifyMultiSelectHintText);
}

FMAModifyWidgetSubmitResult UMAModifyWidget::RuntimeBuildSubmitResult(
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    EMAAnnotationMode AnnotationMode,
    const FString& NodeId) const
{
    return ModifyWidgetRuntimeAdapter().BuildSubmitResult(this, Actors, LabelText, AnnotationMode, NodeId);
}
