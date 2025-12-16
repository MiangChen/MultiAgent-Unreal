// MATaskPlannerWidget.cpp
// 任务规划工作台主容器 Widget - 纯 C++ 实现
// Requirements: 1.1, 1.2, 1.3, 1.4, 2.1, 2.2, 2.3, 3.1, 3.2, 3.3, 3.4

#include "MATaskPlannerWidget.h"
#include "MADAGCanvasWidget.h"
#include "MANodePaletteWidget.h"
#include "MATaskGraphModel.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogMATaskPlanner);

//=============================================================================
// 构造函数
//=============================================================================

UMATaskPlannerWidget::UMATaskPlannerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMATaskPlannerWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMATaskPlannerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定按钮事件
    if (UpdateGraphButton && !UpdateGraphButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnUpdateGraphButtonClicked))
    {
        UpdateGraphButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnUpdateGraphButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("UpdateGraphButton event bound"));
    }
    
    // 绑定发送指令按钮事件
    if (SendCommandButton && !SendCommandButton->OnClicked.IsAlreadyBound(this, &UMATaskPlannerWidget::OnSendCommandButtonClicked))
    {
        SendCommandButton->OnClicked.AddDynamic(this, &UMATaskPlannerWidget::OnSendCommandButtonClicked);
        UE_LOG(LogMATaskPlanner, Log, TEXT("SendCommandButton event bound"));
    }
    
    // 绑定数据模型事件
    if (GraphModel && !GraphModel->OnDataChanged.IsAlreadyBound(this, &UMATaskPlannerWidget::OnModelDataChanged))
    {
        GraphModel->OnDataChanged.AddDynamic(this, &UMATaskPlannerWidget::OnModelDataChanged);
        UE_LOG(LogMATaskPlanner, Log, TEXT("GraphModel OnDataChanged event bound"));
    }
    
    // 绑定节点工具栏事件
    if (NodePalette && !NodePalette->OnNodeTemplateSelected.IsAlreadyBound(this, &UMATaskPlannerWidget::OnNodeTemplateSelected))
    {
        NodePalette->OnNodeTemplateSelected.AddDynamic(this, &UMATaskPlannerWidget::OnNodeTemplateSelected);
        UE_LOG(LogMATaskPlanner, Log, TEXT("NodePalette OnNodeTemplateSelected event bound"));
    }
    
    // 绑定画布事件
    if (DAGCanvas)
    {
        if (!DAGCanvas->OnNodeDeleteRequested.IsAlreadyBound(this, &UMATaskPlannerWidget::OnNodeDeleteRequested))
        {
            DAGCanvas->OnNodeDeleteRequested.AddDynamic(this, &UMATaskPlannerWidget::OnNodeDeleteRequested);
        }
        if (!DAGCanvas->OnNodeEditRequested.IsAlreadyBound(this, &UMATaskPlannerWidget::OnNodeEditRequested))
        {
            DAGCanvas->OnNodeEditRequested.AddDynamic(this, &UMATaskPlannerWidget::OnNodeEditRequested);
        }
        if (!DAGCanvas->OnEdgeCreated.IsAlreadyBound(this, &UMATaskPlannerWidget::OnEdgeCreated))
        {
            DAGCanvas->OnEdgeCreated.AddDynamic(this, &UMATaskPlannerWidget::OnEdgeCreated);
        }
        if (!DAGCanvas->OnEdgeDeleteRequested.IsAlreadyBound(this, &UMATaskPlannerWidget::OnEdgeDeleteRequested))
        {
            DAGCanvas->OnEdgeDeleteRequested.AddDynamic(this, &UMATaskPlannerWidget::OnEdgeDeleteRequested);
        }
        UE_LOG(LogMATaskPlanner, Log, TEXT("DAGCanvas events bound"));
    }
    
    // 初始化状态日志
    AppendStatusLog(TEXT("任务规划工作台已启动"));
    AppendStatusLog(TEXT("提示: 从左侧工具栏拖拽节点到画布创建任务"));
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("MATaskPlannerWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMATaskPlannerWidget::RebuildWidget()
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

FReply UMATaskPlannerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 拦截所有鼠标按钮事件，防止穿透到游戏场景
    // 让子 Widget 处理具体的交互逻辑
    FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    
    // 无论子 Widget 是否处理，都返回 Handled 防止事件穿透
    return FReply::Handled();
}

FReply UMATaskPlannerWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 拦截所有鼠标按钮释放事件
    FReply Reply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    return FReply::Handled();
}

