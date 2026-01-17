// MASceneListWidget.cpp
// 场景列表面板 Widget 实现

#include "MASceneListWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "../../Core/Manager/MAEditModeManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneListWidget, Log, All);

UMASceneListWidget::UMASceneListWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

void UMASceneListWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 初始刷新列表
    RefreshLists();

    UE_LOG(LogMASceneListWidget, Log, TEXT("MASceneListWidget NativeConstruct completed"));
}

TSharedRef<SWidget> UMASceneListWidget::RebuildWidget()
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

void UMASceneListWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMASceneListWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMASceneListWidget, Log, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMASceneListWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建背景 Border - 位于屏幕左侧
    UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.08f, 0.9f));  // 深蓝色背景
    BackgroundBorder->SetPadding(FMargin(10.0f));

    UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
    BorderSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));  // 左侧锚点，垂直拉伸
    BorderSlot->SetAlignment(FVector2D(0.0f, 0.0f));
    BorderSlot->SetPosition(FVector2D(10, 60));  // 左边距 10，顶部距离 60
    BorderSlot->SetSize(FVector2D(250, 0));  // 宽度 250
    BorderSlot->SetAutoSize(false);
    BorderSlot->SetOffsets(FMargin(10, 60, 250, 60));

    // 创建主滚动框
    UScrollBox* MainScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MainScrollBox"));
    BackgroundBorder->AddChild(MainScrollBox);

    // 创建垂直布局容器
    UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
    MainScrollBox->AddChild(MainVBox);

    // =========================================================================
    // 标题
    // =========================================================================
    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("Scene Elements List")));
    FSlateFontInfo TitleFont = TitleText->GetFont();
    TitleFont.Size = 14;
    TitleText->SetFont(TitleFont);
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.6f, 1.0f)));

    UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 10));

    // =========================================================================
    // Goal 列表区域
    // =========================================================================

    // Goal 标题行
    UHorizontalBox* GoalHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GoalHeaderBox"));
    
    GoalTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalTitleText"));
    GoalTitleText->SetText(FText::FromString(TEXT("🎯 Goals")));
    FSlateFontInfo GoalTitleFont = GoalTitleText->GetFont();
    GoalTitleFont.Size = 12;
    GoalTitleText->SetFont(GoalTitleFont);
    GoalTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.4f, 0.4f)));  // 红色

    UHorizontalBoxSlot* GoalTitleSlot = GoalHeaderBox->AddChildToHorizontalBox(GoalTitleText);
    GoalTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GoalCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalCountText"));
    GoalCountText->SetText(FText::FromString(TEXT("(0)")));
    FSlateFontInfo GoalCountFont = GoalCountText->GetFont();
    GoalCountFont.Size = 11;
    GoalCountText->SetFont(GoalCountFont);
    GoalCountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    GoalHeaderBox->AddChildToHorizontalBox(GoalCountText);

    UVerticalBoxSlot* GoalHeaderSlot = MainVBox->AddChildToVerticalBox(GoalHeaderBox);
    GoalHeaderSlot->SetPadding(FMargin(0, 5, 0, 3));

    // Goal 列表容器
    GoalListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GoalListBox"));
    
    UVerticalBoxSlot* GoalListSlot = MainVBox->AddChildToVerticalBox(GoalListBox);
    GoalListSlot->SetPadding(FMargin(5, 0, 0, 10));

    // =========================================================================
    // Zone 列表区域
    // =========================================================================

    // Zone 标题行
    UHorizontalBox* ZoneHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ZoneHeaderBox"));
    
    ZoneTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneTitleText"));
    ZoneTitleText->SetText(FText::FromString(TEXT("📍 Zones")));
    FSlateFontInfo ZoneTitleFont = ZoneTitleText->GetFont();
    ZoneTitleFont.Size = 12;
    ZoneTitleText->SetFont(ZoneTitleFont);
    ZoneTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.6f, 1.0f)));  // 蓝色

    UHorizontalBoxSlot* ZoneTitleSlot = ZoneHeaderBox->AddChildToHorizontalBox(ZoneTitleText);
    ZoneTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ZoneCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneCountText"));
    ZoneCountText->SetText(FText::FromString(TEXT("(0)")));
    FSlateFontInfo ZoneCountFont = ZoneCountText->GetFont();
    ZoneCountFont.Size = 11;
    ZoneCountText->SetFont(ZoneCountFont);
    ZoneCountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
    
    ZoneHeaderBox->AddChildToHorizontalBox(ZoneCountText);

    UVerticalBoxSlot* ZoneHeaderSlot = MainVBox->AddChildToVerticalBox(ZoneHeaderBox);
    ZoneHeaderSlot->SetPadding(FMargin(0, 5, 0, 3));

    // Zone 列表容器
    ZoneListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ZoneListBox"));
    
    UVerticalBoxSlot* ZoneListSlot = MainVBox->AddChildToVerticalBox(ZoneListBox);
    ZoneListSlot->SetPadding(FMargin(5, 0, 0, 10));

    UE_LOG(LogMASceneListWidget, Log, TEXT("BuildUI: UI construction completed"));
}

