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
#include "../Core/MAEditModeManager.h"
#include "../Core/MASceneGraphManager.h"
#include "../Environment/MAPointOfInterest.h"
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
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMAEditWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAEditWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定按钮事件
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
    
    // 初始化为未选中状态
    ClearSelection();
    
    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("MAEditWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMAEditWidget::RebuildWidget()
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    
    // 构建 UI
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

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAEditWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建背景 Border - 位于屏幕右侧
    // 使用与 MAModifyWidget 相同的固定尺寸布局，确保 UI 点击不会穿透到场景
    UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.08f, 0.95f));  // 深蓝色背景
    BackgroundBorder->SetPadding(FMargin(12.0f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    // 使用固定位置锚点（与 MAModifyWidget 相同），避免垂直拉伸导致的 hit-test 问题
    BorderSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
    BorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    BorderSlot->SetPosition(FVector2D(-20, 60));  // 右边距 20，顶部距离 60
    BorderSlot->SetSize(FVector2D(380, 600));  // 固定尺寸：宽度 380，高度 600
    BorderSlot->SetAutoSize(false);

    // 创建垂直布局容器 (与 MAModifyWidget 相同，不使用外层 ScrollBox)
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    BackgroundBorder->AddChild(MainVBox);

    // =========================================================================
    // 标题区域
    // =========================================================================
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Edit Mode - 场景编辑")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.6f, 1.0f)));  // 蓝色
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 8));

    // 提示文本
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(DefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 错误提示文本
    ErrorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ErrorText"));
    ErrorText->SetText(FText::GetEmpty());
    FSlateFontInfo ErrorFont = ErrorText->GetFont();
    ErrorFont.Size = 11;
    ErrorText->SetFont(ErrorFont);
    ErrorText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f)));  // 红色
    ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ErrorSlot = MainVBox->AddChildToVerticalBox(ErrorText);
    ErrorSlot->SetPadding(FMargin(0, 0, 0, 8));

    // =========================================================================
    // Actor 操作区域 - Requirements: 5.1, 5.2, 5.6
    // =========================================================================
    ActorOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActorOperationBox"));
    ActorOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ActorOpSlot = MainVBox->AddChildToVerticalBox(ActorOperationBox);
    ActorOpSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Node 切换按钮容器 - Requirements: 5.2
    NodeSwitchContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NodeSwitchContainer"));
    NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* NodeSwitchSlot = ActorOperationBox->AddChildToVerticalBox(NodeSwitchContainer);
    NodeSwitchSlot->SetPadding(FMargin(0, 0, 0, 8));

    // JSON 编辑区域标签
    UTextBlock* JsonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonLabel"));
    JsonLabel->SetText(FText::FromString(TEXT("JSON 编辑:")));
    FSlateFontInfo JsonLabelFont = JsonLabel->GetFont();
    JsonLabelFont.Size = 12;
    JsonLabel->SetFont(JsonLabelFont);
    JsonLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
    
    UVerticalBoxSlot* JsonLabelSlot = ActorOperationBox->AddChildToVerticalBox(JsonLabel);
    JsonLabelSlot->SetPadding(FMargin(0, 0, 0, 4));

    // JSON 编辑文本框 - Requirements: 5.1
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

    // Actor 操作按钮区域
    UHorizontalBox* ActorButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActorButtonBox"));
    
    // 确认按钮 - Requirements: 6.4
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

    // 删除按钮 - Requirements: 7.1
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

    // 设为 Goal 按钮 - Requirements: 16.1
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

    // 取消 Goal 按钮 - Requirements: 16.5
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

    // =========================================================================
    // POI 操作区域 - Requirements: 8.1, 9.1, 10.1
    // =========================================================================
    POIOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("POIOperationBox"));
    POIOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* POIOpSlot = MainVBox->AddChildToVerticalBox(POIOperationBox);
    POIOpSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 描述输入区域标签
    UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescLabel"));
    DescLabel->SetText(FText::FromString(TEXT("描述信息:")));
    FSlateFontInfo DescLabelFont = DescLabel->GetFont();
    DescLabelFont.Size = 12;
    DescLabel->SetFont(DescLabelFont);
    DescLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
    
    UVerticalBoxSlot* DescLabelSlot = POIOperationBox->AddChildToVerticalBox(DescLabel);
    DescLabelSlot->SetPadding(FMargin(0, 0, 0, 4));

    // 描述输入文本框 - Requirements: 9.2, 10.2
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

    // POI 操作按钮区域
    UHorizontalBox* POIButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("POIButtonBox"));
    
    // 创建 Goal 按钮 - Requirements: 9.1
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

    // 创建 Zone 按钮 - Requirements: 10.1
    CreateZoneButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateZoneButton"));
    UTextBlock* ZoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneText"));
    ZoneText->SetText(FText::FromString(TEXT(" 创建区域 ")));
    ZoneText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    FSlateFontInfo ZoneFont = ZoneText->GetFont();
    ZoneFont.Size = 12;
    ZoneText->SetFont(ZoneFont);
    CreateZoneButton->AddChild(ZoneText);
    
    POIButtonBox->AddChildToHorizontalBox(CreateZoneButton);
    
    UVerticalBoxSlot* POIButtonSlot = POIOperationBox->AddChildToVerticalBox(POIButtonBox);
    POIButtonSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 预设 Actor 区域 - Requirements: 8.1, 8.5
    UHorizontalBox* PresetActorBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PresetActorBox"));
    
    // 预设 Actor 下拉框
    PresetActorComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PresetActorComboBox"));
    // Requirements: 8.5 - 预留 Actor 选择 UI，即使当前预设 Actor 列表为空
    PresetActorComboBox->AddOption(TEXT("(暂无预设 Actor)"));
    PresetActorComboBox->SetSelectedIndex(0);
    
    UHorizontalBoxSlot* ComboSlot = PresetActorBox->AddChildToHorizontalBox(PresetActorComboBox);
    ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ComboSlot->SetPadding(FMargin(0, 0, 10, 0));

    // 添加 Actor 按钮 - Requirements: 8.2
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

    // 注意: 临时场景图预览已移除，临时场景图数据保存在 datasets/scene_graph_cyberworld_temp.json

    UE_LOG(LogMAEditWidget, Log, TEXT("BuildUI: UI construction completed successfully"));
}