FReply UMATaskPlannerWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 拦截鼠标滚轮事件
    FReply Reply = Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    return FReply::Handled();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMATaskPlannerWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建数据模型
    GraphModel = NewObject<UMATaskGraphModel>(this, TEXT("GraphModel"));

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建主背景 Border
    UBorder* MainBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBackground"));
    MainBackground->SetBrushColor(BackgroundColor);
    MainBackground->SetPadding(FMargin(10.0f));
    
    UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainBackground);
    // 全屏填充
    MainSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    MainSlot->SetOffsets(FMargin(0.0f));

    // 创建主水平布局
    UHorizontalBox* MainHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainHBox"));
    MainBackground->AddChild(MainHBox);

    // 创建左侧面板
    UBorder* LeftPanel = CreateLeftPanel();
    UHorizontalBoxSlot* LeftSlot = MainHBox->AddChildToHorizontalBox(LeftPanel);
    LeftSlot->SetPadding(FMargin(0, 0, 5, 0));
    // 设置左侧面板占比 (使用 FillSize)
    FSlateChildSize LeftSize(ESlateSizeRule::Fill);
    LeftSize.Value = 0.35f;
    LeftSlot->SetSize(LeftSize);

    // 创建右侧面板
    UBorder* RightPanel = CreateRightPanel();
    UHorizontalBoxSlot* RightSlot = MainHBox->AddChildToHorizontalBox(RightPanel);
    // 设置右侧面板占比 (使用 FillSize)
    FSlateChildSize RightSize(ESlateSizeRule::Fill);
    RightSize.Value = 0.65f;
    RightSlot->SetSize(RightSize);

    UE_LOG(LogMATaskPlanner, Log, TEXT("BuildUI: UI construction completed successfully"));
}

UBorder* UMATaskPlannerWidget::CreateLeftPanel()
{
    // 左侧面板背景
    UBorder* LeftPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanelBorder"));
    LeftPanelBorder->SetBrushColor(PanelBackgroundColor);
    LeftPanelBorder->SetPadding(FMargin(10.0f));

    // 垂直布局
    UVerticalBox* LeftVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVBox"));
    LeftPanelBorder->AddChild(LeftVBox);

    // 标题
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LeftPanelTitle"));
    TitleText->SetText(FText::FromString(TEXT("任务规划工作台")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    
    UVerticalBoxSlot* TitleSlot = LeftVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 提示文本
    UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(TEXT("按 Z 键切换显示/隐藏")));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 10;
    HintText->SetFont(HintFont);
    HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.6f)));
    
    UVerticalBoxSlot* HintSlot = LeftVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 15));

    // 状态日志区域
    UVerticalBox* StatusLogSection = CreateStatusLogSection();
    UVerticalBoxSlot* StatusSlot = LeftVBox->AddChildToVerticalBox(StatusLogSection);
    FSlateChildSize StatusSize(ESlateSizeRule::Fill);
    StatusSize.Value = 0.3f;
    StatusSlot->SetSize(StatusSize);
    StatusSlot->SetPadding(FMargin(0, 0, 0, 10));

    // JSON 编辑器区域
    UVerticalBox* JsonEditorSection = CreateJsonEditorSection();
    UVerticalBoxSlot* JsonSlot = LeftVBox->AddChildToVerticalBox(JsonEditorSection);
    FSlateChildSize JsonSize(ESlateSizeRule::Fill);
    JsonSize.Value = 0.5f;
    JsonSlot->SetSize(JsonSize);
    JsonSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 用户指令输入区域
    UVerticalBox* UserInputSection = CreateUserInputSection();
    UVerticalBoxSlot* UserInputSlot = LeftVBox->AddChildToVerticalBox(UserInputSection);
    FSlateChildSize UserInputSize(ESlateSizeRule::Fill);
    UserInputSize.Value = 0.2f;
    UserInputSlot->SetSize(UserInputSize);

    return LeftPanelBorder;
}

UVerticalBox* UMATaskPlannerWidget::CreateStatusLogSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusLogSection"));

    // 标签
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusLogLabel"));
    Label->SetText(FText::FromString(TEXT("状态日志:")));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // 使用 ScrollBox 包装状态日志以支持自动滚动
    StatusLogScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("StatusLogScrollBox"));
    
    // 状态日志文本框 (只读)
    StatusLogBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("StatusLogBox"));
    StatusLogBox->SetIsReadOnly(true);
    StatusLogBox->SetText(FText::GetEmpty());
    
    StatusLogScrollBox->AddChild(StatusLogBox);
    
    // 使用 SizeBox 设置最小高度
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StatusLogSizeBox"));
    SizeBox->SetMinDesiredHeight(100.0f);
    SizeBox->AddChild(StatusLogScrollBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    return Section;
}

