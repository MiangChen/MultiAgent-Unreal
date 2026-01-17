// MAMainHUDWidget.cpp
// 主 HUD Widget 实现
// 集成小地图、通知组件和右侧边栏

#include "MAMainHUDWidget.h"
#include "../Components/MAMiniMapWidget.h"
#include "../Components/MANotificationWidget.h"
#include "../Components/MARightSidebarWidget.h"
#include "../Core/MAUITheme.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/TextureRenderTarget2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAMainHUD, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMAMainHUDWidget::UMAMainHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , MiniMapLeftMargin(20.0f)
    , MiniMapTopMargin(20.0f)
    , NotificationTopMargin(10.0f)
    , SidebarRightMargin(20.0f)
    , SidebarTopMargin(20.0f)
    , Theme(nullptr)
    , RootCanvas(nullptr)
    , MiniMapWidget(nullptr)
    , NotificationWidget(nullptr)
    , RightSidebarWidget(nullptr)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMAMainHUDWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMAMainHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogMAMainHUD, Log, TEXT("MAMainHUDWidget NativeConstruct"));

    // 应用主题（如果已设置）
    if (Theme)
    {
        ApplyTheme(Theme);
    }
}

TSharedRef<SWidget> UMAMainHUDWidget::RebuildWidget()
{
    // 构建 UI 布局
    BuildUI();

    return Super::RebuildWidget();
}

//=============================================================================
// 主题应用
//=============================================================================

void UMAMainHUDWidget::ApplyTheme(UMAUITheme* InTheme)
{
    if (!InTheme)
    {
        UE_LOG(LogMAMainHUD, Warning, TEXT("ApplyTheme: Theme is null"));
        return;
    }

    Theme = InTheme;

    // 应用主题到所有子组件
    ApplyThemeToMiniMap();
    ApplyThemeToNotification();
    ApplyThemeToSidebar();

    UE_LOG(LogMAMainHUD, Log, TEXT("Theme applied to all components"));
}

void UMAMainHUDWidget::ApplyThemeToMiniMap()
{
    // 小地图目前没有主题支持，预留接口
    // 未来可以添加边框颜色、背景色等主题属性
}

void UMAMainHUDWidget::ApplyThemeToNotification()
{
    if (NotificationWidget && Theme)
    {
        NotificationWidget->ApplyTheme(Theme);
    }
}

void UMAMainHUDWidget::ApplyThemeToSidebar()
{
    if (RightSidebarWidget && Theme)
    {
        RightSidebarWidget->ApplyTheme(Theme);
    }
}

//=============================================================================
// 小地图控制
//=============================================================================

void UMAMainHUDWidget::InitializeMiniMap(UTextureRenderTarget2D* InRenderTarget, FVector2D InWorldBounds)
{
    if (MiniMapWidget)
    {
        MiniMapWidget->InitializeMiniMap(InRenderTarget, InWorldBounds);
        UE_LOG(LogMAMainHUD, Log, TEXT("MiniMap initialized with WorldBounds: (%f, %f)"), 
            InWorldBounds.X, InWorldBounds.Y);
    }
    else
    {
        UE_LOG(LogMAMainHUD, Warning, TEXT("InitializeMiniMap: MiniMapWidget is null"));
    }
}

//=============================================================================
// 右侧边栏便捷方法
//=============================================================================

void UMAMainHUDWidget::UpdateTaskGraphPreview(const FMATaskGraphData& Data)
{
    if (RightSidebarWidget)
    {
        RightSidebarWidget->UpdateTaskGraphPreview(Data);
    }
    else
    {
        UE_LOG(LogMAMainHUD, Warning, TEXT("UpdateTaskGraphPreview: RightSidebarWidget is null"));
    }
}

void UMAMainHUDWidget::UpdateSkillListPreview(const FMASkillAllocationData& Data)
{
    if (RightSidebarWidget)
    {
        RightSidebarWidget->UpdateSkillListPreview(Data);
    }
    else
    {
        UE_LOG(LogMAMainHUD, Warning, TEXT("UpdateSkillListPreview: RightSidebarWidget is null"));
    }
}

void UMAMainHUDWidget::AppendLog(const FString& Message, bool bIsError)
{
    if (RightSidebarWidget)
    {
        RightSidebarWidget->AppendLog(Message, bIsError);
    }
    else
    {
        UE_LOG(LogMAMainHUD, Warning, TEXT("AppendLog: RightSidebarWidget is null"));
    }
}

void UMAMainHUDWidget::ClearLogs()
{
    if (RightSidebarWidget)
    {
        RightSidebarWidget->ClearLogs();
    }
    else
    {
        UE_LOG(LogMAMainHUD, Warning, TEXT("ClearLogs: RightSidebarWidget is null"));
    }
}

//=============================================================================
// 内部方法 - UI 构建
//=============================================================================

void UMAMainHUDWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMAMainHUD, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }

    // 创建根 Canvas
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAMainHUD, Error, TEXT("BuildUI: Failed to create RootCanvas"));
        return;
    }

    WidgetTree->RootWidget = RootCanvas;

    // 创建子组件
    CreateMiniMapWidget();
    CreateNotificationWidget();
    CreateRightSidebarWidget();

    UE_LOG(LogMAMainHUD, Log, TEXT("BuildUI: UI layout built successfully"));
}

void UMAMainHUDWidget::CreateMiniMapWidget()
{
    if (!RootCanvas || !WidgetTree)
    {
        return;
    }

    // 创建小地图 Widget
    MiniMapWidget = WidgetTree->ConstructWidget<UMAMiniMapWidget>(UMAMiniMapWidget::StaticClass(), TEXT("MiniMapWidget"));
    if (!MiniMapWidget)
    {
        UE_LOG(LogMAMainHUD, Error, TEXT("CreateMiniMapWidget: Failed to create MiniMapWidget"));
        return;
    }

    // 添加到 Canvas 并设置位置 (左上角)
    UCanvasPanelSlot* MiniMapSlot = RootCanvas->AddChildToCanvas(MiniMapWidget);
    if (MiniMapSlot)
    {
        // 锚点设置为左上角
        MiniMapSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
        MiniMapSlot->SetAlignment(FVector2D(0.0f, 0.0f));
        
        // 设置位置
        MiniMapSlot->SetPosition(FVector2D(MiniMapLeftMargin, MiniMapTopMargin));
        
        // 设置大小 (使用小地图默认大小)
        MiniMapSlot->SetSize(FVector2D(MiniMapWidget->MiniMapSize, MiniMapWidget->MiniMapSize));
        MiniMapSlot->SetAutoSize(false);
    }

    UE_LOG(LogMAMainHUD, Log, TEXT("MiniMapWidget created at position (%f, %f)"), 
        MiniMapLeftMargin, MiniMapTopMargin);
}

void UMAMainHUDWidget::CreateNotificationWidget()
{
    if (!RootCanvas || !WidgetTree)
    {
        return;
    }

    // 创建通知 Widget
    NotificationWidget = WidgetTree->ConstructWidget<UMANotificationWidget>(UMANotificationWidget::StaticClass(), TEXT("NotificationWidget"));
    if (!NotificationWidget)
    {
        UE_LOG(LogMAMainHUD, Error, TEXT("CreateNotificationWidget: Failed to create NotificationWidget"));
        return;
    }

    // 添加到 Canvas 并设置位置 (小地图下方)
    UCanvasPanelSlot* NotificationSlot = RootCanvas->AddChildToCanvas(NotificationWidget);
    if (NotificationSlot)
    {
        // 锚点设置为左上角
        NotificationSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
        NotificationSlot->SetAlignment(FVector2D(0.0f, 0.0f));
        
        // 计算位置：小地图下方
        float MiniMapSize = MiniMapWidget ? MiniMapWidget->MiniMapSize : 200.0f;
        float NotificationY = MiniMapTopMargin + MiniMapSize + NotificationTopMargin;
        NotificationSlot->SetPosition(FVector2D(MiniMapLeftMargin, NotificationY));
        
        // 自动大小
        NotificationSlot->SetAutoSize(true);
    }

    UE_LOG(LogMAMainHUD, Log, TEXT("NotificationWidget created below MiniMap"));
}

void UMAMainHUDWidget::CreateRightSidebarWidget()
{
    if (!RootCanvas || !WidgetTree)
    {
        return;
    }

    // 创建右侧边栏 Widget
    RightSidebarWidget = WidgetTree->ConstructWidget<UMARightSidebarWidget>(UMARightSidebarWidget::StaticClass(), TEXT("RightSidebarWidget"));
    if (!RightSidebarWidget)
    {
        UE_LOG(LogMAMainHUD, Error, TEXT("CreateRightSidebarWidget: Failed to create RightSidebarWidget"));
        return;
    }

    // 添加到 Canvas 并设置位置 (右侧)
    UCanvasPanelSlot* SidebarSlot = RootCanvas->AddChildToCanvas(RightSidebarWidget);
    if (SidebarSlot)
    {
        // 锚点设置为右上角
        SidebarSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 1.0f));
        SidebarSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        
        // 设置位置 (从右边缘偏移)
        SidebarSlot->SetPosition(FVector2D(-SidebarRightMargin, SidebarTopMargin));
        
        // 设置大小 (宽度固定，高度拉伸)
        // 使用 Offsets 来设置边距
        SidebarSlot->SetOffsets(FMargin(0.0f, SidebarTopMargin, SidebarRightMargin, SidebarTopMargin));
        SidebarSlot->SetAutoSize(true);
    }

    UE_LOG(LogMAMainHUD, Log, TEXT("RightSidebarWidget created on right side"));
}
