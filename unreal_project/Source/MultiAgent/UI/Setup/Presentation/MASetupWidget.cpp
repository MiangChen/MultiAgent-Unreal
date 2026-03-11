// MASetupWidget.cpp

#include "MASetupWidget.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Spacer.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "../../Core/MARoundedBorderUtils.h"

UMASetupWidget::UMASetupWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMASetupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // InitializeData 已在 RebuildWidget 中调用
    RefreshAgentList();
    UpdateTotalCount();
    
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] NativeConstruct completed"));
}

TSharedRef<SWidget> UMASetupWidget::RebuildWidget()
{
    // 先初始化数据，再构建 UI
    // RebuildWidget 在 NativeConstruct 之前调用
    InitializeData();
    BuildUI();
    return Super::RebuildWidget();
}

void UMASetupWidget::InitializeData()
{
    Coordinator.InitializeState(State);
    
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] InitializeData: %d scenes, %d agent types"), 
        State.AvailableScenes.Num(), State.AvailableAgentTypes.Num());
}

void UMASetupWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupWidget] WidgetTree is null!"));
        return;
    }

    // ========== 样式常量定义 ==========
    const FLinearColor PanelBgColor(0.08f, 0.09f, 0.12f, 0.98f);      // 主面板深色背景
    const FLinearColor HeaderBgColor(0.12f, 0.14f, 0.18f, 1.0f);      // 标题区域背景
    const FLinearColor SectionBgColor(0.1f, 0.11f, 0.14f, 1.0f);      // 区块背景
    const FLinearColor AccentColor(0.2f, 0.6f, 0.9f, 1.0f);           // 强调色 (蓝色)
    const FLinearColor SuccessColor(0.2f, 0.7f, 0.4f, 1.0f);          // 成功色 (绿色)
    const FLinearColor DangerColor(0.8f, 0.3f, 0.3f, 1.0f);           // 危险色 (红色)
    const FLinearColor TextPrimaryColor(0.95f, 0.95f, 0.97f, 1.0f);   // 主文字颜色
    const FLinearColor TextSecondaryColor(0.7f, 0.72f, 0.78f, 1.0f);  // 次要文字颜色
    const FLinearColor ListItemBgColor(0.14f, 0.15f, 0.19f, 1.0f);    // 列表项背景

    // 创建根容器 - CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // 创建主容器 - Border 作为背景
    UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
    MainBorder->SetPadding(FMargin(0.f));  // 内部处理 padding
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(MainBorder, PanelBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
    BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));  // 居中
    BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    BorderSlot->SetSize(FVector2D(650.f, 580.f));  // 稍微加大
    BorderSlot->SetPosition(FVector2D(0.f, 0.f));

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    MainBorder->AddChild(MainVBox);

    // ========== 标题区域 (带背景) ==========
    UBorder* HeaderBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HeaderBorder"));
    HeaderBorder->SetPadding(FMargin(30.f, 20.f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(HeaderBorder, HeaderBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    MainVBox->AddChildToVerticalBox(HeaderBorder);

    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Multi-Agent Simulation - Squad Setup")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 24;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
    TitleText->SetJustification(ETextJustify::Center);
    HeaderBorder->AddChild(TitleText);

    // 间隔
    USpacer* TitleSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TitleSpacer"));
    TitleSpacer->SetSize(FVector2D(0.f, 15.f));
    MainVBox->AddChildToVerticalBox(TitleSpacer);

    // ========== 场景选择区域 (带背景框) ==========
    UBorder* SceneSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SceneSectionBorder"));
    SceneSectionBorder->SetPadding(FMargin(25.f, 15.f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(SceneSectionBorder, SectionBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    MainVBox->AddChildToVerticalBox(SceneSectionBorder);

    UHorizontalBox* SceneRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SceneRow"));
    SceneSectionBorder->AddChild(SceneRow);
    
    UTextBlock* SceneLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SceneLabel"));
    SceneLabel->SetText(FText::FromString(TEXT("Scene: ")));
    FSlateFontInfo SceneLabelFont = SceneLabel->GetFont();
    SceneLabelFont.Size = 16;
    SceneLabel->SetFont(SceneLabelFont);
    SceneLabel->SetColorAndOpacity(FSlateColor(TextSecondaryColor));
    SceneRow->AddChildToHorizontalBox(SceneLabel);

    // 场景标签和下拉框之间的间隔
    USpacer* SceneLabelSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("SceneLabelSpacer"));
    SceneLabelSpacer->SetSize(FVector2D(15.f, 0.f));
    SceneRow->AddChildToHorizontalBox(SceneLabelSpacer);

    SceneComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("SceneComboBox"));
    SceneComboBox->ClearOptions();
    
    for (const FString& Scene : State.AvailableScenes)
    {
        SceneComboBox->AddOption(Scene);
        UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Added scene option: %s"), *Scene);
    }
    
    SceneComboBox->SetContentPadding(FMargin(12.f, 6.f));
    
    // 设置下拉菜单样式 - 通过 ItemStyle 设置文字颜色
    FTableRowStyle ItemStyle = SceneComboBox->GetItemStyle();
    ItemStyle.SetTextColor(FSlateColor(FLinearColor::Black));
    ItemStyle.SetSelectedTextColor(FSlateColor(FLinearColor::White));
    SceneComboBox->SetItemStyle(ItemStyle);
    
    if (State.AvailableScenes.Num() > 0)
    {
        if (!State.AvailableScenes.Contains(State.SelectedScene))
        {
            State.SelectedScene = State.AvailableScenes[0];
        }
        SceneComboBox->SetSelectedOption(State.SelectedScene);
        UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Scene ComboBox selected: %s"), *State.SelectedScene);
    }
    
    SceneComboBox->OnSelectionChanged.AddDynamic(this, &UMASetupWidget::OnSceneSelectionChanged);
    SceneRow->AddChildToHorizontalBox(SceneComboBox);

    // 间隔
    USpacer* SceneSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("SceneSpacer"));
    SceneSpacer->SetSize(FVector2D(0.f, 10.f));
    MainVBox->AddChildToVerticalBox(SceneSpacer);

    // ========== 添加智能体区域 (带背景框) ==========
    UBorder* AddSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AddSectionBorder"));
    AddSectionBorder->SetPadding(FMargin(25.f, 15.f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(AddSectionBorder, SectionBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    MainVBox->AddChildToVerticalBox(AddSectionBorder);

    UVerticalBox* AddSectionVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AddSectionVBox"));
    AddSectionBorder->AddChild(AddSectionVBox);

    // 区块标题
    UTextBlock* AddSectionTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddSectionTitle"));
    AddSectionTitle->SetText(FText::FromString(TEXT("Add Agent")));
    FSlateFontInfo SectionFont = AddSectionTitle->GetFont();
    SectionFont.Size = 16;
    AddSectionTitle->SetFont(SectionFont);
    AddSectionTitle->SetColorAndOpacity(FSlateColor(AccentColor));
    AddSectionVBox->AddChildToVerticalBox(AddSectionTitle);

    // 标题下间隔
    USpacer* AddTitleSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("AddTitleSpacer"));
    AddTitleSpacer->SetSize(FVector2D(0.f, 10.f));
    AddSectionVBox->AddChildToVerticalBox(AddTitleSpacer);

    UHorizontalBox* AddRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AddRow"));
    AddSectionVBox->AddChildToVerticalBox(AddRow);

    // 智能体类型下拉框
    AgentTypeComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AgentTypeComboBox"));
    AgentTypeComboBox->ClearOptions();
    
    for (const auto& Pair : State.AvailableAgentTypes)
    {
        AgentTypeComboBox->AddOption(Pair.Key);
    }
    
    AgentTypeComboBox->SetContentPadding(FMargin(12.f, 6.f));
    
    // 设置下拉菜单样式 - 通过 ItemStyle 设置文字颜色
    FTableRowStyle AgentItemStyle = AgentTypeComboBox->GetItemStyle();
    AgentItemStyle.SetTextColor(FSlateColor(FLinearColor::Black));
    AgentItemStyle.SetSelectedTextColor(FSlateColor(FLinearColor::White));
    AgentTypeComboBox->SetItemStyle(AgentItemStyle);
    
    if (State.AvailableAgentTypes.Num() > 0)
    {
        AgentTypeComboBox->SetSelectedIndex(0);
    }
    AddRow->AddChildToHorizontalBox(AgentTypeComboBox);

    // 间隔
    USpacer* TypeCountSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TypeCountSpacer"));
    TypeCountSpacer->SetSize(FVector2D(20.f, 0.f));
    AddRow->AddChildToHorizontalBox(TypeCountSpacer);

    // 数量标签
    UTextBlock* CountLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountLabel"));
    CountLabel->SetText(FText::FromString(TEXT("Count:")));
    FSlateFontInfo CountLabelFont = CountLabel->GetFont();
    CountLabelFont.Size = 14;
    CountLabel->SetFont(CountLabelFont);
    CountLabel->SetColorAndOpacity(FSlateColor(TextSecondaryColor));
    AddRow->AddChildToHorizontalBox(CountLabel);

    // 间隔
    USpacer* CountLabelSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("CountLabelSpacer"));
    CountLabelSpacer->SetSize(FVector2D(8.f, 0.f));
    AddRow->AddChildToHorizontalBox(CountLabelSpacer);

    // 数量输入框
    CountInputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("CountInputBox"));
    CountInputBox->SetText(FText::FromString(TEXT("1")));
    CountInputBox->SetMinDesiredWidth(50.0f);
    AddRow->AddChildToHorizontalBox(CountInputBox);

    // 间隔
    USpacer* AddSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("AddSpacer"));
    AddSpacer->SetSize(FVector2D(20.f, 0.f));
    AddRow->AddChildToHorizontalBox(AddSpacer);

    // 添加按钮 (蓝色强调)
    AddButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddButton"));
    AddButton->SetBackgroundColor(AccentColor);
    UTextBlock* AddButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddButtonText"));
    AddButtonText->SetText(FText::FromString(TEXT("  + Add  ")));
    FSlateFontInfo AddBtnFont = AddButtonText->GetFont();
    AddBtnFont.Size = 14;
    AddButtonText->SetFont(AddBtnFont);
    AddButtonText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
    AddButton->AddChild(AddButtonText);
    AddButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnAddButtonClicked);
    AddRow->AddChildToHorizontalBox(AddButton);

    // 间隔
    USpacer* ListSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ListSpacer"));
    ListSpacer->SetSize(FVector2D(0.f, 10.f));
    MainVBox->AddChildToVerticalBox(ListSpacer);

    // ========== 智能体列表区域 (带背景框) ==========
    UBorder* ListSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ListSectionBorder"));
    ListSectionBorder->SetPadding(FMargin(25.f, 15.f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(ListSectionBorder, SectionBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    MainVBox->AddChildToVerticalBox(ListSectionBorder);

    UVerticalBox* ListSectionVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ListSectionVBox"));
    ListSectionBorder->AddChild(ListSectionVBox);

    // 区块标题
    UTextBlock* ListTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ListTitle"));
    ListTitle->SetText(FText::FromString(TEXT("Current Squad")));
    ListTitle->SetFont(SectionFont);
    ListTitle->SetColorAndOpacity(FSlateColor(AccentColor));
    ListSectionVBox->AddChildToVerticalBox(ListTitle);

    // 标题下间隔
    USpacer* ListTitleSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ListTitleSpacer"));
    ListTitleSpacer->SetSize(FVector2D(0.f, 10.f));
    ListSectionVBox->AddChildToVerticalBox(ListTitleSpacer);

    // 列表容器 (带背景)
    UBorder* ListBgBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ListBgBorder"));
    ListBgBorder->SetBrushColor(ListItemBgColor);
    ListBgBorder->SetPadding(FMargin(10.f, 8.f));
    ListSectionVBox->AddChildToVerticalBox(ListBgBorder);

    AgentListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AgentListScrollBox"));
    ListBgBorder->AddChild(AgentListScrollBox);

    // 间隔
    USpacer* TotalSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TotalSpacer"));
    TotalSpacer->SetSize(FVector2D(0.f, 12.f));
    ListSectionVBox->AddChildToVerticalBox(TotalSpacer);

    // ========== 总数显示 ==========
    TotalCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TotalCountText"));
    TotalCountText->SetText(FText::FromString(TEXT("Total: 0 agents")));
    FSlateFontInfo TotalFont = TotalCountText->GetFont();
    TotalFont.Size = 16;
    TotalCountText->SetFont(TotalFont);
    TotalCountText->SetColorAndOpacity(FSlateColor(SuccessColor));
    ListSectionVBox->AddChildToVerticalBox(TotalCountText);

    // 间隔
    USpacer* ButtonSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ButtonSpacer"));
    ButtonSpacer->SetSize(FVector2D(0.f, 15.f));
    MainVBox->AddChildToVerticalBox(ButtonSpacer);

    // ========== 底部按钮区域 (带背景) ==========
    UBorder* ButtonSectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ButtonSectionBorder"));
    ButtonSectionBorder->SetPadding(FMargin(25.f, 18.f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(ButtonSectionBorder, HeaderBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    
    MainVBox->AddChildToVerticalBox(ButtonSectionBorder);

    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
    ButtonSectionBorder->AddChild(ButtonRow);

    // 左侧弹性空间
    USpacer* LeftFlexSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("LeftFlexSpacer"));
    LeftFlexSpacer->SetSize(FVector2D(1.f, 0.f));
    ButtonRow->AddChildToHorizontalBox(LeftFlexSpacer);

    // 清空按钮 (红色危险)
    ClearButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ClearButton"));
    ClearButton->SetBackgroundColor(DangerColor);
    UTextBlock* ClearButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClearButtonText"));
    ClearButtonText->SetText(FText::FromString(TEXT("  Clear All  ")));
    FSlateFontInfo ClearBtnFont = ClearButtonText->GetFont();
    ClearBtnFont.Size = 14;
    ClearButtonText->SetFont(ClearBtnFont);
    ClearButtonText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
    ClearButton->AddChild(ClearButtonText);
    ClearButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnClearButtonClicked);
    ButtonRow->AddChildToHorizontalBox(ClearButton);

    // 间隔
    USpacer* BtnSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("BtnSpacer"));
    BtnSpacer->SetSize(FVector2D(30.0f, 0.0f));
    ButtonRow->AddChildToHorizontalBox(BtnSpacer);

    // 开始按钮 (绿色成功)
    StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
    StartButton->SetBackgroundColor(SuccessColor);
    StartButton->SetClickMethod(EButtonClickMethod::DownAndUp);
    StartButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
    StartButton->SetPressMethod(EButtonPressMethod::DownAndUp);
    UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
    StartButtonText->SetText(FText::FromString(TEXT("  ▶ Start Simulation  ")));
    FSlateFontInfo StartFont = StartButtonText->GetFont();
    StartFont.Size = 16;
    StartButtonText->SetFont(StartFont);
    StartButtonText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
    StartButton->AddChild(StartButtonText);
    StartButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnStartButtonClicked);
    ButtonRow->AddChildToHorizontalBox(StartButton);

    // 右侧弹性空间
    USpacer* RightFlexSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("RightFlexSpacer"));
    RightFlexSpacer->SetSize(FVector2D(1.f, 0.f));
    ButtonRow->AddChildToHorizontalBox(RightFlexSpacer);
    
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] StartButton created and OnClicked bound"));

    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] UI built successfully with WidgetTree"));
}

