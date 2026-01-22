// MAEditWidget.cpp
// Edit Mode 编辑面板 Widget - 纯 C++ 实现

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
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAFrostedGlassUtils.h"
#include "../Core/MAUITheme.h"
#include "../../Core/Manager/MAEditModeManager.h"
#include "../../Core/Manager/MASceneGraphManager.h"
#include "../../Environment/MAPointOfInterest.h"
#include "../../Environment/MAGoalActor.h"
#include "../../Environment/MAZoneActor.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAEditWidget, Log, All);

// Hint text constants - use anonymous namespace to avoid Unity Build conflicts
namespace
{
    const FString EditDefaultHintText = TEXT("Select an Actor or POI to operate");
    const FString ActorSelectedHintText = TEXT("Edit JSON and click Confirm to save");
    const FString POISingleHintText = TEXT("Can create Goal or add preset Actor");
    const FString POIMultiHintText = TEXT("Select 3+ POIs to create a zone");
}

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


//=========================================================================
// ApplyTheme - 应用主题样式
//=========================================================================

void UMAEditWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        UE_LOG(LogMAEditWidget, Warning, TEXT("ApplyTheme: Theme is null, using default colors"));
        return;
    }

    // 更新标题颜色
    if (TitleText)
    {
        TitleText->SetColorAndOpacity(FSlateColor(Theme->PrimaryColor));
    }

    // 更新提示文字颜色
    if (HintText)
    {
        HintText->SetColorAndOpacity(FSlateColor(Theme->HintTextColor));
    }

    // 更新错误文字颜色
    if (ErrorText)
    {
        ErrorText->SetColorAndOpacity(FSlateColor(Theme->DangerColor));
    }

    UE_LOG(LogMAEditWidget, Log, TEXT("ApplyTheme: Theme applied successfully"));
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

    // 获取主题
    if (!Theme)
    {
        Theme = NewObject<UMAUITheme>();
    }

    // === 使用 MAFrostedGlassUtils 创建毛玻璃效果侧边栏 ===
    FMAFrostedGlassResult FrostedGlass = MAFrostedGlassUtils::CreateFixedSizePanelFromTheme(
        WidgetTree,
        RootCanvas,
        Theme,
        FVector2D(20, -20),      // 位置：左边距 20，底部距离 20
        FVector2D(380, 600),     // 尺寸：宽度 380，高度 600
        FVector2D(0.0f, 1.0f),   // 对齐：底部对齐
        FAnchors(0.0f, 1.0f, 0.0f, 1.0f),  // 锚点：左下角
        TEXT("EditPanel")
    );
    
    if (!FrostedGlass.IsValid())
    {
        UE_LOG(LogMAEditWidget, Error, TEXT("BuildUI: Failed to create frosted glass panel!"));
        return;
    }

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    FrostedGlass.ContentContainer->AddChild(MainVBox);

    // =========================================================================
    // 标题区域
    // =========================================================================
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Edit Mode - Scene Editor")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 16;
    TitleText->SetFont(TitleFont);
    // 使用 Theme 颜色，fallback 到默认蓝色
    FLinearColor TitleColor = Theme ? Theme->PrimaryColor : FLinearColor(0.3f, 0.6f, 1.0f);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));

    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 8));

    // 提示文本
    HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
    HintText->SetText(FText::FromString(EditDefaultHintText));
    FSlateFontInfo HintFont = HintText->GetFont();
    HintFont.Size = 11;
    HintText->SetFont(HintFont);
    // 使用 Theme 颜色，fallback 到默认灰色
    FLinearColor HintColor = Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
    HintText->SetColorAndOpacity(FSlateColor(HintColor));
    HintText->SetAutoWrapText(true);  // 启用自动换行
    
    UVerticalBoxSlot* HintSlot = MainVBox->AddChildToVerticalBox(HintText);
    HintSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 错误提示文本
    ErrorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ErrorText"));
    ErrorText->SetText(FText::GetEmpty());
    FSlateFontInfo ErrorFont = ErrorText->GetFont();
    ErrorFont.Size = 11;
    ErrorText->SetFont(ErrorFont);
    // 使用 Theme 颜色，fallback 到默认红色
    FLinearColor ErrorColor = Theme ? Theme->DangerColor : FLinearColor(1.0f, 0.3f, 0.3f);
    ErrorText->SetColorAndOpacity(FSlateColor(ErrorColor));
    ErrorText->SetAutoWrapText(true);  // 启用自动换行
    ErrorText->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ErrorSlot = MainVBox->AddChildToVerticalBox(ErrorText);
    ErrorSlot->SetPadding(FMargin(0, 0, 0, 8));

    // =========================================================================
    // Actor 操作区域
    // =========================================================================
    ActorOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActorOperationBox"));
    ActorOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* ActorOpSlot = MainVBox->AddChildToVerticalBox(ActorOperationBox);
    ActorOpSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Node 切换按钮容器
    NodeSwitchContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NodeSwitchContainer"));
    NodeSwitchContainer->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* NodeSwitchSlot = ActorOperationBox->AddChildToVerticalBox(NodeSwitchContainer);
    NodeSwitchSlot->SetPadding(FMargin(0, 0, 0, 8));

    // JSON editor label
    UTextBlock* JsonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JsonLabel"));
    JsonLabel->SetText(FText::FromString(TEXT("JSON Editor:")));
    FSlateFontInfo JsonLabelFont = JsonLabel->GetFont();
    JsonLabelFont.Size = 12;
    JsonLabel->SetFont(JsonLabelFont);
    // 使用 Theme 颜色，fallback 到默认浅灰色
    FLinearColor JsonLabelColor = Theme ? Theme->LabelTextColor : FLinearColor(0.8f, 0.8f, 0.8f);
    JsonLabel->SetColorAndOpacity(FSlateColor(JsonLabelColor));
    
    UVerticalBoxSlot* JsonLabelSlot = ActorOperationBox->AddChildToVerticalBox(JsonLabel);
    JsonLabelSlot->SetPadding(FMargin(0, 0, 0, 4));

    // JSON edit text box - wrapped in rounded border
    JsonEditBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("JsonEditBox"));
    JsonEditBox->SetHintText(FText::FromString(TEXT("Select an Actor to display JSON")));

    FEditableTextBoxStyle JsonTextBoxStyle;
    // 使用 Theme 颜色，fallback 到默认黑色
    FLinearColor JsonInputTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor BlackColor = FSlateColor(JsonInputTextColor);
    JsonTextBoxStyle.SetForegroundColor(BlackColor);
    JsonTextBoxStyle.SetFocusedForegroundColor(BlackColor);
    FSlateFontInfo JsonFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
    JsonTextBoxStyle.SetFont(JsonFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush TransparentBrush1;
    TransparentBrush1.TintColor = FSlateColor(FLinearColor::Transparent);
    JsonTextBoxStyle.SetBackgroundImageNormal(TransparentBrush1);
    JsonTextBoxStyle.SetBackgroundImageHovered(TransparentBrush1);
    JsonTextBoxStyle.SetBackgroundImageFocused(TransparentBrush1);
    JsonTextBoxStyle.SetBackgroundImageReadOnly(TransparentBrush1);
    
    JsonEditBox->WidgetStyle = JsonTextBoxStyle;
    
    // 创建圆角 Border 包装文本框
    UBorder* JsonEditBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JsonEditBorder"));
    JsonEditBorder->SetPadding(FMargin(8.0f, 4.0f));
    
    // 应用圆角效果 - 可编辑文本框使用白色背景
    // 使用 Theme 颜色，fallback 到默认白色
    FLinearColor JsonEditBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(JsonEditBorder, JsonEditBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    JsonEditBorder->AddChild(JsonEditBox);
    
    USizeBox* JsonEditSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("JsonEditSizeBox"));
    JsonEditSizeBox->SetMinDesiredHeight(150.0f);
    JsonEditSizeBox->SetMaxDesiredHeight(200.0f);
    JsonEditSizeBox->AddChild(JsonEditBorder);
    
    UVerticalBoxSlot* JsonEditSlot = ActorOperationBox->AddChildToVerticalBox(JsonEditSizeBox);
    JsonEditSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Actor 操作按钮区域
    UHorizontalBox* ActorButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActorButtonBox"));
    
    // Confirm button
    ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
    UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmText"));
    ConfirmText->SetText(FText::FromString(TEXT("  Confirm  ")));
    // 使用 Theme 颜色，fallback 到默认黑色
    FLinearColor ButtonTextColor = Theme ? Theme->InputTextColor : FLinearColor::Black;
    ConfirmText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
    FSlateFontInfo ConfirmFont = ConfirmText->GetFont();
    ConfirmFont.Size = 14;
    ConfirmText->SetFont(ConfirmFont);
    ConfirmButton->AddChild(ConfirmText);
    
    UHorizontalBoxSlot* ConfirmSlot = ActorButtonBox->AddChildToHorizontalBox(ConfirmButton);
    ConfirmSlot->SetPadding(FMargin(0, 0, 10, 0));

    // Delete button
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

    // Set as Goal button
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

    // Unset Goal button
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

    // =========================================================================
    // POI 操作区域
    // =========================================================================
    POIOperationBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("POIOperationBox"));
    POIOperationBox->SetVisibility(ESlateVisibility::Collapsed);
    
    UVerticalBoxSlot* POIOpSlot = MainVBox->AddChildToVerticalBox(POIOperationBox);
    POIOpSlot->SetPadding(FMargin(0, 0, 0, 10));

    // Description input label
    UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescLabel"));
    DescLabel->SetText(FText::FromString(TEXT("Description:")));
    FSlateFontInfo DescLabelFont = DescLabel->GetFont();
    DescLabelFont.Size = 12;
    DescLabel->SetFont(DescLabelFont);
    // 使用 Theme 颜色，fallback 到默认浅灰色
    FLinearColor DescLabelColor = Theme ? Theme->LabelTextColor : FLinearColor(0.8f, 0.8f, 0.8f);
    DescLabel->SetColorAndOpacity(FSlateColor(DescLabelColor));
    
    UVerticalBoxSlot* DescLabelSlot = POIOperationBox->AddChildToVerticalBox(DescLabel);
    DescLabelSlot->SetPadding(FMargin(0, 0, 0, 4));

    // Description input text box - wrapped in rounded border
    DescriptionBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("DescriptionBox"));
    DescriptionBox->SetHintText(FText::FromString(TEXT("Enter description for Goal/Zone...")));

    FEditableTextBoxStyle DescTextBoxStyle;
    // 使用 Theme 颜色，fallback 到默认黑色
    FLinearColor DescInputTextColor = Theme ? Theme->InputTextColor : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    FSlateColor BlackColor2 = FSlateColor(DescInputTextColor);
    DescTextBoxStyle.SetForegroundColor(BlackColor2);
    DescTextBoxStyle.SetFocusedForegroundColor(BlackColor2);
    FSlateFontInfo DescFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
    DescTextBoxStyle.SetFont(DescFont);
    
    // 设置透明背景，让外层圆角 Border 的背景显示出来
    FSlateBrush TransparentBrush2;
    TransparentBrush2.TintColor = FSlateColor(FLinearColor::Transparent);
    DescTextBoxStyle.SetBackgroundImageNormal(TransparentBrush2);
    DescTextBoxStyle.SetBackgroundImageHovered(TransparentBrush2);
    DescTextBoxStyle.SetBackgroundImageFocused(TransparentBrush2);
    DescTextBoxStyle.SetBackgroundImageReadOnly(TransparentBrush2);
    
    DescriptionBox->WidgetStyle = DescTextBoxStyle;
    
    // 创建圆角 Border 包装文本框
    UBorder* DescriptionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DescriptionBorder"));
    DescriptionBorder->SetPadding(FMargin(8.0f, 4.0f));
    
    // 应用圆角效果 - 可编辑文本框使用白色背景
    // 使用 Theme 颜色，fallback 到默认白色
    FLinearColor DescBgColor = Theme ? Theme->InputBackgroundColor : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    MARoundedBorderUtils::ApplyRoundedCorners(DescriptionBorder, DescBgColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    DescriptionBorder->AddChild(DescriptionBox);
    
    USizeBox* DescSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DescSizeBox"));
    DescSizeBox->SetMinDesiredHeight(80.0f);
    DescSizeBox->SetMaxDesiredHeight(100.0f);
    DescSizeBox->AddChild(DescriptionBorder);
    
    UVerticalBoxSlot* DescSlot = POIOperationBox->AddChildToVerticalBox(DescSizeBox);
    DescSlot->SetPadding(FMargin(0, 0, 0, 10));

    // POI 操作按钮区域
    UHorizontalBox* POIButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("POIButtonBox"));
    
    // 使用 Theme 颜色，fallback 到默认黑色
    FLinearColor POIButtonTextColor = Theme ? Theme->InputTextColor : FLinearColor::Black;
    
    // Create Goal button
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

    // Create Zone button
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

    // Delete POI button
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

    // 预设 Actor 区域
    UHorizontalBox* PresetActorBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PresetActorBox"));
    
    // Preset Actor dropdown
    PresetActorComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PresetActorComboBox"));
    PresetActorComboBox->AddOption(TEXT("(No preset Actors)"));
    PresetActorComboBox->SetSelectedIndex(0);
    
    UHorizontalBoxSlot* ComboSlot = PresetActorBox->AddChildToHorizontalBox(PresetActorComboBox);
    ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ComboSlot->SetPadding(FMargin(0, 0, 10, 0));

    // Add Actor button
    AddActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddActorButton"));
    UTextBlock* AddActorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddActorText"));
    AddActorText->SetText(FText::FromString(TEXT(" Add ")));
    // 使用 Theme 颜色，fallback 到默认黑色
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

