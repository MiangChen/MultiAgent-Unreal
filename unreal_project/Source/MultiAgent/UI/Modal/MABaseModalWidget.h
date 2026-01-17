// MABaseModalWidget.h
// 基础模态窗口抽象类 - 所有模态窗口的父类
// 确保所有模态窗口具有一致的样式和行为
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Core/MAHUDTypes.h"
#include "MABaseModalWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UTextBlock;
class UNamedSlot;
class UHorizontalBox;
class UVerticalBox;
class USizeBox;
class UOverlay;
class UMAStyledButton;
class UMAUITheme;

//=============================================================================
// 模态窗口委托
//=============================================================================

/** 模态确认委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModalConfirm);

/** 模态拒绝委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModalReject);

/** 模态编辑委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModalEdit);

/** 模态动画完成委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModalAnimationComplete);

//=============================================================================
// MABaseModalWidget 类
//=============================================================================

/**
 * 基础模态窗口抽象类
 * 
 * 所有模态窗口的父类，提供：
 * - 统一的布局结构：标题栏、内容区、按钮容器
 * - 圆角矩形样式（从 UI_Theme 应用）
 * - 淡入淡出动画（200ms）
 * - Confirm/Reject/Edit 按钮及其回调
 * 
 * 子类需要：
 * - 重写 OnEditModeChanged 处理编辑模式切换
 * - 重写 GetModalTitle 返回模态标题
 * - 在 ContentContainer 中添加自定义内容
 * 
 * Requirements: 8.1, 8.2, 8.3, 8.4, 8.5
 */
