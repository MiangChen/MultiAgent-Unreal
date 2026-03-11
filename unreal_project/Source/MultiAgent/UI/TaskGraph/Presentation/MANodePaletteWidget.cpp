// MANodePaletteWidget.cpp
// 节点工具栏 Widget - 提供可拖拽的节点模板

#include "MANodePaletteWidget.h"
#include "../../Core/MAUITheme.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY(LogMANodePalette);

//=============================================================================
// 构造函数
//=============================================================================

UMANodePaletteWidget::UMANodePaletteWidget(const FObjectInitializer& ObjectInitializer)
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

void UMANodePaletteWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    // 在这里构建 UI，确保 WidgetTree 已经初始化
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMANodePaletteWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 确保 UI 已经构建
    if (!TemplateListBox)
    {
        BuildUI();
    }
    
    UE_LOG(LogMANodePalette, Log, TEXT("MANodePaletteWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMANodePaletteWidget::RebuildWidget()
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

//=============================================================================
// UI 构建
//=============================================================================

void UMANodePaletteWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMANodePalette, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMANodePalette, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 Border 作为背景 - 使用圆角效果
    BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    if (!BackgroundBorder)
    {
        UE_LOG(LogMANodePalette, Error, TEXT("BuildUI: Failed to create BackgroundBorder!"));
        return;
    }
    MARoundedBorderUtils::ApplyRoundedCorners(BackgroundBorder, BackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    BackgroundBorder->SetPadding(FMargin(10.0f));
    WidgetTree->RootWidget = BackgroundBorder;

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    BackgroundBorder->AddChild(MainVBox);

    // Title
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Node Templates")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 14;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    
    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // 创建滚动容器
    TemplateScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TemplateScrollBox"));
    
    UVerticalBoxSlot* ScrollSlot = MainVBox->AddChildToVerticalBox(TemplateScrollBox);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    // 创建模板列表容器
    TemplateListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TemplateListBox"));
    TemplateScrollBox->AddChild(TemplateListBox);

    UE_LOG(LogMANodePalette, Log, TEXT("BuildUI: UI construction completed successfully"));
}

void UMANodePaletteWidget::RefreshTemplateList()
{
    if (!TemplateListBox)
    {
        UE_LOG(LogMANodePalette, Warning, TEXT("RefreshTemplateList: TemplateListBox is null"));
        return;
    }

    if (!WidgetTree)
    {
        UE_LOG(LogMANodePalette, Warning, TEXT("RefreshTemplateList: WidgetTree is null"));
        return;
    }

    // 清空现有按钮 - 先解绑事件
    for (UButton* OldButton : TemplateButtons)
    {
        if (OldButton)
        {
            OldButton->OnClicked.RemoveAll(this);
        }
    }
    TemplateListBox->ClearChildren();
    TemplateButtons.Empty();

    UE_LOG(LogMANodePalette, Log, TEXT("RefreshTemplateList: Creating %d template buttons..."), NodeTemplates.Num());

    // 为每个模板创建按钮
    for (int32 i = 0; i < NodeTemplates.Num(); ++i)
    {
        const FMANodeTemplate& Template = NodeTemplates[i];
        
        // 创建按钮
        FString ButtonName = FString::Printf(TEXT("TemplateButton_%d"), i);
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*ButtonName));
        if (!Button)
        {
            UE_LOG(LogMANodePalette, Error, TEXT("RefreshTemplateList: Failed to create button %d"), i);
            continue;
        }

        // 设置按钮样式
        FButtonStyle ButtonStyle = Button->GetStyle();
        ButtonStyle.Normal.TintColor = FSlateColor(ButtonDefaultColor);
        ButtonStyle.Hovered.TintColor = FSlateColor(ButtonHoverColor);
        ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.15f, 0.15f, 0.2f, 1.0f));
        Button->SetStyle(ButtonStyle);

        // 创建按钮文本
        FString TextName = FString::Printf(TEXT("TemplateText_%d"), i);
        UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*TextName));
        if (ButtonText)
        {
            // 根据模板名称添加前缀
            FString DisplayText;
            if (Template.TemplateName == TEXT("Navigate"))
            {
                DisplayText = TEXT("[Nav] Navigate");
            }
            else if (Template.TemplateName == TEXT("Patrol"))
            {
                DisplayText = TEXT("[Pat] Patrol");
            }
            else if (Template.TemplateName == TEXT("Perceive"))
            {
                DisplayText = TEXT("[Per] Perceive");
            }
            else if (Template.TemplateName == TEXT("Broadcast"))
            {
                DisplayText = TEXT("[Brd] Broadcast");
            }
            else if (Template.TemplateName == TEXT("Custom"))
            {
                DisplayText = TEXT("[Cus] Custom");
            }
            else
            {
                DisplayText = Template.TemplateName;
            }
            
            ButtonText->SetText(FText::FromString(DisplayText));
            ButtonText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
            FSlateFontInfo ButtonFont = ButtonText->GetFont();
            ButtonFont.Size = 11;
            ButtonText->SetFont(ButtonFont);
            ButtonText->SetJustification(ETextJustify::Center);
            
            Button->AddChild(ButtonText);
        }

        // 添加到列表
        UVerticalBoxSlot* ButtonSlot = TemplateListBox->AddChildToVerticalBox(Button);
        if (ButtonSlot)
        {
            ButtonSlot->SetPadding(FMargin(0, 0, 0, ButtonSpacing));
            ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        }
        
        TemplateButtons.Add(Button);
        UE_LOG(LogMANodePalette, Log, TEXT("RefreshTemplateList: Created button %d: %s"), i, *Template.TemplateName);
    }

    // 绑定按钮事件 - 在按钮创建后立即绑定
    if (TemplateButtons.Num() > 0 && TemplateButtons[0])
    {
        TemplateButtons[0]->OnClicked.AddDynamic(this, &UMANodePaletteWidget::OnTemplateButton0Clicked);
    }
    if (TemplateButtons.Num() > 1 && TemplateButtons[1])
    {
        TemplateButtons[1]->OnClicked.AddDynamic(this, &UMANodePaletteWidget::OnTemplateButton1Clicked);
    }
    if (TemplateButtons.Num() > 2 && TemplateButtons[2])
    {
        TemplateButtons[2]->OnClicked.AddDynamic(this, &UMANodePaletteWidget::OnTemplateButton2Clicked);
    }
    if (TemplateButtons.Num() > 3 && TemplateButtons[3])
    {
        TemplateButtons[3]->OnClicked.AddDynamic(this, &UMANodePaletteWidget::OnTemplateButton3Clicked);
    }
    if (TemplateButtons.Num() > 4 && TemplateButtons[4])
    {
        TemplateButtons[4]->OnClicked.AddDynamic(this, &UMANodePaletteWidget::OnTemplateButton4Clicked);
    }

    UE_LOG(LogMANodePalette, Log, TEXT("RefreshTemplateList: Created %d template buttons with events bound"), TemplateButtons.Num());
}