//=========================================================================
// SetSelectedActor - 设置选中的 Actor
// Requirements: 5.1, 5.2, 5.3, 5.4, 5.5
//=========================================================================

void UMAEditWidget::SetSelectedActor(AActor* Actor)
{
    if (!Actor)
    {
        ClearSelection();
        return;
    }
    
    // 清除 POI 选择 (Actor 和 POI 互斥)
    CurrentPOIs.Empty();
    CurrentActor = Actor;
    CurrentNodeIndex = 0;
    ActorNodes.Empty();
    
    // 获取 Actor 对应的所有 Node
    UWorld* World = GetWorld();
    if (World)
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>())
            {
                FString ActorGuid = Actor->GetActorGuid().ToString();
                ActorNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
                
                // 也检查单个 guid 字段匹配 (point 类型)
                TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
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
    
    // 更新 UI
    UpdateUIState();
    UpdateJsonEditBox();
    UpdateNodeSwitchButtons();
    ClearError();
    
    // 更新提示文本
    if (HintText)
    {
        HintText->SetText(FText::FromString(ActorSelectedHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)));
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: %s, found %d nodes"), *Actor->GetName(), ActorNodes.Num());
}

//=========================================================================
// SetSelectedPOIs - 设置选中的 POI 列表
// Requirements: 8.1, 9.1, 10.1
//=========================================================================

