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

UMASetupWidget::UMASetupWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , SelectedScene(TEXT("CyberCity"))
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
    // 防止重复初始化
    if (AvailableScenes.Num() > 0)
    {
        return;
    }
    
    // 可用的智能体类型: 内部名称 -> 显示名称
    AvailableAgentTypes.Add(TEXT("DronePhantom4"), TEXT("Drone Phantom 4"));
    AvailableAgentTypes.Add(TEXT("DroneInspire2"), TEXT("Drone Inspire 2"));
    AvailableAgentTypes.Add(TEXT("RobotDog"), TEXT("Robot Dog"));
    AvailableAgentTypes.Add(TEXT("Human"), TEXT("Human"));

    // 可用的场景 (使用英文避免字体问题)
    AvailableScenes.Add(TEXT("CyberCity"));
    AvailableScenes.Add(TEXT("OldTown"));
    AvailableScenes.Add(TEXT("Factory"));
    AvailableScenes.Add(TEXT("Forest"));
    AvailableScenes.Add(TEXT("DesertBase"));

    // 默认添加一些智能体
    AgentConfigs.Add(FMAAgentSetupConfig(TEXT("DronePhantom4"), TEXT("Drone Phantom 4"), 3));
    
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] InitializeData: %d scenes, %d agent types"), 
        AvailableScenes.Num(), AvailableAgentTypes.Num());
}

void UMASetupWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("[MASetupWidget] WidgetTree is null!"));
        return;
    }

    // 创建根容器 - CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // 创建主容器 - Border 作为背景
    UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
    MainBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.15f, 0.95f));  // 深色半透明背景
    MainBorder->SetPadding(FMargin(40.f));
    
    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
    BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));  // 居中
    BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    BorderSlot->SetSize(FVector2D(600.f, 500.f));
    BorderSlot->SetPosition(FVector2D(0.f, 0.f));

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    MainBorder->AddChild(MainVBox);

    // ========== 标题 ==========
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("多智能体仿真 - 小队配置")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 28;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    TitleText->SetJustification(ETextJustify::Center);
    MainVBox->AddChildToVerticalBox(TitleText);

    // 间隔
    USpacer* TitleSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TitleSpacer"));
    TitleSpacer->SetSize(FVector2D(0.f, 20.f));
    MainVBox->AddChildToVerticalBox(TitleSpacer);

    // ========== 场景选择区域 ==========
    UHorizontalBox* SceneRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SceneRow"));
    MainVBox->AddChildToVerticalBox(SceneRow);
    
    UTextBlock* SceneLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SceneLabel"));
    SceneLabel->SetText(FText::FromString(TEXT("选择场景: ")));
    SceneLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    SceneRow->AddChildToHorizontalBox(SceneLabel);

    SceneComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("SceneComboBox"));
    for (const FString& Scene : AvailableScenes)
    {
        SceneComboBox->AddOption(Scene);
    }
    SceneComboBox->SetSelectedOption(SelectedScene);
    SceneComboBox->OnSelectionChanged.AddDynamic(this, &UMASetupWidget::OnSceneSelectionChanged);
    SceneRow->AddChildToHorizontalBox(SceneComboBox);

    // 间隔
    USpacer* SceneSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("SceneSpacer"));
    SceneSpacer->SetSize(FVector2D(0.f, 15.f));
    MainVBox->AddChildToVerticalBox(SceneSpacer);

    // ========== 添加智能体区域 ==========
    UTextBlock* AddSectionTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddSectionTitle"));
    AddSectionTitle->SetText(FText::FromString(TEXT("添加智能体")));
    FSlateFontInfo SectionFont = AddSectionTitle->GetFont();
    SectionFont.Size = 18;
    AddSectionTitle->SetFont(SectionFont);
    AddSectionTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
    MainVBox->AddChildToVerticalBox(AddSectionTitle);

    UHorizontalBox* AddRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AddRow"));
    MainVBox->AddChildToVerticalBox(AddRow);

    // 智能体类型下拉框
    AgentTypeComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AgentTypeComboBox"));
    for (const auto& Pair : AvailableAgentTypes)
    {
        AgentTypeComboBox->AddOption(Pair.Key);
    }
    AgentTypeComboBox->SetSelectedIndex(0);
    AddRow->AddChildToHorizontalBox(AgentTypeComboBox);

    // 数量标签
    UTextBlock* CountLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountLabel"));
    CountLabel->SetText(FText::FromString(TEXT("  数量: ")));
    CountLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    AddRow->AddChildToHorizontalBox(CountLabel);

    // 数量输入框
    CountInputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("CountInputBox"));
    CountInputBox->SetText(FText::FromString(TEXT("1")));
    CountInputBox->SetMinDesiredWidth(60.0f);
    AddRow->AddChildToHorizontalBox(CountInputBox);

    // 间隔
    USpacer* AddSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("AddSpacer"));
    AddSpacer->SetSize(FVector2D(10.f, 0.f));
    AddRow->AddChildToHorizontalBox(AddSpacer);

    // 添加按钮
    AddButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddButton"));
    UTextBlock* AddButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddButtonText"));
    AddButtonText->SetText(FText::FromString(TEXT("添加")));
    AddButton->AddChild(AddButtonText);
    AddButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnAddButtonClicked);
    AddRow->AddChildToHorizontalBox(AddButton);

    // 间隔
    USpacer* ListSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ListSpacer"));
    ListSpacer->SetSize(FVector2D(0.f, 15.f));
    MainVBox->AddChildToVerticalBox(ListSpacer);

    // ========== 智能体列表区域 ==========
    UTextBlock* ListTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ListTitle"));
    ListTitle->SetText(FText::FromString(TEXT("当前小队成员")));
    ListTitle->SetFont(SectionFont);
    ListTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
    MainVBox->AddChildToVerticalBox(ListTitle);

    AgentListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AgentListScrollBox"));
    MainVBox->AddChildToVerticalBox(AgentListScrollBox);

    // 间隔
    USpacer* TotalSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TotalSpacer"));
    TotalSpacer->SetSize(FVector2D(0.f, 10.f));
    MainVBox->AddChildToVerticalBox(TotalSpacer);

    // ========== 总数显示 ==========
    TotalCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TotalCountText"));
    TotalCountText->SetText(FText::FromString(TEXT("总计: 0 个智能体")));
    TotalCountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.9f, 0.6f)));
    MainVBox->AddChildToVerticalBox(TotalCountText);

    // 间隔
    USpacer* ButtonSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ButtonSpacer"));
    ButtonSpacer->SetSize(FVector2D(0.f, 20.f));
    MainVBox->AddChildToVerticalBox(ButtonSpacer);

    // ========== 底部按钮区域 ==========
    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
    MainVBox->AddChildToVerticalBox(ButtonRow);

    // 清空按钮
    ClearButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ClearButton"));
    UTextBlock* ClearButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClearButtonText"));
    ClearButtonText->SetText(FText::FromString(TEXT("清空列表")));
    ClearButton->AddChild(ClearButtonText);
    ClearButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnClearButtonClicked);
    ButtonRow->AddChildToHorizontalBox(ClearButton);

    // 间隔
    USpacer* BtnSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("BtnSpacer"));
    BtnSpacer->SetSize(FVector2D(20.0f, 0.0f));
    ButtonRow->AddChildToHorizontalBox(BtnSpacer);

    // 开始按钮
    StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
    StartButton->SetBackgroundColor(FLinearColor(0.2f, 0.6f, 0.2f));
    StartButton->SetClickMethod(EButtonClickMethod::DownAndUp);
    StartButton->SetTouchMethod(EButtonTouchMethod::DownAndUp);
    StartButton->SetPressMethod(EButtonPressMethod::DownAndUp);
    StartButton->IsFocusable = true;
    UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
    StartButtonText->SetText(FText::FromString(TEXT("Start Simulation")));  // 用英文测试
    FSlateFontInfo StartFont = StartButtonText->GetFont();
    StartFont.Size = 18;
    StartButtonText->SetFont(StartFont);
    StartButton->AddChild(StartButtonText);
    StartButton->OnClicked.AddDynamic(this, &UMASetupWidget::OnStartButtonClicked);
    ButtonRow->AddChildToHorizontalBox(StartButton);
    
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

    // 清空现有列表
    AgentListScrollBox->ClearChildren();

    // 为每个配置创建一行
    for (int32 i = 0; i < AgentConfigs.Num(); ++i)
    {
        const FMAAgentSetupConfig& Config = AgentConfigs[i];

        UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

        // 类型和数量显示
        UTextBlock* InfoText = NewObject<UTextBlock>(this);
        FString InfoStr = FString::Printf(TEXT("  %s x%d"), *Config.DisplayName, Config.Count);
        InfoText->SetText(FText::FromString(InfoStr));
        InfoText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Row->AddChildToHorizontalBox(InfoText);

        // 间隔
        USpacer* Spacer = NewObject<USpacer>(this);
        Spacer->SetSize(FVector2D(20.0f, 0.0f));
        Row->AddChildToHorizontalBox(Spacer);

        // 删除按钮 (简化处理，暂不实现删除功能)
        UButton* RemoveButton = NewObject<UButton>(this);
        UTextBlock* RemoveText = NewObject<UTextBlock>(this);
        RemoveText->SetText(FText::FromString(TEXT("X")));
        RemoveButton->AddChild(RemoveText);
        Row->AddChildToHorizontalBox(RemoveButton);

        AgentListScrollBox->AddChild(Row);
    }

    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Agent list refreshed, %d configs"), AgentConfigs.Num());
}