void UMASetupWidget::RefreshAgentList()
{
    if (!AgentListScrollBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] AgentListScrollBox is null in RefreshAgentList"));
        return;
    }

    // 样式常量
    const FLinearColor TextPrimaryColor(0.95f, 0.95f, 0.97f, 1.0f);
    const FLinearColor TextSecondaryColor(0.7f, 0.72f, 0.78f, 1.0f);
    const FLinearColor CountBadgeColor(0.2f, 0.6f, 0.9f, 1.0f);
    const FLinearColor RemoveBtnColor(0.6f, 0.25f, 0.25f, 1.0f);

    // 清空现有列表和按钮映射
    AgentListScrollBox->ClearChildren();
    RemoveButtonIndexMap.Empty();
    DecreaseButtonIndexMap.Empty();
    IncreaseButtonIndexMap.Empty();

    if (State.AgentConfigs.Num() == 0)
    {
        // 空列表提示
        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("No agents added yet. Add agents above.")));
        FSlateFontInfo EmptyFont = EmptyText->GetFont();
        EmptyFont.Size = 12;
        EmptyText->SetFont(EmptyFont);
        EmptyText->SetColorAndOpacity(FSlateColor(TextSecondaryColor));
        AgentListScrollBox->AddChild(EmptyText);
        return;
    }

    // 为每个配置创建一行
    for (int32 i = 0; i < State.AgentConfigs.Num(); ++i)
    {
        const FMAAgentSetupConfig& Config = State.AgentConfigs[i];

        UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

        // Agent 类型图标/标记 (用文字代替)
        UTextBlock* IconText = NewObject<UTextBlock>(this);
        IconText->SetText(FText::FromString(TEXT("●")));
        IconText->SetColorAndOpacity(FSlateColor(CountBadgeColor));
        Row->AddChildToHorizontalBox(IconText);

        // 间隔
        USpacer* IconSpacer = NewObject<USpacer>(this);
        IconSpacer->SetSize(FVector2D(10.0f, 0.0f));
        Row->AddChildToHorizontalBox(IconSpacer);

        // 类型名称
        UTextBlock* NameText = NewObject<UTextBlock>(this);
        NameText->SetText(FText::FromString(Config.DisplayName));
        FSlateFontInfo NameFont = NameText->GetFont();
        NameFont.Size = 14;
        NameText->SetFont(NameFont);
        NameText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
        Row->AddChildToHorizontalBox(NameText);

        // 弹性间隔
        USpacer* FlexSpacer = NewObject<USpacer>(this);
        FlexSpacer->SetSize(FVector2D(50.0f, 0.0f));
        Row->AddChildToHorizontalBox(FlexSpacer);

        // 数量显示
        UTextBlock* CountText = NewObject<UTextBlock>(this);
        CountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Config.Count)));
        FSlateFontInfo CountFont = CountText->GetFont();
        CountFont.Size = 14;
        CountText->SetFont(CountFont);
        CountText->SetColorAndOpacity(FSlateColor(CountBadgeColor));
        Row->AddChildToHorizontalBox(CountText);

        // 间隔
        USpacer* BtnSpacer = NewObject<USpacer>(this);
        BtnSpacer->SetSize(FVector2D(10.0f, 0.0f));
        Row->AddChildToHorizontalBox(BtnSpacer);

        // 加号按钮 (绿色)
        const FLinearColor IncreaseBtnColor(0.2f, 0.6f, 0.3f, 1.0f);
        UButton* IncreaseButton = NewObject<UButton>(this);
        IncreaseButton->SetBackgroundColor(IncreaseBtnColor);
        UTextBlock* IncreaseText = NewObject<UTextBlock>(this);
        IncreaseText->SetText(FText::FromString(TEXT(" + ")));
        IncreaseText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
        IncreaseButton->AddChild(IncreaseText);
        
        // 存储加号按钮到索引的映射
        IncreaseButtonIndexMap.Add(IncreaseButton, i);
        IncreaseButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnIncreaseButtonClicked);
        
        Row->AddChildToHorizontalBox(IncreaseButton);

        // 按钮间隔
        USpacer* IncDecSpacer = NewObject<USpacer>(this);
        IncDecSpacer->SetSize(FVector2D(3.0f, 0.0f));
        Row->AddChildToHorizontalBox(IncDecSpacer);

        // 减号按钮 (橙色/黄色)
        const FLinearColor DecreaseBtnColor(0.7f, 0.5f, 0.2f, 1.0f);
        UButton* DecreaseButton = NewObject<UButton>(this);
        DecreaseButton->SetBackgroundColor(DecreaseBtnColor);
        UTextBlock* DecreaseText = NewObject<UTextBlock>(this);
        DecreaseText->SetText(FText::FromString(TEXT(" − ")));
        DecreaseText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
        DecreaseButton->AddChild(DecreaseText);
        
        // 存储减号按钮到索引的映射
        DecreaseButtonIndexMap.Add(DecreaseButton, i);
        DecreaseButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnDecreaseButtonClicked);
        
        Row->AddChildToHorizontalBox(DecreaseButton);

        // 按钮间隔
        USpacer* BtnGapSpacer = NewObject<USpacer>(this);
        BtnGapSpacer->SetSize(FVector2D(5.0f, 0.0f));
        Row->AddChildToHorizontalBox(BtnGapSpacer);

        // 删除按钮 (红色)
        UButton* RemoveButton = NewObject<UButton>(this);
        RemoveButton->SetBackgroundColor(RemoveBtnColor);
        UTextBlock* RemoveText = NewObject<UTextBlock>(this);
        RemoveText->SetText(FText::FromString(TEXT(" ✕ ")));
        RemoveText->SetColorAndOpacity(FSlateColor(TextPrimaryColor));
        RemoveButton->AddChild(RemoveText);
        
        // 存储删除按钮到索引的映射
        RemoveButtonIndexMap.Add(RemoveButton, i);
        RemoveButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnRemoveButtonClicked);
        
        Row->AddChildToHorizontalBox(RemoveButton);

        // 行间隔
        USpacer* RowSpacer = NewObject<USpacer>(this);
        RowSpacer->SetSize(FVector2D(0.0f, 6.0f));

        AgentListScrollBox->AddChild(Row);
        AgentListScrollBox->AddChild(RowSpacer);
    }

    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Agent list refreshed, %d configs"), State.AgentConfigs.Num());
}