UVerticalBox* UMATaskPlannerWidget::CreateJsonEditorSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JsonEditorSection"));

    // 标签
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonEditorLabel"));
    Label->SetText(FText::FromString(TEXT("JSON 编辑器:")));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // JSON 编辑器文本框 (可编辑)
    JsonEditorBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditorBox"));
    JsonEditorBox->SetIsReadOnly(false);
    JsonEditorBox->SetText(FText::FromString(TEXT("{\n  \"description\": \"\",\n  \"nodes\": [],\n  \"edges\": []\n}")));
    
    // 使用 SizeBox 设置最小高度
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditorSizeBox"));
    SizeBox->SetMinDesiredHeight(200.0f);
    SizeBox->AddChild(JsonEditorBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 10));

    // "更新任务图" 按钮
    UpdateGraphButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpdateGraphButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpdateButtonText"));
    ButtonText->SetText(FText::FromString(TEXT(" 更新任务图 ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    UpdateGraphButton->AddChild(ButtonText);
    
    UVerticalBoxSlot* ButtonSlot = Section->AddChildToVerticalBox(UpdateGraphButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UVerticalBox* UMATaskPlannerWidget::CreateUserInputSection()
{
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UserInputSection"));

    // 标签
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UserInputLabel"));
    Label->SetText(FText::FromString(TEXT("指令输入:")));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    
    UVerticalBoxSlot* LabelSlot = Section->AddChildToVerticalBox(Label);
    LabelSlot->SetPadding(FMargin(0, 0, 0, 5));

    // 用户输入文本框 (可编辑)
    UserInputBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("UserInputBox"));
    UserInputBox->SetIsReadOnly(false);
    UserInputBox->SetHintText(FText::FromString(TEXT("输入自然语言指令，例如：让机器人去巡逻...")));
    
    // 使用 SizeBox 设置最小高度
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("UserInputSizeBox"));
    SizeBox->SetMinDesiredHeight(60.0f);
    SizeBox->AddChild(UserInputBox);
    
    UVerticalBoxSlot* BoxSlot = Section->AddChildToVerticalBox(SizeBox);
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BoxSlot->SetPadding(FMargin(0, 0, 0, 10));

    // "发送指令" 按钮
    SendCommandButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendCommandButton"));
    
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SendButtonText"));
    ButtonText->SetText(FText::FromString(TEXT(" 发送指令 ")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    SendCommandButton->AddChild(ButtonText);
    
    UVerticalBoxSlot* ButtonSlot = Section->AddChildToVerticalBox(SendCommandButton);
    ButtonSlot->SetHorizontalAlignment(HAlign_Left);

    return Section;
}

UBorder* UMATaskPlannerWidget::CreateRightPanel()
{
    // 右侧面板背景
    UBorder* RightPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBorder"));
    RightPanelBorder->SetBrushColor(PanelBackgroundColor);
    RightPanelBorder->SetPadding(FMargin(10.0f));

    // 水平布局 (画布 + 工具栏)
    UHorizontalBox* RightHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RightHBox"));
    RightPanelBorder->AddChild(RightHBox);

    // DAG 画布
    DAGCanvas = WidgetTree->ConstructWidget<UMADAGCanvasWidget>(UMADAGCanvasWidget::StaticClass(), TEXT("DAGCanvas"));
    DAGCanvas->BindToModel(GraphModel);
    
    UHorizontalBoxSlot* CanvasSlot = RightHBox->AddChildToHorizontalBox(DAGCanvas);
    CanvasSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CanvasSlot->SetPadding(FMargin(0, 0, 10, 0));

    // 节点工具栏
    NodePalette = WidgetTree->ConstructWidget<UMANodePaletteWidget>(UMANodePaletteWidget::StaticClass(), TEXT("NodePalette"));
    NodePalette->InitializeTemplates();
    
    UHorizontalBoxSlot* PaletteSlot = RightHBox->AddChildToHorizontalBox(NodePalette);
    PaletteSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    return RightPanelBorder;
}

//=============================================================================
// 公共接口
//=============================================================================

void UMATaskPlannerWidget::LoadTaskGraph(const FMATaskGraphData& Data)
{
    if (!GraphModel)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("LoadTaskGraph: GraphModel is null!"));
        return;
    }
    
    GraphModel->LoadFromData(Data);
    SyncJsonEditorFromModel();
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(FString::Printf(TEXT("已加载任务图: %d 个节点, %d 条边"), 
        Data.Nodes.Num(), Data.Edges.Num()));
    
    OnTaskGraphChanged.Broadcast(Data);
}

bool UMATaskPlannerWidget::LoadTaskGraphFromJson(const FString& JsonString)
{
    if (!GraphModel)
    {
        UE_LOG(LogMATaskPlanner, Error, TEXT("LoadTaskGraphFromJson: GraphModel is null!"));
        return false;
    }
    
    FString ErrorMessage;
    if (!GraphModel->LoadFromJsonWithError(JsonString, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] JSON 解析失败: %s"), *ErrorMessage));
        return false;
    }
    
    SyncJsonEditorFromModel();
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    FMATaskGraphData Data = GraphModel->GetWorkingData();
    AppendStatusLog(FString::Printf(TEXT("已加载任务图: %d 个节点, %d 条边"), 
        Data.Nodes.Num(), Data.Edges.Num()));
    
    OnTaskGraphChanged.Broadcast(Data);
    return true;
}

void UMATaskPlannerWidget::AppendStatusLog(const FString& Message)
{
    if (!StatusLogBox)
    {
        return;
    }
    
    FString Timestamp = GetTimestamp();
    FString CurrentText = StatusLogBox->GetText().ToString();
    FString NewLine = FString::Printf(TEXT("[%s] %s"), *Timestamp, *Message);
    
    if (CurrentText.IsEmpty())
    {
        StatusLogBox->SetText(FText::FromString(NewLine));
    }
    else
    {
        StatusLogBox->SetText(FText::FromString(CurrentText + TEXT("\n") + NewLine));
    }
    
    // 自动滚动到最新内容 (Requirements: 2.2)
    if (StatusLogScrollBox)
    {
        StatusLogScrollBox->ScrollToEnd();
    }
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("StatusLog: %s"), *Message);
}

void UMATaskPlannerWidget::ClearStatusLog()
{
    if (StatusLogBox)
    {
        StatusLogBox->SetText(FText::GetEmpty());
    }
}

FString UMATaskPlannerWidget::GetJsonText() const
{
    if (JsonEditorBox)
    {
        return JsonEditorBox->GetText().ToString();
    }
    return FString();
}

void UMATaskPlannerWidget::SetJsonText(const FString& JsonText)
{
    if (JsonEditorBox)
    {
        JsonEditorBox->SetText(FText::FromString(JsonText));
    }
}

void UMATaskPlannerWidget::FocusJsonEditor()
{
    if (JsonEditorBox)
    {
        JsonEditorBox->SetKeyboardFocus();
    }
}

//=============================================================================
// 事件处理
//=============================================================================

void UMATaskPlannerWidget::OnUpdateGraphButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("UpdateGraphButton clicked"));
    
    FString JsonText = GetJsonText();
    
    if (JsonText.IsEmpty())
    {
        AppendStatusLog(TEXT("[警告] JSON 编辑器为空"));
        return;
    }
    
    FString ErrorMessage;
    if (!GraphModel->LoadFromJsonWithError(JsonText, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] JSON 解析失败: %s"), *ErrorMessage));
        return;
    }
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
    
    AppendStatusLog(TEXT("[成功] 任务图已更新"));
    
    OnTaskGraphChanged.Broadcast(GraphModel->GetWorkingData());
}