UButton* UMANodePaletteWidget::CreateTemplateButton(const FMANodeTemplate& Template, int32 Index)
{
    // 此方法已废弃，按钮创建逻辑已移至 RefreshTemplateList
    UE_LOG(LogMANodePalette, Warning, TEXT("CreateTemplateButton: This method is deprecated, use RefreshTemplateList instead"));
    return nullptr;
}

//=============================================================================
// 初始化
//=============================================================================

void UMANodePaletteWidget::InitializeTemplates(const TArray<FMANodeTemplate>& InTemplates)
{
    UE_LOG(LogMANodePalette, Log, TEXT("InitializeTemplates: Applying %d templates"), InTemplates.Num());

    if (!TemplateListBox)
    {
        BuildUI();
    }

    NodeTemplates = InTemplates;
    bTemplatesInitialized = true;
    RefreshTemplateList();
}

void UMANodePaletteWidget::AddTemplate(const FMANodeTemplate& Template)
{
    NodeTemplates.Add(Template);
    RefreshTemplateList();
    
    UE_LOG(LogMANodePalette, Log, TEXT("AddTemplate: Added template '%s'"), *Template.TemplateName);
}

void UMANodePaletteWidget::ClearTemplates()
{
    NodeTemplates.Empty();
    bTemplatesInitialized = false;
    RefreshTemplateList();
    
    UE_LOG(LogMANodePalette, Log, TEXT("ClearTemplates: All templates cleared"));
}

//=============================================================================
// 模板访问
//=============================================================================

FMANodeTemplate UMANodePaletteWidget::GetTemplateByName(const FString& TemplateName) const
{
    for (const FMANodeTemplate& Template : NodeTemplates)
    {
        if (Template.TemplateName == TemplateName)
        {
            return Template;
        }
    }
    
    // 返回空模板
    return FMANodeTemplate();
}

//=============================================================================
// 事件处理
//=============================================================================

void UMANodePaletteWidget::OnTemplateButton0Clicked()
{
    HandleTemplateClicked(0);
}

void UMANodePaletteWidget::OnTemplateButton1Clicked()
{
    HandleTemplateClicked(1);
}

void UMANodePaletteWidget::OnTemplateButton2Clicked()
{
    HandleTemplateClicked(2);
}

void UMANodePaletteWidget::OnTemplateButton3Clicked()
{
    HandleTemplateClicked(3);
}

void UMANodePaletteWidget::OnTemplateButton4Clicked()
{
    HandleTemplateClicked(4);
}

void UMANodePaletteWidget::HandleTemplateClicked(int32 TemplateIndex)
{
    if (TemplateIndex >= 0 && TemplateIndex < NodeTemplates.Num())
    {
        const FMANodeTemplate& Template = NodeTemplates[TemplateIndex];
        UE_LOG(LogMANodePalette, Log, TEXT("Template clicked: %s"), *Template.TemplateName);
        
        // 广播委托
        OnNodeTemplateSelected.Broadcast(Template);
    }
}

//=============================================================================
// 主题应用
//=============================================================================

void UMANodePaletteWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        return;
    }
    
    // 从 Theme 更新颜色配置
    BackgroundColor = Theme->BackgroundColor;
    TitleColor = Theme->TextColor;
    ButtonDefaultColor = Theme->SecondaryColor;
    ButtonHoverColor = Theme->HighlightColor;
    ButtonTextColor = Theme->TextColor;
    
    // 更新背景颜色 - 使用圆角效果
    if (BackgroundBorder)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(BackgroundBorder, BackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 更新标题颜色
    if (TitleText)
    {
        TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
    }
    
    // 更新按钮样式
    for (UButton* Button : TemplateButtons)
    {
        if (Button)
        {
            FButtonStyle ButtonStyle = Button->GetStyle();
            ButtonStyle.Normal.TintColor = FSlateColor(ButtonDefaultColor);
            ButtonStyle.Hovered.TintColor = FSlateColor(ButtonHoverColor);
            ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.15f, 0.15f, 0.2f, 1.0f));
            Button->SetStyle(ButtonStyle);
            
            // 更新按钮文本颜色
            if (Button->GetChildrenCount() > 0)
            {
                UTextBlock* ButtonText = Cast<UTextBlock>(Button->GetChildAt(0));
                if (ButtonText)
                {
                    ButtonText->SetColorAndOpacity(FSlateColor(ButtonTextColor));
                }
            }
        }
    }
    
    UE_LOG(LogMANodePalette, Verbose, TEXT("ApplyTheme: Theme applied to node palette"));
}
