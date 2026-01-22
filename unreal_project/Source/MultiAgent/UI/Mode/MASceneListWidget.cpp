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
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAUITheme.h"
#include "../../Core/Manager/MAEditModeManager.h"
#include "../../Core/Manager/MASceneGraphManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneListWidget, Log, All);

UMASceneListWidget::UMASceneListWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=========================================================================
// ApplyTheme - 应用主题样式
//=========================================================================

void UMASceneListWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        UE_LOG(LogMASceneListWidget, Warning, TEXT("ApplyTheme: Theme is null, using default colors"));
        return;
    }

    // 更新 Goal 标题颜色
    if (GoalTitleText)
    {
        GoalTitleText->SetColorAndOpacity(FSlateColor(Theme->DangerColor));
    }

    // 更新 Zone 标题颜色
    if (ZoneTitleText)
    {
        ZoneTitleText->SetColorAndOpacity(FSlateColor(Theme->PrimaryColor));
    }

    // 更新计数文字颜色
    if (GoalCountText)
    {
        GoalCountText->SetColorAndOpacity(FSlateColor(Theme->SecondaryTextColor));
    }

    if (ZoneCountText)
    {
        ZoneCountText->SetColorAndOpacity(FSlateColor(Theme->SecondaryTextColor));
    }

    UE_LOG(LogMASceneListWidget, Log, TEXT("ApplyTheme: Theme applied successfully"));
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
    BackgroundBorder->SetPadding(FMargin(10.0f));

    // 应用圆角效果 (使用默认主题值)
    MARoundedBorderUtils::ApplyRoundedCornersFromTheme(
        BackgroundBorder,
        nullptr,  // 使用默认主题值
        EMARoundedElementType::Panel
    );

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
    // 使用 Theme 颜色，fallback 到默认蓝色
    FLinearColor TitleColor = Theme ? Theme->PrimaryColor : FLinearColor(0.3f, 0.6f, 1.0f);
    TitleText->SetColorAndOpacity(FSlateColor(TitleColor));

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
    // 使用 Theme 颜色，fallback 到默认红色
    FLinearColor GoalTitleColor = Theme ? Theme->DangerColor : FLinearColor(1.0f, 0.4f, 0.4f);
    GoalTitleText->SetColorAndOpacity(FSlateColor(GoalTitleColor));

    UHorizontalBoxSlot* GoalTitleSlot = GoalHeaderBox->AddChildToHorizontalBox(GoalTitleText);
    GoalTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GoalCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoalCountText"));
    GoalCountText->SetText(FText::FromString(TEXT("(0)")));
    FSlateFontInfo GoalCountFont = GoalCountText->GetFont();
    GoalCountFont.Size = 11;
    GoalCountText->SetFont(GoalCountFont);
    // 使用 Theme 颜色，fallback 到默认灰色
    FLinearColor CountTextColor = Theme ? Theme->SecondaryTextColor : FLinearColor(0.6f, 0.6f, 0.6f);
    GoalCountText->SetColorAndOpacity(FSlateColor(CountTextColor));
    
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
    // 使用 Theme 颜色，fallback 到默认蓝色
    FLinearColor ZoneTitleColor = Theme ? Theme->PrimaryColor : FLinearColor(0.3f, 0.6f, 1.0f);
    ZoneTitleText->SetColorAndOpacity(FSlateColor(ZoneTitleColor));

    UHorizontalBoxSlot* ZoneTitleSlot = ZoneHeaderBox->AddChildToHorizontalBox(ZoneTitleText);
    ZoneTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ZoneCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneCountText"));
    ZoneCountText->SetText(FText::FromString(TEXT("(0)")));
    FSlateFontInfo ZoneCountFont = ZoneCountText->GetFont();
    ZoneCountFont.Size = 11;
    ZoneCountText->SetFont(ZoneCountFont);
    // 使用 Theme 颜色，fallback 到默认灰色
    ZoneCountText->SetColorAndOpacity(FSlateColor(CountTextColor));
    
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
    if (!GoalListBox)
    {
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        UE_LOG(LogMASceneListWidget, Warning, TEXT("PopulateGoalList: SceneGraphManager not found"));
        return;
    }

    // 清除现有按钮
    GoalListBox->ClearChildren();
    GoalButtons.Empty();
    GoalIds.Empty();

    // 获取所有 Goal 节点
    TArray<FMASceneGraphNode> AllGoals = SceneGraphManager->GetAllGoals();

    // 更新计数
    if (GoalCountText)
    {
        GoalCountText->SetText(FText::FromString(FString::Printf(TEXT("(%d)"), AllGoals.Num())));
    }

    // 为每个 Goal 创建按钮
    for (const FMASceneGraphNode& GoalNode : AllGoals)
    {
        FString Label = GoalNode.Label;
        if (Label.IsEmpty())
        {
            Label = GoalNode.Id;
        }

        UButton* GoalButton = CreateListItemButton(Label, GoalNode.Id, true);
        if (GoalButton)
        {
            GoalButton->OnClicked.AddDynamic(this, &UMASceneListWidget::OnGoalButtonClicked);
            GoalButtons.Add(GoalButton);
            GoalIds.Add(GoalNode.Id);

            UVerticalBoxSlot* ButtonSlot = GoalListBox->AddChildToVerticalBox(GoalButton);
            ButtonSlot->SetPadding(FMargin(0, 2, 0, 2));
        }
    }

    // 如果没有 Goal，显示提示
    if (AllGoals.Num() == 0)
    {
        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("  (No Goals)")));
        FSlateFontInfo EmptyFont = EmptyText->GetFont();
        EmptyFont.Size = 10;
        EmptyText->SetFont(EmptyFont);
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
        GoalListBox->AddChildToVerticalBox(EmptyText);
    }

    UE_LOG(LogMASceneListWidget, Log, TEXT("PopulateGoalList: Added %d goals"), AllGoals.Num());
}