void UMASetupWidget::UpdateTotalCount()
{
    const int32 Total = Coordinator.GetTotalAgentCount(State);
    if (TotalCountText)
    {
        TotalCountText->SetText(FText::FromString(FString::Printf(TEXT("Total: %d agent(s)"), Total)));
    }
}

int32 UMASetupWidget::GetTotalAgentCount() const
{
    return Coordinator.GetTotalAgentCount(State);
}

FMASetupLaunchRequest UMASetupWidget::BuildLaunchRequest() const
{
    return Coordinator.BuildLaunchRequest(State);
}

void UMASetupWidget::OnSceneSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    Coordinator.SetSelectedScene(State, SelectedItem);
    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Scene changed to: %s"), *State.SelectedScene);
}

void UMASetupWidget::OnAddButtonClicked()
{
    if (!AgentTypeComboBox || !CountInputBox)
    {
        return;
    }

    FString SelectedType = AgentTypeComboBox->GetSelectedOption();
    FString CountStr = CountInputBox->GetText().ToString();
    int32 Count = FCString::Atoi(*CountStr);

    if (Count <= 0)
    {
        Count = 1;
    }

    // 获取显示名称
    Coordinator.AddAgent(State, SelectedType, Count);

    RefreshAgentList();
    UpdateTotalCount();

    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Added %d x %s"), Count, *SelectedType);
}

