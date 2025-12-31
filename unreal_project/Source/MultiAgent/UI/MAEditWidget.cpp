// MAEditWidget.cpp
// Edit Mode 编辑面板 Widget - 纯 C++ 实现
// Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 6.1, 6.2, 6.3, 6.4, 7.1, 8.1, 8.2, 8.5, 9.1, 9.2, 9.6, 10.1, 10.2, 10.7, 13.1, 13.2, 13.3, 13.4

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
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "../Core/Manager/MAEditModeManager.h"
#include "../Core/Manager/MASceneGraphManager.h"
#include "../Environment/MAPointOfInterest.h"
#include "../Environment/MAGoalActor.h"
#include "../Environment/MAZoneActor.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAEditWidget, Log, All);

// 提示文字常量
static const FString DefaultHintText = TEXT("选中 Actor 或 POI 进行操作");
static const FString ActorSelectedHintText = TEXT("编辑 JSON 后点击确认保存");
static const FString POISingleHintText = TEXT("可创建 Goal 或添加预设 Actor");
static const FString POIMultiHintText = TEXT("选中 3 个以上 POI 可创建区域");

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


void UMAEditWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (ConfirmButton)
    {
        if (!ConfirmButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnConfirmButtonClicked))
        {
            ConfirmButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnConfirmButtonClicked);
        }
    }
    
    if (DeleteButton)
    {
        if (!DeleteButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnDeleteButtonClicked))
        {
            DeleteButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnDeleteButtonClicked);
        }
    }
    
    if (CreateGoalButton)
    {
        if (!CreateGoalButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnCreateGoalButtonClicked))
        {
            CreateGoalButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnCreateGoalButtonClicked);
        }
    }
    
    if (CreateZoneButton)
    {
        if (!CreateZoneButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnCreateZoneButtonClicked))
        {
            CreateZoneButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnCreateZoneButtonClicked);
        }
    }
    
    if (AddActorButton)
    {
        if (!AddActorButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnAddActorButtonClicked))
        {
            AddActorButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnAddActorButtonClicked);
        }
    }
    
    if (DeletePOIButton)
    {
        if (!DeletePOIButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnDeletePOIButtonClicked))
        {
            DeletePOIButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnDeletePOIButtonClicked);
        }
    }
    
    if (SetAsGoalButton)
    {
        if (!SetAsGoalButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnSetAsGoalButtonClicked))
        {
            SetAsGoalButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnSetAsGoalButtonClicked);
        }
    }
    
    if (UnsetAsGoalButton)
    {
        if (!UnsetAsGoalButton->OnClicked.IsAlreadyBound(this, &UMAEditWidget::OnUnsetAsGoalButtonClicked))
        {
            UnsetAsGoalButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnUnsetAsGoalButtonClicked);
        }
    }
    
    ClearSelection();
    RefreshSceneGraphPreview();
    
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

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.08f, 0.95f));
    BackgroundBorder->SetPadding(FMargin(12.0f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    BorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
    BorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    BorderSlot->SetPosition(FVector2D(-20, 60));
    BorderSlot->SetSize(FVector2D(380, 600));
    BorderSlot->SetAutoSize(false);

    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    BackgroundBorder->AddChild(MainVBox);

    // Title
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Edit Mode - 场景编辑")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.6f, 1.0f)));
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 8));

    // Hint text
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(DefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Error text
    ErrorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ErrorText"));
    ErrorText->SetText(FText::GetEmpty());
    FSlateFontInfo ErrorFont = ErrorText->GetFont();
    ErrorFont.Size = 11;
    ErrorText->SetFont(ErrorFont);
    ErrorText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f)));
    ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ErrorSlot = MainVBox->AddChildToVerticalBox(ErrorText);
    ErrorSlot->SetPadding(FMargin(0, 0, 0, 8));

    // Actor operation box
    ActorOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActorOperationBox"));
    ActorOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ActorOpSlot = MainVBox->AddChildToVerticalBox(ActorOperationBox);
    ActorOpSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Node switch container
    NodeSwitchContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NodeSwitchContainer"));
    NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* NodeSwitchSlot = ActorOperationBox->AddChildToVerticalBox(NodeSwitchContainer);
    NodeSwitchSlot->SetPadding(FMargin(0, 0, 0, 8));

    // JSON label
    UTextBlock* JsonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonLabel"));
    JsonLabel->SetText(FText::FromString(TEXT("JSON 编辑:")));
    FSlateFontInfo JsonLabelFont = JsonLabel->GetFont();
    JsonLabelFont.Size = 12;
    JsonLabel->SetFont(JsonLabelFont);
    JsonLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
    
    UVerticalBoxSlot* JsonLabelSlot = ActorOperationBox->AddChildToVerticalBox(JsonLabel);
    JsonLabelSlot->SetPadding(FMargin(0, 0, 0, 4));

    // JSON edit box
    JsonEditBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditBox"));
    JsonEditBox->SetHintText(FText::FromString(TEXT("选中 Actor 后显示 JSON")));
    
    FEditableTextBoxStyle JsonTextBoxStyle;
    JsonTextBoxStyle.SetForegroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));
    JsonEditBox->WidgetStyle = JsonTextBoxStyle;
    
    USizeBox* JsonEditSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditSizeBox"));
    JsonEditSizeBox->SetMinDesiredHeight(150.0f);
    JsonEditSizeBox->SetMaxDesiredHeight(200.0f);
    JsonEditSizeBox->AddChild(JsonEditBox);
    
    UVerticalBoxSlot* JsonEditSlot = ActorOperationBox->AddChildToVerticalBox(JsonEditSizeBox);
    JsonEditSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Actor buttons
    UHorizontalBox* ActorButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActorButtonBox"));
    
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmText"));
    ConfirmText->SetText(FText::FromString(TEXT("  确认  ")));
    ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo ConfirmFont = ConfirmText->GetFont();
    ConfirmFont.Size = 12;
    ConfirmText->SetFont(ConfirmFont);
    ConfirmButton->AddChild(ConfirmText);
    
    UHorizontalBoxSlot* ConfirmSlot = ActorButtonBox->AddChildToHorizontalBox(ConfirmButton);
    ConfirmSlot->SetPadding(FMargin(0, 0, 10, 0));

    DeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteButton"));
    UTextBlock* DeleteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteText"));
    DeleteText->SetText(FText::FromString(TEXT("  删除  ")));
    DeleteText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo DeleteFont = DeleteText->GetFont();
    DeleteFont.Size = 12;
    DeleteText->SetFont(DeleteFont);
    DeleteButton->AddChild(DeleteText);
    
    UHorizontalBoxSlot* DeleteSlot = ActorButtonBox->AddChildToHorizontalBox(DeleteButton);
    DeleteSlot->SetPadding(FMargin(0, 0, 10, 0));

    SetAsGoalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SetAsGoalButton"));
    UTextBlock* SetGoalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SetGoalText"));
    SetGoalText->SetText(FText::FromString(TEXT(" 设为Goal ")));
    SetGoalText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo SetGoalFont = SetGoalText->GetFont();
    SetGoalFont.Size = 12;
    SetGoalText->SetFont(SetGoalFont);
    SetAsGoalButton->AddChild(SetGoalText);
    
    UHorizontalBoxSlot* SetGoalSlot = ActorButtonBox->AddChildToHorizontalBox(SetAsGoalButton);
    SetGoalSlot->SetPadding(FMargin(0, 0, 10, 0));

    UnsetAsGoalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UnsetAsGoalButton"));
    UTextBlock* UnsetGoalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnsetGoalText"));
    UnsetGoalText->SetText(FText::FromString(TEXT(" 取消Goal ")));
    UnsetGoalText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo UnsetGoalFont = UnsetGoalText->GetFont();
    UnsetGoalFont.Size = 12;
    UnsetGoalText->SetFont(UnsetGoalFont);
    UnsetAsGoalButton->AddChild(UnsetGoalText);
    
    ActorButtonBox->AddChildToHorizontalBox(UnsetAsGoalButton);
    
    UVerticalBoxSlot* ActorButtonSlot = ActorOperationBox->AddChildToVerticalBox(ActorButtonBox);
    ActorButtonSlot->SetHorizontalAlignment(HAlign_Left);

    // POI operation box
    POIOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("POIOperationBox"));
    POIOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* POIOpSlot = MainVBox->AddChildToVerticalBox(POIOperationBox);
    POIOpSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Description label
    UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescLabel"));
    DescLabel->SetText(FText::FromString(TEXT("描述信息:")));
    FSlateFontInfo DescLabelFont = DescLabel->GetFont();
    DescLabelFont.Size = 12;
    DescLabel->SetFont(DescLabelFont);
    DescLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
    
    UVerticalBoxSlot* DescLabelSlot = POIOperationBox->AddChildToVerticalBox(DescLabel);
    DescLabelSlot->SetPadding(FMargin(0, 0, 0, 4));

    // Description box
    DescriptionBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("DescriptionBox"));
    DescriptionBox->SetHintText(FText::FromString(TEXT("输入 Goal/Zone 的描述信息...")));
    
    FEditableTextBoxStyle DescTextBoxStyle;
    DescTextBoxStyle.SetForegroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));
    DescriptionBox->WidgetStyle = DescTextBoxStyle;
    
    USizeBox* DescSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DescSizeBox"));
    DescSizeBox->SetMinDesiredHeight(80.0f);
    DescSizeBox->SetMaxDesiredHeight(100.0f);
    DescSizeBox->AddChild(DescriptionBox);
    
    UVerticalBoxSlot* DescSlot = POIOperationBox->AddChildToVerticalBox(DescSizeBox);
    DescSlot->SetPadding(FMargin(0, 0, 0, 10));

    // POI buttons
    UHorizontalBox* POIButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("POIButtonBox"));
    
    CreateGoalButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateGoalButton"));
    UTextBlock* GoalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalText"));
    GoalText->SetText(FText::FromString(TEXT(" 创建 Goal ")));
    GoalText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo GoalFont = GoalText->GetFont();
    GoalFont.Size = 12;
    GoalText->SetFont(GoalFont);
    CreateGoalButton->AddChild(GoalText);
    
    UHorizontalBoxSlot* GoalSlot = POIButtonBox->AddChildToHorizontalBox(CreateGoalButton);
    GoalSlot->SetPadding(FMargin(0, 0, 10, 0));

    CreateZoneButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateZoneButton"));
    UTextBlock* ZoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneText"));
    ZoneText->SetText(FText::FromString(TEXT(" 创建区域 ")));
    ZoneText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo ZoneFont = ZoneText->GetFont();
    ZoneFont.Size = 12;
    ZoneText->SetFont(ZoneFont);
    CreateZoneButton->AddChild(ZoneText);
    
    UHorizontalBoxSlot* ZoneSlot = POIButtonBox->AddChildToHorizontalBox(CreateZoneButton);
    ZoneSlot->SetPadding(FMargin(0, 0, 10, 0));

    DeletePOIButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeletePOIButton"));
    UTextBlock* DeletePOIText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeletePOIText"));
    DeletePOIText->SetText(FText::FromString(TEXT(" 删除 POI ")));
    DeletePOIText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo DeletePOIFont = DeletePOIText->GetFont();
    DeletePOIFont.Size = 12;
    DeletePOIText->SetFont(DeletePOIFont);
    DeletePOIButton->AddChild(DeletePOIText);
    
    POIButtonBox->AddChildToHorizontalBox(DeletePOIButton);
    
    UVerticalBoxSlot* POIButtonSlot = POIOperationBox->AddChildToVerticalBox(POIButtonBox);
    POIButtonSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Preset actor box
    UHorizontalBox* PresetActorBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PresetActorBox"));
    
    PresetActorComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PresetActorComboBox"));
    PresetActorComboBox->AddOption(TEXT("(暂无预设 Actor)"));
    PresetActorComboBox->SetSelectedIndex(0);
    
    UHorizontalBoxSlot* ComboSlot = PresetActorBox->AddChildToHorizontalBox(PresetActorComboBox);
    ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ComboSlot->SetPadding(FMargin(0, 0, 10, 0));

    AddActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddActorButton"));
    UTextBlock* AddActorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddActorText"));
    AddActorText->SetText(FText::FromString(TEXT(" 添加 ")));
    AddActorText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo AddActorFont = AddActorText->GetFont();
    AddActorFont.Size = 12;
    AddActorText->SetFont(AddActorFont);
    AddActorButton->AddChild(AddActorText);
    
    PresetActorBox->AddChildToHorizontalBox(AddActorButton);
    
    UVerticalBoxSlot* PresetSlot = POIOperationBox->AddChildToVerticalBox(PresetActorBox);
    PresetSlot->SetPadding(FMargin(0, 0, 0, 0));

    UE_LOG(LogMAEditWidget, Log, TEXT("BuildUI: UI construction completed"));
}