void UMASceneListWidget::PopulateZoneList()
{
    if (!ZoneListBox)
    {
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        UE_LOG(LogMASceneListWidget, Warning, TEXT("PopulateZoneList: SceneGraphManager not found"));
        return;
    }

    // 清除现有按钮
    ZoneListBox->ClearChildren();
    ZoneButtons.Empty();
    ZoneIds.Empty();

    // 获取所有 Zone 节点
    TArray<FMASceneGraphNode> AllZones = SceneGraphManager->GetAllZones();

    // 更新计数
    if (ZoneCountText)
    {
        ZoneCountText->SetText(FText::FromString(FString::Printf(TEXT("(%d)"), AllZones.Num())));
    }

    // 为每个 Zone 创建按钮
    for (const FMASceneGraphNode& ZoneNode : AllZones)
    {
        FString Label = ZoneNode.Label;
        if (Label.IsEmpty())
        {
            Label = ZoneNode.Id;
        }

        UButton* ZoneButton = CreateListItemButton(Label, ZoneNode.Id, false);
        if (ZoneButton)
        {
            ZoneButton->OnClicked.AddDynamic(this, &UMASceneListWidget::OnZoneButtonClicked);
            ZoneButtons.Add(ZoneButton);
            ZoneIds.Add(ZoneNode.Id);

            UVerticalBoxSlot* ButtonSlot = ZoneListBox->AddChildToVerticalBox(ZoneButton);
            ButtonSlot->SetPadding(FMargin(0, 2, 0, 2));
        }
    }

    // 如果没有 Zone，显示提示
    if (AllZones.Num() == 0)
    {
        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("  (No Zones)")));
        FSlateFontInfo EmptyFont = EmptyText->GetFont();
        EmptyFont.Size = 10;
        EmptyText->SetFont(EmptyFont);
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
        ZoneListBox->AddChildToVerticalBox(EmptyText);
    }

    UE_LOG(LogMASceneListWidget, Log, TEXT("PopulateZoneList: Added %d zones"), AllZones.Num());
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
    // 使用 Theme 颜色，fallback 到默认颜色
    if (bIsGoal)
    {
        FLinearColor GoalButtonColor = Theme ? Theme->DangerColor : FLinearColor(0.8f, 0.2f, 0.2f);
        ButtonText->SetColorAndOpacity(FSlateColor(GoalButtonColor));
    }
    else
    {
        FLinearColor ZoneButtonColor = Theme ? Theme->PrimaryColor : FLinearColor(0.2f, 0.4f, 0.8f);
        ButtonText->SetColorAndOpacity(FSlateColor(ZoneButtonColor));
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