void UMASceneListWidget::SetEditModeManager(UMAEditModeManager* InManager)
{
    EditModeManager = InManager;
    
    // 绑定场景图变化事件
    if (EditModeManager)
    {
        EditModeManager->OnTempSceneGraphChanged.AddDynamic(this, &UMASceneListWidget::RefreshLists);
    }
    
    RefreshLists();
}

void UMASceneListWidget::RefreshLists()
{
    PopulateGoalList();
    PopulateZoneList();

    UE_LOG(LogMASceneListWidget, Log, TEXT("RefreshLists: Lists refreshed"));
}

void UMASceneListWidget::PopulateGoalList()
{
    if (!GoalListBox || !EditModeManager)
    {
        return;
    }

    // 清除现有按钮
    GoalListBox->ClearChildren();
    GoalButtons.Empty();
    GoalIds.Empty();

    // 获取所有 Goal Node ID
    TArray<FString> AllGoalIds = EditModeManager->GetAllGoalNodeIds();

    // 更新计数
    if (GoalCountText)
    {
        GoalCountText->SetText(FText::FromString(FString::Printf(TEXT("(%d)"), AllGoalIds.Num())));
    }

    // 为每个 Goal 创建按钮
    for (const FString& GoalId : AllGoalIds)
    {
        FString Label = EditModeManager->GetNodeLabel(GoalId);
        if (Label.IsEmpty())
        {
            Label = GoalId;
        }

        UButton* GoalButton = CreateListItemButton(Label, GoalId, true);
        if (GoalButton)
        {
            GoalButton->OnClicked.AddDynamic(this, &UMASceneListWidget::OnGoalButtonClicked);
            GoalButtons.Add(GoalButton);
            GoalIds.Add(GoalId);

            UVerticalBoxSlot* ButtonSlot = GoalListBox->AddChildToVerticalBox(GoalButton);
            ButtonSlot->SetPadding(FMargin(0, 2, 0, 2));
        }
    }

    // 如果没有 Goal，显示提示
    if (AllGoalIds.Num() == 0)
    {
        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("  (No Goals)")));
        FSlateFontInfo EmptyFont = EmptyText->GetFont();
        EmptyFont.Size = 10;
        EmptyText->SetFont(EmptyFont);
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
        GoalListBox->AddChildToVerticalBox(EmptyText);
    }

    UE_LOG(LogMASceneListWidget, Log, TEXT("PopulateGoalList: Added %d goals"), AllGoalIds.Num());
}

void UMASceneListWidget::PopulateZoneList()
{
    if (!ZoneListBox || !EditModeManager)
    {
        return;
    }

    // 清除现有按钮
    ZoneListBox->ClearChildren();
    ZoneButtons.Empty();
    ZoneIds.Empty();

    // 获取所有 Zone Node ID
    TArray<FString> AllZoneIds = EditModeManager->GetAllZoneNodeIds();

    // 更新计数
    if (ZoneCountText)
    {
        ZoneCountText->SetText(FText::FromString(FString::Printf(TEXT("(%d)"), AllZoneIds.Num())));
    }

    // 为每个 Zone 创建按钮
    for (const FString& ZoneId : AllZoneIds)
    {
        FString Label = EditModeManager->GetNodeLabel(ZoneId);
        if (Label.IsEmpty())
        {
            Label = ZoneId;
        }

        UButton* ZoneButton = CreateListItemButton(Label, ZoneId, false);
        if (ZoneButton)
        {
            ZoneButton->OnClicked.AddDynamic(this, &UMASceneListWidget::OnZoneButtonClicked);
            ZoneButtons.Add(ZoneButton);
            ZoneIds.Add(ZoneId);

            UVerticalBoxSlot* ButtonSlot = ZoneListBox->AddChildToVerticalBox(ZoneButton);
            ButtonSlot->SetPadding(FMargin(0, 2, 0, 2));
        }
    }

    // 如果没有 Zone，显示提示
    if (AllZoneIds.Num() == 0)
    {
        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("  (No Zones)")));
        FSlateFontInfo EmptyFont = EmptyText->GetFont();
        EmptyFont.Size = 10;
        EmptyText->SetFont(EmptyFont);
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
        ZoneListBox->AddChildToVerticalBox(EmptyText);
    }

    UE_LOG(LogMASceneListWidget, Log, TEXT("PopulateZoneList: Added %d zones"), AllZoneIds.Num());
}

