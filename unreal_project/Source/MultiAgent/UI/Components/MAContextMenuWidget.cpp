// MAContextMenuWidget.cpp
// 上下文菜单 Widget 实现

#include "MAContextMenuWidget.h"
#include "../Core/MARoundedBorderUtils.h"
#include "../Core/MAUITheme.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogMAContextMenu);

//=============================================================================
// 主题辅助函数
//=============================================================================

namespace
{
    /** 获取主题或创建默认主题 */
    UMAUITheme* GetOrCreateTheme(UMAContextMenuWidget* Widget)
    {
        // 尝试从 Widget 获取已设置的主题
        // 由于 Theme 是 protected，我们需要在 Widget 内部处理
        // 这里创建一个静态默认主题作为 fallback
        static UMAUITheme* DefaultTheme = nullptr;
        if (!DefaultTheme)
        {
            DefaultTheme = NewObject<UMAUITheme>();
            DefaultTheme->AddToRoot(); // 防止被 GC
        }
        return DefaultTheme;
    }
}

//=============================================================================
// 构造函数
//=============================================================================

UMAContextMenuWidget::UMAContextMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// 菜单操作
//=============================================================================

void UMAContextMenuWidget::SetMenuItems(const TArray<FMAContextMenuItem>& Items)
{
    MenuItems = Items;
    RebuildMenuItems();
}

void UMAContextMenuWidget::ShowAtPosition(FVector2D ScreenPosition)
{
    // 使用 UWidgetLayoutLibrary 获取视口缩放
    APlayerController* PC = GetOwningPlayer();
    float ViewportScale = 1.0f;
    if (PC)
    {
        ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
    }
    
    // 将屏幕坐标转换为视口坐标 (考虑 DPI 缩放)
    // SetPositionInViewport 期望的是视口坐标，而 GetScreenSpacePosition 返回的是屏幕坐标
    // 在大多数情况下，它们是相同的，但在高 DPI 显示器上可能不同
    FVector2D ViewportPosition = ScreenPosition / ViewportScale;
    
    // 设置位置
    SetPositionInViewport(ViewportPosition, false);
    
    UE_LOG(LogMAContextMenu, Log, TEXT("ShowAtPosition: Screen(%.1f, %.1f) -> Viewport(%.1f, %.1f), Scale=%.2f"), 
           ScreenPosition.X, ScreenPosition.Y, ViewportPosition.X, ViewportPosition.Y, ViewportScale);
    
    // 显示
    SetVisibility(ESlateVisibility::Visible);
    
    // 获取焦点
    SetKeyboardFocus();

    UE_LOG(LogMAContextMenu, Log, TEXT("Context menu shown with %d items"), MenuItems.Num());
}

void UMAContextMenuWidget::CloseMenu()
{
    SetVisibility(ESlateVisibility::Collapsed);
    OnMenuClosed.Broadcast();

    UE_LOG(LogMAContextMenu, Verbose, TEXT("CloseMenu"));
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAContextMenuWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMAContextMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 默认隐藏
    SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UMAContextMenuWidget::RebuildWidget()
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

FReply UMAContextMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 点击菜单外部时关闭
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    FVector2D Size = InGeometry.GetLocalSize();

    if (LocalPosition.X < 0 || LocalPosition.Y < 0 || 
        LocalPosition.X > Size.X || LocalPosition.Y > Size.Y)
    {
        CloseMenu();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMAContextMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    
    // 鼠标离开时关闭菜单
    // 注意：这可能太敏感，可以考虑添加延迟
    // CloseMenu();
}

//=============================================================================
// UI 构建
//=============================================================================

void UMAContextMenuWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAContextMenu, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMAContextMenu, Verbose, TEXT("BuildUI: Starting UI construction..."));

    // 如果没有主题，使用默认主题初始化颜色
    if (!Theme)
    {
        UMAUITheme* DefaultTheme = GetOrCreateTheme(this);
        // 从默认主题初始化颜色变量
        MenuBackgroundColor = DefaultTheme->BackgroundColor;
        ItemDefaultColor = DefaultTheme->SecondaryColor;
        ItemHoverColor = DefaultTheme->HighlightColor;
        ItemDisabledColor = FLinearColor(
            DefaultTheme->SecondaryColor.R * 0.5f,
            DefaultTheme->SecondaryColor.G * 0.5f,
            DefaultTheme->SecondaryColor.B * 0.5f,
            0.5f
        );
        TextColor = DefaultTheme->TextColor;
        DisabledTextColor = FLinearColor(
            DefaultTheme->SecondaryTextColor.R,
            DefaultTheme->SecondaryTextColor.G,
            DefaultTheme->SecondaryTextColor.B,
            0.5f
        );
    }

    // 创建菜单背景，带圆角
    MenuBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBackground"));
    
    // 应用圆角效果 - 使用 Panel 类型的圆角半径
    MARoundedBorderUtils::ApplyRoundedCorners(
        MenuBackground,
        MenuBackgroundColor,
        MARoundedBorderUtils::DefaultPanelCornerRadius
    );
    
    MenuBackground->SetPadding(FMargin(2.0f));
    WidgetTree->RootWidget = MenuBackground;

    // 创建菜单项容器
    ItemContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemContainer"));
    MenuBackground->AddChild(ItemContainer);

    UE_LOG(LogMAContextMenu, Verbose, TEXT("BuildUI: UI construction completed"));
}