void UMAEditWidget::SetSelectedActor(AActor* Actor)
{
    if (!Actor)
    {
        ClearSelection();
        return;
    }
    
    CurrentPOIs.Empty();
    CurrentActor = Actor;
    CurrentNodeIndex = 0;
    ActorNodes.Empty();
    
    UWorld* World = GetWorld();
    if (World)
    {
        UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
        
        UMASceneGraphManager* SceneGraphManager = nullptr;
        if (UGameInstance* GI = World->GetGameInstance())
        {
            SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
        }
        
        FString SearchId;
        bool bIsGoalOrZone = false;
        
        if (AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
        {
            SearchId = GoalActor->GetNodeId();
            bIsGoalOrZone = true;
        }
        else if (AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
        {
            SearchId = ZoneActor->GetNodeId();
            bIsGoalOrZone = true;
        }
        else
        {
            SearchId = Actor->GetActorGuid().ToString();
        }
        
        if (bIsGoalOrZone && EditModeManager && !SearchId.IsEmpty())
        {
            FString NodeJsonStr = EditModeManager->GetNodeJsonById(SearchId);
            if (!NodeJsonStr.IsEmpty())
            {
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodeJsonStr);
                TSharedPtr<FJsonObject> NodeJson;
                if (FJsonSerializer::Deserialize(Reader, NodeJson) && NodeJson.IsValid())
                {
                    FMASceneGraphNode Node;
                    Node.Id = SearchId;
                    
                    if (NodeJson->HasField(TEXT("properties")))
                    {
                        TSharedPtr<FJsonObject> Props = NodeJson->GetObjectField(TEXT("properties"));
                        if (Props.IsValid())
                        {
                            Node.Type = Props->GetStringField(TEXT("type"));
                            Node.Label = Props->GetStringField(TEXT("label"));
                        }
                    }
                    
                    if (NodeJson->HasField(TEXT("shape")))
                    {
                        TSharedPtr<FJsonObject> Shape = NodeJson->GetObjectField(TEXT("shape"));
                        if (Shape.IsValid())
                        {
                            Node.ShapeType = Shape->GetStringField(TEXT("type"));
                            
                            if (Shape->HasField(TEXT("center")))
                            {
                                TArray<TSharedPtr<FJsonValue>> CenterArray = Shape->GetArrayField(TEXT("center"));
                                if (CenterArray.Num() >= 3)
                                {
                                    Node.Center = FVector(
                                        CenterArray[0]->AsNumber(),
                                        CenterArray[1]->AsNumber(),
                                        CenterArray[2]->AsNumber()
                                    );
                                }
                            }
                        }
                    }
                    
                    if (NodeJson->HasField(TEXT("guid")))
                    {
                        Node.Guid = NodeJson->GetStringField(TEXT("guid"));
                    }
                    
                    Node.RawJson = NodeJsonStr;
                    ActorNodes.Add(Node);
                }
            }
        }
        else if (SceneGraphManager && !SearchId.IsEmpty())
        {
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            for (const FMASceneGraphNode& Node : AllNodes)
            {
                if (Node.Id == SearchId)
                {
                    ActorNodes.Add(Node);
                    break;
                }
            }
            
            if (ActorNodes.Num() == 0)
            {
                FString ActorGuid = Actor->GetActorGuid().ToString();
                ActorNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
                
                for (const FMASceneGraphNode& Node : AllNodes)
                {
                    if (Node.Guid == ActorGuid)
                    {
                        bool bAlreadyAdded = false;
                        for (const FMASceneGraphNode& ExistingNode : ActorNodes)
                        {
                            if (ExistingNode.Id == Node.Id)
                            {
                                bAlreadyAdded = true;
                                break;
                            }
                        }
                        if (!bAlreadyAdded)
                        {
                            ActorNodes.Add(Node);
                        }
                    }
                }
            }
        }
    }
    
    UpdateUIState();
    UpdateJsonEditBox();
    UpdateNodeSwitchButtons();
    ClearError();
    
    if (HintText)
    {
        HintText->SetText(FText::FromString(ActorSelectedHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: %s, found %d nodes"), *Actor->GetName(), ActorNodes.Num());
}

void UMAEditWidget::SetSelectedPOIs(const TArray<AMAPointOfInterest*>& POIs)
{
    CurrentActor = nullptr;
    ActorNodes.Empty();
    CurrentNodeIndex = 0;
    
    CurrentPOIs.Empty();
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            CurrentPOIs.Add(POI);
        }
    }
    
    if (CurrentPOIs.Num() == 0)
    {
        ClearSelection();
        return;
    }
    
    UpdateUIState();
    ClearError();
    
    if (HintText)
    {
        if (CurrentPOIs.Num() == 1)
        {
            HintText->SetText(FText::FromString(POISingleHintText));
        }
        else if (CurrentPOIs.Num() >= 3)
        {
            HintText->SetText(FText::FromString(FString::Printf(TEXT("已选中 %d 个 POI，可创建区域"), CurrentPOIs.Num())));
        }
        else
        {
            HintText->SetText(FText::FromString(POIMultiHintText));
        }
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.6f, 1.0f)));
    }
    
    if (DescriptionBox)
    {
        DescriptionBox->SetText(FText::GetEmpty());
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedPOIs: %d POIs selected"), CurrentPOIs.Num());
}

