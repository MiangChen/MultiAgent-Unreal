// MAMainHUDWidget.cpp
// 主 HUD Widget 实现
// 集成小地图和通知组件
// 注意：右侧边栏功能已迁移到独立面板 (SystemLogPanel, PreviewPanel, InstructionPanel)

#include "MAMainHUDWidget.h"
#include "../../Components/Presentation/MAMiniMapWidget.h"
#include "../../Components/Presentation/MANotificationWidget.h"
#include "../../Core/MAUITheme.h"
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
    , Theme(nullptr)
    , RootCanvas(nullptr)
    , MiniMapWidget(nullptr)
    , NotificationWidget(nullptr)
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

    if (MiniMapWidget)
    {
        MiniMapWidget->ApplyTheme(Theme);
    }

    ApplyThemeToNotification();

    UE_LOG(LogMAMainHUD, Log, TEXT("Theme applied to all components"));
}

void UMAMainHUDWidget::ApplyThemeToNotification()
{
    if (NotificationWidget && Theme)
    {
        NotificationWidget->ApplyTheme(Theme);
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
    // 注意：右侧边栏已移除，面板由 MAUIManager 直接管理

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
