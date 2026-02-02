// MAPIPCameraContainerWidget.h
// 画中画相机容器 Widget
// 管理多个 MAPIPCameraWidget 的显示和布局

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MAPIPCameraTypes.h"
#include "MAPIPCameraContainerWidget.generated.h"

class UCanvasPanel;
class UMAPIPCameraWidget;
class UTextureRenderTarget2D;

/**
 * 画中画相机容器 Widget
 * 
 * 功能：
 * - 管理多个画中画 Widget 的添加/移除
 * - 处理屏幕位置计算
 * - 支持动态布局
 */
UCLASS()
class MULTIAGENT_API UMAPIPCameraContainerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 添加画中画相机
     * @param RenderTarget 渲染目标纹理
     * @param DisplayConfig 显示配置
     * @return Widget 索引
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    int32 AddPIPCamera(UTextureRenderTarget2D* RenderTarget, const FMAPIPDisplayConfig& DisplayConfig);

    /**
     * 移除画中画相机
     * @param Index Widget 索引
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void RemovePIPCamera(int32 Index);

    /**
     * 更新画中画相机纹理
     * @param Index Widget 索引
     * @param RenderTarget 新的渲染目标
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void UpdatePIPCameraTexture(int32 Index, UTextureRenderTarget2D* RenderTarget);

    /**
     * 更新画中画相机显示配置
     * @param Index Widget 索引
     * @param DisplayConfig 新的显示配置
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void UpdatePIPCameraDisplay(int32 Index, const FMAPIPDisplayConfig& DisplayConfig);

    /**
     * 获取当前画中画数量
     */
    UFUNCTION(BlueprintPure, Category = "PIPCamera")
    int32 GetPIPCameraCount() const { return PIPWidgets.Num(); }

    /**
     * 清除所有画中画
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void ClearAllPIPCameras();

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 Canvas 面板 */
    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    /** 所有画中画 Widget */
    UPROPERTY()
    TArray<TObjectPtr<UMAPIPCameraWidget>> PIPWidgets;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 结构 */
    void BuildUI();

    /** 计算屏幕位置（从归一化坐标转换为像素） */
    FVector2D CalculateScreenPosition(const FVector2D& NormalizedPosition, const FVector2D& WidgetSize) const;

    /** 获取视口大小 */
    FVector2D GetViewportSize() const;
};