UCLASS(Abstract)
class MULTIAGENT_API UMABaseModalWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMABaseModalWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 确认按钮点击事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalConfirm OnConfirmClicked;

    /** 拒绝按钮点击事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalReject OnRejectClicked;

    /** 编辑按钮点击事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalEdit OnEditClicked;

    /** 显示动画完成事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalAnimationComplete OnShowAnimationComplete;

    /** 隐藏动画完成事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalAnimationComplete OnHideAnimationComplete;

    //=========================================================================
    // 模式控制
    //=========================================================================

    /**
     * 设置编辑模式
     * @param bEditable 是否可编辑
     */
    UFUNCTION(BlueprintCallable, Category = "Mode")
    void SetEditMode(bool bEditable);

    /**
     * 检查是否处于编辑模式
     * @return true 如果处于编辑模式
     */
    UFUNCTION(BlueprintPure, Category = "Mode")
    bool IsEditMode() const { return bIsEditMode; }

    /**
     * 设置模态类型
     * @param InModalType 模态类型
     */
    UFUNCTION(BlueprintCallable, Category = "Mode")
    void SetModalType(EMAModalType InModalType);

    /**
     * 获取模态类型
     * @return 当前模态类型
     */
    UFUNCTION(BlueprintPure, Category = "Mode")
    EMAModalType GetModalType() const { return ModalType; }

    //=========================================================================
    // 动画控制 (Requirements: 5.6, 7.1, 7.4, 8.5)
    //=========================================================================

    /**
     * 播放显示动画（淡入，200ms）
     */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayShowAnimation();

    /**
     * 播放隐藏动画（淡出，200ms）
     */
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayHideAnimation();

    /**
     * 检查动画是否正在播放
     * @return true 如果动画正在播放
     */
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsAnimating() const { return bIsAnimating; }

    //=========================================================================
    // 主题应用
    //=========================================================================

    /**
     * 应用主题
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    virtual void ApplyTheme(UMAUITheme* InTheme);

    /**
     * 获取当前主题
     * @return 当前主题
     */
    UFUNCTION(BlueprintPure, Category = "Theme")
    UMAUITheme* GetTheme() const { return Theme; }

    //=========================================================================
    // 标题设置
    //=========================================================================

    /**
     * 设置模态标题
     * @param InTitle 标题文本
     */
    UFUNCTION(BlueprintCallable, Category = "Content")
    void SetTitle(const FText& InTitle);

    /**
     * 获取模态标题
     * @return 当前标题
     */
    UFUNCTION(BlueprintPure, Category = "Content")
    FText GetTitle() const { return ModalTitle; }

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 虚函数 - 子类实现
    //=========================================================================

    /**
     * 编辑模式变化回调
     * 子类重写此方法以处理编辑模式切换
     * @param bEditable 是否可编辑
     */
    virtual void OnEditModeChanged(bool bEditable) {}

    /**
     * 获取模态标题
     * 子类可重写此方法返回自定义标题
     * @return 模态标题
     */
    virtual FText GetModalTitleText() const;

    /**
     * 构建内容区域
     * 子类重写此方法添加自定义内容
     * @param InContentContainer 内容容器
     */
    virtual void BuildContentArea(UVerticalBox* InContentContainer) {}

    /**
     * 主题应用后回调
     * 子类可重写此方法应用额外的主题样式
     */
    virtual void OnThemeApplied() {}

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根面板 */
    UPROPERTY()
    UCanvasPanel* RootPanel;

    /** 背景遮罩 */
    UPROPERTY()
    UBorder* BackgroundOverlay;

    /** 模态容器 (居中) */
    UPROPERTY()
    USizeBox* ModalSizeBox;

    /** 模态背景边框 (圆角) */
    UPROPERTY()
    UBorder* BackgroundBorder;

    /** 模态内容垂直布局 */
    UPROPERTY()
    UVerticalBox* ModalVerticalBox;

    /** 标题文本 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 内容容器 (子类填充) */
    UPROPERTY()
    UVerticalBox* ContentContainer;

    /** 按钮容器 */
    UPROPERTY()
    UHorizontalBox* ButtonContainer;

    /** 确认按钮 */
    UPROPERTY()
    UMAStyledButton* ConfirmButton;

    /** 拒绝按钮 */
    UPROPERTY()
    UMAStyledButton* RejectButton;

    /** 编辑按钮 (可选) */
    UPROPERTY()
    UMAStyledButton* EditButton;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

    /** 模态标题 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText ModalTitle;

    /** 模态宽度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float ModalWidth = 800.0f;

    /** 模态高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float ModalHeight = 600.0f;

    /** 是否显示编辑按钮 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buttons")
    bool bShowEditButton = true;

    /** 确认按钮文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buttons")
    FText ConfirmButtonText;

    /** 拒绝按钮文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buttons")
    FText RejectButtonText;

    /** 编辑按钮文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buttons")
    FText EditButtonText;

private:
    //=========================================================================
    // 状态
    //=========================================================================

    /** 是否处于编辑模式 */
    bool bIsEditMode = false;

    /** 模态类型 */
    EMAModalType ModalType = EMAModalType::None;

    /** 是否正在播放动画 */
    bool bIsAnimating = false;

    /** 是否正在显示 */
    bool bIsShowing = false;

    /** 当前动画时间 */
    float CurrentAnimTime = 0.0f;

    /** 动画目标透明度 */
    float TargetOpacity = 1.0f;

    /** 动画时长 (秒) */
    static constexpr float AnimationDuration = 0.2f;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 更新按钮可见性 (根据编辑模式) */
    void UpdateButtonVisibility();

    /** 更新动画状态 */
    void UpdateAnimation(float DeltaTime);

    /** 动画完成回调 */
    void OnAnimationFinished();

    //=========================================================================
    // 按钮回调 (Requirements: 5.4, 6.5)
    //=========================================================================

    /** 确认按钮点击回调 */
    UFUNCTION()
    void OnConfirmButtonClicked();

    /** 拒绝按钮点击回调 */
    UFUNCTION()
    void OnRejectButtonClicked();

    /** 编辑按钮点击回调 */
    UFUNCTION()
    void OnEditButtonClicked();
};
