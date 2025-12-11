// MAEmergencyWidget.h
// 突发事件详情界面 Widget - 纯 C++ 实现
// 显示相机画面、信息文本、操作按钮和输入框
// Requirements: 2.2, 2.3, 3.1, 3.2, 3.3, 3.4

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAEmergencyWidget.generated.h"

class UImage;
class UMultiLineEditableTextBox;
class UButton;
class UCanvasPanel;
class UBorder;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMACameraSensorComponent;

/**
 * 按钮点击委托
 * @param ButtonIndex 按钮索引 (0=扩大搜索范围, 1=忽略并返回, 2=切换灭火任务)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionButtonClicked, int32, ButtonIndex);

/**
 * 消息发送委托
 * @param Message 用户输入的消息内容
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageSent, const FString&, Message);

/**
 * 突发事件详情界面 Widget
 * 
 * 布局:
 * - 左中: 相机画面区域 (CameraFeedImage)
 * - 右中: 信息显示区域 (InfoTextBox)
 * - 右侧: 三个操作按钮 (ActionButton1/2/3)
 * - 底部: 输入框和发送按钮 (InputTextBox, SendButton)
 * 
 * Requirements: 2.2, 2.3, 3.1, 3.2, 3.3, 3.4
 */
UCLASS()
class MULTIAGENT_API UMAEmergencyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAEmergencyWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 相机源管理
    //=========================================================================

    /**
     * 设置相机源，连接到渲染目标
     * @param Camera 相机传感器组件
     * Requirements: 2.2, 2.3, 4.4
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Camera")
    void SetCameraSource(UMACameraSensorComponent* Camera);

    /**
     * 清除相机源，显示黑屏
     * Requirements: 2.5
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Camera")
    void ClearCameraSource();

    //=========================================================================
    // 信息显示
    //=========================================================================

    /**
     * 设置信息显示文本
     * @param Text 要显示的信息文本
     * Requirements: 3.1
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Info")
    void SetInfoText(const FString& Text);

    /**
     * 添加信息文本（追加模式）
     * @param Text 要追加的信息文本
     * Requirements: 3.1
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Info")
    void AppendInfoText(const FString& Text);

    /**
     * 清空信息显示
     * Requirements: 3.1
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Info")
    void ClearInfoText();

    //=========================================================================
    // 输入控制
    //=========================================================================

    /**
     * 将焦点设置到输入框
     * Requirements: 3.3
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Input")
    void FocusInputBox();

    /**
     * 清空输入框内容
     * Requirements: 3.3
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency|Input")
    void ClearInputBox();

    /**
     * 获取输入框当前文本
     * @return 输入框中的文本内容
     * Requirements: 3.4
     */
    UFUNCTION(BlueprintPure, Category = "Emergency|Input")
    FString GetInputText() const;

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 按钮点击委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnActionButtonClicked OnActionButtonClicked;

    /** 消息发送委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMessageSent OnMessageSent;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI 组件 (动态创建)
    //=========================================================================

    /** 根面板 */
    UPROPERTY()
    UCanvasPanel* RootCanvas;
    
    /** 背景边框 */
    UPROPERTY()
    class UBorder* BackgroundBorder;

    /** 相机画面显示 */
    UPROPERTY()
    UImage* CameraFeedImage;

    /** 信息显示文本框 */
    UPROPERTY()
    UMultiLineEditableTextBox* InfoTextBox;

    /** 操作按钮 1: 扩大搜索范围 */
    UPROPERTY()
    UButton* ActionButton1;

    /** 操作按钮 2: 忽略并返回 */
    UPROPERTY()
    UButton* ActionButton2;

    /** 操作按钮 3: 切换灭火任务 */
    UPROPERTY()
    UButton* ActionButton3;

    /** 输入文本框 */
    UPROPERTY()
    UMultiLineEditableTextBox* InputTextBox;

    /** 发送按钮 */
    UPROPERTY()
    UButton* SendButton;

    //=========================================================================
    // 相机渲染资源
    //=========================================================================

    /** 渲染目标 */
    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    /** 相机材质实例 */
    UPROPERTY()
    UMaterialInstanceDynamic* CameraMaterial;

    /** 当前相机源 */
    UPROPERTY()
    TWeakObjectPtr<UMACameraSensorComponent> CurrentCameraSource;

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 构建 UI 布局
     * Requirements: 2.2, 2.3, 3.1, 3.2, 3.3
     */
    void BuildUI();

    /**
     * 创建相机渲染资源
     * Requirements: 2.2, 2.3, 4.4
     */
    void CreateCameraRenderResources();

    /**
     * 绑定按钮事件
     * Requirements: 3.2, 3.4
     */
    void BindButtonEvents();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** 操作按钮 1 点击处理 */
    UFUNCTION()
    void OnActionButton1Clicked();

    /** 操作按钮 2 点击处理 */
    UFUNCTION()
    void OnActionButton2Clicked();

    /** 操作按钮 3 点击处理 */
    UFUNCTION()
    void OnActionButton3Clicked();

    /** 发送按钮点击处理 */
    UFUNCTION()
    void OnSendButtonClicked();

    /** 输入框回车提交处理 */
    UFUNCTION()
    void OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /** 提交消息 */
    void SubmitMessage();
};