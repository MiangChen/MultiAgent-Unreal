// MAMainHUDWidget.h
// 主 HUD Widget - 集成小地图、通知组件和右侧边栏
// 作为正常 HUD 状态 (State A) 的主要显示组件
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAMainHUDWidget.generated.h"

class UCanvasPanel;
class UMAMiniMapWidget;
class UMANotificationWidget;
class UMAUITheme;
class UTextureRenderTarget2D;

//=============================================================================
// MAMainHUDWidget 类
//=============================================================================

/**
 * 主 HUD Widget
 * 
 * 集成以下组件：
 * - MiniMapWidget: 小地图，位于左上角
 * - NotificationWidget: 通知组件，位于小地图下方
 * 
 * 注意：右侧边栏功能已拆分为三个独立面板 (SystemLogPanel, PreviewPanel, InstructionPanel)
 * 这些面板由 MAUIManager 直接管理，不再是 MAMainHUDWidget 的子组件
 * 
 * 布局结构：
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ ┌──────────┐                                                    │
 * │ │ MiniMap  │                                                    │
 * │ │ (左上角)  │                                                    │
 * │ └──────────┘                                                    │
 * │ ┌──────────┐                                                    │
 * │ │Notifica- │                                                    │
 * │ │tion Area │                                                    │
 * │ │(小地图下) │                                                    │
 * │ └──────────┘                                                    │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 */
UCLASS()
class MULTIAGENT_API UMAMainHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAMainHUDWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 组件访问接口
    //=========================================================================

    /**
     * 获取小地图组件
     * @return 小地图 Widget 指针
     */
    UFUNCTION(BlueprintPure, Category = "MainHUD|Components")
    UMAMiniMapWidget* GetMiniMap() const { return MiniMapWidget; }

    /**
     * 获取通知组件
     * @return 通知 Widget 指针
     */
    UFUNCTION(BlueprintPure, Category = "MainHUD|Components")
    UMANotificationWidget* GetNotification() const { return NotificationWidget; }

    //=========================================================================
    // 主题应用
    //=========================================================================

    /**
     * 应用主题到所有子组件
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "MainHUD|Theme")
    void ApplyTheme(UMAUITheme* InTheme);

    /**
     * 获取当前主题
     * @return 当前主题
     */
    UFUNCTION(BlueprintPure, Category = "MainHUD|Theme")
    UMAUITheme* GetTheme() const { return Theme; }

    //=========================================================================
    // 小地图控制
    //=========================================================================

    /**
     * 初始化小地图
     * @param InRenderTarget 渲染目标纹理
     * @param InWorldBounds 世界范围
     */
    UFUNCTION(BlueprintCallable, Category = "MainHUD|MiniMap")
    void InitializeMiniMap(UTextureRenderTarget2D* InRenderTarget, FVector2D InWorldBounds);

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 小地图距离左边缘的距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|MiniMap")
    float MiniMapLeftMargin = 20.0f;

    /** 小地图距离顶部的距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|MiniMap")
    float MiniMapTopMargin = 20.0f;

    /** 通知组件距离小地图的间距 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Notification")
    float NotificationTopMargin = 10.0f;

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 Canvas */
    UPROPERTY()
    UCanvasPanel* RootCanvas;

    /** 小地图组件 (左上角) */
    UPROPERTY()
    UMAMiniMapWidget* MiniMapWidget;

    /** 通知组件 (小地图下方) */
    UPROPERTY()
    UMANotificationWidget* NotificationWidget;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 创建并配置小地图组件 */
    void CreateMiniMapWidget();

    /** 创建并配置通知组件 */
    void CreateNotificationWidget();

    /** 应用主题到通知组件 */
    void ApplyThemeToNotification();
};