void UMASetupWidget::OnClearButtonClicked()
{
    Coordinator.ClearAgents(State);
    RefreshAgentList();
    UpdateTotalCount();

    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Agent list cleared"));
}

void UMASetupWidget::OnStartButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] >>>>>> OnStartButtonClicked CALLED! <<<<<<"));
    
    if (State.AgentConfigs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Cannot start: no agents configured"));
        // 即使没有配置也允许开始（用于测试）
    }

    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Starting simulation with %d agent types, scene: %s"), 
        State.AgentConfigs.Num(), *State.SelectedScene);

    // 广播开始事件
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Broadcasting OnStartSimulation..."));
    OnStartSimulation.Broadcast();
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] OnStartSimulation broadcasted"));
}

void UMASetupWidget::OnRemoveAgentClicked(int32 Index)
{
    if (State.AgentConfigs.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Removing agent at index %d"), Index);
        Coordinator.RemoveAgentAt(State, Index);
        RefreshAgentList();
        UpdateTotalCount();
    }
}

void UMASetupWidget::OnRemoveButtonClicked()
{
    // 查找是哪个按钮被点击了 - 检查 IsHovered 作为备选
    for (const auto& Pair : RemoveButtonIndexMap)
    {
        UButton* Button = Pair.Key;
        if (Button && (Button->IsPressed() || Button->IsHovered()))
        {
            int32 Index = Pair.Value;
            UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Remove button clicked for index %d"), Index);
            OnRemoveAgentClicked(Index);
            return;
        }
    }
    
    // 备用方案：如果上面的方法都不行，删除最后一个
    // 这种情况不应该发生，但作为保险
    if (State.AgentConfigs.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Fallback: removing last agent"));
        OnRemoveAgentClicked(State.AgentConfigs.Num() - 1);
    }
}

