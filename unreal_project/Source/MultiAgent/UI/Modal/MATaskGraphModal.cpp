// MATaskGraphModal.cpp
// 任务图模态窗口实现

#include "MATaskGraphModal.h"
#include "Core/TaskGraph/Application/MATaskGraphUseCases.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../TaskGraph/MADAGCanvasWidget.h"
#include "../TaskGraph/MATaskGraphModel.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY(LogMATaskGraphModal);

//=============================================================================
// 构造函数
//=============================================================================

UMATaskGraphModal::UMATaskGraphModal(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , DAGCanvas(nullptr)
    , DAGCanvasContainer(nullptr)
    , JsonPreviewBox(nullptr)
    , JsonPreviewContainer(nullptr)
    , JsonPreviewLabel(nullptr)
    , UpdateButton(nullptr)
    , JsonScrollBox(nullptr)
    , GraphModel(nullptr)
    , DAGCanvasHeightRatio(0.7f)
    , JsonPreviewHeightRatio(0.3f)
{
    // 设置模态类型
    SetModalType(EMAModalType::TaskGraph);
    
    // 设置模态大小 (覆盖约 80% 画面，假设 1920x1080 分辨率)
    // 80% 宽度 = 1536, 80% 高度 = 864
    // 但为了适应不同分辨率，使用稍小的固定值
    ModalWidth = 1200.0f;
    ModalHeight = 750.0f;
    
    // 设置按钮文本
    ConfirmButtonText = FText::FromString(TEXT("Confirm"));
    RejectButtonText = FText::FromString(TEXT("Reject"));
    EditButtonText = FText::FromString(TEXT("Edit"));
    
    // 显示编辑按钮
    bShowEditButton = true;
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMATaskGraphModal::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 创建数据模型
    if (!GraphModel)
    {
        GraphModel = NewObject<UMATaskGraphModel>(this);
        if (GraphModel)
        {
            GraphModel->OnDataChanged.AddDynamic(this, &UMATaskGraphModal::OnModelDataChanged);
        }
    }
    
    // Bind DAG canvas to model if both exist
    if (DAGCanvas && GraphModel)
    {
        DAGCanvas->BindToModel(GraphModel);
    }
    
    // 绑定基类按钮事件
    OnConfirmClicked.AddDynamic(this, &UMATaskGraphModal::HandleConfirm);
    OnRejectClicked.AddDynamic(this, &UMATaskGraphModal::HandleReject);
    
    UE_LOG(LogMATaskGraphModal, Log, TEXT("MATaskGraphModal constructed"));
}

void UMATaskGraphModal::NativeDestruct()
{
    // 解绑事件
    if (GraphModel)
    {
        GraphModel->OnDataChanged.RemoveDynamic(this, &UMATaskGraphModal::OnModelDataChanged);
    }
    
    OnConfirmClicked.RemoveDynamic(this, &UMATaskGraphModal::HandleConfirm);
    OnRejectClicked.RemoveDynamic(this, &UMATaskGraphModal::HandleReject);
    
    Super::NativeDestruct();
}

//=============================================================================
// UMABaseModalWidget 重写
//=============================================================================

void UMATaskGraphModal::OnEditModeChanged(bool bEditable)
{
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Edit mode changed to: %s"), bEditable ? TEXT("true") : TEXT("false"));
    
    // 更新 JSON 预览框的可编辑状态
    if (JsonPreviewBox)
    {
        JsonPreviewBox->SetIsReadOnly(!bEditable);
    }
    
    // 更新按钮可见性
    if (UpdateButton)
    {
        UpdateButton->SetVisibility(bEditable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    // 更新 DAG 画布的交互模式
    // Note: DAG canvas interaction is controlled through model binding
    // In read-only mode, we don't bind event handlers for editing
    
    // 更新标签文本
    if (JsonPreviewLabel)
    {
        FString LabelText = bEditable ? TEXT("JSON Editor (Editable)") : TEXT("JSON Preview (Read-Only)");
        JsonPreviewLabel->SetText(FText::FromString(LabelText));
    }
}

FText UMATaskGraphModal::GetModalTitleText() const
{
    if (IsEditMode())
    {
        return FText::FromString(TEXT("Edit Task Graph"));
    }
    return FText::FromString(TEXT("Task Graph Preview"));
}

void UMATaskGraphModal::BuildContentArea(UVerticalBox* InContentContainer)
{
    if (!InContentContainer || !WidgetTree)
    {
        UE_LOG(LogMATaskGraphModal, Error, TEXT("BuildContentArea: Invalid container or WidgetTree"));
        return;
    }
    
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Building content area..."));
    
    // 创建 DAG 画布区域 (占满整个内容区域)
    UBorder* DAGArea = CreateDAGCanvasArea();
    if (DAGArea)
    {
        InContentContainer->AddChild(DAGArea);
        
        if (UVerticalBoxSlot* DAGSlot = Cast<UVerticalBoxSlot>(DAGArea->Slot))
        {
            DAGSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            DAGSlot->SetPadding(FMargin(0.0f));
        }
    }
    
    // JSON 预览区域已移除 - 只显示 DAG 画布
    
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Content area built successfully"));
}

void UMATaskGraphModal::OnThemeApplied()
{
    // 应用主题到 DAG 画布容器 - 使用画布背景色和圆角效果
    if (DAGCanvasContainer && Theme)
    {
        FLinearColor CanvasBgColor = Theme->CanvasBackgroundColor;
        MARoundedBorderUtils::ApplyRoundedCorners(DAGCanvasContainer, CanvasBgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 应用主题到 JSON 预览标签
    if (JsonPreviewLabel && Theme)
    {
        JsonPreviewLabel->SetColorAndOpacity(FSlateColor(Theme->TextColor));
    }
}

//=============================================================================
// 公共接口
//=============================================================================

void UMATaskGraphModal::LoadTaskGraph(const FMATaskGraphData& Data)
{
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Loading task graph with %d nodes and %d edges"),
        Data.Nodes.Num(), Data.Edges.Num());
    
    // 保存原始数据
    OriginalData = Data;
    
    // 加载到模型
    if (GraphModel)
    {
        GraphModel->LoadFromData(Data);
    }
    
    // 更新 DAG 画布
    if (DAGCanvas)
    {
        DAGCanvas->BindToModel(GraphModel);
        DAGCanvas->RefreshFromModel();
    }
    
    // 同步 JSON 预览
    SyncJsonPreview();
}

bool UMATaskGraphModal::LoadTaskGraphFromJson(const FString& JsonString)
{
    const FTaskGraphLoadResult LoadResult = FTaskGraphUseCases::ParseFlexibleJson(JsonString);
    if (!LoadResult.bSuccess)
    {
        UE_LOG(LogMATaskGraphModal, Warning, TEXT("Failed to parse JSON: %s"), *LoadResult.Feedback.Message);
        return false;
    }
    
    LoadTaskGraph(LoadResult.Data);
    return true;
}

FMATaskGraphData UMATaskGraphModal::GetTaskGraphData() const
{
    if (GraphModel)
    {
        return GraphModel->GetWorkingData();
    }
    return FMATaskGraphData();
}

FString UMATaskGraphModal::GetTaskGraphJson() const
{
    return FTaskGraphUseCases::Serialize(GetTaskGraphData());
}

//=============================================================================
// 内部方法
//=============================================================================

UBorder* UMATaskGraphModal::CreateDAGCanvasArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框
    DAGCanvasContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DAGCanvasContainer"));
    if (!DAGCanvasContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色 - 使用画布背景色和圆角效果
    FLinearColor BgColor = Theme ? Theme->CanvasBackgroundColor : FLinearColor(0.647f, 0.886f, 0.969f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(DAGCanvasContainer, BgColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    DAGCanvasContainer->SetPadding(FMargin(4.0f));
    
    // 启用裁剪 - 确保 DAG 内容不会溢出边界
    DAGCanvasContainer->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    
    // 创建 DAG 画布
    DAGCanvas = CreateWidget<UMADAGCanvasWidget>(this, UMADAGCanvasWidget::StaticClass());
    if (DAGCanvas)
    {
        DAGCanvasContainer->AddChild(DAGCanvas);
        
        // Bind to model if available
        if (GraphModel)
        {
            DAGCanvas->BindToModel(GraphModel);
        }
    }
    
    return DAGCanvasContainer;
}

UVerticalBox* UMATaskGraphModal::CreateJsonPreviewArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器
    JsonPreviewContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("JsonPreviewContainer"));
    if (!JsonPreviewContainer)
    {
        return nullptr;
    }
    
    // 创建标签和按钮的水平布局
    UHorizontalBox* HeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("JsonHeaderBox"));
    if (HeaderBox)
    {
        JsonPreviewContainer->AddChild(HeaderBox);
        
        if (UVerticalBoxSlot* HeaderSlot = Cast<UVerticalBoxSlot>(HeaderBox->Slot))
        {
            HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        }
        
        // 创建标签
        JsonPreviewLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonPreviewLabel"));
        if (JsonPreviewLabel)
        {
            HeaderBox->AddChild(JsonPreviewLabel);
            JsonPreviewLabel->SetText(FText::FromString(TEXT("JSON Preview (Read-Only)")));
            
            FLinearColor TextColor = Theme ? Theme->TextColor : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
            JsonPreviewLabel->SetColorAndOpacity(FSlateColor(TextColor));
            
            FSlateFontInfo Font = JsonPreviewLabel->GetFont();
            Font.Size = 12;
            JsonPreviewLabel->SetFont(Font);
            
            if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(JsonPreviewLabel->Slot))
            {
                LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                LabelSlot->SetVerticalAlignment(VAlign_Center);
            }
        }
        
        // 创建更新按钮 (编辑模式)
        UpdateButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpdateButton"));
        if (UpdateButton)
        {
            HeaderBox->AddChild(UpdateButton);
            UpdateButton->SetVisibility(ESlateVisibility::Collapsed); // 初始隐藏
            UpdateButton->OnClicked.AddDynamic(this, &UMATaskGraphModal::OnUpdateButtonClicked);
            
            // 创建按钮文本
            UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpdateButtonText"));
            if (ButtonText)
            {
                UpdateButton->AddChild(ButtonText);
                ButtonText->SetText(FText::FromString(TEXT("Update")));
                ButtonText->SetJustification(ETextJustify::Center);
            }
        }
    }
    
    // 创建滚动容器
    JsonScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JsonScrollBox"));
    if (JsonScrollBox)
    {
        JsonPreviewContainer->AddChild(JsonScrollBox);
        
        // 创建 JSON 文本框
        JsonPreviewBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
            UMultiLineEditableTextBox::StaticClass(), TEXT("JsonPreviewBox"));
        if (JsonPreviewBox)
        {
            JsonScrollBox->AddChild(JsonPreviewBox);
            JsonPreviewBox->SetIsReadOnly(true); // 初始只读
            
            // 设置样式：使用主题输入框文字颜色，字号 12
            FEditableTextBoxStyle JsonStyle;
            FLinearColor InputTextCol = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
            FSlateColor InputTextColor = FSlateColor(InputTextCol);
            JsonStyle.SetForegroundColor(InputTextColor);
            JsonStyle.SetFocusedForegroundColor(InputTextColor);
            FSlateFontInfo JsonFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
            JsonStyle.SetFont(JsonFont);
            JsonPreviewBox->WidgetStyle = JsonStyle;
        }
        
        if (UVerticalBoxSlot* ScrollSlot = Cast<UVerticalBoxSlot>(JsonScrollBox->Slot))
        {
            ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }
    
    return JsonPreviewContainer;
}

void UMATaskGraphModal::SyncJsonPreview()
{
    if (!JsonPreviewBox || !GraphModel)
    {
        return;
    }
    
    const FString JsonString = FTaskGraphUseCases::Serialize(GraphModel->GetWorkingData());
    
    JsonPreviewBox->SetText(FText::FromString(JsonString));
}

void UMATaskGraphModal::UpdateModelFromJson()
{
    if (!JsonPreviewBox || !GraphModel)
    {
        return;
    }
    
    FString JsonString = JsonPreviewBox->GetText().ToString();
    const FTaskGraphLoadResult LoadResult = FTaskGraphUseCases::ParseFlexibleJson(JsonString);
    if (LoadResult.bSuccess)
    {
        GraphModel->LoadFromData(LoadResult.Data);
        
        // 更新 DAG 画布
        if (DAGCanvas && GraphModel)
        {
            DAGCanvas->BindToModel(GraphModel);
            DAGCanvas->RefreshFromModel();
        }
        
        UE_LOG(LogMATaskGraphModal, Log, TEXT("Updated model from JSON"));
    }
    else
    {
        UE_LOG(LogMATaskGraphModal, Warning, TEXT("Failed to parse JSON: %s"), *LoadResult.Feedback.Message);
    }
}

void UMATaskGraphModal::HandleConfirm()
{
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Task graph confirmed"));
    
    // 获取当前数据
    FMATaskGraphData Data = GetTaskGraphData();
    
    // 广播确认事件
    OnTaskGraphConfirmed.Broadcast(Data);
}

void UMATaskGraphModal::HandleReject()
{
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Task graph rejected"));
    
    // 恢复原始数据
    if (GraphModel)
    {
        GraphModel->LoadFromData(OriginalData);
    }
    
    if (DAGCanvas && GraphModel)
    {
        DAGCanvas->BindToModel(GraphModel);
        DAGCanvas->RefreshFromModel();
    }
    
    SyncJsonPreview();
    
    // 广播拒绝事件
    OnTaskGraphRejected.Broadcast();
}

//=============================================================================
// 事件处理
//=============================================================================

void UMATaskGraphModal::OnModelDataChanged()
{
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Model data changed"));
    
    // 同步 JSON 预览
    SyncJsonPreview();
    
    // 更新 DAG 画布
    if (DAGCanvas && GraphModel)
    {
        DAGCanvas->RefreshFromModel();
    }
}

void UMATaskGraphModal::OnUpdateButtonClicked()
{
    UE_LOG(LogMATaskGraphModal, Log, TEXT("Update button clicked"));
    
    // 从 JSON 编辑器更新模型
    UpdateModelFromJson();
}