UButton* UMASceneListWidget::CreateListItemButton(const FString& Label, const FString& Id, bool bIsGoal)
{
    UButton* Button = NewObject<UButton>(this);
    
    UTextBlock* ButtonText = NewObject<UTextBlock>(this);
    ButtonText->SetText(FText::FromString(Label));
    FSlateFontInfo ButtonFont = ButtonText->GetFont();
    ButtonFont.Size = 11;
    ButtonText->SetFont(ButtonFont);
    
    // Goal 用红色，Zone 用蓝色
    if (bIsGoal)
    {
        ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f)));
    }
    else
    {
        ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.4f, 0.8f)));
    }
    
    Button->AddChild(ButtonText);
    
    return Button;
}

void UMASceneListWidget::OnGoalButtonClicked()
{
    // 查找点击的按钮
    for (int32 i = 0; i < GoalButtons.Num(); ++i)
    {
        if (GoalButtons[i] && GoalButtons[i]->HasKeyboardFocus())
        {
            if (i < GoalIds.Num())
            {
                UE_LOG(LogMASceneListWidget, Log, TEXT("OnGoalButtonClicked: Goal %s clicked"), *GoalIds[i]);
                OnGoalItemClicked.Broadcast(GoalIds[i]);
            }
            return;
        }
    }

    // 备选方案：检查鼠标位置
    FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
    for (int32 i = 0; i < GoalButtons.Num(); ++i)
    {
        if (GoalButtons[i])
        {
            FGeometry Geometry = GoalButtons[i]->GetCachedGeometry();
            FVector2D LocalPos = Geometry.AbsoluteToLocal(MousePos);
            FVector2D Size = Geometry.GetLocalSize();

            if (LocalPos.X >= 0 && LocalPos.X <= Size.X && LocalPos.Y >= 0 && LocalPos.Y <= Size.Y)
            {
                if (i < GoalIds.Num())
                {
                    UE_LOG(LogMASceneListWidget, Log, TEXT("OnGoalButtonClicked: Goal %s clicked (mouse)"), *GoalIds[i]);
                    OnGoalItemClicked.Broadcast(GoalIds[i]);
                }
                return;
            }
        }
    }
}

void UMASceneListWidget::OnZoneButtonClicked()
{
    // 查找点击的按钮
    for (int32 i = 0; i < ZoneButtons.Num(); ++i)
    {
        if (ZoneButtons[i] && ZoneButtons[i]->HasKeyboardFocus())
        {
            if (i < ZoneIds.Num())
            {
                UE_LOG(LogMASceneListWidget, Log, TEXT("OnZoneButtonClicked: Zone %s clicked"), *ZoneIds[i]);
                OnZoneItemClicked.Broadcast(ZoneIds[i]);
            }
            return;
        }
    }

    // 备选方案：检查鼠标位置
    FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
    for (int32 i = 0; i < ZoneButtons.Num(); ++i)
    {
        if (ZoneButtons[i])
        {
            FGeometry Geometry = ZoneButtons[i]->GetCachedGeometry();
            FVector2D LocalPos = Geometry.AbsoluteToLocal(MousePos);
            FVector2D Size = Geometry.GetLocalSize();

            if (LocalPos.X >= 0 && LocalPos.X <= Size.X && LocalPos.Y >= 0 && LocalPos.Y <= Size.Y)
            {
                if (i < ZoneIds.Num())
                {
                    UE_LOG(LogMASceneListWidget, Log, TEXT("OnZoneButtonClicked: Zone %s clicked (mouse)"), *ZoneIds[i]);
                    OnZoneItemClicked.Broadcast(ZoneIds[i]);
                }
                return;
            }
        }
    }
}