//=========================================================================
// SetSelectedActor - 设置选中的 Actor
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
        // MAEditModeManager 是 WorldSubsystem，从 World 获取
        UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
        
        FString SearchId;
        bool bIsGoalOrZone = false;
        
        // 检查是否为 Goal Actor 或 Zone Actor，使用 GetNodeId() 获取 Node ID
        if (AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
        {
            SearchId = GoalActor->GetNodeId();
            bIsGoalOrZone = true;
            UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: GoalActor detected, NodeId = %s"), *SearchId);
        }
        else if (AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
        {
            SearchId = ZoneActor->GetNodeId();
            bIsGoalOrZone = true;
            UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: ZoneActor detected, NodeId = %s"), *SearchId);
        }
        else
        {
            // 普通 Actor，使用 GUID
            SearchId = Actor->GetActorGuid().ToString();
            UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: Regular Actor, GUID = %s"), *SearchId);
        }
        
        // 辅助函数：从 JSON 字符串解析 Node
        auto ParseNodeFromJson = [](const FString& NodeJsonStr, const FString& NodeId) -> FMASceneGraphNode
        {
            FMASceneGraphNode Node;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodeJsonStr);
            TSharedPtr<FJsonObject> NodeJson;
            if (FJsonSerializer::Deserialize(Reader, NodeJson) && NodeJson.IsValid())
            {
                Node.Id = NodeId;

                // 获取 properties
                if (NodeJson->HasField(TEXT("properties")))
                {
                    TSharedPtr<FJsonObject> Props = NodeJson->GetObjectField(TEXT("properties"));
                    if (Props.IsValid())
                    {
                        Node.Type = Props->GetStringField(TEXT("type"));
                        Node.Label = Props->GetStringField(TEXT("label"));
                    }
                }

                // 获取 shape
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

                // 获取 guid (point 类型使用小写 guid 字符串)
                if (NodeJson->HasField(TEXT("guid")))
                {
                    Node.Guid = NodeJson->GetStringField(TEXT("guid"));
                }
                // 获取 Guid 数组 (polygon/linestring/prism 类型使用大写 Guid 数组)
                else if (NodeJson->HasField(TEXT("Guid")))
                {
                    const TArray<TSharedPtr<FJsonValue>>* GuidArray;
                    if (NodeJson->TryGetArrayField(TEXT("Guid"), GuidArray) && GuidArray->Num() > 0)
                    {
                        // 将第一个 GUID 存入 Node.Guid 用于兼容
                        Node.Guid = (*GuidArray)[0]->AsString();
                        // 将所有 GUID 存入 GuidArray
                        for (const TSharedPtr<FJsonValue>& GuidValue : *GuidArray)
                        {
                            if (GuidValue.IsValid())
                            {
                                Node.GuidArray.Add(GuidValue->AsString());
                            }
                        }
                    }
                }

                // 存储 RawJson
                Node.RawJson = NodeJsonStr;
            }
            return Node;
        };

        // 对于 Goal/Zone Actor，从 SceneGraphManager 中查找
        if (bIsGoalOrZone && !SearchId.IsEmpty())
        {
            // 获取 SceneGraphManager
            UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
            UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
            
            if (SceneGraphManager)
            {
                FMASceneGraphNode Node = SceneGraphManager->GetNodeById(SearchId);
                if (Node.IsValid())
                {
                    ActorNodes.Add(Node);
                    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: Found Goal/Zone node by ID: %s"), *Node.Id);
                }
                else
                {
                    UE_LOG(LogMAEditWidget, Warning, TEXT("SetSelectedActor: Goal/Zone node not found: %s"), *SearchId);
                }
            }
        }
        // 对于普通 Actor，从 SceneGraphManager 中通过 GUID 查找
        else if (!SearchId.IsEmpty())
        {
            // 获取 SceneGraphManager
            UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
            UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: GameInstance=%s"), GI ? TEXT("Valid") : TEXT("NULL"));
            
            UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
            UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: SceneGraphManager=%s"), SceneGraphManager ? TEXT("Valid") : TEXT("NULL"));
            
            if (SceneGraphManager)
            {
                // 通过 GUID 从场景图查找 Node
                FString ActorGuid = Actor->GetActorGuid().ToString();
                UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: Calling FindNodesByGuid with GUID=%s"), *ActorGuid);
                TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);

                // 添加所有找到的节点
                for (const FMASceneGraphNode& Node : MatchingNodes)
                {
                    ActorNodes.Add(Node);
                    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: Found node %s from scene graph"), *Node.Id);
                }

                if (MatchingNodes.Num() == 0)
                {
                    UE_LOG(LogMAEditWidget, Warning, TEXT("SetSelectedActor: No nodes found in scene graph for GUID %s"), *ActorGuid);
                }
            }
            else
            {
                UE_LOG(LogMAEditWidget, Error, TEXT("SetSelectedActor: SceneGraphManager is NULL, cannot search for nodes"));
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
        // 使用 Theme 颜色，fallback 到默认绿色
        FLinearColor SelectedHintColor = Theme ? Theme->SuccessColor : FLinearColor(0.3f, 0.8f, 0.3f);
        HintText->SetColorAndOpacity(FSlateColor(SelectedHintColor));
    }
    
    UE_LOG(LogMAEditWidget, Log, TEXT("SetSelectedActor: %s, found %d nodes"), *Actor->GetName(), ActorNodes.Num());
}