void UMATaskPlannerWidget::OnSendCommandButtonClicked()
{
    UE_LOG(LogMATaskPlanner, Log, TEXT("SendCommandButton clicked"));
    
    if (!UserInputBox)
    {
        return;
    }
    
    FString Command = UserInputBox->GetText().ToString().TrimStartAndEnd();
    
    if (Command.IsEmpty())
    {
        AppendStatusLog(TEXT("[警告] 请输入指令内容"));
        return;
    }
    
    // 记录发送的指令
    AppendStatusLog(FString::Printf(TEXT("[发送] %s"), *Command));
    
    // 清空输入框
    UserInputBox->SetText(FText::GetEmpty());
    
    // 广播指令提交事件 (由 HUD 或其他组件处理转发到后端)
    OnCommandSubmitted.Broadcast(Command);
    
    UE_LOG(LogMATaskPlanner, Log, TEXT("Command submitted: %s"), *Command);
}

void UMATaskPlannerWidget::OnModelDataChanged()
{
    // 画布操作时实时同步到 JSON 编辑器
    SyncJsonEditorFromModel();
}

void UMATaskPlannerWidget::OnNodeTemplateSelected(const FMANodeTemplate& Template)
{
    if (!GraphModel)
    {
        return;
    }
    
    // 创建新节点
    FString NewNodeId = GraphModel->CreateNode(Template.DefaultDescription, Template.DefaultLocation);
    
    AppendStatusLog(FString::Printf(TEXT("已创建节点: %s (%s)"), *NewNodeId, *Template.TemplateName));
    
    if (DAGCanvas)
    {
        DAGCanvas->RefreshFromModel();
    }
}