void UMAEditWidget::ClearSelection()
{
    CurrentActor = nullptr;
    CurrentPOIs.Empty();
    ActorNodes.Empty();
    CurrentNodeIndex = 0;
    
    if (JsonEditBox)
    {
        JsonEditBox->SetText(FText::GetEmpty());
    }
    
    if (DescriptionBox)
    {
        DescriptionBox->SetText(FText::GetEmpty());
    }
    
    UpdateUIState();
    ClearError();
    
    if (HintText)
    {
        HintText->SetText(FText::FromString(DefaultHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("ClearSelection: Selection cleared"));
}

void UMAEditWidget::RefreshSceneGraphPreview()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("RefreshSceneGraphPreview: Scene graph data synced to temp file"));
}

FString UMAEditWidget::GetDescriptionText() const
{
    if (DescriptionBox)
    {
        return DescriptionBox->GetText().ToString();
    }
    return FString();
}

FString UMAEditWidget::GetJsonEditContent() const
{
    if (JsonEditBox)
    {
        return JsonEditBox->GetText().ToString();
    }
    return FString();
}

void UMAEditWidget::UpdateUIState()
{
    bool bHasActor = (CurrentActor != nullptr);
    bool bHasPOIs = (CurrentPOIs.Num() > 0);
    bool bCanCreateZone = (CurrentPOIs.Num() >= 3);
    
    if (ActorOperationBox)
    {
        ActorOperationBox->SetVisibility(bHasActor ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (POIOperationBox)
    {
        POIOperationBox->SetVisibility(bHasPOIs ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (CreateGoalButton)
    {
        CreateGoalButton->SetVisibility(bHasPOIs ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (CreateZoneButton)
    {
        CreateZoneButton->SetVisibility(bCanCreateZone ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (PresetActorComboBox)
    {
        PresetActorComboBox->SetVisibility((bHasPOIs && CurrentPOIs.Num() == 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (AddActorButton)
    {
        AddActorButton->SetVisibility((bHasPOIs && CurrentPOIs.Num() == 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (DeleteButton && bHasActor)
    {
        bool bCanDelete = false;
        if (ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
        {
            bCanDelete = IsPointTypeNode(ActorNodes[CurrentNodeIndex]);
        }
        DeleteButton->SetVisibility(bCanDelete ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    else if (DeleteButton)
    {
        DeleteButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    bool bIsCurrentNodeGoal = false;
    if (bHasActor && ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
    {
        const FMASceneGraphNode& Node = ActorNodes[CurrentNodeIndex];
        bIsCurrentNodeGoal = Node.RawJson.Contains(TEXT("\"is_goal\"")) && 
                             (Node.RawJson.Contains(TEXT("\"is_goal\": true")) || 
                              Node.RawJson.Contains(TEXT("\"is_goal\":true")));
    }
    
    if (SetAsGoalButton)
    {
        SetAsGoalButton->SetVisibility((bHasActor && !bIsCurrentNodeGoal) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (UnsetAsGoalButton)
    {
        UnsetAsGoalButton->SetVisibility((bHasActor && bIsCurrentNodeGoal) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}


void UMAEditWidget::UpdateJsonEditBox()
{
    if (!JsonEditBox)
    {
        return;
    }
    
    if (!CurrentActor || ActorNodes.Num() == 0)
    {
        JsonEditBox->SetText(FText::FromString(TEXT("未找到对应的场景图节点")));
        JsonEditBox->SetIsReadOnly(true);
        return;
    }
    
    if (CurrentNodeIndex >= ActorNodes.Num())
    {
        CurrentNodeIndex = 0;
    }
    
    const FMASceneGraphNode& Node = ActorNodes[CurrentNodeIndex];
    
    FString JsonContent = Node.RawJson;
    if (JsonContent.IsEmpty())
    {
        JsonContent = FString::Printf(
            TEXT("{\n  \"id\": \"%s\",\n  \"type\": \"%s\",\n  \"label\": \"%s\",\n  \"shape\": {\n    \"type\": \"%s\",\n    \"center\": [%.0f, %.0f, %.0f]\n  }\n}"),
            *Node.Id,
            *Node.Type,
            *Node.Label,
            *Node.ShapeType,
            Node.Center.X, Node.Center.Y, Node.Center.Z
        );
    }
    
    JsonEditBox->SetText(FText::FromString(JsonContent));
    JsonEditBox->SetIsReadOnly(false);
    
    bool bIsPoint = IsPointTypeNode(Node);
    if (!bIsPoint)
    {
        if (HintText)
        {
            HintText->SetText(FText::FromString(TEXT("polygon/linestring 类型: 仅 properties 字段的修改会被保存")));
            HintText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.0f)));
        }
    }
    else
    {
        if (HintText)
        {
            HintText->SetText(FText::FromString(ActorSelectedHintText));
            HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));
        }
    }
}

void UMAEditWidget::UpdateNodeSwitchButtons()
{
    if (!NodeSwitchContainer)
    {
        return;
    }
    
    NodeSwitchContainer->ClearChildren();
    NodeSwitchButtons.Empty();
    
    if (ActorNodes.Num() <= 1)
    {
        NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    
    NodeSwitchContainer->SetVisibility(ESlateVisibility::Visible);
    
    for (int32 i = 0; i < ActorNodes.Num(); ++i)
    {
        UButton* NodeButton = NewObject<UButton>(this);
        UTextBlock* ButtonText = NewObject<UTextBlock>(this);
        
        FString ButtonLabel = FString::Printf(TEXT(" Node %d "), i + 1);
        ButtonText->SetText(FText::FromString(ButtonLabel));
        
        if (i == CurrentNodeIndex)
        {
            ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.5f, 1.0f)));
        }
        else
        {
            ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
        }
        
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

bool UMAEditWidget::ValidateJson(const FString& Json, FString& OutError)
{
    if (Json.IsEmpty())
    {
        OutError = TEXT("JSON 内容不能为空");
        return false;
    }
    
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    TSharedPtr<FJsonObject> JsonObject;
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("JSON 格式无效，请检查语法");
        return false;
    }
    
    if (!JsonObject->HasField(TEXT("id")))
    {
        OutError = TEXT("缺少必需字段: id");
        return false;
    }
    
    OutError.Empty();
    return true;
}

bool UMAEditWidget::IsPointTypeNode(const FMASceneGraphNode& Node) const
{
    return Node.ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase);
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

void UMAEditWidget::HighlightNodeInPreview(const FString& NodeId)
{
    // UI preview removed
}

void UMAEditWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnConfirmButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    FString JsonContent = GetJsonEditContent();
    
    FString ValidationError;
    if (!ValidateJson(JsonContent, ValidationError))
    {
        ShowError(ValidationError);
        return;
    }
    
    ClearError();
    OnEditConfirmed.Broadcast(CurrentActor, JsonContent);
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnConfirmButtonClicked: Edit confirmed for Actor %s"), *CurrentActor->GetName());
}

void UMAEditWidget::OnDeleteButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeleteButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    if (ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
    {
        if (!IsPointTypeNode(ActorNodes[CurrentNodeIndex]))
        {
            ShowError(TEXT("仅支持删除 point 类型节点"));
            return;
        }
    }
    
    ClearError();
    OnDeleteActor.Broadcast(CurrentActor);
    ClearSelection();
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeleteButtonClicked: Delete requested"));
}

void UMAEditWidget::OnCreateGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateGoalButtonClicked"));
    
    if (CurrentPOIs.Num() == 0)
    {
        ShowError(TEXT("未选中 POI"));
        return;
    }
    
    FString Description = GetDescriptionText();
    if (Description.IsEmpty())
    {
        Description = TEXT("New Goal");
    }
    
    ClearError();
    OnCreateGoal.Broadcast(CurrentPOIs[0], Description);
    ClearSelection();
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateGoalButtonClicked: Goal creation requested with description: %s"), *Description);
}

void UMAEditWidget::OnCreateZoneButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateZoneButtonClicked"));
    
    if (CurrentPOIs.Num() < 3)
    {
        ShowError(TEXT("创建区域需要至少 3 个 POI"));
        return;
    }
    
    FString Description = GetDescriptionText();
    if (Description.IsEmpty())
    {
        Description = TEXT("New Zone");
    }
    
    ClearError();
    OnCreateZone.Broadcast(CurrentPOIs, Description);
    ClearSelection();
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateZoneButtonClicked: Zone creation requested with %d POIs, description: %s"), CurrentPOIs.Num(), *Description);
}

void UMAEditWidget::OnAddActorButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnAddActorButtonClicked"));
    
    if (CurrentPOIs.Num() != 1)
    {
        ShowError(TEXT("请选中单个 POI"));
        return;
    }
    
    FString ActorType;
    if (PresetActorComboBox)
    {
        ActorType = PresetActorComboBox->GetSelectedOption();
    }
    
    if (ActorType.IsEmpty() || ActorType == TEXT("(暂无预设 Actor)"))
    {
        ShowError(TEXT("请选择预设 Actor 类型"));
        return;
    }
    
    ClearError();
    OnAddPresetActor.Broadcast(CurrentPOIs[0], ActorType);
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnAddActorButtonClicked: Add preset Actor '%s' requested"), *ActorType);
}

void UMAEditWidget::OnNodeSwitchButtonClicked(int32 NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= ActorNodes.Num())
    {
        return;
    }
    
    CurrentNodeIndex = NodeIndex;
    UpdateJsonEditBox();
    UpdateNodeSwitchButtons();
    UpdateUIState();
    HighlightNodeInPreview(ActorNodes[CurrentNodeIndex].Id);
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnNodeSwitchButtonClicked: Switched to Node %d (ID: %s)"), NodeIndex, *ActorNodes[CurrentNodeIndex].Id);
}

void UMAEditWidget::OnNodeSwitchButtonClickedInternal()
{
    for (int32 i = 0; i < NodeSwitchButtons.Num(); ++i)
    {
        if (NodeSwitchButtons[i] && NodeSwitchButtons[i]->HasKeyboardFocus())
        {
            OnNodeSwitchButtonClicked(i);
            return;
        }
    }
    
    TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (FocusedWidget.IsValid())
    {
        for (int32 i = 0; i < NodeSwitchButtons.Num(); ++i)
        {
            if (NodeSwitchButtons[i])
            {
                TSharedPtr<SWidget> ButtonWidget = NodeSwitchButtons[i]->GetCachedWidget();
                if (ButtonWidget.IsValid() && ButtonWidget == FocusedWidget)
                {
                    OnNodeSwitchButtonClicked(i);
                    return;
                }
            }
        }
    }
    
    FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
    for (int32 i = 0; i < NodeSwitchButtons.Num(); ++i)
    {
        if (NodeSwitchButtons[i])
        {
            FGeometry Geometry = NodeSwitchButtons[i]->GetCachedGeometry();
            FVector2D LocalPos = Geometry.AbsoluteToLocal(MousePos);
            FVector2D Size = Geometry.GetLocalSize();
            
            if (LocalPos.X >= 0 && LocalPos.X <= Size.X && LocalPos.Y >= 0 && LocalPos.Y <= Size.Y)
            {
                OnNodeSwitchButtonClicked(i);
                return;
            }
        }
    }
}

void UMAEditWidget::OnSetAsGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnSetAsGoalButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    ClearError();
    OnSetAsGoal.Broadcast(CurrentActor);
    UpdateUIState();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnSetAsGoalButtonClicked: Set as Goal requested for Actor %s"), *CurrentActor->GetName());
}

void UMAEditWidget::OnUnsetAsGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnUnsetAsGoalButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    ClearError();
    OnUnsetAsGoal.Broadcast(CurrentActor);
    UpdateUIState();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnUnsetAsGoalButtonClicked: Unset as Goal requested for Actor %s"), *CurrentActor->GetName());
}

void UMAEditWidget::OnDeletePOIButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeletePOIButtonClicked"));
    
    if (CurrentPOIs.Num() == 0)
    {
        ShowError(TEXT("未选中 POI"));
        return;
    }
    
    ClearError();
    OnDeletePOIs.Broadcast(CurrentPOIs);
    ClearSelection();
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeletePOIButtonClicked: Delete requested for %d POIs"), CurrentPOIs.Num());
}