void UMASetupWidget::UpdateTotalCount()
{
    int32 Total = GetTotalAgentCount();
    if (TotalCountText)
    {
        TotalCountText->SetText(FText::FromString(FString::Printf(TEXT("总计: %d 个智能体"), Total)));
    }
}

int32 UMASetupWidget::GetTotalAgentCount() const
{
    int32 Total = 0;
    for (const FMAAgentSetupConfig& Config : AgentConfigs)
    {
        Total += Config.Count;
    }
    return Total;
}

void UMASetupWidget::OnSceneSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    SelectedScene = SelectedItem;
    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Scene changed to: %s"), *SelectedScene);
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
    FString DisplayName = SelectedType;
    if (FString* FoundName = AvailableAgentTypes.Find(SelectedType))
    {
        DisplayName = *FoundName;
    }

    // 检查是否已存在相同类型，如果是则增加数量
    bool bFound = false;
    for (FMAAgentSetupConfig& Config : AgentConfigs)
    {
        if (Config.AgentType == SelectedType)
        {
            Config.Count += Count;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        AgentConfigs.Add(FMAAgentSetupConfig(SelectedType, DisplayName, Count));
    }

    RefreshAgentList();
    UpdateTotalCount();

    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Added %d x %s"), Count, *SelectedType);
}

void UMASetupWidget::OnClearButtonClicked()
{
    AgentConfigs.Empty();
    RefreshAgentList();
    UpdateTotalCount();

    UE_LOG(LogTemp, Log, TEXT("[MASetupWidget] Agent list cleared"));
}

void UMASetupWidget::OnStartButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] >>>>>> OnStartButtonClicked CALLED! <<<<<<"));
    
    if (AgentConfigs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Cannot start: no agents configured"));
        // 即使没有配置也允许开始（用于测试）
    }

    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Starting simulation with %d agent types, scene: %s"), 
        AgentConfigs.Num(), *SelectedScene);

    // 广播开始事件
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] Broadcasting OnStartSimulation..."));
    OnStartSimulation.Broadcast();
    UE_LOG(LogTemp, Warning, TEXT("[MASetupWidget] OnStartSimulation broadcasted"));
}

void UMASetupWidget::OnRemoveAgentClicked(int32 Index)
{
    if (AgentConfigs.IsValidIndex(Index))
    {
        AgentConfigs.RemoveAt(Index);
        RefreshAgentList();
        UpdateTotalCount();
    }
}