void UMATaskPlannerWidget::OnNodeDeleteRequested(const FString& NodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    if (GraphModel->RemoveNode(NodeId))
    {
        AppendStatusLog(FString::Printf(TEXT("已删除节点: %s"), *NodeId));
        
        if (DAGCanvas)
        {
            DAGCanvas->RefreshFromModel();
        }
    }
}

void UMATaskPlannerWidget::OnNodeEditRequested(const FString& NodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    // 获取节点数据
    FMATaskNodeData NodeData;
    if (!GraphModel->FindNode(NodeId, NodeData))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] 找不到节点: %s"), *NodeId));
        return;
    }
    
    // TODO: 显示节点编辑对话框
    // 目前只是在状态日志中显示节点信息
    AppendStatusLog(FString::Printf(TEXT("编辑节点: %s"), *NodeId));
    AppendStatusLog(FString::Printf(TEXT("  描述: %s"), *NodeData.Description));
    AppendStatusLog(FString::Printf(TEXT("  位置: %s"), *NodeData.Location));
    
    // 选中该节点
    if (DAGCanvas)
    {
        DAGCanvas->SelectNode(NodeId);
    }
}

void UMATaskPlannerWidget::OnEdgeCreated(const FString& FromNodeId, const FString& ToNodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    if (GraphModel->AddEdge(FromNodeId, ToNodeId))
    {
        AppendStatusLog(FString::Printf(TEXT("已创建连线: %s -> %s"), *FromNodeId, *ToNodeId));
    }
}

void UMATaskPlannerWidget::OnEdgeDeleteRequested(const FString& FromNodeId, const FString& ToNodeId)
{
    if (!GraphModel)
    {
        return;
    }
    
    if (GraphModel->RemoveEdge(FromNodeId, ToNodeId))
    {
        AppendStatusLog(FString::Printf(TEXT("已删除连线: %s -> %s"), *FromNodeId, *ToNodeId));
        
        if (DAGCanvas)
        {
            DAGCanvas->RefreshFromModel();
        }
    }
}

//=============================================================================
// 辅助方法
//=============================================================================

void UMATaskPlannerWidget::SyncJsonEditorFromModel()
{
    if (!GraphModel || !JsonEditorBox)
    {
        return;
    }
    
    FString JsonText = GraphModel->ToJson();
    JsonEditorBox->SetText(FText::FromString(JsonText));
}

FString UMATaskPlannerWidget::GetTimestamp() const
{
    FDateTime Now = FDateTime::Now();
    return FString::Printf(TEXT("%02d:%02d:%02d"), Now.GetHour(), Now.GetMinute(), Now.GetSecond());
}

bool UMATaskPlannerWidget::LoadMockData()
{
    // 构建 Mock 数据文件路径
    FString FilePath = FPaths::ProjectDir() / TEXT("datasets/response_example.json");
    
    // 检查文件是否存在
    if (!FPaths::FileExists(FilePath))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] Mock 数据文件不存在: %s"), *FilePath));
        UE_LOG(LogMATaskPlanner, Warning, TEXT("Mock data file not found: %s"), *FilePath);
        return false;
    }
    
    // 读取文件内容
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] 无法读取文件: %s"), *FilePath));
        UE_LOG(LogMATaskPlanner, Error, TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }
    
    // 解析 JSON (使用 response_example.json 格式)
    FMATaskGraphData Data;
    FString ErrorMessage;
    if (!FMATaskGraphData::FromResponseJson(JsonContent, Data, ErrorMessage))
    {
        AppendStatusLog(FString::Printf(TEXT("[错误] Mock 数据格式错误: %s"), *ErrorMessage));
        UE_LOG(LogMATaskPlanner, Error, TEXT("Failed to parse mock data: %s"), *ErrorMessage);
        return false;
    }
    
    // 加载数据
    LoadTaskGraph(Data);
    AppendStatusLog(TEXT("[成功] Mock 数据加载完成"));
    
    return true;
}
