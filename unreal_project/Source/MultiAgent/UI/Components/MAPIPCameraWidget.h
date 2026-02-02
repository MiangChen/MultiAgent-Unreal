// MAPIPCameraWidget.h
// 单个画中画相机显示 Widget
// 显示渲染目标纹理，带边框和阴影效果

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MAPIPCameraTypes.h"
#include "MAPIPCameraWidget.generated.h"

class UImage;
class UTextBlock;
class UBorder;
class UOverlay;
class UCanvasPanel;
class UTextureRenderTarget2D;

/**
 * 单个画中画相机 Widget
 * 
 * 显示内容：
 * - 渲染目标纹理（相机画面）
 * - 可选标题栏
 * - 边框效果
 * - 阴影效果
 */
UCLASS()
class MULTIAGENT_API UMAPIPCameraWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 设置渲染目标纹理
     * @param RenderTarget 渲染目标
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void SetRenderTarget(UTextureRenderTarget2D* RenderTarget);

    /**
     * 设置显示配置
     * @param Config 显示配置
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void SetDisplayConfig(const FMAPIPDisplayConfig& Config);

    /**
     * 设置标题
     * @param Title 标题文本
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void SetTitle(const FString& Title);

    /**
     * 设置大小
     * @param Size 大小（像素）
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void SetSize(const FVector2D& Size);

    /**
     * 获取当前大小
     */
    UFUNCTION(BlueprintPure, Category = "PIPCamera")
    FVector2D GetCurrentSize() const { return CurrentSize; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根容器 */
    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    /** 阴影层 */
    UPROPERTY()
    TObjectPtr<UImage> ShadowImage;

    /** 主容器（包含边框） */
    UPROPERTY()
    TObjectPtr<UBorder> MainBorder;

    /** 内容叠加层 */
    UPROPERTY()
    TObjectPtr<UOverlay> ContentOverlay;

    /** 相机画面 */
    UPROPERTY()
    TObjectPtr<UImage> CameraImage;

    /** 标题栏背景 */
    UPROPERTY()
    TObjectPtr<UBorder> TitleBarBorder;

    /** 标题文本 */
    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 当前显示配置 */
    FMAPIPDisplayConfig DisplayConfig;

    /** 当前大小 */
    FVector2D CurrentSize = FVector2D(400.f, 300.f);

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 结构 */
    void BuildUI();

    /** 更新阴影 */
    void UpdateShadow();

    /** 更新边框 */
    void UpdateBorder();

    /** 更新标题栏 */
    void UpdateTitleBar();
};
