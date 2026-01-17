// MASkillListModal.cpp
// 技能列表模态窗口实现
// Requirements: 5.2, 5.3, 6.1, 6.4

#include "MASkillListModal.h"
#include "../Core/MAUITheme.h"
#include "../SkillList/MAGanttCanvas.h"
#include "../SkillList/MASkillAllocationModel.h"
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

DEFINE_LOG_CATEGORY(LogMASkillListModal);

//=============================================================================
// 构造函数
//=============================================================================

UMASkillListModal::UMASkillListModal(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , GanttCanvas(nullptr)
    , GanttCanvasContainer(nullptr)
    , JsonPreviewBox(nullptr)
    , JsonPreviewContainer(nullptr)
    , JsonPreviewLabel(nullptr)
    , UpdateButton(nullptr)
    , JsonScrollBox(nullptr)
    , AllocationModel(nullptr)
    , GanttCanvasHeightRatio(0.7f)
    , JsonPreviewHeightRatio(0.3f)
{
    // 设置模态类型
    SetModalType(EMAModalType::SkillList);
    
    // 设置模态大小 (覆盖约 80% 画面，假设 1920x1080 分辨率)
    // 与 TaskGraphModal 保持一致
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

void UMASkillListModal::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 创建数据模型
    if (!AllocationModel)
    {
        AllocationModel = NewObject<UMASkillAllocationModel>(this);
        if (AllocationModel)
        {
            AllocationModel->OnDataChanged.AddDynamic(this, &UMASkillListModal::OnModelDataChanged);
        }
    }
    
    // Bind Gantt canvas to model if both exist
    if (GanttCanvas && AllocationModel)
    {
        GanttCanvas->BindToModel(AllocationModel);
    }
    
    // 绑定基类按钮事件
    OnConfirmClicked.AddDynamic(this, &UMASkillListModal::HandleConfirm);
    OnRejectClicked.AddDynamic(this, &UMASkillListModal::HandleReject);
    
    UE_LOG(LogMASkillListModal, Log, TEXT("MASkillListModal constructed"));
}

void UMASkillListModal::NativeDestruct()
{
    // 解绑事件
    if (AllocationModel)
    {
        AllocationModel->OnDataChanged.RemoveDynamic(this, &UMASkillListModal::OnModelDataChanged);
    }
    
    OnConfirmClicked.RemoveDynamic(this, &UMASkillListModal::HandleConfirm);
    OnRejectClicked.RemoveDynamic(this, &UMASkillListModal::HandleReject);
    
    Super::NativeDestruct();
}

//=============================================================================
// UMABaseModalWidget 重写
//=============================================================================

void UMASkillListModal::OnEditModeChanged(bool bEditable)
{
    UE_LOG(LogMASkillListModal, Log, TEXT("Edit mode changed to: %s"), bEditable ? TEXT("true") : TEXT("false"));
    
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
    
    // 更新甘特图画布的交互模式
    if (GanttCanvas)
    {
        GanttCanvas->SetDragEnabled(bEditable);
    }
    
    // 更新标签文本
    if (JsonPreviewLabel)
    {
        FString LabelText = bEditable ? TEXT("JSON Editor (Editable)") : TEXT("JSON Preview (Read-Only)");
        JsonPreviewLabel->SetText(FText::FromString(LabelText));
    }
}

FText UMASkillListModal::GetModalTitleText() const
{
    if (IsEditMode())
    {
        return FText::FromString(TEXT("Edit Skill List"));
    }
    return FText::FromString(TEXT("Skill List"));
}

void UMASkillListModal::BuildContentArea(UVerticalBox* InContentContainer)
{
    if (!InContentContainer || !WidgetTree)
    {
        UE_LOG(LogMASkillListModal, Error, TEXT("BuildContentArea: Invalid container or WidgetTree"));
        return;
    }
    
    UE_LOG(LogMASkillListModal, Log, TEXT("Building content area..."));
    
    // 创建甘特图画布区域 (占满整个内容区域)
    UBorder* GanttArea = CreateGanttCanvasArea();
    if (GanttArea)
    {
        InContentContainer->AddChild(GanttArea);
        
        if (UVerticalBoxSlot* GanttSlot = Cast<UVerticalBoxSlot>(GanttArea->Slot))
        {
            GanttSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            GanttSlot->SetPadding(FMargin(0.0f));
        }
    }
    
    // JSON 预览区域已移除 - 只显示甘特图画布
    
    UE_LOG(LogMASkillListModal, Log, TEXT("Content area built successfully"));
}

void UMASkillListModal::OnThemeApplied()
{
    // 应用主题到甘特图画布容器
    if (GanttCanvasContainer && Theme)
    {
        FLinearColor CanvasBgColor = Theme->SecondaryColor;
        CanvasBgColor.A = 0.5f;
        GanttCanvasContainer->SetBrushColor(CanvasBgColor);
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

void UMASkillListModal::LoadSkillAllocation(const FMASkillAllocationData& Data)
{
    UE_LOG(LogMASkillListModal, Log, TEXT("Loading skill allocation with %d time steps"),
        Data.Data.Num());
    
    // 保存原始数据
    OriginalData = Data;
    
    // 加载到模型
    if (AllocationModel)
    {
        AllocationModel->LoadFromData(Data);
    }
    
    // 更新甘特图画布
    if (GanttCanvas && AllocationModel)
    {
        GanttCanvas->BindToModel(AllocationModel);
        GanttCanvas->RefreshFromModel();
    }
    
    // 同步 JSON 预览
    SyncJsonPreview();
}

bool UMASkillListModal::LoadSkillAllocationFromJson(const FString& JsonString)
{
    FMASkillAllocationData Data;
    FString ErrorMessage;
    
    if (!FMASkillAllocationData::FromJson(JsonString, Data, ErrorMessage))
    {
        UE_LOG(LogMASkillListModal, Warning, TEXT("Failed to parse JSON: %s"), *ErrorMessage);
        return false;
    }
    
    LoadSkillAllocation(Data);
    return true;
}

FMASkillAllocationData UMASkillListModal::GetSkillAllocationData() const
{
    if (AllocationModel)
    {
        return AllocationModel->GetWorkingData();
    }
    return FMASkillAllocationData();
}

//=============================================================================
// 内部方法
//=============================================================================

UBorder* UMASkillListModal::CreateGanttCanvasArea()
{
    if (!WidgetTree)
    {
        return nullptr;
    }
    
    // 创建容器边框
    GanttCanvasContainer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GanttCanvasContainer"));
    if (!GanttCanvasContainer)
    {
        return nullptr;
    }
    
    // 设置背景颜色
    FLinearColor BgColor = Theme ? Theme->SecondaryColor : FLinearColor(0.08f, 0.08f, 0.1f, 0.5f);
    BgColor.A = 0.5f;
    GanttCanvasContainer->SetBrushColor(BgColor);
    GanttCanvasContainer->SetPadding(FMargin(4.0f));
    
    // 启用裁剪 - 确保甘特图内容不会溢出边界
    GanttCanvasContainer->SetClipping(EWidgetClipping::ClipToBounds);
    
    // 创建甘特图画布
    GanttCanvas = CreateWidget<UMAGanttCanvas>(this, UMAGanttCanvas::StaticClass());
    if (GanttCanvas)
    {
        GanttCanvasContainer->AddChild(GanttCanvas);
        
        // Bind to model if available
        if (AllocationModel)
        {
            GanttCanvas->BindToModel(AllocationModel);
        }
        
        // 初始设置为只读模式
        GanttCanvas->SetDragEnabled(false);
    }
    
    return GanttCanvasContainer;
}

UVerticalBox* UMASkillListModal::CreateJsonPreviewArea()
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
            UpdateButton->OnClicked.AddDynamic(this, &UMASkillListModal::OnUpdateButtonClicked);
            
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
            
            // 设置样式：纯黑色，字号 12
            FEditableTextBoxStyle JsonStyle;
            FSlateColor BlackColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
            JsonStyle.SetForegroundColor(BlackColor);
            JsonStyle.SetFocusedForegroundColor(BlackColor);
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

void UMASkillListModal::SyncJsonPreview()
{
    if (!JsonPreviewBox || !AllocationModel)
    {
        return;
    }
    
    FMASkillAllocationData Data = AllocationModel->GetWorkingData();
    FString JsonString = Data.ToJson();
    
    JsonPreviewBox->SetText(FText::FromString(JsonString));
}

void UMASkillListModal::UpdateModelFromJson()
{
    if (!JsonPreviewBox || !AllocationModel)
    {
        return;
    }
    
    FString JsonString = JsonPreviewBox->GetText().ToString();
    FMASkillAllocationData Data;
    FString ErrorMessage;
    
    if (FMASkillAllocationData::FromJson(JsonString, Data, ErrorMessage))
    {
        AllocationModel->LoadFromData(Data);
        
        // 更新甘特图画布
        if (GanttCanvas)
        {
            GanttCanvas->RefreshFromModel();
        }
        
        UE_LOG(LogMASkillListModal, Log, TEXT("Updated model from JSON"));
    }
    else
    {
        UE_LOG(LogMASkillListModal, Warning, TEXT("Failed to parse JSON: %s"), *ErrorMessage);
    }
}

void UMASkillListModal::HandleConfirm()
{
    UE_LOG(LogMASkillListModal, Log, TEXT("Skill list confirmed"));
    
    // 获取当前数据
    FMASkillAllocationData Data = GetSkillAllocationData();
    
    // 广播确认事件
    OnSkillListConfirmed.Broadcast(Data);
}

void UMASkillListModal::HandleReject()
{
    UE_LOG(LogMASkillListModal, Log, TEXT("Skill list rejected"));
    
    // 恢复原始数据
    if (AllocationModel)
    {
        AllocationModel->LoadFromData(OriginalData);
    }
    
    if (GanttCanvas && AllocationModel)
    {
        GanttCanvas->RefreshFromModel();
    }
    
    SyncJsonPreview();
    
    // 广播拒绝事件
    OnSkillListRejected.Broadcast();
}

//=============================================================================
// 事件处理
//=============================================================================

void UMASkillListModal::OnModelDataChanged()
{
    UE_LOG(LogMASkillListModal, Log, TEXT("Model data changed"));
    
    // 同步 JSON 预览
    SyncJsonPreview();
    
    // 更新甘特图画布
    if (GanttCanvas)
    {
        GanttCanvas->RefreshFromModel();
    }
}

void UMASkillListModal::OnUpdateButtonClicked()
{
    UE_LOG(LogMASkillListModal, Log, TEXT("Update button clicked"));
    
    // 从 JSON 编辑器更新模型
    UpdateModelFromJson();
}

FMASkillAllocationMessage UMASkillListModal::GetSkillAllocationMessage() const
{
    FMASkillAllocationMessage Message;
    
    // 获取当前技能分配数据
    FMASkillAllocationData Data = GetSkillAllocationData();
    
    // 填充消息字段
    Message.Name = Data.Name;
    Message.Description = Data.Description;
    Message.DataJson = Data.ToJson();
    
    UE_LOG(LogMASkillListModal, Log, TEXT("GetSkillAllocationMessage: Name=%s, Description=%s"),
        *Message.Name, *Message.Description);
    
    return Message;
}