void UMAContextMenuWidget::RebuildMenuItems()
{
    if (!ItemContainer)
    {
        return;
    }

    // 清空现有按钮
    ItemContainer->ClearChildren();
    ItemButtons.Empty();

    // 创建新按钮
    for (const FMAContextMenuItem& Item : MenuItems)
    {
        UButton* Button = CreateMenuItemButton(Item);
        if (Button)
        {
            UVerticalBoxSlot* Slot = ItemContainer->AddChildToVerticalBox(Button);
            Slot->SetPadding(FMargin(0, 1, 0, 1));
            ItemButtons.Add(Button);
        }
    }

    UE_LOG(LogMAContextMenu, Verbose, TEXT("RebuildMenuItems: Created %d items"), MenuItems.Num());
}

UButton* UMAContextMenuWidget::CreateMenuItemButton(const FMAContextMenuItem& Item)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    // 创建按钮
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Button_%s"), *Item.ItemId));
    
    // 设置按钮样式
    FButtonStyle ButtonStyle;
    FSlateBrush NormalBrush;
    NormalBrush.TintColor = FSlateColor(Item.bEnabled ? ItemDefaultColor : ItemDisabledColor);
    ButtonStyle.SetNormal(NormalBrush);
    
    FSlateBrush HoveredBrush;
    HoveredBrush.TintColor = FSlateColor(Item.bEnabled ? ItemHoverColor : ItemDisabledColor);
    ButtonStyle.SetHovered(HoveredBrush);
    
    FSlateBrush PressedBrush;
    PressedBrush.TintColor = FSlateColor(Item.bEnabled ? ItemHoverColor : ItemDisabledColor);
    ButtonStyle.SetPressed(PressedBrush);
    
    Button->SetStyle(ButtonStyle);

    // 创建文本
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Text_%s"), *Item.ItemId));
    Text->SetText(FText::FromString(Item.DisplayText));
    Text->SetColorAndOpacity(FSlateColor(Item.bEnabled ? TextColor : DisabledTextColor));
    
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = 11;
    Text->SetFont(Font);

    Button->AddChild(Text);

    // 绑定点击事件 - 使用索引来识别按钮
    if (Item.bEnabled)
    {
        int32 ItemIndex = MenuItems.IndexOfByPredicate([&Item](const FMAContextMenuItem& Other) {
            return Other.ItemId == Item.ItemId;
        });
        
        // 根据索引绑定不同的处理函数
        switch (ItemIndex)
        {
            case 0: Button->OnClicked.AddDynamic(this, &UMAContextMenuWidget::OnItem0Clicked); break;
            case 1: Button->OnClicked.AddDynamic(this, &UMAContextMenuWidget::OnItem1Clicked); break;
            case 2: Button->OnClicked.AddDynamic(this, &UMAContextMenuWidget::OnItem2Clicked); break;
            case 3: Button->OnClicked.AddDynamic(this, &UMAContextMenuWidget::OnItem3Clicked); break;
            default: break;
        }
    }

    return Button;
}

void UMAContextMenuWidget::OnMenuItemButtonClicked(const FString& ItemId)
{
    OnItemClicked.Broadcast(ItemId);
    CloseMenu();
}

void UMAContextMenuWidget::OnItem0Clicked()
{
    if (MenuItems.Num() > 0)
    {
        OnMenuItemButtonClicked(MenuItems[0].ItemId);
    }
}

void UMAContextMenuWidget::OnItem1Clicked()
{
    if (MenuItems.Num() > 1)
    {
        OnMenuItemButtonClicked(MenuItems[1].ItemId);
    }
}

void UMAContextMenuWidget::OnItem2Clicked()
{
    if (MenuItems.Num() > 2)
    {
        OnMenuItemButtonClicked(MenuItems[2].ItemId);
    }
}

void UMAContextMenuWidget::OnItem3Clicked()
{
    if (MenuItems.Num() > 3)
    {
        OnMenuItemButtonClicked(MenuItems[3].ItemId);
    }
}

//=============================================================================
// 主题
//=============================================================================

void UMAContextMenuWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        return;
    }

    // 从主题更新颜色变量
    // 菜单背景使用 BackgroundColor
    MenuBackgroundColor = Theme->BackgroundColor;
    
    // 菜单项默认颜色使用 SecondaryColor
    ItemDefaultColor = Theme->SecondaryColor;
    
    // 菜单项悬浮颜色使用 HighlightColor
    ItemHoverColor = Theme->HighlightColor;
    
    // 菜单项禁用颜色 - 使用 SecondaryColor 的暗化版本
    ItemDisabledColor = FLinearColor(
        Theme->SecondaryColor.R * 0.5f,
        Theme->SecondaryColor.G * 0.5f,
        Theme->SecondaryColor.B * 0.5f,
        0.5f
    );
    
    // 文本颜色使用 TextColor
    TextColor = Theme->TextColor;
    
    // 禁用文本颜色使用 SecondaryTextColor
    DisabledTextColor = FLinearColor(
        Theme->SecondaryTextColor.R,
        Theme->SecondaryTextColor.G,
        Theme->SecondaryTextColor.B,
        0.5f
    );

    // 更新菜单背景
    if (MenuBackground)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(
            MenuBackground,
            MenuBackgroundColor,
            MARoundedBorderUtils::DefaultPanelCornerRadius
        );
    }

    // 重建菜单项以应用新颜色
    RebuildMenuItems();

    UE_LOG(LogMAContextMenu, Verbose, TEXT("ApplyTheme: Theme applied successfully"));
}