void UMASetupWidget::OnDecreaseButtonClicked()
{
    // 查找是哪个减号按钮被点击了
    for (const auto& Pair : DecreaseButtonIndexMap)
    {
        UButton* Button = Pair.Key;
        if (Button && (Button->IsPressed() || Button->IsHovered()))
        {
            int32 Index = Pair.Value;
            UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Decrease button clicked for index %d"), Index);
            OnDecreaseAgentCount(Index);
            return;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] OnDecreaseButtonClicked called but couldn't identify which button"));
}

void UMASetupWidget::OnDecreaseAgentCount(int32 Index)
{
    if (State.AgentConfigs.IsValidIndex(Index))
    {
        const FString DisplayName = State.AgentConfigs[Index].DisplayName;
        Coordinator.DecreaseAgentCount(State, Index);
        UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Decreased %s count"), *DisplayName);
        RefreshAgentList();
        UpdateTotalCount();
    }
}

void UMASetupWidget::OnIncreaseButtonClicked()
{
    // 查找是哪个加号按钮被点击了
    for (const auto& Pair : IncreaseButtonIndexMap)
    {
        UButton* Button = Pair.Key;
        if (Button && (Button->IsPressed() || Button->IsHovered()))
        {
            int32 Index = Pair.Value;
            UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Increase button clicked for index %d"), Index);
            OnIncreaseAgentCount(Index);
            return;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] OnIncreaseButtonClicked called but couldn't identify which button"));
}

void UMASetupWidget::OnIncreaseAgentCount(int32 Index)
{
    if (State.AgentConfigs.IsValidIndex(Index))
    {
        const FString DisplayName = State.AgentConfigs[Index].DisplayName;
        Coordinator.IncreaseAgentCount(State, Index);
        UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Increased %s count"), *DisplayName);
        RefreshAgentList();
        UpdateTotalCount();
    }
}