void UMAEditWidget::SetSelectedPOIs(const TArray<AMAPointOfInterest*>& POIs)
{
    // 清除 Actor 选择 (Actor 和 POI 互斥)
    CurrentActor = nullptr;
    ActorNodes.Empty();
    CurrentNodeIndex = 0;
    
    // 过滤掉 nullptr
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
    
    // 更新 UI
    UpdateUIState();
    ClearError();
    
    // 更新提示文本
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
    
    // 清空描述输入框
    if (DescriptionBox)
    {
        DescriptionBox->SetText(FText::GetEmpty());
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedPOIs: %d POIs selected"), CurrentPOIs.Num());
}

//=========================================================================
// ClearSelection - 清除选择
//=========================================================================

void UMAEditWidget::ClearSelection()
{
    CurrentActor = nullptr;
    CurrentPOIs.Empty();
    ActorNodes.Empty();
    CurrentNodeIndex = 0;
    
    // 清空编辑框
    if (JsonEditBox)
    {
        JsonEditBox->SetText(FText::GetEmpty());
    }
    
    if (DescriptionBox)
    {
        DescriptionBox->SetText(FText::GetEmpty());
    }
    
    // 更新 UI 状态
    UpdateUIState();
    ClearError();
    
    // 显示默认提示
    if (HintText)
    {
        HintText->SetText(FText::FromString(DefaultHintText));
        HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("ClearSelection: Selection cleared"));
}

//=========================================================================
// RefreshSceneGraphPreview - 刷新临时场景图预览
// 注意: UI 预览已移除，此函数现在仅用于触发数据同步
//=========================================================================

void UMAEditWidget::RefreshSceneGraphPreview()
{
    // 临时场景图预览 UI 已移除
    // 数据会自动同步到 datasets/scene_graph_cyberworld_temp.json
    UE_LOG(LogMAEditWidget, Log, TEXT("RefreshSceneGraphPreview: Scene graph data synced to temp file"));
}

//=========================================================================
// GetDescriptionText - 获取描述文本
//=========================================================================

FString UMAEditWidget::GetDescriptionText() const
{
    if (DescriptionBox)
    {
        return DescriptionBox->GetText().ToString();
    }
    return FString();
}

//=========================================================================
// GetJsonEditContent - 获取 JSON 编辑内容
//=========================================================================

FString UMAEditWidget::GetJsonEditContent() const
{
    if (JsonEditBox)
    {
        return JsonEditBox->GetText().ToString();
    }
    return FString();
}

//=========================================================================
// UpdateUIState - 更新 UI 状态
// 根据当前选择状态显示/隐藏相应的 UI 元素
//=========================================================================

void UMAEditWidget::UpdateUIState()
{
    bool bHasActor = (CurrentActor != nullptr);
    bool bHasPOIs = (CurrentPOIs.Num() > 0);
    bool bCanCreateZone = (CurrentPOIs.Num() >= 3);
    
    // Actor 操作区域
    if (ActorOperationBox)
    {
        ActorOperationBox->SetVisibility(bHasActor ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    // POI 操作区域
    if (POIOperationBox)
    {
        POIOperationBox->SetVisibility(bHasPOIs ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    // 创建 Goal 按钮 (单选 POI 时显示)
    if (CreateGoalButton)
    {
        CreateGoalButton->SetVisibility(bHasPOIs ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    // 创建 Zone 按钮 (多选 POI >= 3 时显示)
    if (CreateZoneButton)
    {
        CreateZoneButton->SetVisibility(bCanCreateZone ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    // 预设 Actor 下拉框和添加按钮 (单选 POI 时显示)
    if (PresetActorComboBox)
    {
        PresetActorComboBox->SetVisibility((bHasPOIs && CurrentPOIs.Num() == 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (AddActorButton)
    {
        AddActorButton->SetVisibility((bHasPOIs && CurrentPOIs.Num() == 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    // 删除按钮 - 仅对 point 类型 Node 显示
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
    
    // 设为 Goal / 取消 Goal 按钮 - Requirements: 16.1, 16.5
    // 检查当前 Node 是否已经是 Goal
    bool bIsCurrentNodeGoal = false;
    if (bHasActor && ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
    {
        // 检查 properties 中是否有 is_goal: true
        const FMASceneGraphNode& Node = ActorNodes[CurrentNodeIndex];
        // 通过 RawJson 检查 is_goal 字段
        bIsCurrentNodeGoal = Node.RawJson.Contains(TEXT("\"is_goal\"")) && 
                             (Node.RawJson.Contains(TEXT("\"is_goal\": true")) || 
                              Node.RawJson.Contains(TEXT("\"is_goal\":true")));
    }
    
    if (SetAsGoalButton)
    {
        // 如果已经是 Goal，隐藏 "设为 Goal" 按钮
        SetAsGoalButton->SetVisibility((bHasActor && !bIsCurrentNodeGoal) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (UnsetAsGoalButton)
    {
        // 如果已经是 Goal，显示 "取消 Goal" 按钮
        UnsetAsGoalButton->SetVisibility((bHasActor && bIsCurrentNodeGoal) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

//=========================================================================
// UpdateJsonEditBox - 更新 JSON 编辑框内容
// Requirements: 5.1, 5.2, 5.3, 5.4, 5.5
//=========================================================================

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
    
    // 确保索引有效
    if (CurrentNodeIndex >= ActorNodes.Num())
    {
        CurrentNodeIndex = 0;
    }
    
    const FMASceneGraphNode& Node = ActorNodes[CurrentNodeIndex];
    
    // 显示 Node 的 JSON
    FString JsonContent = Node.RawJson;
    if (JsonContent.IsEmpty())
    {
        // 如果没有 RawJson，构建基本信息
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
    
    // Requirements: 5.3, 5.4, 5.5 - 所有类型都允许编辑
    // point 类型: 可编辑 properties 和 shape.center
    // polygon/linestring 类型: 仅可编辑 properties (在提交时验证)
    JsonEditBox->SetIsReadOnly(false);
    
    bool bIsPoint = IsPointTypeNode(Node);
    if (!bIsPoint)
    {
        // 非 point 类型，显示提示
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

//=========================================================================
// UpdateNodeSwitchButtons - 更新 Node 切换按钮
// Requirements: 5.2
//=========================================================================

void UMAEditWidget::UpdateNodeSwitchButtons()
{
    if (!NodeSwitchContainer)
    {
        return;
    }
    
    // 清除现有按钮
    NodeSwitchContainer->ClearChildren();
    NodeSwitchButtons.Empty();
    
    // 如果只有一个或没有 Node，隐藏切换容器
    if (ActorNodes.Num() <= 1)
    {
        NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    
    NodeSwitchContainer->SetVisibility(ESlateVisibility::Visible);
    
    // 为每个 Node 创建切换按钮
    for (int32 i = 0; i < ActorNodes.Num(); ++i)
    {
        UButton* NodeButton = NewObject<UButton>(this);
        UTextBlock* ButtonText = NewObject<UTextBlock>(this);
        
        FString ButtonLabel = FString::Printf(TEXT(" Node %d "), i + 1);
        ButtonText->SetText(FText::FromString(ButtonLabel));
        
        // 当前选中的 Node 使用不同颜色
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
        
        // 存储按钮引用以便后续查找索引
        NodeSwitchButtons.Add(NodeButton);
        
        // 绑定到统一的处理函数
        NodeButton->OnClicked.AddDynamic(this, &UMAEditWidget::OnNodeSwitchButtonClickedInternal);
        
        UHorizontalBoxSlot* ButtonSlot = NodeSwitchContainer->AddChildToHorizontalBox(NodeButton);
        ButtonSlot->SetPadding(FMargin(0, 0, 5, 0));
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("UpdateNodeSwitchButtons: Created %d buttons, current index: %d"), ActorNodes.Num(), CurrentNodeIndex);
}

//=========================================================================
// ValidateJson - 验证 JSON 格式
// Requirements: 6.1, 6.2
//=========================================================================

bool UMAEditWidget::ValidateJson(const FString& Json, FString& OutError)
{
    if (Json.IsEmpty())
    {
        OutError = TEXT("JSON 内容不能为空");
        return false;
    }
    
    // 尝试解析 JSON
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    TSharedPtr<FJsonObject> JsonObject;
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("JSON 格式无效，请检查语法");
        return false;
    }
    
    // 检查必需字段
    if (!JsonObject->HasField(TEXT("id")))
    {
        OutError = TEXT("缺少必需字段: id");
        return false;
    }
    
    OutError.Empty();
    return true;
}

//=========================================================================
// IsPointTypeNode - 检查是否为 point 类型 Node
// Requirements: 5.3, 5.5
//=========================================================================

bool UMAEditWidget::IsPointTypeNode(const FMASceneGraphNode& Node) const
{
    return Node.ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase);
}

//=========================================================================
// ShowError - 显示错误信息
//=========================================================================

void UMAEditWidget::ShowError(const FString& ErrorMessage)
{
    if (ErrorText)
    {
        ErrorText->SetText(FText::FromString(ErrorMessage));
        ErrorText->SetVisibility(ESlateVisibility::Visible);
    }
    
    UE_LOG(LogMAEditWidget, Warning, TEXT("ShowError: %s"), *ErrorMessage);
}

//=========================================================================
// ClearError - 清除错误信息
//=========================================================================

void UMAEditWidget::ClearError()
{
    if (ErrorText)
    {
        ErrorText->SetText(FText::GetEmpty());
        ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

//=========================================================================
// HighlightNodeInPreview - 高亮显示选中 Node 的 JSON
// 注意: UI 预览已移除，此函数现在为空操作
//=========================================================================

void UMAEditWidget::HighlightNodeInPreview(const FString& NodeId)
{
    // 临时场景图预览 UI 已移除，此函数不再需要
}

//=========================================================================
// OnConfirmButtonClicked - 确认按钮点击处理
// Requirements: 6.4
//=========================================================================

void UMAEditWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnConfirmButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    // 获取 JSON 内容
    FString JsonContent = GetJsonEditContent();
    
    // 验证 JSON
    FString ValidationError;
    if (!ValidateJson(JsonContent, ValidationError))
    {
        ShowError(ValidationError);
        return;
    }
    
    ClearError();
    
    // 广播确认委托
    OnEditConfirmed.Broadcast(CurrentActor, JsonContent);
    
    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnConfirmButtonClicked: Edit confirmed for Actor %s"), *CurrentActor->GetName());
}

//=========================================================================
// OnDeleteButtonClicked - 删除按钮点击处理
// Requirements: 7.1
//=========================================================================

void UMAEditWidget::OnDeleteButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeleteButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    // 检查是否为 point 类型
    if (ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
    {
        if (!IsPointTypeNode(ActorNodes[CurrentNodeIndex]))
        {
            ShowError(TEXT("仅支持删除 point 类型节点"));
            return;
        }
    }
    
    ClearError();
    
    // 广播删除委托
    OnDeleteActor.Broadcast(CurrentActor);
    
    // 清除选择
    ClearSelection();
    
    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeleteButtonClicked: Delete requested"));
}

//=========================================================================
// OnCreateGoalButtonClicked - 创建 Goal 按钮点击处理
// Requirements: 9.6
//=========================================================================

void UMAEditWidget::OnCreateGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateGoalButtonClicked"));
    
    if (CurrentPOIs.Num() == 0)
    {
        ShowError(TEXT("未选中 POI"));
        return;
    }
    
    // 获取描述
    FString Description = GetDescriptionText();
    if (Description.IsEmpty())
    {
        Description = TEXT("New Goal");
    }
    
    ClearError();
    
    // 广播创建 Goal 委托 (使用第一个 POI)
    OnCreateGoal.Broadcast(CurrentPOIs[0], Description);
    
    // 清除选择
    ClearSelection();
    
    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateGoalButtonClicked: Goal creation requested with description: %s"), *Description);
}

//=========================================================================
// OnCreateZoneButtonClicked - 创建 Zone 按钮点击处理
// Requirements: 10.7
//=========================================================================

void UMAEditWidget::OnCreateZoneButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateZoneButtonClicked"));
    
    if (CurrentPOIs.Num() < 3)
    {
        ShowError(TEXT("创建区域需要至少 3 个 POI"));
        return;
    }
    
    // 获取描述
    FString Description = GetDescriptionText();
    if (Description.IsEmpty())
    {
        Description = TEXT("New Zone");
    }
    
    ClearError();
    
    // 广播创建 Zone 委托
    OnCreateZone.Broadcast(CurrentPOIs, Description);
    
    // 清除选择
    ClearSelection();
    
    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateZoneButtonClicked: Zone creation requested with %d POIs, description: %s"), CurrentPOIs.Num(), *Description);
}

//=========================================================================
// OnAddActorButtonClicked - 添加预设 Actor 按钮点击处理
// Requirements: 8.2
//=========================================================================

void UMAEditWidget::OnAddActorButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnAddActorButtonClicked"));
    
    if (CurrentPOIs.Num() != 1)
    {
        ShowError(TEXT("请选中单个 POI"));
        return;
    }
    
    // 获取选中的预设 Actor 类型
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
    
    // 广播添加预设 Actor 委托
    OnAddPresetActor.Broadcast(CurrentPOIs[0], ActorType);
    
    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnAddActorButtonClicked: Add preset Actor '%s' requested"), *ActorType);
}

//=========================================================================
// OnNodeSwitchButtonClicked - Node 切换按钮点击处理
//=========================================================================

void UMAEditWidget::OnNodeSwitchButtonClicked(int32 NodeIndex)
{
    if (NodeIndex < 0 || NodeIndex >= ActorNodes.Num())
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("OnNodeSwitchButtonClicked: Invalid index %d"), NodeIndex);
        return;
    }
    
    CurrentNodeIndex = NodeIndex;
    
    // 更新 UI
    UpdateJsonEditBox();
    UpdateNodeSwitchButtons();
    UpdateUIState();
    
    // 高亮预览中的 Node
    HighlightNodeInPreview(ActorNodes[CurrentNodeIndex].Id);
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnNodeSwitchButtonClicked: Switched to Node %d (ID: %s)"), NodeIndex, *ActorNodes[CurrentNodeIndex].Id);
}

void UMAEditWidget::OnNodeSwitchButtonClickedInternal()
{
    // 查找是哪个按钮被点击了
    for (int32 i = 0; i < NodeSwitchButtons.Num(); ++i)
    {
        if (NodeSwitchButtons[i] && NodeSwitchButtons[i]->HasKeyboardFocus())
        {
            OnNodeSwitchButtonClicked(i);
            return;
        }
    }
    
    // 如果无法通过焦点确定，尝试通过 Slate 获取
    // 遍历按钮检查哪个是当前激活的
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
    
    // 备选方案：检查鼠标位置下的按钮
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
    
    UE_LOG(LogMAEditWidget, Warning, TEXT("OnNodeSwitchButtonClickedInternal: Could not determine which button was clicked"));
}

//=========================================================================
// OnSetAsGoalButtonClicked - 设为 Goal 按钮点击处理
// Requirements: 16.2
//=========================================================================

void UMAEditWidget::OnSetAsGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnSetAsGoalButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    ClearError();
    
    // 广播设为 Goal 委托
    OnSetAsGoal.Broadcast(CurrentActor);
    
    // 刷新 UI 状态
    UpdateUIState();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnSetAsGoalButtonClicked: Set as Goal requested for Actor %s"), *CurrentActor->GetName());
}

//=========================================================================
// OnUnsetAsGoalButtonClicked - 取消 Goal 按钮点击处理
// Requirements: 16.6
//=========================================================================

void UMAEditWidget::OnUnsetAsGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnUnsetAsGoalButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("未选中 Actor"));
        return;
    }
    
    ClearError();
    
    // 广播取消 Goal 委托
    OnUnsetAsGoal.Broadcast(CurrentActor);
    
    // 刷新 UI 状态
    UpdateUIState();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnUnsetAsGoalButtonClicked: Unset as Goal requested for Actor %s"), *CurrentActor->GetName());
}