//=========================================================================
// SetSelectedPOIs - 设置选中的 POI 列表
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
            HintText->SetText(FText::FromString(FString::Printf(TEXT("%d POIs selected, can create zone"), CurrentPOIs.Num())));
        }
        else
        {
            HintText->SetText(FText::FromString(POIMultiHintText));
        }
        // 使用 Theme 颜色，fallback 到默认蓝色
        FLinearColor POIHintColor = Theme ? Theme->PrimaryColor : FLinearColor(0.3f, 0.6f, 1.0f);
        HintText->SetColorAndOpacity(FSlateColor(POIHintColor));
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
        HintText->SetText(FText::FromString(EditDefaultHintText));
        // 使用 Theme 颜色，fallback 到默认灰色
        FLinearColor DefaultHintColor = Theme ? Theme->HintTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
        HintText->SetColorAndOpacity(FSlateColor(DefaultHintColor));
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
    
    // 删除按钮 - 对 point 类型 Node、GoalActor、ZoneActor 显示
    if (DeleteButton && bHasActor)
    {
        bool bCanDelete = false;

        // 检查是否为 GoalActor 或 ZoneActor (这些始终可删除)
        if (CurrentActor->IsA<AMAGoalActor>() || CurrentActor->IsA<AMAZoneActor>())
        {
            bCanDelete = true;
        }
        // 普通 Actor：检查是否为 point 类型
        else if (ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
        {
            bCanDelete = IsPointTypeNode(ActorNodes[CurrentNodeIndex]);
        }
        DeleteButton->SetVisibility(bCanDelete ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    else if (DeleteButton)
    {
        DeleteButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    // 设为 Goal / 取消 Goal 按钮
    // 仅对普通 Actor（非 GoalActor/ZoneActor）显示这些按钮
    // 所有类型的节点（point, prism, linestring 等）都可以设为 Goal
    bool bIsGoalOrZoneActor = false;
    if (CurrentActor)
    {
        bIsGoalOrZoneActor = CurrentActor->IsA<AMAGoalActor>() || CurrentActor->IsA<AMAZoneActor>();
    }

    // 检查当前 Node 是否已经是 Goal (通过 is_goal 字段)
    bool bIsCurrentNodeGoal = false;
    bool bCanSetAsGoal = false;  // 是否可以设为 Goal（所有类型都可以）
    if (bHasActor && !bIsGoalOrZoneActor && ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
    {
        // 检查 properties 中是否有 is_goal: true
        const FMASceneGraphNode& Node = ActorNodes[CurrentNodeIndex];
        // 通过 RawJson 检查 is_goal 字段
        bIsCurrentNodeGoal = Node.RawJson.Contains(TEXT("\"is_goal\"")) &&
                             (Node.RawJson.Contains(TEXT("\"is_goal\": true")) || 
                              Node.RawJson.Contains(TEXT("\"is_goal\":true")));

        // 所有类型的节点都可以设为 Goal（移除 point 类型限制）
        bCanSetAsGoal = true;
    }
    
    if (SetAsGoalButton)
    {
        // 如果是普通 Actor、是 point 类型、且还不是 Goal，显示 "设为 Goal" 按钮
        SetAsGoalButton->SetVisibility((bHasActor && !bIsGoalOrZoneActor && bCanSetAsGoal && !bIsCurrentNodeGoal)
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    if (UnsetAsGoalButton)
    {
        // 如果是普通 Actor、是 point 类型、且已经是 Goal，显示 "取消 Goal" 按钮
        UnsetAsGoalButton->SetVisibility((bHasActor && !bIsGoalOrZoneActor && bCanSetAsGoal && bIsCurrentNodeGoal)
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

//=========================================================================
// UpdateJsonEditBox - 更新 JSON 编辑框内容
//=========================================================================

void UMAEditWidget::UpdateJsonEditBox()
{
    if (!JsonEditBox)
    {
        return;
    }
    
    if (!CurrentActor || ActorNodes.Num() == 0)
    {
        JsonEditBox->SetText(FText::FromString(TEXT("No matching scene graph node found")));
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
    // point 类型: 可编辑 properties 和 shape.center
    // polygon/linestring 类型: 仅可编辑 properties (在提交时验证)
    JsonEditBox->SetIsReadOnly(false);
    
    bool bIsPoint = IsPointTypeNode(Node);
    if (!bIsPoint)
    {
        // Non-point type, show hint
        if (HintText)
        {
            HintText->SetText(FText::FromString(TEXT("polygon/linestring type: only properties field modifications will be saved")));
            // 使用 Theme 颜色，fallback 到默认橙色
            FLinearColor WarningHintColor = Theme ? Theme->WarningColor : FLinearColor(1.0f, 0.6f, 0.0f);
            HintText->SetColorAndOpacity(FSlateColor(WarningHintColor));
        }
    }
    else
    {
        if (HintText)
        {
            HintText->SetText(FText::FromString(ActorSelectedHintText));
            // 使用 Theme 颜色，fallback 到默认绿色
            FLinearColor SuccessHintColor = Theme ? Theme->SuccessColor : FLinearColor(0.3f, 0.8f, 0.3f);
            HintText->SetColorAndOpacity(FSlateColor(SuccessHintColor));
        }
    }
}

//=========================================================================
// UpdateNodeSwitchButtons - 更新 Node 切换按钮
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
            // 使用 Theme 颜色，fallback 到默认蓝色
            FLinearColor SelectedNodeColor = Theme ? Theme->ModeEditColor : FLinearColor(0.0f, 0.5f, 1.0f);
            ButtonText->SetColorAndOpacity(FSlateColor(SelectedNodeColor));
        }
        else
        {
            // 使用 Theme 颜色，fallback 到默认黑色
            FLinearColor UnselectedNodeColor = Theme ? Theme->InputTextColor : FLinearColor::Black;
            ButtonText->SetColorAndOpacity(FSlateColor(UnselectedNodeColor));
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
//=========================================================================

bool UMAEditWidget::ValidateJson(const FString& Json, FString& OutError)
{
    if (Json.IsEmpty())
    {
        OutError = TEXT("JSON content cannot be empty");
        return false;
    }
    
    // Try to parse JSON
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    TSharedPtr<FJsonObject> JsonObject;
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format, please check syntax");
        return false;
    }
    
    // Check required fields
    if (!JsonObject->HasField(TEXT("id")))
    {
        OutError = TEXT("Missing required field: id");
        return false;
    }
    
    OutError.Empty();
    return true;
}

//=========================================================================
// IsPointTypeNode - 检查是否为 point 类型 Node
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
//=========================================================================

void UMAEditWidget::OnConfirmButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnConfirmButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("No Actor selected"));
        return;
    }
    
    // 保存 Actor 名称用于日志（因为 Broadcast 后 CurrentActor 可能被清空）
    FString ActorName = CurrentActor->GetName();

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
    // 注意: Broadcast 会触发 MAHUD::OnEditConfirmed，其中会调用 ClearSelection，
    // 这会将 CurrentActor 设置为 nullptr
    OnEditConfirmed.Broadcast(CurrentActor, JsonContent);

    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnConfirmButtonClicked: Edit confirmed for Actor %s"), *ActorName);
}

//=========================================================================
// OnDeleteButtonClicked - 删除按钮点击处理
//=========================================================================

void UMAEditWidget::OnDeleteButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeleteButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("No Actor selected"));
        return;
    }
    
    // GoalActor and ZoneActor can always be deleted, skip point type check
    bool bIsGoalOrZone = CurrentActor->IsA<AMAGoalActor>() || CurrentActor->IsA<AMAZoneActor>();

    // Regular Actor: check if it's point type
    if (!bIsGoalOrZone && ActorNodes.Num() > 0 && CurrentNodeIndex < ActorNodes.Num())
    {
        if (!IsPointTypeNode(ActorNodes[CurrentNodeIndex]))
        {
            ShowError(TEXT("Only point type nodes can be deleted"));
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
//=========================================================================

void UMAEditWidget::OnCreateGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateGoalButtonClicked"));
    
    if (CurrentPOIs.Num() == 0)
    {
        ShowError(TEXT("No POI selected"));
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
//=========================================================================

void UMAEditWidget::OnCreateZoneButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnCreateZoneButtonClicked"));
    
    if (CurrentPOIs.Num() < 3)
    {
        ShowError(TEXT("Creating a zone requires at least 3 POIs"));
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
//=========================================================================

void UMAEditWidget::OnAddActorButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnAddActorButtonClicked"));
    
    if (CurrentPOIs.Num() != 1)
    {
        ShowError(TEXT("Please select a single POI"));
        return;
    }
    
    // Get selected preset Actor type
    FString ActorType;
    if (PresetActorComboBox)
    {
        ActorType = PresetActorComboBox->GetSelectedOption();
    }
    
    if (ActorType.IsEmpty() || ActorType == TEXT("(No preset Actors)"))
    {
        ShowError(TEXT("Please select a preset Actor type"));
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
//=========================================================================

void UMAEditWidget::OnSetAsGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnSetAsGoalButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("No Actor selected"));
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
//=========================================================================

void UMAEditWidget::OnUnsetAsGoalButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnUnsetAsGoalButtonClicked"));
    
    if (!CurrentActor)
    {
        ShowError(TEXT("No Actor selected"));
        return;
    }
    
    ClearError();

    // 广播取消 Goal 委托
    OnUnsetAsGoal.Broadcast(CurrentActor);

    // 刷新 UI 状态
    UpdateUIState();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnUnsetAsGoalButtonClicked: Unset as Goal requested for Actor %s"), *CurrentActor->GetName());
}


//=========================================================================
// OnDeletePOIButtonClicked - 删除 POI 按钮点击处理
//=========================================================================

void UMAEditWidget::OnDeletePOIButtonClicked()
{
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeletePOIButtonClicked"));
    
    if (CurrentPOIs.Num() == 0)
    {
        ShowError(TEXT("No POI selected"));
        return;
    }
    
    ClearError();

    // 广播删除 POI 委托
    OnDeletePOIs.Broadcast(CurrentPOIs);

    // 清除选择
    ClearSelection();

    // 刷新场景图预览
    RefreshSceneGraphPreview();
    
    UE_LOG(LogMAEditWidget, Log, TEXT("OnDeletePOIButtonClicked: Delete requested for %d POIs"), CurrentPOIs.Num());
}
